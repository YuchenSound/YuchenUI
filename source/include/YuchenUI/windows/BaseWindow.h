/****************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Windows module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
****************************************************************************/

#pragma once

#include "YuchenUI/windows/Window.h"
#include "YuchenUI/core/Types.h"
#include "YuchenUI/events/EventManager.h"
#include "YuchenUI/windows/IWindowContent.h"
#include "YuchenUI/platform/WindowImpl.h"
#include <memory>
#include <functional>

namespace YuchenUI {

class GraphicsContext;
class UIComponent;

/** Callback function type for dialog result notifications.
    
    @param result    The result value from the dialog
    @param userData  User-defined data passed when showing the dialog
*/
using DialogResultCallback = std::function<void(WindowContentResult result, void* userData)>;

/** Window state enumeration tracking initialization and display status. */
enum class WindowState {
    Uninitialized,    ///< Window has not been created
    Created,          ///< Window structure created
    RendererReady,    ///< Graphics renderer initialized
    Shown             ///< Window is visible on screen
};

//==========================================================================================
/**
    Base window implementation providing platform-independent window management.
    
    BaseWindow handles window creation, event processing, rendering, and content management.
    It supports multiple window types including main windows, dialogs, and tool windows.
    
    Windows transition through states: Uninitialized → Created → RendererReady → Shown
    
    @see Window, IWindowContent, WindowManager
*/
class BaseWindow : public Window
{
public:
    //======================================================================================
    /** Creates a BaseWindow of the specified type.
        
        @param type  The window type (Main, Dialog, or ToolWindow)
    */
    explicit BaseWindow (WindowType type = WindowType::Main);
    
    /** Destructor. Cleans up all window resources. */
    virtual ~BaseWindow();

    //======================================================================================
    /** Creates the window with the specified parameters.
        
        @param width   Window width in pixels
        @param height  Window height in pixels
        @param title   Window title text
        @param parent  Optional parent window (nullptr for top-level windows)
        
        @returns True if window creation succeeded, false otherwise
    */
    bool create (int width, int height, const char* title, Window* parent = nullptr) override;
    
    /** Destroys the window and releases all resources. */
    void destroy() override;
    
    /** Returns true if the window should close. */
    bool shouldClose() override;
    
    /** Returns the current window size. */
    Vec2 getSize() const override;
    
    /** Returns the current mouse position in window coordinates. */
    Vec2 getMousePosition() const override;
    
    /** Returns true if the left mouse button is currently pressed. */
    bool isMousePressed() const override;
    
    /** Returns the platform-specific native window handle. */
    void* getNativeWindowHandle() const override;
    
    /** Returns the window's position on screen. */
    Vec2 getWindowPosition() const override;
    
    /** Enables text input for the window. */
    void enableTextInput() override;
    
    /** Disables text input for the window. */
    void disableTextInput() override;
    
    /** Enables or disables Input Method Editor (IME) support.
        
        @param enabled  True to enable IME, false to disable
    */
    void setIMEEnabled (bool enabled) override;

    //======================================================================================
    /** Processes a platform-specific native event.
        
        @param event  Platform-specific event pointer
    */
    void handleNativeEvent (void* event);
    
    /** Renders the window content to screen. */
    void renderContent();
    
    /** Makes the window visible. */
    void show();
    
    /** Hides the window. */
    void hide();
    
    /** Returns true if the window is currently visible. */
    bool isVisible() const;
    
    //======================================================================================
    /** Shows the window as a modal dialog (blocks until closed).
        
        This method blocks the calling thread until the dialog is closed.
        Only valid for Dialog window types.
    */
    void showModal();
    
    /** Closes a modal dialog. */
    void closeModal();
    
    /** Closes the window with the specified result.
        
        @param result  The result value to return
    */
    void closeWithResult (WindowContentResult result);
    
    /** Sets the callback to invoke when a dialog closes.
        
        @param callback  Function to call with the dialog result
    */
    void setResultCallback (DialogResultCallback callback);
    
    //======================================================================================
    /** Handles window resize events.
        
        @param width   New width in pixels
        @param height  New height in pixels
    */
    void onResize (int width, int height);
    
    /** Creates a window with content in a single operation.
        
        This template method creates both the window and its content, ensuring
        proper initialization order.
        
        @tparam ContentType  The IWindowContent-derived class to create
        @param width         Window width in pixels
        @param height        Window height in pixels
        @param title         Window title text
        @param parent        Optional parent window
        @param args          Arguments forwarded to ContentType constructor
        
        @returns True if creation succeeded, false otherwise
    */
    template<typename ContentType, typename... Args>
    bool createWithContent (int width, int height, const char* title, Window* parent, Args&&... args)
    {
        if (!create (width, height, title, parent))
        {
            return false;
        }
        
        auto content = std::make_unique<ContentType> (std::forward<Args>(args)...);
        setContent (std::move (content));
        return true;
    }
    
    /** Sets the window's content.
        
        @param content  The content object to display in this window
    */
    void setContent (std::unique_ptr<IWindowContent> content);
    
    /** Returns the window's current content. */
    IWindowContent* getContent() const;
    
    //======================================================================================
    /** Captures mouse input to a specific component.
        
        All mouse events will be sent to this component until releaseMouse() is called.
        
        @param component  The component to receive mouse events
    */
    void captureMouse (UIComponent* component);
    
    /** Releases mouse capture. */
    void releaseMouse();
    
    /** Handles IME marked text (composition text).
        
        @param text              The marked text
        @param cursorPos         Cursor position within the marked text
        @param selectionLength   Length of selected text
    */
    void handleMarkedText (const char* text, int cursorPos, int selectionLength);
    
    /** Handles unmarking of IME text (commits composition). */
    void handleUnmarkText();
    
    /** Handles an input event.
        
        @param event  The event to process
    */
    void handleEvent (const Event& event);
    
    /** Returns the cursor rectangle for IME positioning. */
    Rect getInputMethodCursorRect() const;
    
    /** Maps window coordinates to screen coordinates.
        
        @param windowPos  Position in window coordinates
        @returns Position in screen coordinates
    */
    Vec2 mapToScreen (const Vec2& windowPos) const override;

protected:
    //======================================================================================
    /** Returns the graphics context for rendering. */
    GraphicsContext* getGraphicsContext() { return m_renderer; }
    
    /** Called when the window is ready and fully initialized.
        
        Subclasses can override this to perform initialization after window creation.
    */
    virtual void onWindowReady() {}
    
    /** Sets up the user interface content.
        
        This creates the window content if not already created.
    */
    virtual void setupUserInterface();
    
    /** Returns true if the window is in the specified state. */
    bool isInState (WindowState state) const { return m_state == state; }
    
    /** Returns true if the window has reached at least the specified state. */
    bool hasReachedState (WindowState state) const { return m_state >= state; }
    
    /** Calculates the content area rectangle based on window size. */
    Rect calculateContentArea() const;
    
    /** Returns the background color for this window type. */
    Vec4 getBackgroundColor() const;

    //======================================================================================
    std::unique_ptr<WindowImpl> m_impl;
    GraphicsContext* m_renderer;
    Window* m_parentWindow;
    WindowType m_windowType;
    
    WindowState m_state;
    
    bool m_shouldClose;
    int m_width;
    int m_height;
    float m_dpiScale;

    std::unique_ptr<EventManager> m_eventManager;
    std::unique_ptr<IWindowContent> m_content;
    bool m_contentCreated;
    
    DialogResultCallback m_resultCallback;
    bool m_isModal;
    
    UIComponent* m_capturedComponent;

private:
    //======================================================================================
    /** Initializes the graphics renderer. */
    bool initializeRenderer();
    
    /** Detects the display's DPI scale factor. */
    void detectDPIScale();
    
    /** Releases graphics and other resources. */
    void releaseResources();
    
    /** Transitions the window to a new state. */
    void transitionToState (WindowState newState);
    
    /** Returns true if the window can transition to the new state. */
    bool canTransitionTo (WindowState newState) const;
    
    friend class MetalRenderer;
};

} // namespace YuchenUI
