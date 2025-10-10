/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Platform module (Windows).
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

#pragma once

#include "YuchenUI/platform/WindowImpl.h"
#include "YuchenUI/core/Types.h"
#include <Windows.h>
#include <imm.h>
#include <string>

#pragma comment(lib, "Imm32.lib")

namespace YuchenUI {

class BaseWindow;

//==========================================================================================
/**
    Windows platform implementation of WindowImpl using Win32 API.
    
    This class provides native window functionality on Windows, including:
    - Window creation with proper styles for different window types
    - DPI awareness and scaling
    - Modal dialog support
    - Input Method Editor (IME) integration for Asian text input
    - Window positioning and centering
    
    The implementation uses Win32 APIs and handles Windows-specific messages through
    a window procedure callback.
    
    @see WindowImpl, BaseWindow
*/
class Win32WindowImpl : public WindowImpl {
public:
    //======================================================================================
    /** Creates a new Win32 window implementation instance. */
    Win32WindowImpl();
    
    /** Destructor. Destroys the native window if it exists. */
    virtual ~Win32WindowImpl();
    
    //======================================================================================
    /** Creates the native Windows window.
        
        Registers the window class on first use and creates a window with appropriate
        styles based on the window type. Sets up DPI awareness.
        
        @param config  Window configuration parameters
        @returns True if window creation succeeded, false otherwise
    */
    bool create(const WindowConfig& config) override;
    
    /** Destroys the native window and releases resources. */
    void destroy() override;
    
    /** Makes the window visible on screen.
        
        For main windows that are not modal, centers the window on the screen.
    */
    void show() override;
    
    /** Hides the window from screen. */
    void hide() override;
    
    /** Shows the window as a modal dialog.
        
        This method blocks until closeModal() is called. Only valid for Dialog window types.
        Disables the parent window while the dialog is active.
    */
    void showModal() override;
    
    /** Closes a modal dialog and returns control to the caller of showModal(). */
    void closeModal() override;
    
    /** Returns the window's client area size in pixels. */
    Vec2 getSize() const override;
    
    /** Returns the window's position on screen in screen coordinates. */
    Vec2 getPosition() const override;
    
    /** Returns true if the window is currently visible. */
    bool isVisible() const override;
    
    /** Returns the native window handle (HWND). */
    void* getNativeHandle() const override;
    
    //======================================================================================
    /** Sets the BaseWindow that owns this implementation.
        
        The BaseWindow receives callbacks for events and rendering.
        
        @param baseWindow  The owning BaseWindow instance
    */
    void setBaseWindow(BaseWindow* baseWindow) override;
    
    /** Returns the render surface (HWND) for the renderer. */
    void* getRenderSurface() const override;
    
    /** Returns the DPI scale factor for this window.
        
        @returns Scale factor (1.0 = 96 DPI, 2.0 = 192 DPI, etc.)
    */
    float getDpiScale() const override;
    
    /** Maps window coordinates to screen coordinates.
        
        @param windowPos  Position in window client coordinates
        @returns Position in screen coordinates
    */
    Vec2 mapToScreen(const Vec2& windowPos) const override;
    
    /** Gets the cursor rectangle for IME composition window positioning.
        
        Queries the BaseWindow for the current input cursor position.
        
        @returns Rectangle defining where IME composition window should appear
    */
    Rect getInputMethodCursorWindowRect() const override;
    
    /** Enables or disables the Input Method Editor (IME).
        
        When enabled, allows input of Asian languages and other complex scripts.
        
        @param enabled  True to enable IME, false to disable
    */
    void setIMEEnabled(bool enabled) override;
    
    //======================================================================================
    /** Called when the window needs to be rendered.
        
        Delegates to the BaseWindow's renderContent() method.
    */
    void onRender();
    
    /** Called when the window is resized.
        
        Updates cached size and notifies the BaseWindow.
        
        @param width   New width in pixels
        @param height  New height in pixels
    */
    void onResize(int width, int height);
    
    /** Returns the BaseWindow that owns this implementation. */
    BaseWindow* getBaseWindow() const { return m_baseWindow; }

private:
    //======================================================================================
    /** Computes the appropriate window style flags for the window type.
        
        @param config  Window configuration
        @returns Win32 window style flags (WS_* constants)
    */
    DWORD getWindowStyle(const WindowConfig& config) const;
    
    /** Computes the appropriate extended window style flags.
        
        @param config  Window configuration
        @returns Win32 extended style flags (WS_EX_* constants)
    */
    DWORD getWindowExStyle(const WindowConfig& config) const;
    
    /** Centers the window on the primary monitor. */
    void centerWindow();
    
    /** Queries the current DPI for this window and updates m_dpiScale. */
    void calculateDpiScale();
    
    //======================================================================================
    HWND m_hwnd;                ///< Native window handle
    BaseWindow* m_baseWindow;   ///< Owning BaseWindow instance
    WindowType m_windowType;    ///< Type of window (Main, Dialog, ToolWindow)
    int m_width;                ///< Current window width in pixels
    int m_height;               ///< Current window height in pixels
    float m_dpiScale;           ///< DPI scale factor
    bool m_isModal;             ///< True if this is a modal dialog
    bool m_isVisible;           ///< True if window is currently visible
    bool m_imeEnabled;          ///< True if IME is enabled
    
    /** Win32 window procedure callback.
        
        This static method receives all Windows messages for YuchenUI windows and
        dispatches them to the appropriate Win32WindowImpl instance.
        
        @param hwnd    Window handle
        @param msg     Message identifier
        @param wParam  First message parameter
        @param lParam  Second message parameter
        @returns Message-specific result
    */
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    static const wchar_t* WINDOW_CLASS_NAME;    ///< Window class name for registration
    static bool s_classRegistered;              ///< True if window class has been registered
};

}
