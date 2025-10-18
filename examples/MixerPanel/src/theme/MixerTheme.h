#pragma once

#include <YuchenUI/core/Types.h>
#include <YuchenUI/theme/Theme.h>
#include <memory>

/**
 * MixerTheme - Mixer 面板业务主题接口
 *
 * 职责：
 * - 定义 Mixer 面板所有业务组件的颜色
 * - 通过工厂方法根据框架主题类型创建对应的业务主题实现
 */
class MixerTheme {
public:
    virtual ~MixerTheme() = default;
    
    // ========================================================================
    // 颜色接口
    // ========================================================================
    
    /**
     * FaderMeterSection 的背景色
     */
    virtual YuchenUI::Vec4 getFaderMeterSectionBackground() const = 0;
    
    /**
     * ChannelStrip 的背景色
     */
    virtual YuchenUI::Vec4 getChannelStripBackground() const = 0;
    
    /**
     * ChannelStrip 的边框色
     */
    virtual YuchenUI::Vec4 getChannelStripBorder() const = 0;
    
    /**
     * NameSection 的背景色
     */
    virtual YuchenUI::Vec4 getNameSectionBackground() const = 0;
    
    // ========================================================================
    // 工厂方法
    // ========================================================================
    
    /**
     * 根据框架主题类型创建对应的 Mixer 主题
     *
     * @param type 框架主题类型
     * @return Mixer 主题实例（所有权转移）
     */
    static std::unique_ptr<MixerTheme> create(YuchenUI::StyleType type);
};
