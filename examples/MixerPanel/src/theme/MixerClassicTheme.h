#pragma once

#include "theme/MixerTheme.h"

/**
 * MixerClassicTheme - Classic 主题的 Mixer 面板颜色实现
 */
class MixerClassicTheme : public MixerTheme {
public:
    MixerClassicTheme() = default;
    ~MixerClassicTheme() override = default;
    
    YuchenUI::Vec4 getFaderMeterSectionBackground() const override;
    YuchenUI::Vec4 getChannelStripBackground() const override;
    YuchenUI::Vec4 getChannelStripBorder() const override;
    YuchenUI::Vec4 getNameSectionBackground() const override;
};
