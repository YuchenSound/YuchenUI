#pragma once

#include <YuchenUI/widgets/UIComponent.h>
#include <string>
#include <vector>

class FaderMeterSection;
class NameSection;

class ChannelStrip : public YuchenUI::UIComponent {
public:
    explicit ChannelStrip(const YuchenUI::Rect& bounds, int channelNumber);
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
    
    void setOwnerContext(YuchenUI::UIContext* context) override;  // 改为 setOwnerContext
    
    static constexpr float STRIP_WIDTH = 79.0f;
    static constexpr float STRIP_HEIGHT = 224.0f;
    static constexpr float FADER_METER_WIDTH = 44.0f;
    static constexpr float NAME_WIDTH = 35.0f;

private:
    void createSections();
    
    int m_channelNumber;
    FaderMeterSection* m_faderMeterSection;
    NameSection* m_nameSection;
};
