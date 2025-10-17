/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

#include "YuchenUI/widgets_expand/NumberBackground.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/theme/ThemeManager.h"
#include "YuchenUI/core/Validation.h"
#include "YuchenUI/core/Assert.h"
#include "YuchenUI/core/UIContext.h"

namespace YuchenUI {

NumberBackground::NumberBackground(const Rect& bounds)
    : Widget()
{
    Rect adjustedBounds = bounds;
    
    if (bounds.width == 0.0f || bounds.height == 0.0f)
    {
        adjustedBounds = Rect(bounds.x, bounds.y, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    }
    
    Validation::AssertRect(adjustedBounds);
    setBounds(adjustedBounds);
}

NumberBackground::~NumberBackground()
{
}

void NumberBackground::addDrawCommands(RenderList& commandList, const Vec2& offset) const
{
    if (!m_isVisible) return;
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    YUCHEN_ASSERT(style);
    
    NumberBackgroundDrawInfo info;
    info.bounds = Rect(absPos.x, absPos.y, m_bounds.width, m_bounds.height);
    
    style->drawNumberBackground(info, commandList);
    
    renderChildren(commandList, absPos);
}

bool NumberBackground::handleMouseMove(const Vec2& position, const Vec2& offset)
{
    return dispatchMouseEvent(position, false, offset, true);
}

bool NumberBackground::handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset)
{
    return dispatchMouseEvent(position, pressed, offset, false);
}

bool NumberBackground::isValid() const
{
    return m_bounds.isValid();
}

} // namespace YuchenUI
