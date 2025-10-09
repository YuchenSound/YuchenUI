/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Platform module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

//==========================================================================================
/** @file MacEventManager.h
    
    macOS-specific event management implementation using Cocoa NSEvent system.
    
    Implementation notes:
    - Wraps NSEvent types into platform-independent Event structures
    - Maintains keyboard and mouse state tracking for immediate queries
    - Supports Input Method Editor (IME) for multi-byte character input
    - Uses event queue with fixed size for buffering
    - Thread-safe event callback mechanism
*/

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

//==========================================================================================
/**
    macOS implementation of the EventManager interface.
    
    MacEventManager converts native NSEvent objects into platform-independent Event
    structures and manages input state tracking. It supports both polling-based
    event retrieval and callback-based event notification.
    
    Key responsibilities:
    - Converting NSEvent to YuchenUI Event format
    - Tracking keyboard and mouse button states
    - Managing Input Method Editor (IME) composition
    - Event queue management with merging for mouse move events
    - Mapping macOS key codes to platform-independent KeyCode enum
    
    @see EventManager, Event, KeyCode
*/
class MacEventManager : public EventManager {
public:
    //======================================================================================
    /** Creates a MacEventManager for the specified window.
        
        @param window  The NSWindow to monitor for events
    */
    explicit MacEventManager(NSWindow* window);
    
    /** Destructor. Cleans up event tracking resources. */
    virtual ~MacEventManager();
    
    //======================================================================================
    // EventManager Interface Implementation
    
    /** Initializes the event manager and state trackers.
        
        @returns True if initialization succeeded, false otherwise
    */
    bool initialize() override;
    
    /** Destroys the event manager and releases resources. */
    void destroy() override;
    
    /** Returns true if the event manager has been initialized. */
    bool isInitialized() const override;
    
    //======================================================================================
    /** Returns true if there are events in the queue. */
    bool hasEvents() const override;
    
    /** Retrieves and removes the next event from the queue.
        
        @returns The next event in the queue
    */
    Event getNextEvent() override;
    
    /** Clears all events from the queue. */
    void clearEvents() override;
    
    /** Returns the number of events currently in the queue. */
    size_t getEventCount() const override;
    
    //======================================================================================
    /** Sets the callback function to be invoked when events are received.
        
        The callback is invoked immediately when events are pushed to the queue.
        
        @param callback  The function to call with each event
    */
    void setEventCallback(EventCallback callback) override;
    
    /** Clears the event callback. */
    void clearEventCallback() override;
    
    /** Returns true if an event callback is currently set. */
    bool hasEventCallback() const override;
    
    //======================================================================================
    /** Checks if a specific key is currently pressed.
        
        @param key  The key code to check
        @returns True if the key is pressed, false otherwise
    */
    bool isKeyPressed(KeyCode key) const override;
    
    /** Checks if a mouse button is currently pressed.
        
        @param button  The mouse button to check
        @returns True if the button is pressed, false otherwise
    */
    bool isMouseButtonPressed(MouseButton button) const override;
    
    /** Returns the current mouse position in window coordinates. */
    Vec2 getMousePosition() const override;
    
    /** Returns the current state of modifier keys. */
    KeyModifiers getCurrentModifiers() const override;
    
    //======================================================================================
    /** Enables text input mode for receiving character input. */
    void enableTextInput() override;
    
    /** Disables text input mode. */
    void disableTextInput() override;
    
    /** Returns true if text input mode is enabled. */
    bool isTextInputEnabled() const override;
    
    /** Handles IME marked text (composition in progress).
        
        @param text              The marked text string
        @param cursorPos         Cursor position within the marked text
        @param selectionLength   Length of selected portion
    */
    void handleMarkedText(const char* text, int cursorPos, int selectionLength) override;
    
    /** Handles IME unmark (composition complete or cancelled). */
    void handleUnmarkText() override;
    
    /** Pushes an event directly to the queue (for internal use).
        
        @param event  The event to push
    */
    void pushEventDirect(const Event& event) { pushEvent(event); }
    
    /** Processes a platform-specific native event.
        
        @param event  Pointer to the native event (NSEvent*)
    */
    void handleNativeEvent(void* event) override;

private:
    //======================================================================================
    /** Maximum number of events the queue can hold. */
    static constexpr size_t EVENT_QUEUE_SIZE = 512;
    
    //======================================================================================
    /** Processes an NSEvent and converts it to YuchenUI events.
        
        @param event  The NSEvent to process
    */
    void handleNSEvent(NSEvent* event);
    
    /** Handles keyboard press/release events.
        
        @param event      The NSEvent containing key information
        @param eventType  The event type (KeyPressed or KeyReleased)
    */
    void handleKeyEvent(NSEvent* event, EventType eventType);
    
    /** Handles text input events for regular characters.
        
        @param event  The NSEvent containing text input
    */
    void handleTextInputEvent(NSEvent* event);
    
    /** Handles mouse button press/release events.
        
        @param event      The NSEvent containing mouse button info
        @param eventType  The event type (MouseButtonPressed or Released)
    */
    void handleMouseButtonEvent(NSEvent* event, EventType eventType);
    
    /** Handles mouse movement events.
        
        @param event  The NSEvent containing mouse position
    */
    void handleMouseMoveEvent(NSEvent* event);
    
    /** Handles mouse scroll wheel events.
        
        @param event  The NSEvent containing scroll delta
    */
    void handleMouseScrollEvent(NSEvent* event);
    
    /** Handles modifier key state changes.
        
        @param event  The NSEvent containing modifier flags
    */
    void handleModifierFlagsEvent(NSEvent* event);
    
    //======================================================================================
    /** Maps a macOS virtual key code to YuchenUI KeyCode.
        
        @param keyCode  The macOS virtual key code
        @returns The corresponding KeyCode, or KeyCode::Unknown if not mapped
    */
    KeyCode mapKeyCode(unsigned short keyCode) const;
    
    /** Maps a mouse button number to MouseButton enum.
        
        @param buttonNumber  The mouse button number (0=left, 1=right, etc.)
        @returns The corresponding MouseButton value
    */
    MouseButton mapMouseButton(int buttonNumber) const;
    
    /** Extracts modifier key states from NSEvent modifier flags.
        
        @param modifierFlags  The NSEvent modifierFlags value
        @returns KeyModifiers structure with individual modifier states
    */
    KeyModifiers extractModifiers(unsigned long modifierFlags) const;
    
    /** Extracts the Unicode character from an NSEvent.
        
        @param event  The NSEvent to extract from
        @returns The Unicode code point, or 0 if invalid
    */
    uint32_t extractUnicodeFromNSEvent(NSEvent* event) const;
    
    /** Converts NSEvent mouse position to window coordinates.
        
        Flips the Y coordinate to match YuchenUI's top-left origin.
        
        @param event  The NSEvent containing location
        @returns Mouse position in window coordinates
    */
    Vec2 convertMousePosition(NSEvent* event) const;
    
    /** Returns the current time in seconds for event timestamps. */
    double getCurrentTime() const;
    
    /** Pushes an event to the queue and invokes callbacks.
        
        @param event  The event to push
    */
    void pushEvent(const Event& event);
    
    /** Attempts to merge consecutive mouse move events.
        
        Replaces the last mouse move event in the queue to reduce event count.
        
        @param event  The new mouse move event
    */
    void tryMergeMouseMove(Event& event);
    
    //======================================================================================
    NSWindow* m_window;                          ///< The associated NSWindow
    EventQueue<EVENT_QUEUE_SIZE> m_eventQueue;   ///< Fixed-size event queue
    EventCallback m_eventCallback;                ///< Optional event callback
    KeyStateTracker m_keyTracker;                 ///< Tracks pressed keys
    MouseStateTracker m_mouseTracker;             ///< Tracks mouse button states
    bool m_isInitialized;                         ///< Initialization state
    bool m_textInputEnabled;                      ///< Text input mode flag
    char m_markedText[256];                       ///< Current IME composition text
    int m_markedCursorPos;                        ///< Cursor position in marked text
    int m_markedSelectionLength;                  ///< Selection length in marked text
};

//==========================================================================================
/**
    Helper class for mapping macOS virtual key codes to YuchenUI KeyCode enum.
    
    Provides a static mapping table between Carbon framework key codes and
    the platform-independent KeyCode enumeration.
*/
class MacKeyCodeMapper {
public:
    /** Maps a macOS virtual key code to YuchenUI KeyCode.
        
        @param macKeyCode  The macOS virtual key code (from Carbon.framework)
        @returns The corresponding KeyCode, or KeyCode::Unknown if not mapped
    */
    static KeyCode mapKeyCode(unsigned short macKeyCode);
    
private:
    /** Structure defining a key code mapping entry. */
    struct KeyMapping {
        unsigned short macKeyCode;  ///< macOS virtual key code
        KeyCode yuchenKeyCode;      ///< YuchenUI key code
    };
    
    /** Static array of all key mappings. */
    static const KeyMapping s_keyMappings[];
    
    /** Size of the key mapping table. */
    static constexpr size_t s_keyMappingCount = 128;
};

} // namespace YuchenUI
