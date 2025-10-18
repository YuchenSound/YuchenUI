#pragma once

#include <YuchenUI/widgets/Widget.h>

namespace YuchenUI {
    class UIContext;
}

class MixerTheme;

class ChannelSection : public YuchenUI::Widget {
public:
    explicit ChannelSection(const YuchenUI::Rect& bounds);
    virtual ~ChannelSection();
    
    void addDrawCommands(YuchenUI::RenderList& commandList,
                        const YuchenUI::Vec2& offset = YuchenUI::Vec2()) const override;
    
    void update(float deltaTime) override;
    
    bool handleMouseMove(const YuchenUI::Vec2& position, const YuchenUI::Vec2& offset = YuchenUI::Vec2()) override;
    bool handleMouseClick(const YuchenUI::Vec2& position, bool pressed, const YuchenUI::Vec2& offset = YuchenUI::Vec2()) override;
    
    /**
     * 设置 Mixer 主题（由父组件调用）
     */
    virtual void setMixerTheme(MixerTheme* theme) { m_mixerTheme = theme; }

protected:
    MixerTheme* m_mixerTheme = nullptr;
};
