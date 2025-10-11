/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Core module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

#pragma once

#include "YuchenUI/core/Types.h"
#include "YuchenUI/events/Event.h"
#include <vector>
#include <memory>
#include <functional>

namespace YuchenUI {

class UIContext;
class RenderList;
class UIComponent;

//==========================================================================================
/** Result codes for window content operations */
enum class WindowContentResult {
    None,       ///< No result yet
    Close,      ///< Request window close
    Minimize,   ///< Request window minimize
    Custom      ///< Custom result code
};

/** Callback invoked when content requests to close */
using ContentCloseCallback = std::function<void(WindowContentResult result)>;

//==========================================================================================
/** Abstract interface for window content.
    
    Defines lifecycle methods and event handling for content displayed in a window.
    Implementations manage their own UI components and rendering.
    
    Lifecycle:
    1. onCreate() - Initialize resources, create UI components
    2. onShow() - Content becomes visible
    3. onUpdate() / render() - Frame updates
    4. onHide() - Content hidden
    5. onDestroy() - Cleanup resources
    
    @see UIContext, UIComponent, RenderList
*/
class IUIContent {
public:
    IUIContent();
    virtual ~IUIContent();
    
    //======================================================================================
    // Lifecycle methods
    
    /** Called when content is created and added to a window.
        
        @param context      Parent UI context
        @param contentArea  Initial content area bounds
    */
    virtual void onCreate(UIContext* context, const Rect& contentArea) = 0;
    
    /** Called when content is being destroyed */
    virtual void onDestroy() {}
    
    /** Called when content area is resized.
        
        @param newArea  New content area bounds
    */
    virtual void onResize(const Rect& newArea) {}
    
    /** Called each frame for updates.
        
        @param deltaTime  Time elapsed since last frame (seconds)
    */
    virtual void onUpdate(float deltaTime) {}
    
    /** Called each frame to render content.
        
        @param commandList  Render command list to populate
    */
    virtual void render(RenderList& commandList) = 0;
    
    /** Called when content becomes visible */
    virtual void onShow() {}
    
    /** Called when content is hidden */
    virtual void onHide() {}
    
    //======================================================================================
    // Event handling
    
    /** Handles mouse move events.
        
        @param position  Mouse position in window coordinates
        @returns True if event was handled
    */
    virtual bool handleMouseMove(const Vec2& position);
    
    /** Handles mouse button events.
        
        @param position  Mouse position in window coordinates
        @param pressed   True for press, false for release
        @returns True if event was handled
    */
    virtual bool handleMouseClick(const Vec2& position, bool pressed);
    
    /** Handles mouse wheel events.
        
        @param delta     Scroll delta (positive = up/right)
        @param position  Mouse position in window coordinates
        @returns True if event was handled
    */
    virtual bool handleMouseWheel(const Vec2& delta, const Vec2& position);
    
    /** Handles keyboard events.
        
        @param event  Key event
        @returns True if event was handled
    */
    virtual bool handleKeyEvent(const Event& event);
    
    /** Handles text input events.
        
        @param event  Text input event
        @returns True if event was handled
    */
    virtual bool handleTextInput(const Event& event);
    
    //======================================================================================
    // IME support
    
    /** Returns cursor rectangle for input method positioning.
        
        @returns Cursor rect in window coordinates, or empty rect if no active input
    */
    virtual Rect getInputMethodCursorRect() const;
    
    //======================================================================================
    // Result handling
    
    virtual WindowContentResult getResult() const { return m_result; }
    virtual void setResult(WindowContentResult result) { m_result = result; }
    
    //======================================================================================
    // User data
    
    virtual void* getUserData() const { return m_userData; }
    virtual void setUserData(void* data) { m_userData = data; }
    
    /** Sets callback for close requests */
    void setCloseCallback(ContentCloseCallback callback) { m_closeCallback = callback; }
    
    //======================================================================================
    // Component management
    
    void addComponent(UIComponent* component);
    void removeComponent(UIComponent* component);
    void clearComponents();
    
    UIComponent* getFocusedComponent() const;
    void registerFocusableComponent(UIComponent* component);
    void unregisterFocusableComponent(UIComponent* component);

protected:
    /** Requests content to close with specified result */
    void requestClose(WindowContentResult result);
    
    UIContext* m_context;                           ///< Parent context
    Rect m_contentArea;                             ///< Content area bounds
    WindowContentResult m_result;                   ///< Current result state
    void* m_userData;                               ///< User-defined data
    std::vector<UIComponent*> m_components;         ///< Managed components
    ContentCloseCallback m_closeCallback;           ///< Close callback

private:
    /** Common mouse event handling logic */
    bool handleMouseEvent(const Vec2& position, bool pressed, bool isMoveEvent);
};

} // namespace YuchenUI

