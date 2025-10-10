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

#pragma once

#include "YuchenUI/events/EventManager.h"
#include "YuchenUI/core/Assert.h"
#include "YuchenUI/core/Config.h"
#include <Windows.h>
#include <imm.h>
#include <vector>

#pragma comment(lib, "Imm32.lib")

namespace YuchenUI {

//==========================================================================================
/**
    Windows platform implementation of EventManager using Win32 API.
    
    This class handles Windows input events and converts them to YuchenUI Event objects.
    It provides:
    - Keyboard event handling with virtual key mapping
    - Mouse event handling (button presses, moves, wheel)
    - Text input handling with WM_CHAR messages
    - Input Method Editor (IME) support for Asian text input
    - Event queue management with callback support
    
    IME support includes:
    - Composition text tracking (partially entered characters)
    - Candidate window positioning
    - Result string conversion to text input events
    
    @see EventManager, Event, Win32WindowImpl
*/
class Win32EventManager : public EventManager {
public:
    //======================================================================================
    /** Creates a Win32 event manager for the specified window.
        
        @param hwnd  The window handle to manage events for
    */
    explicit Win32EventManager(HWND hwnd);
    
    /** Destructor. Cleans up event queue and state. */
    virtual ~Win32EventManager();

    //======================================================================================
    /** Initializes the event manager.
        
        Clears event queue and input state trackers.
        
        @returns True if initialization succeeded, false otherwise
    */
    bool initialize() override;
    
    /** Destroys the event manager and releases resources. */
    void destroy() override;
    
    /** Returns true if the event manager is initialized. */
    bool isInitialized() const override;

    //======================================================================================
    /** Returns true if there are pending events in the queue. */
    bool hasEvents() const override;
    
    /** Retrieves and removes the next event from the queue.
        
        @returns The next Event object
    */
    Event getNextEvent() override;
    
    /** Clears all pending events from the queue. */
    void clearEvents() override;
    
    /** Returns the number of events currently in the queue. */
    size_t getEventCount() const override;

    //======================================================================================
    /** Sets the callback function to be invoked when events occur.
        
        The callback is called immediately when an event is added to the queue.
        
        @param callback  Function to call for each event
    */
    void setEventCallback(EventCallback callback) override;
    
    /** Removes the event callback. */
    void clearEventCallback() override;
    
    /** Returns true if an event callback is registered. */
    bool hasEventCallback() const override;

    //======================================================================================
    /** Handles a native Windows message.
        
        Converts Win32 messages (WM_*) into YuchenUI events and adds them to the queue.
        
        @param event  Pointer to MSG structure
    */
    void handleNativeEvent(void* event) override;

    //======================================================================================
    /** Queries if a specific key is currently pressed.
        
        @param key  The key code to check
        @returns True if the key is pressed, false otherwise
    */
    bool isKeyPressed(KeyCode key) const override;
    
    /** Queries if a mouse button is currently pressed.
        
        @param button  The mouse button to check
        @returns True if the button is pressed, false otherwise
    */
    bool isMouseButtonPressed(MouseButton button) const override;
    
    /** Returns the current mouse position in window coordinates. */
    Vec2 getMousePosition() const override;
    
    /** Returns the current keyboard modifier state. */
    KeyModifiers getCurrentModifiers() const override;

    //======================================================================================
    /** Enables text input event generation.
        
        When enabled, WM_CHAR and IME messages generate text input events.
    */
    void enableTextInput() override;
    
    /** Disables text input event generation. */
    void disableTextInput() override;
    
    /** Returns true if text input is currently enabled. */
    bool isTextInputEnabled() const override;

private:
    //======================================================================================
    /** Dispatches a Windows message to the appropriate handler.
        
        @param msg     Message identifier (WM_*)
        @param wParam  First message parameter
        @param lParam  Second message parameter
    */
    void handleWindowsMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    
    //======================================================================================
    // Keyboard Event Handlers
    
    /** Handles keyboard press and release events.
        
        @param msg     WM_KEYDOWN, WM_KEYUP, WM_SYSKEYDOWN, or WM_SYSKEYUP
        @param wParam  Virtual key code
        @param lParam  Additional key information (repeat count, scan code, etc.)
    */
    void handleKeyEvent(UINT msg, WPARAM wParam, LPARAM lParam);
    
    //======================================================================================
    // Mouse Event Handlers
    
    /** Handles mouse button press and release events.
        
        @param msg     WM_LBUTTONDOWN, WM_RBUTTONDOWN, etc.
        @param wParam  Mouse button state
        @param lParam  Mouse position (packed into LPARAM)
    */
    void handleMouseButtonEvent(UINT msg, WPARAM wParam, LPARAM lParam);
    
    /** Handles mouse move events.
        
        @param wParam  Mouse button state
        @param lParam  Mouse position (packed into LPARAM)
    */
    void handleMouseMoveEvent(WPARAM wParam, LPARAM lParam);
    
    /** Handles mouse wheel scroll events.
        
        @param msg     WM_MOUSEWHEEL or WM_MOUSEHWHEEL
        @param wParam  Wheel delta and key state
        @param lParam  Mouse position (packed into LPARAM)
    */
    void handleMouseWheelEvent(UINT msg, WPARAM wParam, LPARAM lParam);
    
    //======================================================================================
    // Text Input Event Handlers
    
    /** Handles character input events (WM_CHAR).
        
        Filters out control characters and generates TextInput events.
        
        @param wParam  Character code (UTF-16)
    */
    void handleCharEvent(WPARAM wParam);
    
    //======================================================================================
    // IME Event Handlers
    
    /** Handles IME composition start.
        
        Called when the user begins entering composed text (e.g., typing Pinyin for Chinese).
    */
    void handleImeStartComposition();
    
    /** Handles IME composition updates.
        
        Processes composition string updates and result strings.
        
        @param lParam  Flags indicating which IME strings changed
    */
    void handleImeComposition(LPARAM lParam);
    
    /** Handles IME composition end.
        
        Called when composition is complete or cancelled.
    */
    void handleImeEndComposition();
    
    /** Handles IME notification messages.
        
        Currently not used but provided for future IME features.
        
        @param wParam  Notification command
    */
    void handleImeNotify(WPARAM wParam);

    //======================================================================================
    // Mapping Functions
    
    /** Maps a Windows virtual key code to YuchenUI KeyCode.
        
        @param vk  Virtual key code (VK_*)
        @returns Corresponding KeyCode, or KeyCode::Unknown if not mapped
    */
    KeyCode mapVirtualKey(WPARAM vk) const;
    
    /** Maps a Windows mouse message to YuchenUI MouseButton.
        
        @param msg  Mouse message (WM_LBUTTONDOWN, etc.)
        @returns Corresponding MouseButton
    */
    MouseButton mapMouseButton(UINT msg) const;
    
    /** Extracts current keyboard modifiers from Windows key state.
        
        Uses GetKeyState() to query shift, control, alt, and caps lock.
        
        @returns KeyModifiers structure with current modifier state
    */
    KeyModifiers extractModifiers() const;
    
    /** Extracts mouse position from LPARAM.
        
        @param lParam  Mouse position packed in LPARAM (from GET_X_LPARAM/GET_Y_LPARAM)
        @returns Mouse position as Vec2
    */
    Vec2 getMousePositionFromLParam(LPARAM lParam) const;

    //======================================================================================
    // Utility Functions
    
    /** Gets the current high-resolution timestamp.
        
        @returns Time in seconds since an arbitrary epoch
    */
    double getCurrentTime() const;
    
    /** Adds an event to the queue and invokes the callback if set.
        
        If the queue is full, discards the oldest event.
        
        @param event  The event to add
    */
    void pushEvent(const Event& event);

    //======================================================================================
    HWND m_hwnd;                                                      ///< Window handle
    EventQueue<Config::Events::EVENT_QUEUE_SIZE> m_eventQueue;       ///< Event queue
    EventCallback m_eventCallback;                                   ///< Optional event callback
    KeyStateTracker m_keyTracker;                                    ///< Keyboard state tracker
    MouseStateTracker m_mouseTracker;                                ///< Mouse state tracker
    bool m_isInitialized;                                            ///< True if initialized
    bool m_textInputEnabled;                                         ///< True if text input enabled
    bool m_imeComposing;                                             ///< True if IME composition active
    std::vector<char> m_imeCompositionBuffer;                        ///< Buffer for IME composition string
};

}
