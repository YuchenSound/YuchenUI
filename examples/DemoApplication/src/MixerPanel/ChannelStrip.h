#pragma once

#include <YuchenUI/widgets/Widget.h>
#include <string>
#include <vector>

class FaderMeterSection;
class NameSection;

class ChannelStrip : public YuchenUI::Widget {
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
    
    void setOwnerContext(YuchenUI::UIContext* context) override;
    
    static constexpr float STRIP_WIDTH = 79.0f;
    static constexpr float BORDER_SIZE = 1.0f;
    static constexpr float CONTENT_WIDTH = STRIP_WIDTH - BORDER_SIZE * 2.0f;
    
    static float getStripHeight();

private:
    void createSections();
    
    int m_channelNumber;
    FaderMeterSection* m_faderMeterSection;
    NameSection* m_nameSection;
};
