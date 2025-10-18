#pragma once

#include "YuchenUI/windows/WindowManager.h"
#include "YuchenUI/text/FontManager.h"
#include "YuchenUI/theme/ThemeManager.h"
#include <memory>

namespace YuchenUI {

class Application {
public:
    Application();
    ~Application();
    
    bool initialize();
    int run();
    void quit();
    
    WindowManager& getWindowManager();
    IFontProvider* getFontProvider() { return &m_fontManager; }
    IThemeProvider* getThemeProvider() { return &m_themeManager; }
    FontManager& getFontManager() { return m_fontManager; }
    ThemeManager& getThemeManager() { return m_themeManager; }
    
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

}
