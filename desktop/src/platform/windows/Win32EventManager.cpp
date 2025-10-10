/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Events module (Windows).
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

//==========================================================================================
/** @file Win32EventManager.cpp
    
    Implementation notes:
    - Virtual key codes are mapped to YuchenUI KeyCode enum
    - Mouse position is extracted from LPARAM using GET_X_LPARAM/GET_Y_LPARAM macros
    - IME composition text is stored in UTF-8 format
    - Text input filters out control characters (< 32, 127-159)
    - Wheel delta is normalized (WHEEL_DELTA = 120 clicks)
    - Event queue uses circular buffer with overflow protection
*/

#include "Win32EventManager.h"
#include <chrono>
#include <windowsx.h>

namespace YuchenUI {

//==========================================================================================
// Construction and Destruction

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

//==========================================================================================
// Initialization

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

//==========================================================================================
// Event Queue Management

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

//==========================================================================================
// Event Callback

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

//==========================================================================================
// Native Event Handling

void Win32EventManager::handleNativeEvent(void* event)
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "Not initialized");
    YUCHEN_ASSERT(event);

    MSG* msg = reinterpret_cast<MSG*>(event);
    handleWindowsMessage(msg->message, msg->wParam, msg->lParam);
}

//==========================================================================================
// Input State Queries

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

//==========================================================================================
// Text Input Control

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

//==========================================================================================
// Message Dispatching

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
        // Only process character input when text input is enabled and not composing
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

//==========================================================================================
// Keyboard Event Handling

void Win32EventManager::handleKeyEvent(UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Map virtual key to YuchenUI key code
    KeyCode key = mapVirtualKey(wParam);
    if (key == KeyCode::Unknown) return;

    // Determine if this is a press or release
    bool pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
    // Check if this is a repeat (bit 30 of lParam indicates previous key state)
    bool isRepeat = (lParam & 0x40000000) != 0;

    // Update key state tracker
    m_keyTracker.setKeyState(key, pressed);

    // Create key event
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

//==========================================================================================
// Mouse Event Handling

void Win32EventManager::handleMouseButtonEvent(UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Determine which button and whether pressed or released
    MouseButton button = mapMouseButton(msg);
    Vec2 position = getMousePositionFromLParam(lParam);
    bool pressed = (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN);

    // Update mouse state tracker
    m_mouseTracker.setButtonState(button, pressed);
    m_mouseTracker.setPosition(position);

    // Create mouse button event
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
    // Calculate movement delta
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

    // Get wheel delta (typically +/-120 per notch)
    int delta = GET_WHEEL_DELTA_WPARAM(wParam);
    // Normalize to notches (1.0 = one notch)
    float scrollDelta = static_cast<float>(delta) / WHEEL_DELTA;

    // WM_MOUSEWHEEL is vertical, WM_MOUSEHWHEEL is horizontal
    Vec2 scrollVec = (msg == WM_MOUSEWHEEL) ?
        Vec2(0.0f, scrollDelta) :   // Vertical scroll
        Vec2(scrollDelta, 0.0f);    // Horizontal scroll

    KeyModifiers modifiers = extractModifiers();
    double timestamp = getCurrentTime();

    Event event = Event::createMouseScrollEvent(position, scrollVec, modifiers, timestamp);
    YUCHEN_ASSERT(event.isValid());
    pushEvent(event);
}

//==========================================================================================
// Text Input Event Handling

void Win32EventManager::handleCharEvent(WPARAM wParam)
{
    uint32_t codepoint = static_cast<uint32_t>(wParam);

    // Filter out control characters
    // Skip characters in ranges: 0-31 (control chars) and 127-159 (extended control)
    if (codepoint < 32 || (codepoint >= 127 && codepoint < 160))
    {
        return;
    }

    double timestamp = getCurrentTime();
    Event event = Event::createTextInputEvent(codepoint, timestamp);
    YUCHEN_ASSERT(event.isValid());
    pushEvent(event);
}

//==========================================================================================
// IME Event Handling

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

    // Handle composition string (partially entered text)
    if (lParam & GCS_COMPSTR)
    {
        // Get length of composition string
        LONG len = ImmGetCompositionStringW(hImc, GCS_COMPSTR, NULL, 0);

        if (len > 0)
        {
            // Read composition string as UTF-16
            std::vector<wchar_t> wbuf(len / sizeof(wchar_t) + 1);
            ImmGetCompositionStringW(hImc, GCS_COMPSTR, wbuf.data(), len);
            wbuf[len / sizeof(wchar_t)] = 0;

            // Convert to UTF-8
            int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wbuf.data(), -1, NULL, 0, NULL, NULL);
            m_imeCompositionBuffer.resize(utf8Len);
            WideCharToMultiByte(CP_UTF8, 0, wbuf.data(), -1, m_imeCompositionBuffer.data(), utf8Len, NULL, NULL);

            // Get cursor position within composition string
            LONG cursorPos = ImmGetCompositionStringW(hImc, GCS_CURSORPOS, NULL, 0);
            if (cursorPos < 0) cursorPos = 0;

            // Create text composition event
            Event event = Event::createTextCompositionEvent(
                m_imeCompositionBuffer.data(),
                static_cast<int>(cursorPos),
                0,  // selectionLength (not used in current implementation)
                timestamp
            );
            pushEvent(event);
        }
    }

    // Handle result string (finalized text)
    if (lParam & GCS_RESULTSTR)
    {
        // Get length of result string
        LONG len = ImmGetCompositionStringW(hImc, GCS_RESULTSTR, NULL, 0);

        if (len > 0)
        {
            // Read result string as UTF-16
            std::vector<wchar_t> wbuf(len / sizeof(wchar_t) + 1);
            ImmGetCompositionStringW(hImc, GCS_RESULTSTR, wbuf.data(), len);
            wbuf[len / sizeof(wchar_t)] = 0;

            // Convert each character to text input events
            for (size_t i = 0; i < wbuf.size() && wbuf[i] != 0; ++i)
            {
                wchar_t wc = wbuf[i];

                // Handle surrogate pairs for characters outside Basic Multilingual Plane
                if (wc >= 0xD800 && wc <= 0xDBFF)  // High surrogate
                {
                    if (i + 1 < wbuf.size())
                    {
                        wchar_t low = wbuf[i + 1];
                        if (low >= 0xDC00 && low <= 0xDFFF)  // Low surrogate
                        {
                            // Combine surrogates to get full codepoint
                            uint32_t codepoint = 0x10000 + ((wc - 0xD800) << 10) + (low - 0xDC00);
                            Event event = Event::createTextInputEvent(codepoint, timestamp);
                            pushEvent(event);
                            ++i;  // Skip the low surrogate
                            continue;
                        }
                    }
                }

                // Regular character (BMP)
                Event event = Event::createTextInputEvent(static_cast<uint32_t>(wc), timestamp);
                pushEvent(event);
            }

            // Clear composition display if no more composition string
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

    // Send empty composition event to clear any displayed composition
    double timestamp = getCurrentTime();
    Event event = Event::createTextCompositionEvent("", 0, 0, timestamp);
    pushEvent(event);
}

void Win32EventManager::handleImeNotify(WPARAM wParam)
{
    // IME notification handler
    // Currently not processing specific notifications
    // This could be extended for advanced IME features like candidate list changes
}

//==========================================================================================
// Mapping Functions

KeyCode Win32EventManager::mapVirtualKey(WPARAM vk) const
{
    // Map Windows virtual key codes to YuchenUI KeyCode enum
    switch (vk)
    {
    // Letter keys
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

    // Number keys
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

    // Function keys
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

    // Special keys
    case VK_RETURN: return KeyCode::Return;
    case VK_TAB: return KeyCode::Tab;
    case VK_SPACE: return KeyCode::Space;
    case VK_BACK: return KeyCode::Backspace;
    case VK_ESCAPE: return KeyCode::Escape;
    case VK_DELETE: return KeyCode::Delete;

    // Arrow keys
    case VK_LEFT: return KeyCode::LeftArrow;
    case VK_RIGHT: return KeyCode::RightArrow;
    case VK_UP: return KeyCode::UpArrow;
    case VK_DOWN: return KeyCode::DownArrow;

    // Modifier keys (note: Left/Right distinction not always available)
    case VK_SHIFT: return KeyCode::LeftShift;
    case VK_CONTROL: return KeyCode::LeftControl;
    case VK_MENU: return KeyCode::LeftAlt;  // VK_MENU is Alt key

    // Navigation keys
    case VK_HOME: return KeyCode::Home;
    case VK_END: return KeyCode::End;
    case VK_PRIOR: return KeyCode::PageUp;   // VK_PRIOR is Page Up
    case VK_NEXT: return KeyCode::PageDown;  // VK_NEXT is Page Down

    default: return KeyCode::Unknown;
    }
}

MouseButton Win32EventManager::mapMouseButton(UINT msg) const
{
    // Map Windows mouse messages to MouseButton enum
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
    // Query current modifier key states
    KeyModifiers modifiers;
    // GetKeyState returns high-order bit set if key is down
    modifiers.leftShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    modifiers.leftControl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    modifiers.leftAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
    // Low-order bit indicates toggle state (on/off) for Caps Lock
    modifiers.capsLock = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
    return modifiers;
}

Vec2 Win32EventManager::getMousePositionFromLParam(LPARAM lParam) const
{
    // Extract mouse coordinates from LPARAM
    // Windows packs X in low word, Y in high word
    int x = GET_X_LPARAM(lParam);
    int y = GET_Y_LPARAM(lParam);
    return Vec2(static_cast<float>(x), static_cast<float>(y));
}

//==========================================================================================
// Utility Functions

double Win32EventManager::getCurrentTime() const
{
    // Get high-resolution timestamp
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration<double>(duration).count();
}

void Win32EventManager::pushEvent(const Event& event)
{
    YUCHEN_ASSERT(event.isValid());

    // If queue is full, discard oldest event to make room
    if (m_eventQueue.isFull())
    {
        Event discarded;
        m_eventQueue.pop(discarded);
    }

    bool success = m_eventQueue.push(event);
    YUCHEN_ASSERT(success);

    // Invoke callback if set
    if (m_eventCallback)
    {
        m_eventCallback(event);
    }
}

}
