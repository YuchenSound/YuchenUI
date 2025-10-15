/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI-Desktop module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

#pragma once

#include "YuchenUI/windows/WindowManager.h"
#include "YuchenUI/text/FontManager.h"
#include "YuchenUI/theme/ThemeManager.h"
#include <memory>

namespace YuchenUI {

//==========================================================================================
/**
    Desktop application framework class.
    
    Application provides a complete desktop application framework by managing
    core services: WindowManager, FontManager, and ThemeManager. It serves as
    the dependency injection root for the entire application.
    
    This is the new recommended way to structure YuchenUI Desktop applications.
    
    Usage:
        int main() {
            YuchenUI::Application app;
            if (!app.initialize()) return -1;
            
            auto* window = app.createWindow<MyContent>(800, 600, "My App");
            window->show();
            
            return app.run();
        }
    
    @see WindowManager, FontManager, ThemeManager
*/
class Application {
public:
    //======================================================================================
    /**
        Creates application without initialization.
        
        Call initialize() after construction to set up services.
    */
    Application();
    
    /**
        Destructor. Cleans up all services.
    */
    ~Application();
    
    //======================================================================================
    // Lifecycle
    
    /**
        Initializes application and all services.
        
        Initializes WindowManager, FontManager, and ThemeManager in correct order.
        
        @returns True if initialization succeeded
    */
    bool initialize();
    
    /**
        Runs the application event loop.
        
        Blocks until quit() is called or all main windows close.
        
        @returns Exit code
    */
    int run();
    
    /**
        Requests application to quit.
    */
    void quit();
    
    //======================================================================================
    // Service Access
    
    /**
        Returns window manager instance.
        
        @returns Window manager reference
    */
    WindowManager& getWindowManager();
    
    /**
        Returns font provider interface.
        
        @returns Font provider interface (implemented by FontManager)
    */
    IFontProvider* getFontProvider() { return &m_fontManager; }
    
    /**
        Returns theme provider interface.
        
        @returns Theme provider interface (implemented by ThemeManager)
    */
    IThemeProvider* getThemeProvider() { return &m_themeManager; }
    
    /**
        Returns font manager instance.
        
        @returns Font manager reference
    */
    FontManager& getFontManager() { return m_fontManager; }
    
    /**
        Returns theme manager instance.
        
        @returns Theme manager reference
    */
    ThemeManager& getThemeManager() { return m_themeManager; }
    
    //==========================================================================================
    // Convenience Methods

    /**
        Creates a main window with content.
        
        Convenience wrapper around WindowManager::createMainWindow.
        
        @tparam ContentType  IUIContent-derived class
        @param width         Window width in pixels
        @param height        Window height in pixels
        @param title         Window title
        @param fps           Target frame rate (15-240)
        @param args          Arguments forwarded to ContentType constructor
        @returns Pointer to created window, or nullptr on failure
    */
    template<typename ContentType, typename... Args>
    BaseWindow* createWindow(int width, int height, const char* title, int fps, Args&&... args)
    {
        WindowManager& windowManager = getWindowManager();
        
        return windowManager.createMainWindow<ContentType>(
            width, height, title, fps, std::forward<Args>(args)...
        );
    }

private:
    FontManager m_fontManager;
    ThemeManager m_themeManager;
    
    bool m_isInitialized;
    
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
};

} // namespace YuchenUI
