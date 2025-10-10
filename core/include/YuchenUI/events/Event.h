#pragma once

#include "YuchenUI/core/Types.h"
#include "YuchenUI/core/Assert.h"
#include <cstdint>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)
#endif

namespace YuchenUI {

enum class EventType : uint8_t {
    KeyPressed = 0,
    KeyReleased,
    TextInput,
    TextComposition,
    MouseButtonPressed,
    MouseButtonReleased,
    MouseMoved,
    MouseScrolled,
    WindowClosed,
    WindowResized,
    WindowFocusGained,
    WindowFocusLost,
    ModifierFlagsChanged
};

enum class KeyCode : uint16_t {
    Unknown = 0,
    
    A = 1, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    
    Num0 = 30, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    
    F1 = 50, F2, F3, F4, F5, F6, F7, F8, F9, F10,
    F11, F12, F13, F14, F15, F16, F17, F18, F19, F20,
    
    Keypad0 = 80, Keypad1, Keypad2, Keypad3, Keypad4,
    Keypad5, Keypad6, Keypad7, Keypad8, Keypad9,
    KeypadDecimal = 90,
    KeypadMultiply = 91,
    KeypadPlus = 92,
    KeypadClear = 93,
    KeypadDivide = 94,
    KeypadEnter = 95,
    KeypadMinus = 96,
    KeypadEquals = 97,
    
    Return = 100,
    Enter = 101,
    Tab = 102,
    Space = 103,
    Delete = 104,
    Escape = 105,
    Backspace = 106,
    PageUp = 107,
    PageDown = 108,
    End = 109,
    Home = 110,
    Insert = 111,
    
    LeftArrow = 120,
    RightArrow = 121,
    DownArrow = 122,
    UpArrow = 123,
    
    LeftShift = 130,
    RightShift = 131,
    LeftControl = 132,
    RightControl = 133,
    LeftAlt = 134,
    RightAlt = 135,
    LeftSuper = 136,
    RightSuper = 137,
    LeftCommand = 138,
    RightCommand = 139,
    
    CapsLock = 150,
    NumLock = 151,
    ScrollLock = 152,
    
    Semicolon = 160,
    Equal = 161,
    Comma = 162,
    Minus = 163,
    Period = 164,
    Slash = 165,
    Grave = 166,
    LeftBracket = 167,
    Backslash = 168,
    RightBracket = 169,
    Quote = 170,
    
    PrintScreen = 180,
    Pause = 181,
    Menu = 182,
    
    VolumeUp = 190,
    VolumeDown = 191,
    Mute = 192,
    
    Function = 200
};

enum class MouseButton : uint8_t {
    Left = 0,
    Right = 1,
    Middle = 2,
    X1 = 3,
    X2 = 4
};

struct KeyModifiers {
    bool leftShift : 1;
    bool rightShift : 1;
    bool leftControl : 1;
    bool rightControl : 1;
    bool leftAlt : 1;
    bool rightAlt : 1;
    bool leftSuper : 1;
    bool rightSuper : 1;
    bool leftCommand : 1;
    bool rightCommand : 1;
    bool capsLock : 1;
    bool numLock : 1;
    bool function : 1;
    
    KeyModifiers()
        : leftShift(false), rightShift(false)
        , leftControl(false), rightControl(false)
        , leftAlt(false), rightAlt(false)
        , leftSuper(false), rightSuper(false)
        , leftCommand(false), rightCommand(false)
        , capsLock(false), numLock(false)
        , function(false)
    {}
    
    bool hasShift() const { return leftShift || rightShift; }
    bool hasControl() const { return leftControl || rightControl; }
    bool hasAlt() const { return leftAlt || rightAlt; }
    bool hasSuper() const { return leftSuper || rightSuper; }
    bool hasCommand() const { return leftCommand || rightCommand; }
};

struct KeyEvent {
    KeyCode key;
    uint16_t platformKeyCode;
    KeyModifiers modifiers;
    bool isRepeat;
    
    KeyEvent() : key(KeyCode::Unknown), platformKeyCode(0), modifiers(), isRepeat(false) {}
    KeyEvent(KeyCode k, uint16_t platformCode, const KeyModifiers& mods, bool repeat)
        : key(k), platformKeyCode(platformCode), modifiers(mods), isRepeat(repeat) {}
    
    bool isValid() const { return key != KeyCode::Unknown; }
};

struct TextInputEvent {
    uint32_t codepoint;
    
    TextInputEvent() : codepoint(0) {}
    explicit TextInputEvent(uint32_t cp) : codepoint(cp) {}
    
    bool isValid() const { return codepoint > 0 && codepoint <= 0x10FFFF; }
};

struct TextCompositionEvent {
    char text[256];
    int cursorPosition;
    int selectionLength;
    
    TextCompositionEvent() : text{0}, cursorPosition(0), selectionLength(0) {}
    
    TextCompositionEvent(const char* t, int cursor, int length)
        : cursorPosition(cursor), selectionLength(length) {
        if (t) {
            strncpy(text, t, 255);
            text[255] = '\0';
        } else {
            text[0] = '\0';
        }
    }
    
    bool isValid() const { return text[0] != '\0'; }
};

struct MouseButtonEvent {
    MouseButton button;
    Vec2 position;
    uint8_t clickCount;
    KeyModifiers modifiers;
    
    MouseButtonEvent() : button(MouseButton::Left), position(), clickCount(1), modifiers() {}
    MouseButtonEvent(MouseButton btn, const Vec2& pos, uint8_t clicks, const KeyModifiers& mods)
        : button(btn), position(pos), clickCount(clicks), modifiers(mods) {}
    
    bool isValid() const { return position.isValid() && clickCount > 0; }
};

struct MouseMoveEvent {
    Vec2 position;
    Vec2 delta;
    KeyModifiers modifiers;
    
    MouseMoveEvent() : position(), delta(), modifiers() {}
    MouseMoveEvent(const Vec2& pos, const Vec2& d, const KeyModifiers& mods)
        : position(pos), delta(d), modifiers(mods) {}
    
    bool isValid() const { return position.isValid() && delta.isValid(); }
};

struct MouseScrollEvent {
    Vec2 position;
    Vec2 delta;
    KeyModifiers modifiers;
    
    MouseScrollEvent() : position(), delta(), modifiers() {}
    MouseScrollEvent(const Vec2& pos, const Vec2& d, const KeyModifiers& mods)
        : position(pos), delta(d), modifiers(mods) {}
    
    bool isValid() const { return position.isValid() && delta.isValid(); }
};

struct WindowEvent {
    Vec2 size;
    
    WindowEvent() : size() {}
    explicit WindowEvent(const Vec2& s) : size(s) {}
    
    bool isValid() const { return size.isValid() && size.x >= 0.0f && size.y >= 0.0f; }
};

struct ModifierFlagsEvent {
    KeyModifiers modifiers;
    
    ModifierFlagsEvent() : modifiers() {}
    explicit ModifierFlagsEvent(const KeyModifiers& mods) : modifiers(mods) {}
};

struct Event {
    EventType type;
    double timestamp;
    
    union {
        KeyEvent key;
        TextInputEvent textInput;
        TextCompositionEvent textComposition;
        MouseButtonEvent mouseButton;
        MouseMoveEvent mouseMove;
        MouseScrollEvent mouseScroll;
        WindowEvent window;
        ModifierFlagsEvent modifierFlags;
    };
    
    Event() : type(EventType::KeyPressed), timestamp(0.0) {
        key = KeyEvent();
    }
    
    static Event createKeyEvent(EventType type, KeyCode keyCode, uint16_t platformCode,
                               const KeyModifiers& modifiers, bool isRepeat, double ts) {
        YUCHEN_ASSERT(type == EventType::KeyPressed || type == EventType::KeyReleased);
        Event event;
        event.type = type;
        event.timestamp = ts;
        event.key = KeyEvent(keyCode, platformCode, modifiers, isRepeat);
        return event;
    }
    
    static Event createTextInputEvent(uint32_t codepoint, double ts) {
        Event event;
        event.type = EventType::TextInput;
        event.timestamp = ts;
        event.textInput = TextInputEvent(codepoint);
        return event;
    }
    
    static Event createTextCompositionEvent(const char* text, int cursor, int length, double ts) {
        Event event;
        event.type = EventType::TextComposition;
        event.timestamp = ts;
        event.textComposition = TextCompositionEvent(text, cursor, length);
        return event;
    }
    
    static Event createMouseButtonEvent(EventType type, MouseButton button, const Vec2& position,
                                       uint8_t clickCount, const KeyModifiers& modifiers, double ts) {
        YUCHEN_ASSERT(type == EventType::MouseButtonPressed || type == EventType::MouseButtonReleased);
        Event event;
        event.type = type;
        event.timestamp = ts;
        event.mouseButton = MouseButtonEvent(button, position, clickCount, modifiers);
        return event;
    }
    
    static Event createMouseMoveEvent(const Vec2& position, const Vec2& delta,
                                     const KeyModifiers& modifiers, double ts) {
        Event event;
        event.type = EventType::MouseMoved;
        event.timestamp = ts;
        event.mouseMove = MouseMoveEvent(position, delta, modifiers);
        return event;
    }
    
    static Event createMouseScrollEvent(const Vec2& position, const Vec2& delta,
                                       const KeyModifiers& modifiers, double ts) {
        Event event;
        event.type = EventType::MouseScrolled;
        event.timestamp = ts;
        event.mouseScroll = MouseScrollEvent(position, delta, modifiers);
        return event;
    }
    
    static Event createWindowEvent(EventType type, const Vec2& size, double ts) {
        YUCHEN_ASSERT(type == EventType::WindowClosed || type == EventType::WindowResized ||
                     type == EventType::WindowFocusGained || type == EventType::WindowFocusLost);
        Event event;
        event.type = type;
        event.timestamp = ts;
        if (type == EventType::WindowResized) {
            event.window = WindowEvent(size);
        } else {
            event.window = WindowEvent();
        }
        return event;
    }
    
    static Event createModifierFlagsEvent(const KeyModifiers& modifiers, double ts) {
        Event event;
        event.type = EventType::ModifierFlagsChanged;
        event.timestamp = ts;
        event.modifierFlags = ModifierFlagsEvent(modifiers);
        return event;
    }
    
    bool isValid() const {
        switch (type) {
            case EventType::KeyPressed:
            case EventType::KeyReleased:
                return key.isValid();
            case EventType::TextInput:
                return textInput.isValid();
            case EventType::TextComposition:
                            return true; 
            case EventType::MouseButtonPressed:
            case EventType::MouseButtonReleased:
                return mouseButton.isValid();
            case EventType::MouseMoved:
                return mouseMove.isValid();
            case EventType::MouseScrolled:
                return mouseScroll.isValid();
            case EventType::WindowResized:
                return window.isValid();
            case EventType::WindowClosed:
            case EventType::WindowFocusGained:
            case EventType::WindowFocusLost:
            case EventType::ModifierFlagsChanged:
                return true;
            default:
                return false;
        }
    }
};

}
