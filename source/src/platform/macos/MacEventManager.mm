#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#include "MacEventManager.h"

namespace YuchenUI
{

const MacKeyCodeMapper::KeyMapping MacKeyCodeMapper::s_keyMappings[] =
{
    {kVK_ANSI_A,                KeyCode::A},
    {kVK_ANSI_S,                KeyCode::S},
    {kVK_ANSI_D,                KeyCode::D},
    {kVK_ANSI_F,                KeyCode::F},
    {kVK_ANSI_H,                KeyCode::H},
    {kVK_ANSI_G,                KeyCode::G},
    {kVK_ANSI_Z,                KeyCode::Z},
    {kVK_ANSI_X,                KeyCode::X},
    {kVK_ANSI_C,                KeyCode::C},
    {kVK_ANSI_V,                KeyCode::V},
    {kVK_ANSI_B,                KeyCode::B},
    {kVK_ANSI_Q,                KeyCode::Q},
    {kVK_ANSI_W,                KeyCode::W},
    {kVK_ANSI_E,                KeyCode::E},
    {kVK_ANSI_R,                KeyCode::R},
    {kVK_ANSI_Y,                KeyCode::Y},
    {kVK_ANSI_T,                KeyCode::T},
    {kVK_ANSI_1,                KeyCode::Num1},
    {kVK_ANSI_2,                KeyCode::Num2},
    {kVK_ANSI_3,                KeyCode::Num3},
    {kVK_ANSI_4,                KeyCode::Num4},
    {kVK_ANSI_6,                KeyCode::Num6},
    {kVK_ANSI_5,                KeyCode::Num5},
    {kVK_ANSI_Equal,            KeyCode::Equal},
    {kVK_ANSI_9,                KeyCode::Num9},
    {kVK_ANSI_7,                KeyCode::Num7},
    {kVK_ANSI_Minus,            KeyCode::Minus},
    {kVK_ANSI_8,                KeyCode::Num8},
    {kVK_ANSI_0,                KeyCode::Num0},
    {kVK_ANSI_RightBracket,     KeyCode::RightBracket},
    {kVK_ANSI_O,                KeyCode::O},
    {kVK_ANSI_U,                KeyCode::U},
    {kVK_ANSI_LeftBracket,      KeyCode::LeftBracket},
    {kVK_ANSI_I,                KeyCode::I},
    {kVK_ANSI_P,                KeyCode::P},
    {kVK_Return,                KeyCode::Return},
    {kVK_ANSI_L,                KeyCode::L},
    {kVK_ANSI_J,                KeyCode::J},
    {kVK_ANSI_Quote,            KeyCode::Quote},
    {kVK_ANSI_K,                KeyCode::K},
    {kVK_ANSI_Semicolon,        KeyCode::Semicolon},
    {kVK_ANSI_Backslash,        KeyCode::Backslash},
    {kVK_ANSI_Comma,            KeyCode::Comma},
    {kVK_ANSI_Slash,            KeyCode::Slash},
    {kVK_ANSI_N,                KeyCode::N},
    {kVK_ANSI_M,                KeyCode::M},
    {kVK_ANSI_Period,           KeyCode::Period},
    {kVK_Tab,                   KeyCode::Tab},
    {kVK_Space,                 KeyCode::Space},
    {kVK_ANSI_Grave,            KeyCode::Grave},
    {kVK_Delete,                KeyCode::Backspace},
    {kVK_Escape,                KeyCode::Escape},
    {kVK_Command,               KeyCode::LeftCommand},
    {kVK_Shift,                 KeyCode::LeftShift},
    {kVK_CapsLock,              KeyCode::CapsLock},
    {kVK_Option,                KeyCode::LeftAlt},
    {kVK_Control,               KeyCode::LeftControl},
    {kVK_RightShift,            KeyCode::RightShift},
    {kVK_RightOption,           KeyCode::RightAlt},
    {kVK_RightControl,          KeyCode::RightControl},
    {kVK_Function,              KeyCode::Function},
    {kVK_F17,                   KeyCode::F17},
    {kVK_ANSI_KeypadDecimal,    KeyCode::KeypadDecimal},
    {kVK_ANSI_KeypadMultiply,   KeyCode::KeypadMultiply},
    {kVK_ANSI_KeypadPlus,       KeyCode::KeypadPlus},
    {kVK_ANSI_KeypadClear,      KeyCode::KeypadClear},
    {kVK_VolumeUp,              KeyCode::VolumeUp},
    {kVK_VolumeDown,            KeyCode::VolumeDown},
    {kVK_Mute,                  KeyCode::Mute},
    {kVK_ANSI_KeypadDivide,     KeyCode::KeypadDivide},
    {kVK_ANSI_KeypadEnter,      KeyCode::KeypadEnter},
    {kVK_ANSI_KeypadMinus,      KeyCode::KeypadMinus},
    {kVK_F18,                   KeyCode::F18},
    {kVK_F19,                   KeyCode::F19},
    {kVK_ANSI_KeypadEquals,     KeyCode::KeypadEquals},
    {kVK_ANSI_Keypad0,          KeyCode::Keypad0},
    {kVK_ANSI_Keypad1,          KeyCode::Keypad1},
    {kVK_ANSI_Keypad2,          KeyCode::Keypad2},
    {kVK_ANSI_Keypad3,          KeyCode::Keypad3},
    {kVK_ANSI_Keypad4,          KeyCode::Keypad4},
    {kVK_ANSI_Keypad5,          KeyCode::Keypad5},
    {kVK_ANSI_Keypad6,          KeyCode::Keypad6},
    {kVK_ANSI_Keypad7,          KeyCode::Keypad7},
    {kVK_F20,                   KeyCode::F20},
    {kVK_ANSI_Keypad8,          KeyCode::Keypad8},
    {kVK_ANSI_Keypad9,          KeyCode::Keypad9},
    {kVK_F5,                    KeyCode::F5},
    {kVK_F6,                    KeyCode::F6},
    {kVK_F7,                    KeyCode::F7},
    {kVK_F3,                    KeyCode::F3},
    {kVK_F8,                    KeyCode::F8},
    {kVK_F9,                    KeyCode::F9},
    {kVK_F11,                   KeyCode::F11},
    {kVK_F13,                   KeyCode::F13},
    {kVK_F16,                   KeyCode::F16},
    {kVK_F14,                   KeyCode::F14},
    {kVK_F10,                   KeyCode::F10},
    {kVK_F12,                   KeyCode::F12},
    {kVK_F15,                   KeyCode::F15},
    {kVK_Help,                  KeyCode::Insert},
    {kVK_Home,                  KeyCode::Home},
    {kVK_PageUp,                KeyCode::PageUp},
    {kVK_ForwardDelete,         KeyCode::Delete},
    {kVK_F4,                    KeyCode::F4},
    {kVK_End,                   KeyCode::End},
    {kVK_F2,                    KeyCode::F2},
    {kVK_PageDown,              KeyCode::PageDown},
    {kVK_F1,                    KeyCode::F1},
    {kVK_LeftArrow,             KeyCode::LeftArrow},
    {kVK_RightArrow,            KeyCode::RightArrow},
    {kVK_DownArrow,             KeyCode::DownArrow},
    {kVK_UpArrow,               KeyCode::UpArrow}
};

KeyCode MacKeyCodeMapper::mapKeyCode(unsigned short macKeyCode)
{
    constexpr size_t actualCount = sizeof(s_keyMappings) / sizeof(s_keyMappings[0]);
    for (size_t i = 0; i < actualCount; ++i)
    {
        if (s_keyMappings[i].macKeyCode == macKeyCode)
            return s_keyMappings[i].yuchenKeyCode;
    }
    return KeyCode::Unknown;
}

// MARK: - Lifecycle
MacEventManager::MacEventManager(NSWindow* window)
    : m_window(window)
    , m_eventQueue()
    , m_eventCallback(nullptr)
    , m_keyTracker()
    , m_mouseTracker()
    , m_isInitialized(false)
    , m_textInputEnabled(false)
    , m_markedText{0}
    , m_markedCursorPos(0)
    , m_markedSelectionLength(0)
{
}

MacEventManager::~MacEventManager()
{
    destroy();
}

bool MacEventManager::initialize()
{
    YUCHEN_ASSERT_MSG(!m_isInitialized, "MacEventManager already initialized");
    
    m_eventQueue.clear();
    m_keyTracker.clear();
    m_mouseTracker.clear();
    m_isInitialized = true;
    return true;
}

void MacEventManager::destroy()
{
    if (!m_isInitialized) return;
    
    clearEventCallback();
    clearEvents();
    
    m_keyTracker.clear();
    m_mouseTracker.clear();
    
    m_isInitialized = false;
}

bool MacEventManager::isInitialized() const
{
    return m_isInitialized;
}

// MARK: - Event Queue
bool MacEventManager::hasEvents() const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "MacEventManager not initialized");
    return !m_eventQueue.isEmpty();
}

Event MacEventManager::getNextEvent()
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "MacEventManager not initialized");
    YUCHEN_ASSERT_MSG(hasEvents(), "No events available");
    
    Event event;
    bool success = m_eventQueue.pop(event);
    (void)success;
    YUCHEN_ASSERT(success);
    YUCHEN_ASSERT(event.isValid());
    
    return event;
}

void MacEventManager::clearEvents()
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "MacEventManager not initialized");
    m_eventQueue.clear();
}

size_t MacEventManager::getEventCount() const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "MacEventManager not initialized");
    return m_eventQueue.size();
}

// MARK: - Event Callback
void MacEventManager::setEventCallback(EventCallback callback)
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "MacEventManager not initialized");
    m_eventCallback = callback;
}

void MacEventManager::clearEventCallback()
{
    m_eventCallback = nullptr;
}

bool MacEventManager::hasEventCallback() const
{
    return m_eventCallback != nullptr;
}

// MARK: - State Query
bool MacEventManager::isKeyPressed(KeyCode key) const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "MacEventManager not initialized");
    return m_keyTracker.isKeyPressed(key);
}

bool MacEventManager::isMouseButtonPressed(MouseButton button) const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "MacEventManager not initialized");
    return m_mouseTracker.isButtonPressed(button);
}

Vec2 MacEventManager::getMousePosition() const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "MacEventManager not initialized");
    return m_mouseTracker.getPosition();
}

KeyModifiers MacEventManager::getCurrentModifiers() const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "MacEventManager not initialized");
    
    KeyModifiers modifiers;
    modifiers.leftShift = m_keyTracker.isKeyPressed(KeyCode::LeftShift);
    modifiers.rightShift = m_keyTracker.isKeyPressed(KeyCode::RightShift);
    modifiers.leftControl = m_keyTracker.isKeyPressed(KeyCode::LeftControl);
    modifiers.rightControl = m_keyTracker.isKeyPressed(KeyCode::RightControl);
    modifiers.leftAlt = m_keyTracker.isKeyPressed(KeyCode::LeftAlt);
    modifiers.rightAlt = m_keyTracker.isKeyPressed(KeyCode::RightAlt);
    modifiers.leftCommand = m_keyTracker.isKeyPressed(KeyCode::LeftCommand);
    modifiers.rightCommand = m_keyTracker.isKeyPressed(KeyCode::RightCommand);
    modifiers.capsLock = m_keyTracker.isKeyPressed(KeyCode::CapsLock);
    modifiers.numLock = m_keyTracker.isKeyPressed(KeyCode::NumLock);
    modifiers.function = m_keyTracker.isKeyPressed(KeyCode::Function);
    
    return modifiers;
}

// MARK: - Text Input
void MacEventManager::enableTextInput()
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "MacEventManager not initialized");
    m_textInputEnabled = true;
}

void MacEventManager::disableTextInput()
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "MacEventManager not initialized");
    m_textInputEnabled = false;
}

bool MacEventManager::isTextInputEnabled() const
{
    return m_textInputEnabled;
}

// MARK: - Native Event Handling
void MacEventManager::handleNSEvent(NSEvent* event)
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "MacEventManager not initialized");
    
    @autoreleasepool
    {
        NSEventType eventType = [event type];
        
        switch (eventType)
        {
            case NSEventTypeKeyDown:
                handleKeyEvent(event, EventType::KeyPressed);
                break;
            case NSEventTypeKeyUp:
                handleKeyEvent(event, EventType::KeyReleased);
                break;
            case NSEventTypeLeftMouseDown:
                handleMouseButtonEvent(event, EventType::MouseButtonPressed);
                break;
            case NSEventTypeLeftMouseUp:
                handleMouseButtonEvent(event, EventType::MouseButtonReleased);
                break;
            case NSEventTypeRightMouseDown:
                handleMouseButtonEvent(event, EventType::MouseButtonPressed);
                break;
            case NSEventTypeRightMouseUp:
                handleMouseButtonEvent(event, EventType::MouseButtonReleased);
                break;
            case NSEventTypeOtherMouseDown:
                handleMouseButtonEvent(event, EventType::MouseButtonPressed);
                break;
            case NSEventTypeOtherMouseUp:
                handleMouseButtonEvent(event, EventType::MouseButtonReleased);
                break;
            case NSEventTypeMouseMoved:
            case NSEventTypeLeftMouseDragged:
            case NSEventTypeRightMouseDragged:
            case NSEventTypeOtherMouseDragged:
                handleMouseMoveEvent(event);
                break;
            case NSEventTypeScrollWheel:
                handleMouseScrollEvent(event);
                break;
            case NSEventTypeFlagsChanged:
                handleModifierFlagsEvent(event);
                break;
            default:
                break;
        }
    }
}

void MacEventManager::handleNativeEvent(void* event)
{
    handleNSEvent((__bridge NSEvent*)event);
}

// MARK: - Event Handlers
void MacEventManager::handleKeyEvent(NSEvent* event, EventType eventType)
{
    YUCHEN_ASSERT(eventType == EventType::KeyPressed || eventType == EventType::KeyReleased);
    
    @autoreleasepool
    {
        unsigned short keyCode = [event keyCode];
        KeyCode mappedKey = mapKeyCode(keyCode);
        if (mappedKey == KeyCode::Unknown) return;
        
        KeyModifiers modifiers = extractModifiers([event modifierFlags]);
        bool isRepeat = (eventType == EventType::KeyPressed) ? [event isARepeat] : false;
        double timestamp = getCurrentTime();
        
        bool pressed = (eventType == EventType::KeyPressed);
        m_keyTracker.setKeyState(mappedKey, pressed);
        
        Event keyEvent = Event::createKeyEvent(eventType, mappedKey, keyCode, modifiers, isRepeat, timestamp);
        YUCHEN_ASSERT(keyEvent.isValid());
        pushEvent(keyEvent);
    }
}

void MacEventManager::handleTextInputEvent(NSEvent* event)
{
    @autoreleasepool
    {
        uint32_t codepoint = extractUnicodeFromNSEvent(event);
        if (codepoint == 0 || codepoint > 0x10FFFF) return;
        
        double timestamp = getCurrentTime();
        Event textEvent = Event::createTextInputEvent(codepoint, timestamp);
        YUCHEN_ASSERT(textEvent.isValid());
        pushEvent(textEvent);
    }
}

void MacEventManager::handleMouseButtonEvent(NSEvent* event, EventType eventType)
{
    YUCHEN_ASSERT(eventType == EventType::MouseButtonPressed || eventType == EventType::MouseButtonReleased);
    
    @autoreleasepool
    {
        int buttonNumber = (int)[event buttonNumber];
        MouseButton mappedButton = mapMouseButton(buttonNumber);
        Vec2 position = convertMousePosition(event);
        
        if (!position.isValid()) position = Vec2(0.0f, 0.0f);
        
        NSInteger nsClickCount = [event clickCount];
        uint8_t clickCount = (nsClickCount > 0 && nsClickCount <= 255) ? (uint8_t)nsClickCount : 1;
        KeyModifiers modifiers = extractModifiers([event modifierFlags]);
        double timestamp = getCurrentTime();
        
        bool pressed = (eventType == EventType::MouseButtonPressed);
        m_mouseTracker.setButtonState(mappedButton, pressed);
        m_mouseTracker.setPosition(position);
        
        Event mouseEvent = Event::createMouseButtonEvent(eventType, mappedButton, position, clickCount, modifiers, timestamp);
        YUCHEN_ASSERT(mouseEvent.isValid());
        
        pushEvent(mouseEvent);
    }
}

void MacEventManager::handleMouseMoveEvent(NSEvent* event)
{
    @autoreleasepool
    {
        Vec2 position = convertMousePosition(event);
        if (!position.isValid()) return;
        
        Vec2 delta = Vec2([event deltaX], [event deltaY]);
        KeyModifiers modifiers = extractModifiers([event modifierFlags]);
        double timestamp = getCurrentTime();
        
        m_mouseTracker.setPosition(position);
        
        Event moveEvent = Event::createMouseMoveEvent(position, delta, modifiers, timestamp);
        YUCHEN_ASSERT(moveEvent.isValid());
        
        tryMergeMouseMove(moveEvent);
    }
}

void MacEventManager::handleMouseScrollEvent(NSEvent* event)
{
    @autoreleasepool
    {
        Vec2 position = convertMousePosition(event);
        Vec2 delta = Vec2([event scrollingDeltaX], [event scrollingDeltaY]);
        KeyModifiers modifiers = extractModifiers([event modifierFlags]);
        double timestamp = getCurrentTime();
        
        Event scrollEvent = Event::createMouseScrollEvent(position, delta, modifiers, timestamp);
        YUCHEN_ASSERT(scrollEvent.isValid());
        
        pushEvent(scrollEvent);
    }
}

void MacEventManager::handleModifierFlagsEvent(NSEvent* event)
{
    @autoreleasepool
    {
        unsigned long modifierFlags = [event modifierFlags];
        
        m_keyTracker.setKeyState(KeyCode::LeftShift, (modifierFlags & NSEventModifierFlagShift) != 0);
        m_keyTracker.setKeyState(KeyCode::LeftControl, (modifierFlags & NSEventModifierFlagControl) != 0);
        m_keyTracker.setKeyState(KeyCode::LeftAlt, (modifierFlags & NSEventModifierFlagOption) != 0);
        m_keyTracker.setKeyState(KeyCode::LeftCommand, (modifierFlags & NSEventModifierFlagCommand) != 0);
        m_keyTracker.setKeyState(KeyCode::CapsLock, (modifierFlags & NSEventModifierFlagCapsLock) != 0);
        m_keyTracker.setKeyState(KeyCode::NumLock, (modifierFlags & NSEventModifierFlagNumericPad) != 0);
        m_keyTracker.setKeyState(KeyCode::Function, (modifierFlags & NSEventModifierFlagFunction) != 0);
        
        KeyModifiers modifiers = extractModifiers(modifierFlags);
        double timestamp = getCurrentTime();
        
        Event modifierEvent = Event::createModifierFlagsEvent(modifiers, timestamp);
        pushEvent(modifierEvent);
    }
}

// MARK: - Mapping and Conversion
KeyCode MacEventManager::mapKeyCode(unsigned short keyCode) const
{
    return MacKeyCodeMapper::mapKeyCode(keyCode);
}

MouseButton MacEventManager::mapMouseButton(int buttonNumber) const
{
    switch (buttonNumber)
    {
        case 0: return MouseButton::Left;
        case 1: return MouseButton::Right;
        case 2: return MouseButton::Middle;
        case 3: return MouseButton::X1;
        case 4: return MouseButton::X2;
        default: return MouseButton::Left;
    }
}

KeyModifiers MacEventManager::extractModifiers(unsigned long modifierFlags) const
{
    KeyModifiers modifiers;
    modifiers.leftShift = (modifierFlags & NSEventModifierFlagShift) != 0;
    modifiers.leftControl = (modifierFlags & NSEventModifierFlagControl) != 0;
    modifiers.leftAlt = (modifierFlags & NSEventModifierFlagOption) != 0;
    modifiers.leftCommand = (modifierFlags & NSEventModifierFlagCommand) != 0;
    modifiers.capsLock = (modifierFlags & NSEventModifierFlagCapsLock) != 0;
    modifiers.numLock = (modifierFlags & NSEventModifierFlagNumericPad) != 0;
    modifiers.function = (modifierFlags & NSEventModifierFlagFunction) != 0;
    return modifiers;
}

uint32_t MacEventManager::extractUnicodeFromNSEvent(NSEvent* event) const
{
    @autoreleasepool
    {
        NSString* characters = [event characters];
        if (!characters || [characters length] == 0) return 0;
        unichar character = [characters characterAtIndex:0];
        if (character < 32 || (character >= 127 && character < 160)) return 0;
        return (uint32_t)character;
    }
}

Vec2 MacEventManager::convertMousePosition(NSEvent* event) const
{
    @autoreleasepool
    {
        NSPoint locationInWindow = [event locationInWindow];
        NSRect windowFrame = [m_window frame];
        NSRect contentRect = [m_window contentRectForFrameRect:windowFrame];
        
        float x = locationInWindow.x;
        float y = contentRect.size.height - locationInWindow.y;
        
        return Vec2(x, y);
    }
}

double MacEventManager::getCurrentTime() const
{
    return CACurrentMediaTime();
}

// MARK: - Event Queue Management
void MacEventManager::pushEvent(const Event& event)
{
    YUCHEN_ASSERT(event.isValid());
    
    if (m_eventQueue.isFull())
    {
        Event discarded;
        m_eventQueue.pop(discarded);
    }
    
    bool success = m_eventQueue.push(event);
    (void)success;
    YUCHEN_ASSERT(success);
    
    if (m_eventCallback)
    {
        m_eventCallback(event);
    }
}

void MacEventManager::tryMergeMouseMove(Event& event)
{
    YUCHEN_ASSERT(event.type == EventType::MouseMoved);
    
    if (m_eventQueue.isEmpty())
    {
        pushEvent(event);
        return;
    }
    
    Event lastEvent;
    if (!m_eventQueue.peek(lastEvent))
    {
        pushEvent(event);
        return;
    }
    
    if (lastEvent.type == EventType::MouseMoved)
    {
        Event temp;
        m_eventQueue.pop(temp);
        pushEvent(event);
    }
    else
    {
        pushEvent(event);
    }
}

// MARK: - IME Manager
void MacEventManager::handleMarkedText(const char* text, int cursorPos, int selectionLength)
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "MacEventManager not initialized");
    
    if (text) {
        strncpy(m_markedText, text, 255);
        m_markedText[255] = '\0';
    } else {
        m_markedText[0] = '\0';
    }
    
    m_markedCursorPos = cursorPos;
    m_markedSelectionLength = selectionLength;
    
    double timestamp = getCurrentTime();
    Event compositionEvent = Event::createTextCompositionEvent(m_markedText, cursorPos, selectionLength, timestamp);
    YUCHEN_ASSERT(compositionEvent.type == EventType::TextComposition);
    pushEvent(compositionEvent);
}

void MacEventManager::handleUnmarkText()
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "MacEventManager not initialized");
    
    if (m_markedText[0] != '\0') {
        m_markedText[0] = '\0';
        m_markedCursorPos = 0;
        m_markedSelectionLength = 0;
        
        double timestamp = getCurrentTime();
        Event compositionEvent = Event::createTextCompositionEvent("", 0, 0, timestamp);
        pushEvent(compositionEvent);
    }
}

} // namespace YuchenUI
