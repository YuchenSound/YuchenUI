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

namespace YuchenUI {

class GraphicsContext;

//==========================================================================================
/**
    Abstract base class for all window implementations.
    
    Window defines the platform-independent interface for window operations.
    Platform-specific implementations provide concrete implementations of these methods.
    
    @see BaseWindow
*/
class Window
{
public:
    //======================================================================================
    /** Virtual destructor. */
    virtual ~Window() = default;
    
    //======================================================================================
    /** Creates the window with the specified parameters.
        
        @param width   Window width in pixels
        @param height  Window height in pixels
        @param title   Window title text
        @param parent  Optional parent window (nullptr for top-level windows)
        
        @returns True if window creation succeeded, false otherwise
    */
    virtual bool create (int width, int height, const char* title, Window* parent = nullptr) = 0;
    
    /** Destroys the window and releases all resources. */
    virtual void destroy() = 0;
    
    /** Returns true if the window should close. */
    virtual bool shouldClose() = 0;
    
    /** Returns the current window size. */
    virtual Vec2 getSize() const = 0;
    
    //======================================================================================
    /** Returns the current mouse position in window coordinates. */
    virtual Vec2 getMousePosition() const = 0;
    
    /** Returns true if the left mouse button is currently pressed. */
    virtual bool isMousePressed() const = 0;
    
    //======================================================================================
    /** Returns the platform-specific native window handle. */
    virtual void* getNativeWindowHandle() const = 0;
    
    /** Returns the window's position on screen. */
    virtual Vec2 getWindowPosition() const = 0;
    
    //======================================================================================
    /** Enables text input for the window. */
    virtual void enableTextInput() = 0;
    
    /** Disables text input for the window. */
    virtual void disableTextInput() = 0;
    
    /** Enables or disables Input Method Editor (IME) support.
        
        @param enabled  True to enable IME, false to disable
    */
    virtual void setIMEEnabled (bool enabled) = 0;
    
    /** Maps window coordinates to screen coordinates.
        
        @param windowPos  Position in window coordinates
        @returns Position in screen coordinates
    */
    virtual Vec2 mapToScreen (const Vec2& windowPos) const = 0;
    
    //======================================================================================
    /** Creates a new window instance.
        
        @returns A new window instance
    */
    static Window* create();
};

} // namespace YuchenUI
