#pragma once

#include "YuchenUI/events/EventManager.h"
#include "YuchenUI/core/Assert.h"
#include <unordered_map>

#ifdef __OBJC__
@class NSWindow;
@class NSEvent;
#else
typedef void NSWindow;
typedef void NSEvent;
#endif

namespace YuchenUI {

class MacEventManager : public EventManager {
public:
    explicit MacEventManager(NSWindow* window);
    virtual ~MacEventManager();
    
    bool initialize() override;
    void destroy() override;
    bool isInitialized() const override;
    
    bool hasEvents() const override;
    Event getNextEvent() override;
    void clearEvents() override;
    size_t getEventCount() const override;
    
    void setEventCallback(EventCallback callback) override;
    void clearEventCallback() override;
    bool hasEventCallback() const override;
    
    bool isKeyPressed(KeyCode key) const override;
    bool isMouseButtonPressed(MouseButton button) const override;
    Vec2 getMousePosition() const override;
    KeyModifiers getCurrentModifiers() const override;
    
    void enableTextInput() override;
    void disableTextInput() override;
    bool isTextInputEnabled() const override;
    void handleMarkedText(const char* text, int cursorPos, int selectionLength) override;
    void handleUnmarkText() override;
    void pushEventDirect(const Event& event){ pushEvent(event); };
    void handleNativeEvent(void* event) override;
private:
    static constexpr size_t EVENT_QUEUE_SIZE = 512;
    void handleNSEvent(NSEvent* event);
    void handleKeyEvent(NSEvent* event, EventType eventType);
    void handleTextInputEvent(NSEvent* event);
    void handleMouseButtonEvent(NSEvent* event, EventType eventType);
    void handleMouseMoveEvent(NSEvent* event);
    void handleMouseScrollEvent(NSEvent* event);
    void handleModifierFlagsEvent(NSEvent* event);
    
    KeyCode mapKeyCode(unsigned short keyCode) const;
    MouseButton mapMouseButton(int buttonNumber) const;
    KeyModifiers extractModifiers(unsigned long modifierFlags) const;
    uint32_t extractUnicodeFromNSEvent(NSEvent* event) const;
    Vec2 convertMousePosition(NSEvent* event) const;
    double getCurrentTime() const;
    void pushEvent(const Event& event);
    void tryMergeMouseMove(Event& event);
    
    NSWindow* m_window;
    EventQueue<EVENT_QUEUE_SIZE> m_eventQueue;
    EventCallback m_eventCallback;
    KeyStateTracker m_keyTracker;
    MouseStateTracker m_mouseTracker;
    bool m_isInitialized;
    bool m_textInputEnabled;
    char m_markedText[256];
    int m_markedCursorPos;
    int m_markedSelectionLength;

};

class MacKeyCodeMapper {
public:
    static KeyCode mapKeyCode(unsigned short macKeyCode);
    
private:
    struct KeyMapping {
        unsigned short macKeyCode;
        KeyCode yuchenKeyCode;
    };
    
    static const KeyMapping s_keyMappings[];
    static constexpr size_t s_keyMappingCount = 128;
};

}
