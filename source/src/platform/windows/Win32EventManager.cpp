#include "Win32EventManager.h"
#include <chrono>
#include <windowsx.h>

namespace YuchenUI {

// MARK: - Constructor and Destructor

Win32EventManager::Win32EventManager(HWND hwnd)
    : m_hwnd(hwnd)
    , m_eventQueue()
    , m_eventCallback(nullptr)
    , m_keyTracker()
    , m_mouseTracker()
    , m_isInitialized(false)
    , m_textInputEnabled(false)
    , m_imeComposing(false)
{
    m_imeCompositionBuffer.reserve(256);
}

Win32EventManager::~Win32EventManager()
{
    destroy();
}

// MARK: - Initialization

bool Win32EventManager::initialize()
{
    YUCHEN_ASSERT_MSG(!m_isInitialized, "Already initialized");

    m_eventQueue.clear();
    m_keyTracker.clear();
    m_mouseTracker.clear();
    m_isInitialized = true;

    return true;
}

void Win32EventManager::destroy()
{
    if (!m_isInitialized) return;

    clearEventCallback();
    clearEvents();
    m_keyTracker.clear();
    m_mouseTracker.clear();
    m_isInitialized = false;
}

bool Win32EventManager::isInitialized() const
{
    return m_isInitialized;
}

// MARK: - Event Queue Management

bool Win32EventManager::hasEvents() const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "Not initialized");
    return !m_eventQueue.isEmpty();
}

Event Win32EventManager::getNextEvent()
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "Not initialized");
    YUCHEN_ASSERT_MSG(hasEvents(), "No events available");

    Event event;
    bool success = m_eventQueue.pop(event);
    YUCHEN_ASSERT(success);
    YUCHEN_ASSERT(event.isValid());

    return event;
}

void Win32EventManager::clearEvents()
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "Not initialized");
    m_eventQueue.clear();
}

size_t Win32EventManager::getEventCount() const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "Not initialized");
    return m_eventQueue.size();
}

// MARK: - Event Callback

void Win32EventManager::setEventCallback(EventCallback callback)
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "Not initialized");
    m_eventCallback = callback;
}

void Win32EventManager::clearEventCallback()
{
    m_eventCallback = nullptr;
}

bool Win32EventManager::hasEventCallback() const
{
    return m_eventCallback != nullptr;
}

// MARK: - Native Event Handling

void Win32EventManager::handleNativeEvent(void* event)
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "Not initialized");
    YUCHEN_ASSERT(event);

    MSG* msg = reinterpret_cast<MSG*>(event);
    handleWindowsMessage(msg->message, msg->wParam, msg->lParam);
}

// MARK: - Input State Queries

bool Win32EventManager::isKeyPressed(KeyCode key) const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "Not initialized");
    return m_keyTracker.isKeyPressed(key);
}

bool Win32EventManager::isMouseButtonPressed(MouseButton button) const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "Not initialized");
    return m_mouseTracker.isButtonPressed(button);
}

Vec2 Win32EventManager::getMousePosition() const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "Not initialized");
    return m_mouseTracker.getPosition();
}

KeyModifiers Win32EventManager::getCurrentModifiers() const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "Not initialized");
    return extractModifiers();
}

// MARK: - Text Input Control

void Win32EventManager::enableTextInput()
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "Not initialized");
    m_textInputEnabled = true;
}

void Win32EventManager::disableTextInput()
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "Not initialized");
    m_textInputEnabled = false;
}

bool Win32EventManager::isTextInputEnabled() const
{
    return m_textInputEnabled;
}

// MARK: - Message Dispatching

void Win32EventManager::handleWindowsMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:
        handleKeyEvent(msg, wParam, lParam);
        break;

    case WM_CHAR:
        if (m_textInputEnabled && !m_imeComposing)
        {
            handleCharEvent(wParam);
        }
        break;

    case WM_IME_STARTCOMPOSITION:
        if (m_textInputEnabled)
        {
            handleImeStartComposition();
        }
        break;

    case WM_IME_COMPOSITION:
        if (m_textInputEnabled)
        {
            handleImeComposition(lParam);
        }
        break;

    case WM_IME_ENDCOMPOSITION:
        if (m_textInputEnabled)
        {
            handleImeEndComposition();
        }
        break;

    case WM_IME_NOTIFY:
        if (m_textInputEnabled)
        {
            handleImeNotify(wParam);
        }
        break;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
        handleMouseButtonEvent(msg, wParam, lParam);
        break;

    case WM_MOUSEMOVE:
        handleMouseMoveEvent(wParam, lParam);
        break;

    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
        handleMouseWheelEvent(msg, wParam, lParam);
        break;
    }
}

// MARK: - Keyboard Event Handling

void Win32EventManager::handleKeyEvent(UINT msg, WPARAM wParam, LPARAM lParam)
{
    KeyCode key = mapVirtualKey(wParam);
    if (key == KeyCode::Unknown) return;

    bool pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
    bool isRepeat = (lParam & 0x40000000) != 0;

    m_keyTracker.setKeyState(key, pressed);

    EventType eventType = pressed ? EventType::KeyPressed : EventType::KeyReleased;
    KeyModifiers modifiers = extractModifiers();
    double timestamp = getCurrentTime();

    Event event = Event::createKeyEvent(
        eventType, key, static_cast<uint16_t>(wParam),
        modifiers, isRepeat, timestamp
    );

    YUCHEN_ASSERT(event.isValid());
    pushEvent(event);
}

// MARK: - Mouse Event Handling

void Win32EventManager::handleMouseButtonEvent(UINT msg, WPARAM wParam, LPARAM lParam)
{
    MouseButton button = mapMouseButton(msg);
    Vec2 position = getMousePositionFromLParam(lParam);
    bool pressed = (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN);

    m_mouseTracker.setButtonState(button, pressed);
    m_mouseTracker.setPosition(position);

    EventType eventType = pressed ? EventType::MouseButtonPressed : EventType::MouseButtonReleased;
    KeyModifiers modifiers = extractModifiers();
    double timestamp = getCurrentTime();

    Event event = Event::createMouseButtonEvent(
        eventType, button, position, 1, modifiers, timestamp
    );

    YUCHEN_ASSERT(event.isValid());
    pushEvent(event);
}

void Win32EventManager::handleMouseMoveEvent(WPARAM wParam, LPARAM lParam)
{
    Vec2 position = getMousePositionFromLParam(lParam);
    Vec2 oldPosition = m_mouseTracker.getPosition();
    Vec2 delta = Vec2(position.x - oldPosition.x, position.y - oldPosition.y);

    m_mouseTracker.setPosition(position);

    KeyModifiers modifiers = extractModifiers();
    double timestamp = getCurrentTime();

    Event event = Event::createMouseMoveEvent(position, delta, modifiers, timestamp);
    YUCHEN_ASSERT(event.isValid());
    pushEvent(event);
}

void Win32EventManager::handleMouseWheelEvent(UINT msg, WPARAM wParam, LPARAM lParam)
{
    Vec2 position = m_mouseTracker.getPosition();

    int delta = GET_WHEEL_DELTA_WPARAM(wParam);
    float scrollDelta = static_cast<float>(delta) / WHEEL_DELTA;

    Vec2 scrollVec = (msg == WM_MOUSEWHEEL) ?
        Vec2(0.0f, scrollDelta) :
        Vec2(scrollDelta, 0.0f);

    KeyModifiers modifiers = extractModifiers();
    double timestamp = getCurrentTime();

    Event event = Event::createMouseScrollEvent(position, scrollVec, modifiers, timestamp);
    YUCHEN_ASSERT(event.isValid());
    pushEvent(event);
}

// MARK: - Text Input Event Handling

void Win32EventManager::handleCharEvent(WPARAM wParam)
{
    uint32_t codepoint = static_cast<uint32_t>(wParam);

    if (codepoint < 32 || (codepoint >= 127 && codepoint < 160))
    {
        return;
    }

    double timestamp = getCurrentTime();
    Event event = Event::createTextInputEvent(codepoint, timestamp);
    YUCHEN_ASSERT(event.isValid());
    pushEvent(event);
}

// MARK: - IME Event Handling

void Win32EventManager::handleImeStartComposition()
{
    m_imeComposing = true;
    m_imeCompositionBuffer.clear();
}

void Win32EventManager::handleImeComposition(LPARAM lParam)
{
    HIMC hImc = ImmGetContext(m_hwnd);
    if (!hImc) return;

    double timestamp = getCurrentTime();

    if (lParam & GCS_COMPSTR)
    {
        LONG len = ImmGetCompositionStringW(hImc, GCS_COMPSTR, NULL, 0);

        if (len > 0)
        {
            std::vector<wchar_t> wbuf(len / sizeof(wchar_t) + 1);
            ImmGetCompositionStringW(hImc, GCS_COMPSTR, wbuf.data(), len);
            wbuf[len / sizeof(wchar_t)] = 0;

            int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wbuf.data(), -1, NULL, 0, NULL, NULL);
            m_imeCompositionBuffer.resize(utf8Len);
            WideCharToMultiByte(CP_UTF8, 0, wbuf.data(), -1, m_imeCompositionBuffer.data(), utf8Len, NULL, NULL);

            LONG cursorPos = ImmGetCompositionStringW(hImc, GCS_CURSORPOS, NULL, 0);
            if (cursorPos < 0) cursorPos = 0;

            Event event = Event::createTextCompositionEvent(
                m_imeCompositionBuffer.data(),
                static_cast<int>(cursorPos),
                0,
                timestamp
            );
            pushEvent(event);
        }
    }

    if (lParam & GCS_RESULTSTR)
    {
        LONG len = ImmGetCompositionStringW(hImc, GCS_RESULTSTR, NULL, 0);

        if (len > 0)
        {
            std::vector<wchar_t> wbuf(len / sizeof(wchar_t) + 1);
            ImmGetCompositionStringW(hImc, GCS_RESULTSTR, wbuf.data(), len);
            wbuf[len / sizeof(wchar_t)] = 0;

            for (size_t i = 0; i < wbuf.size() && wbuf[i] != 0; ++i)
            {
                wchar_t wc = wbuf[i];

                if (wc >= 0xD800 && wc <= 0xDBFF)
                {
                    if (i + 1 < wbuf.size())
                    {
                        wchar_t low = wbuf[i + 1];
                        if (low >= 0xDC00 && low <= 0xDFFF)
                        {
                            uint32_t codepoint = 0x10000 + ((wc - 0xD800) << 10) + (low - 0xDC00);
                            Event event = Event::createTextInputEvent(codepoint, timestamp);
                            pushEvent(event);
                            ++i;
                            continue;
                        }
                    }
                }

                Event event = Event::createTextInputEvent(static_cast<uint32_t>(wc), timestamp);
                pushEvent(event);
            }

            LONG compLen = ImmGetCompositionStringW(hImc, GCS_COMPSTR, NULL, 0);
            if (compLen <= 0)
            {
                Event clearEvent = Event::createTextCompositionEvent("", 0, 0, timestamp);
                pushEvent(clearEvent);
            }
        }
    }

    ImmReleaseContext(m_hwnd, hImc);
}

void Win32EventManager::handleImeEndComposition()
{
    m_imeComposing = false;
    m_imeCompositionBuffer.clear();

    double timestamp = getCurrentTime();
    Event event = Event::createTextCompositionEvent("", 0, 0, timestamp);
    pushEvent(event);
}

void Win32EventManager::handleImeNotify(WPARAM wParam)
{
}

// MARK: - Mapping Functions

KeyCode Win32EventManager::mapVirtualKey(WPARAM vk) const
{
    switch (vk)
    {
    case 'A': return KeyCode::A;
    case 'B': return KeyCode::B;
    case 'C': return KeyCode::C;
    case 'D': return KeyCode::D;
    case 'E': return KeyCode::E;
    case 'F': return KeyCode::F;
    case 'G': return KeyCode::G;
    case 'H': return KeyCode::H;
    case 'I': return KeyCode::I;
    case 'J': return KeyCode::J;
    case 'K': return KeyCode::K;
    case 'L': return KeyCode::L;
    case 'M': return KeyCode::M;
    case 'N': return KeyCode::N;
    case 'O': return KeyCode::O;
    case 'P': return KeyCode::P;
    case 'Q': return KeyCode::Q;
    case 'R': return KeyCode::R;
    case 'S': return KeyCode::S;
    case 'T': return KeyCode::T;
    case 'U': return KeyCode::U;
    case 'V': return KeyCode::V;
    case 'W': return KeyCode::W;
    case 'X': return KeyCode::X;
    case 'Y': return KeyCode::Y;
    case 'Z': return KeyCode::Z;

    case '0': return KeyCode::Num0;
    case '1': return KeyCode::Num1;
    case '2': return KeyCode::Num2;
    case '3': return KeyCode::Num3;
    case '4': return KeyCode::Num4;
    case '5': return KeyCode::Num5;
    case '6': return KeyCode::Num6;
    case '7': return KeyCode::Num7;
    case '8': return KeyCode::Num8;
    case '9': return KeyCode::Num9;

    case VK_F1: return KeyCode::F1;
    case VK_F2: return KeyCode::F2;
    case VK_F3: return KeyCode::F3;
    case VK_F4: return KeyCode::F4;
    case VK_F5: return KeyCode::F5;
    case VK_F6: return KeyCode::F6;
    case VK_F7: return KeyCode::F7;
    case VK_F8: return KeyCode::F8;
    case VK_F9: return KeyCode::F9;
    case VK_F10: return KeyCode::F10;
    case VK_F11: return KeyCode::F11;
    case VK_F12: return KeyCode::F12;

    case VK_RETURN: return KeyCode::Return;
    case VK_TAB: return KeyCode::Tab;
    case VK_SPACE: return KeyCode::Space;
    case VK_BACK: return KeyCode::Backspace;
    case VK_ESCAPE: return KeyCode::Escape;
    case VK_DELETE: return KeyCode::Delete;

    case VK_LEFT: return KeyCode::LeftArrow;
    case VK_RIGHT: return KeyCode::RightArrow;
    case VK_UP: return KeyCode::UpArrow;
    case VK_DOWN: return KeyCode::DownArrow;

    case VK_SHIFT: return KeyCode::LeftShift;
    case VK_CONTROL: return KeyCode::LeftControl;
    case VK_MENU: return KeyCode::LeftAlt;

    case VK_HOME: return KeyCode::Home;
    case VK_END: return KeyCode::End;
    case VK_PRIOR: return KeyCode::PageUp;
    case VK_NEXT: return KeyCode::PageDown;

    default: return KeyCode::Unknown;
    }
}

MouseButton Win32EventManager::mapMouseButton(UINT msg) const
{
    switch (msg)
    {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        return MouseButton::Left;
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        return MouseButton::Right;
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
        return MouseButton::Middle;
    default:
        return MouseButton::Left;
    }
}

KeyModifiers Win32EventManager::extractModifiers() const
{
    KeyModifiers modifiers;
    modifiers.leftShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    modifiers.leftControl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    modifiers.leftAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
    modifiers.capsLock = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
    return modifiers;
}

Vec2 Win32EventManager::getMousePositionFromLParam(LPARAM lParam) const
{
    int x = GET_X_LPARAM(lParam);
    int y = GET_Y_LPARAM(lParam);
    return Vec2(static_cast<float>(x), static_cast<float>(y));
}

// MARK: - Utility Functions

double Win32EventManager::getCurrentTime() const
{
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration<double>(duration).count();
}

void Win32EventManager::pushEvent(const Event& event)
{
    YUCHEN_ASSERT(event.isValid());

    if (m_eventQueue.isFull())
    {
        Event discarded;
        m_eventQueue.pop(discarded);
    }

    bool success = m_eventQueue.push(event);
    YUCHEN_ASSERT(success);

    if (m_eventCallback)
    {
        m_eventCallback(event);
    }
}

}
