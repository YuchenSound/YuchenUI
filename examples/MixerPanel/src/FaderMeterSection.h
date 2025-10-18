#pragma once

#include "ChannelSection.h"
#include <YuchenUI/YuchenUI.h>
#include <functional>
#include <vector>

namespace YuchenUI {
    enum class FaderColorTheme;
}

class FaderMeterSection : public ChannelSection {
public:
    explicit FaderMeterSection(const YuchenUI::Rect& bounds);
    virtual ~FaderMeterSection();
    
    void updateLevel(const std::vector<float>& levels);
    
    void setFaderValue(float dbValue);
    float getFaderValue() const;
    
    void setOnFaderValueChanged(std::function<void(float)> callback);
    
    void setFaderColorTheme(YuchenUI::FaderColorTheme theme);
    
    void setOwnerContext(YuchenUI::UIContext* context) override;
    
    static constexpr float PREFERRED_HEIGHT = 245.0f;

private:
    void createComponents();
    void addDrawCommands(YuchenUI::RenderList& commandList,
                        const YuchenUI::Vec2& offset = YuchenUI::Vec2()) const override;

    YuchenUI::Fader* m_fader;
    YuchenUI::LevelMeter* m_levelMeter;
    std::function<void(float)> m_onFaderValueChanged;
    
    static constexpr float FADER_WIDTH = 30.0f;
    static constexpr float METER_WIDTH = 14.0f;
};
