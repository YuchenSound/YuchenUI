#pragma once

#include "ChannelSection.h"
#include <functional>
#include <vector>

namespace YuchenUI {
    class LevelMeter;
    class Fader;
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
    
    void setOwnerContext(YuchenUI::UIContext* context) override;  // 改为 setOwnerContext

private:
    void createComponents();
    
    YuchenUI::Fader* m_fader;
    YuchenUI::LevelMeter* m_levelMeter;
    
    static constexpr float FADER_WIDTH = 30.0f;
    static constexpr float METER_WIDTH = 14.0f;
    static constexpr float SPACING = 0.0f;
};
