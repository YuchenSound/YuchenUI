/*******************************************************************************************
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
********************************************************************************************/

#pragma once

#include "YuchenUI/core/Types.h"
#include "YuchenUI/events/Event.h"
#include "YuchenUI/windows/Window.h"
#include "YuchenUI/widgets/IInputMethodSupport.h"
#include <vector>
#include <memory>

namespace YuchenUI {

class RenderList;
class Window;
class UIComponent;
class FocusManager;

/** Result values that can be returned when a window content closes. */
enum class WindowContentResult {
    None,      ///< No specific result
    Close,     ///< Normal close
    Minimize,  ///< Window minimized
    Custom     ///< Custom result code
};

//==========================================================================================
/**
    Base interface for window content implementations.
    
    IWindowContent defines the contract for objects that provide content to windows.
    It manages UI components, handles input events, and provides focus management.
    
    Subclasses should override the lifecycle methods (onCreate, onDestroy, etc.)
    and event handlers as needed.
    
    @see BaseWindow, UIComponent, FocusManager
*/
class IWindowContent
{
public:
    //======================================================================================
    /** Creates a new window content instance. */
    IWindowContent();
    
    /** Destructor. Clears all components. */
    virtual ~IWindowContent();
    
    //======================================================================================
    /** Called when the content is created and attached to a window.
        
        Subclasses should override this to create and initialize UI components.
        
        @param window       The parent window
        @param contentArea  The available content area rectangle
    */
    virtual void onCreate (Window* window, const Rect& contentArea) = 0;
    
    /** Called when the content is being destroyed.
        
        Subclasses can override this to perform cleanup.
    */
    virtual void onDestroy();
    
    /** Called when the window is shown. */
    virtual void onShow();
    
    /** Called when the window is hidden. */
    virtual void onHide();
    
    /** Called when the window is resized.
        
        @param newArea  The new content area rectangle
    */
    virtual void onResize (const Rect& newArea);
    
    /** Called each frame to update component state. */
    virtual void onUpdate();
    
    /** Called to render the content.
        
        @param commandList  The render command list to populate
    */
    virtual void render (RenderList& commandList);
    
    //======================================================================================
    /** Handles mouse move events.
        
        @param position  Mouse position in window coordinates
        @returns True if the event was handled, false otherwise
    */
    virtual bool handleMouseMove (const Vec2& position);
    
    /** Handles mouse button events.
        
        @param position  Mouse position in window coordinates
        @param pressed   True if button was pressed, false if released
        @returns True if the event was handled, false otherwise
    */
    virtual bool handleMouseClick (const Vec2& position, bool pressed);
    
    /** Handles keyboard events.
        
        @param event  The keyboard event
        @returns True if the event was handled, false otherwise
    */
    virtual bool handleKeyEvent (const Event& event);
    
    /** Handles text input events.
        
        @param event  The text input event
        @returns True if the event was handled, false otherwise
    */
    virtual bool handleTextInput (const Event& event);
    
    /** Handles scroll wheel events.
        
        @param event  The scroll event
        @returns True if the event was handled, false otherwise
    */
    virtual bool handleScroll (const Event& event);
    
    //======================================================================================
    /** Returns the result value set by this content. */
    virtual WindowContentResult getResult() const { return m_result; }
    
    /** Sets the result value.
        
        @param result  The result value to set
    */
    virtual void setResult (WindowContentResult result) { m_result = result; }

    /** Returns the user-defined data pointer. */
    virtual void* getUserData() const { return m_userData; }
    
    /** Sets user-defined data.
        
        @param data  Pointer to user data
    */
    virtual void setUserData (void* data) { m_userData = data; }
    
    //======================================================================================
    /** Adds a component to this content.
        
        The component is registered with the focus manager if it is focusable.
        
        @param component  The component to add
    */
    void addComponent (UIComponent* component);
    
    /** Removes a component from this content.
        
        @param component  The component to remove
    */
    void removeComponent (UIComponent* component);
    
    /** Removes all components. */
    void clearComponents();
    
    /** Returns the currently focused component, or nullptr if no component has focus. */
    UIComponent* getFocusedComponent() const;
    
    //======================================================================================
    /** Registers a focusable component with the focus manager.
        
        @param component  The component to register
    */
    void registerFocusableComponent (UIComponent* component);
    
    /** Unregisters a component from the focus manager.
        
        @param component  The component to unregister
    */
    void unregisterFocusableComponent (UIComponent* component);
    
    /** Sets the focus manager accessor for a component.
        
        This allows the component to request focus and query focus state.
        
        @param component  The component to configure
    */
    void setFocusManagerAccessor (UIComponent* component);
    
    //======================================================================================
    /** Requests text input to be enabled or disabled.
        
        @param enable  True to enable text input, false to disable
    */
    virtual void requestTextInput (bool enable);
    
    /** Enables or disables IME (Input Method Editor).
        
        @param enable  True to enable IME, false to disable
    */
    void setIMEEnabled (bool enable);
    
    /** Returns the cursor rectangle for IME positioning.
        
        The rectangle is in window coordinates.
    */
    virtual Rect getInputMethodCursorRect() const;

protected:
    //======================================================================================
    Window* m_window;
    Rect m_contentArea;
    WindowContentResult m_result;
    void* m_userData;
    std::vector<UIComponent*> m_components;
    std::unique_ptr<FocusManager> m_focusManager;
    
    friend class FocusManager;
    
private:
    //======================================================================================
    bool handleMouseEvent(const Vec2& position, bool pressed, bool isMoveEvent);
};

} // namespace YuchenUI
