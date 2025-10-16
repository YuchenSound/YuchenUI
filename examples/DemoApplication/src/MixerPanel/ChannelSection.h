#pragma once

#include <YuchenUI/widgets/UIComponent.h>

namespace YuchenUI {
    class UIContext;
}

class ChannelSection : public YuchenUI::UIComponent {
public:
    explicit ChannelSection(const YuchenUI::Rect& bounds);
    virtual ~ChannelSection();
    
    void addDrawCommands(YuchenUI::RenderList& commandList,
                        const YuchenUI::Vec2& offset = YuchenUI::Vec2()) const override;
    
    void update(float deltaTime) override;
    
    bool handleMouseMove(const YuchenUI::Vec2& position, const YuchenUI::Vec2& offset = YuchenUI::Vec2()) override;
    bool handleMouseClick(const YuchenUI::Vec2& position, bool pressed, const YuchenUI::Vec2& offset = YuchenUI::Vec2()) override;
};
