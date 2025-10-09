#pragma once

#include "YuchenUI/events/Event.h"
#include "YuchenUI/core/Assert.h"
#include "YuchenUI/core/Config.h"
#include <functional>

namespace YuchenUI {

typedef std::function<void(const Event&)> EventCallback;

class EventManager {
public:
    virtual ~EventManager() = default;
    
    virtual bool initialize() = 0;
    virtual void destroy() = 0;
    virtual bool isInitialized() const = 0;
    
    virtual bool hasEvents() const = 0;
    virtual Event getNextEvent() = 0;
    virtual void clearEvents() = 0;
    virtual size_t getEventCount() const = 0;
    
    virtual void setEventCallback(EventCallback callback) = 0;
    virtual void clearEventCallback() = 0;
    virtual bool hasEventCallback() const = 0;
    
    virtual void handleNativeEvent(void* event) = 0;

    virtual bool isKeyPressed(KeyCode key) const = 0;
    virtual bool isMouseButtonPressed(MouseButton button) const = 0;
    virtual Vec2 getMousePosition() const = 0;
    virtual KeyModifiers getCurrentModifiers() const = 0;
    virtual void handleMarkedText(const char* text, int cursorPos, int selectionLength) {}
    virtual void handleUnmarkText() {}
    virtual void enableTextInput() = 0;
    virtual void disableTextInput() = 0;
    virtual bool isTextInputEnabled() const = 0;
};

template<size_t Capacity>
class EventQueue {
public:
    EventQueue() : m_head(0), m_tail(0), m_size(0) {
        static_assert(Capacity > 0 && (Capacity & (Capacity - 1)) == 0,
                     "Capacity must be a power of 2");
    }
    
    ~EventQueue() = default;
    
    bool push(const Event& event) {
        YUCHEN_ASSERT(event.isValid());
        
        if (isFull()) return false;
        
        m_events[m_tail] = event;
        m_tail = (m_tail + 1) & (Capacity - 1);
        m_size++;
        return true;
    }
    
    bool pop(Event& event) {
        if (isEmpty()) return false;
        
        event = m_events[m_head];
        m_head = (m_head + 1) & (Capacity - 1);
        m_size--;
        return true;
    }
    
    bool peek(Event& event) const {
        if (isEmpty()) return false;
        
        event = m_events[m_head];
        return true;
    }
    
    void clear() {
        m_head = 0;
        m_tail = 0;
        m_size = 0;
    }
    
    bool isEmpty() const { return m_size == 0; }
    bool isFull() const { return m_size >= Capacity; }
    size_t size() const { return m_size; }
    size_t capacity() const { return Capacity; }
    size_t available() const { return Capacity - m_size; }
    
private:
    Event m_events[Capacity];
    size_t m_head;
    size_t m_tail;
    size_t m_size;
    
    EventQueue(const EventQueue&) = delete;
    EventQueue& operator=(const EventQueue&) = delete;
};

class KeyStateTracker {
public:
    KeyStateTracker() {
        for (size_t i = 0; i < Config::Events::MAX_KEYS; ++i) {
            m_keyStates[i] = false;
        }
    }
    
    void setKeyState(KeyCode key, bool pressed) {
        size_t index = static_cast<size_t>(key);
        YUCHEN_ASSERT_MSG(index < Config::Events::MAX_KEYS, "Key index out of range");
        m_keyStates[index] = pressed;
    }
    
    bool isKeyPressed(KeyCode key) const {
        size_t index = static_cast<size_t>(key);
        YUCHEN_ASSERT_MSG(index < Config::Events::MAX_KEYS, "Key index out of range");
        return m_keyStates[index];
    }
    
    void clear() {
        for (size_t i = 0; i < Config::Events::MAX_KEYS; ++i) {
            m_keyStates[i] = false;
        }
    }
    
private:
    bool m_keyStates[Config::Events::MAX_KEYS];
};

class MouseStateTracker {
public:
    MouseStateTracker() : m_position() {
        for (size_t i = 0; i < Config::Events::MAX_BUTTONS; ++i) {
            m_buttonStates[i] = false;
        }
    }
    
    void setButtonState(MouseButton button, bool pressed) {
        size_t index = static_cast<size_t>(button);
        YUCHEN_ASSERT_MSG(index < Config::Events::MAX_BUTTONS, "Button index out of range");
        m_buttonStates[index] = pressed;
    }
    
    bool isButtonPressed(MouseButton button) const {
        size_t index = static_cast<size_t>(button);
        YUCHEN_ASSERT_MSG(index < Config::Events::MAX_BUTTONS, "Button index out of range");
        return m_buttonStates[index];
    }
    
    void setPosition(const Vec2& position) {
        YUCHEN_ASSERT(position.isValid());
        m_position = position;
    }
    
    Vec2 getPosition() const { return m_position; }
    
    void clear() {
        for (size_t i = 0; i < Config::Events::MAX_BUTTONS; ++i) {
            m_buttonStates[i] = false;
        }
        m_position = Vec2();
    }
    
private:
    bool m_buttonStates[Config::Events::MAX_BUTTONS];
    Vec2 m_position;
};

}
