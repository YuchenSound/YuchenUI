#pragma once

#include <YuchenUI/widgets/Widget.h>
#include "TrackType.h"
#include <string>
#include <vector>
#include <functional>

class FaderMeterSection;
class MeterNumberSection;
class SoloMuteSection;
class NameSection;
class MixerTheme;

class ChannelStrip : public YuchenUI::Widget {
public:
    explicit ChannelStrip(const YuchenUI::Rect& bounds, int channelNumber, TrackType trackType = TrackType::Audio);
    virtual ~ChannelStrip();
    
    void addDrawCommands(YuchenUI::RenderList& commandList,
                        const YuchenUI::Vec2& offset = YuchenUI::Vec2()) const override;
    
    bool handleMouseMove(const YuchenUI::Vec2& position, const YuchenUI::Vec2& offset = YuchenUI::Vec2()) override;
    bool handleMouseClick(const YuchenUI::Vec2& position, bool pressed, const YuchenUI::Vec2& offset = YuchenUI::Vec2()) override;
    
    void updateLevel(const std::vector<float>& levels);
    void setChannelName(const std::string& name);
    
    void setFaderValue(float dbValue);
    float getFaderValue() const;
    
    int getChannelNumber() const { return m_channelNumber; }
    TrackType getTrackType() const { return m_trackType; }
    
    void setOwnerContext(YuchenUI::UIContext* context) override;
    
    void setMixerTheme(MixerTheme* theme);
    
    void setOnListenChanged(std::function<void(int, bool)> callback);
    void setOnRecordChanged(std::function<void(int, bool)> callback);
    void setOnSoloChanged(std::function<void(int, bool)> callback);
    void setOnMuteChanged(std::function<void(int, bool)> callback);
    
    void setPassiveMuted(bool muted);
    void updateRecordFlash(bool flashOn);
    
    static constexpr float STRIP_WIDTH = 79.0f;
    static constexpr float BORDER_SIZE = 1.0f;
    static constexpr float CONTENT_WIDTH = STRIP_WIDTH - BORDER_SIZE * 2.0f;
    
    static float getStripHeight();

private:
    void createSections();
    
    int m_channelNumber;
    TrackType m_trackType;
    FaderMeterSection* m_faderMeterSection;
    MeterNumberSection* m_meterNumberSection;
    SoloMuteSection* m_soloMuteSection;
    NameSection* m_nameSection;
    MixerTheme* m_mixerTheme;
    
    std::function<void(int, bool)> m_onListenChanged;
    std::function<void(int, bool)> m_onRecordChanged;
    std::function<void(int, bool)> m_onSoloChanged;
    std::function<void(int, bool)> m_onMuteChanged;
};
