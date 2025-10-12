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

#include "YuchenUI-Desktop/Application.h"
#include <iostream>

namespace YuchenUI {

//==========================================================================================
// Lifecycle

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
    // Initialize font manager first
    if (!m_fontManager.initialize())
    {
        std::cerr << "[Application] Failed to initialize FontManager" << std::endl;
        return false;
    }
    
    // Get window manager singleton and initialize
    WindowManager& windowManager = WindowManager::getInstance();
    if (!windowManager.initialize())
    {
        std::cerr << "[Application] Failed to initialize WindowManager" << std::endl;
        return false;
    }
    
    // Always inject font provider into theme manager
    m_themeManager.setFontProvider(&m_fontManager);
    
    // Inject both providers into window manager
    // All windows created by WindowManager will receive these providers
    windowManager.setFontProvider(&m_fontManager);
    windowManager.setThemeProvider(&m_themeManager);
    
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

} // namespace YuchenUI
