#include <iostream>
#include "YuchenUI-Desktop/Application.h"
#include "YuchenUI/resource/ResourceManager.h"
#include "YuchenUI/resource/EmbeddedResourceProvider.h"
#include <embedded_resources.h>
#include <iostream>

namespace YuchenUI {

Application::Application()
    : m_fontManager()
    , m_themeManager()
    , m_isInitialized(false)
{
}

Application::~Application()
{
    if (m_isInitialized)
    {
        WindowManager::getInstance().destroy();
        m_fontManager.destroy();
    }
}

bool Application::initialize()
{
    ResourceManager::getInstance().registerProvider(
        "YuchenUI",
        new EmbeddedResourceProvider(
            YuchenUI::Resources::getAllResources(),
            YuchenUI::Resources::getResourceCount()
        )
    );
    
    if (!m_fontManager.initialize(&ResourceManager::getInstance()))
    {
        std::cerr << "[Application] Failed to initialize FontManager" << std::endl;
        return false;
    }
    
    WindowManager& windowManager = WindowManager::getInstance();
    if (!windowManager.initialize())
    {
        std::cerr << "[Application] Failed to initialize WindowManager" << std::endl;
        return false;
    }
    
    m_themeManager.setFontProvider(&m_fontManager);
    
    windowManager.setFontProvider(&m_fontManager);
    windowManager.setThemeProvider(&m_themeManager);
    windowManager.setResourceResolver(&ResourceManager::getInstance());
    
    m_isInitialized = true;
    return true;
}

int Application::run()
{
    if (!m_isInitialized)
    {
        std::cerr << "[Application] Not initialized" << std::endl;
        return -1;
    }
    
    WindowManager::getInstance().run();
    return 0;
}

void Application::quit()
{
    if (m_isInitialized)
    {
        WindowManager::getInstance().quit();
    }
}

WindowManager& Application::getWindowManager()
{
    return WindowManager::getInstance();
}

}
