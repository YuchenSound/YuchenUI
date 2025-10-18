#include "MixerApplication.h"
#include "MixerWindow.h"
#include <iostream>

MixerApplication::MixerApplication()
    : m_mixerWindow(nullptr)
{
}

MixerApplication::~MixerApplication()
{
}

bool MixerApplication::initialize()
{
    // 初始化 YuchenUI 框架
    if (!m_frameworkApp.initialize())
    {
        std::cerr << "[MixerApplication] Failed to initialize YuchenUI framework" << std::endl;
        return false;
    }
    
    // 创建 Mixer 窗口
    if (!createMixerWindow())
    {
        std::cerr << "[MixerApplication] Failed to create mixer window" << std::endl;
        return false;
    }
    
    return true;
}

bool MixerApplication::createMixerWindow()
{
    m_mixerWindow = m_frameworkApp.createWindow<MixerWindowContent>(
        MIXER_WINDOW_WIDTH,
        MIXER_WINDOW_HEIGHT,
        "Mixer Panel",
        60  // 60 FPS
    );
    
    if (!m_mixerWindow)
    {
        return false;
    }
    
    m_mixerWindow->show();
    return true;
}

int MixerApplication::run()
{
    return m_frameworkApp.run();
}
