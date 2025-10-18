#include "ChannelSection.h"
#include <YuchenUI/rendering/RenderList.h>

ChannelSection::ChannelSection(const YuchenUI::Rect& bounds)
    : Widget()
    , m_mixerTheme(nullptr)
{
    setBounds(bounds);
}

ChannelSection::~ChannelSection()
{
}

void ChannelSection::addDrawCommands(YuchenUI::RenderList& commandList,
                                     const YuchenUI::Vec2& offset) const
{
    if (!m_isVisible) return;
    
    YuchenUI::Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    
    renderChildren(commandList, absPos);
}

void ChannelSection::update(float deltaTime)
{
    Widget::update(deltaTime);
}

bool ChannelSection::handleMouseMove(const YuchenUI::Vec2& position, const YuchenUI::Vec2& offset)
{
    return dispatchMouseEvent(position, false, offset, true);
}

bool ChannelSection::handleMouseClick(const YuchenUI::Vec2& position, bool pressed, const YuchenUI::Vec2& offset)
{
    return dispatchMouseEvent(position, pressed, offset, false);
}
