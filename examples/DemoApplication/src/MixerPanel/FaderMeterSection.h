#pragma once

#include "ChannelSection.h"
#include <functional>
#include <vector>

namespace YuchenUI {
    class LevelMeter;
    class Fader;
    class NumberBackground;
    class SpinBox;
    class UIContext;
}

class FaderMeterSection : public ChannelSection {
public:
    explicit FaderMeterSection(const YuchenUI::Rect& bounds);
    virtual ~FaderMeterSection();
    
    void updateLevel(const std::vector<float>& levels);
    
    void setFaderValue(float dbValue);
    float getFaderValue() const;
    
    void setOnFaderValueChanged(std::function<void(float)> callback);
    
    void setOwnerContext(YuchenUI::UIContext* context) override;
    
    static constexpr float PREFERRED_HEIGHT = 262.0f;

private:
    void createComponents();
    void addDrawCommands(YuchenUI::RenderList& commandList,
                        const YuchenUI::Vec2& offset = YuchenUI::Vec2()) const override;
    void updateFaderDisplay();
    void updateMeterDisplay();

    YuchenUI::Fader* m_fader;
    YuchenUI::LevelMeter* m_levelMeter;
    YuchenUI::NumberBackground* m_faderMeterNumberBackground;
    YuchenUI::SpinBox* m_faderNumberDisplay;
    YuchenUI::SpinBox* m_meterNumberDisplay;
    
    static constexpr float FADER_WIDTH = 30.0f;
    static constexpr float METER_WIDTH = 14.0f;
    static constexpr float NUMBER_BACKGROUND_WIDTH = 69.0f;
    static constexpr float NUMBER_BACKGROUND_HEIGHT = 17.0f;
    static constexpr float SPACING = 0.0f;
};
