// Frame.cpp
#include "YuchenUI/widgets/Frame.h"
#include "YuchenUI/theme/ThemeManager.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/core/Validation.h"
#include "YuchenUI/core/Assert.h"
#include "YuchenUI/core/UIContext.h"

namespace YuchenUI {

Frame::Frame(const Rect& bounds)
    : Widget(bounds)
    , m_backgroundColor()
    , m_borderColor()
    , m_borderWidth(0.0f)
    , m_cornerRadius()
    , m_hasCustomBackground(false)
    , m_hasCustomBorderColor(false)
{
}

Frame::~Frame() {
}

void Frame::addDrawCommands(RenderList& commandList, const Vec2& offset) const {
    if (!m_isVisible) return;
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    
    // Get style via UIContext instead of deprecated singleton
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    YUCHEN_ASSERT(style);
    
    FrameDrawInfo info;
    info.bounds = Rect(absPos.x, absPos.y, m_bounds.width, m_bounds.height);
    
    info.backgroundColor = m_hasCustomBackground ? m_backgroundColor
                                                 : style->getDefaultFrameBackground();
    info.borderColor = m_hasCustomBorderColor ? m_borderColor
                                              : style->getDefaultFrameBorder();
    
    info.borderWidth = m_borderWidth;
    info.cornerRadius = m_cornerRadius;
    
    style->drawFrame(info, commandList);
    
    renderChildren(commandList, absPos);
}

void Frame::setBackgroundColor(const Vec4& color) {
    Validation::AssertColor(color);
    m_backgroundColor = color;
    m_hasCustomBackground = true;
}

Vec4 Frame::getBackgroundColor() const {
    if (m_hasCustomBackground) {
        return m_backgroundColor;
    }
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    return style ? style->getDefaultFrameBackground() : Vec4();
}
void Frame::resetBackgroundColor() {
    m_hasCustomBackground = false;
    m_backgroundColor = Vec4();
}

void Frame::setBorderColor(const Vec4& color) {
    Validation::AssertColor(color);
    m_borderColor = color;
    m_hasCustomBorderColor = true;
}

Vec4 Frame::getBorderColor() const {
    if (m_hasCustomBorderColor) {
        return m_borderColor;
    }
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    return style ? style->getDefaultFrameBorder() : Vec4();
}

void Frame::resetBorderColor() {
    m_hasCustomBorderColor = false;
    m_borderColor = Vec4();
}

void Frame::setBorderWidth(float width) {
    YUCHEN_ASSERT(width >= 0.0f);
    m_borderWidth = width;
}

void Frame::setCornerRadius(const CornerRadius& radius) {
    Validation::AssertCornerRadius(radius);
    m_cornerRadius = radius;
}

void Frame::setCornerRadius(float radius) {
    YUCHEN_ASSERT(radius >= 0.0f);
    m_cornerRadius = CornerRadius(radius);
}

bool Frame::isValid() const {
    return m_bounds.isValid() &&
           m_borderWidth >= 0.0f &&
           m_cornerRadius.isValid();
}

}
