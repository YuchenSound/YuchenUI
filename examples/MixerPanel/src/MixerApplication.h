#pragma once

#include <YuchenUI/YuchenUI-Desktop.h>
#include <YuchenUI-Desktop/Application.h>

/**
 * MixerPanel 应用程序类
 *
 * 这是一个简化的应用程序类，专门用于 MixerPanel 项目。
 * 它基于 YuchenUI::Application 框架，但只显示 Mixer 窗口。
 */
class MixerApplication {
public:
    MixerApplication();
    ~MixerApplication();
    
    /**
     * 初始化应用程序
     * 创建 Mixer 窗口并显示
     */
    bool initialize();
    
    /**
     * 运行应用程序主循环
     */
    int run();
    
    /**
     * 获取 YuchenUI 框架应用程序实例
     */
    YuchenUI::Application& getFrameworkApp() { return m_frameworkApp; }

private:
    /**
     * 创建 Mixer 窗口
     */
    bool createMixerWindow();

    YuchenUI::Application m_frameworkApp;
    YuchenUI::BaseWindow* m_mixerWindow;
    
    static constexpr int MIXER_WINDOW_WIDTH = 1920;
    static constexpr int MIXER_WINDOW_HEIGHT = 800;
};
