#pragma once

#include "ChannelSection.h"
#include "TrackType.h"

namespace YuchenUI {
    class NumberBackground;
    class SpinBox;
    class Image;
    class UIContext;
}

class MeterNumberSection : public ChannelSection {
public:
    explicit MeterNumberSection(const YuchenUI::Rect& bounds);
    virtual ~MeterNumberSection();
    
    void setFaderValue(float dbValue);
    void setMeterValue(float dbValue);
    
    void setTrackType(TrackType type);
    
    void setOwnerContext(YuchenUI::UIContext* context) override;
    
    static constexpr float PREFERRED_HEIGHT = 39.0f;

private:
    void createComponents();
    void addDrawCommands(YuchenUI::RenderList& commandList,
                        const YuchenUI::Vec2& offset = YuchenUI::Vec2()) const override;
    
    const char* getTrackTypeIconPath() const;

    YuchenUI::NumberBackground* m_numberBackground;
    YuchenUI::SpinBox* m_faderNumberDisplay;
    YuchenUI::SpinBox* m_meterNumberDisplay;
    YuchenUI::Image* m_volumeIcon;
    YuchenUI::Image* m_trackTypeIcon;
    
    TrackType m_trackType;
    
    static constexpr float NUMBER_DISPLAY_HEIGHT = 17.0f;
    static constexpr float ICON_AREA_HEIGHT = 22.0f;
};
