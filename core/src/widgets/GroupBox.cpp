// GroupBox.cpp
#include "YuchenUI/widgets/GroupBox.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/text/IFontProvider.h"
#include "YuchenUI/theme/IThemeProvider.h"
#include "YuchenUI/theme/Theme.h"
#include "YuchenUI/core/Validation.h"
#include "YuchenUI/core/Config.h"
#include "YuchenUI/core/Assert.h"
#include "YuchenUI/core/UIContext.h"

namespace YuchenUI {

GroupBox::GroupBox(const Rect& bounds)
    : Widget(bounds)
    , m_title()
    , m_titleFont(INVALID_FONT_HANDLE)
    , m_titleFontSize(Config::Font::DEFAULT_SIZE)
    , m_titleColor()
    , m_backgroundColor()
    , m_borderColor()
    , m_borderWidth(1.0f)
    , m_cornerRadius()
    , m_hasCustomTitleFont(false)
    , m_hasCustomTitleColor(false)
    , m_hasCustomBackground(false)
    , m_hasCustomBorderColor(false)
{
}

GroupBox::~GroupBox() {
}

void GroupBox::addDrawCommands(RenderList& commandList, const Vec2& offset) const {
    if (!m_isVisible) return;
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    
    // Get style via UIContext instead of deprecated ThemeManager singleton
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    YUCHEN_ASSERT(style);
    
    GroupBoxDrawInfo info;
    info.bounds = Rect(absPos.x, absPos.y, m_bounds.width, m_bounds.height);
    info.title = m_title;
    
    info.titleFont = m_hasCustomTitleFont ? m_titleFont
                                          : style->getDefaultTitleFont();
    info.titleColor = m_hasCustomTitleColor ? m_titleColor
                                            : style->getDefaultTextColor();
    info.backgroundColor = m_hasCustomBackground ? m_backgroundColor
                                                 : style->getDefaultGroupBoxBackground();
    info.borderColor = m_hasCustomBorderColor ? m_borderColor
                                              : style->getDefaultGroupBoxBorder();
    
    info.titleFontSize = m_titleFontSize;
    info.borderWidth = m_borderWidth;
    info.cornerRadius = m_cornerRadius;
    
    style->drawGroupBox(info, commandList);
    
    float titleBarHeight = style->getGroupBoxTitleBarHeight();
    Vec2 contentOffset(absPos.x, absPos.y + titleBarHeight);
    renderChildren(commandList, contentOffset);
}

void GroupBox::setTitle(const std::string& title) {
    m_title = title;
}

void GroupBox::setTitle(const char* title) {
    YUCHEN_ASSERT(title);
    m_title = title;
}

void GroupBox::setTitleFont(FontHandle fontHandle) {
    IFontProvider* fontProvider = m_ownerContext ? m_ownerContext->getFontProvider() : nullptr;
    if (fontProvider && fontProvider->isValidFont(fontHandle)) {
        m_titleFont = fontHandle;
        m_hasCustomTitleFont = true;
    }
}

FontHandle GroupBox::getTitleFont() const {
    if (m_hasCustomTitleFont) {
        return m_titleFont;
    }
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    return style ? style->getDefaultTitleFont() : INVALID_FONT_HANDLE;
}

void GroupBox::resetTitleFont() {
    m_hasCustomTitleFont = false;
    m_titleFont = INVALID_FONT_HANDLE;
}

void GroupBox::setTitleFontSize(float fontSize) {
    if (fontSize >= Config::Font::MIN_SIZE && fontSize <= Config::Font::MAX_SIZE) {
        m_titleFontSize = fontSize;
    }
}

void GroupBox::setTitleColor(const Vec4& color) {
    Validation::AssertColor(color);
    m_titleColor = color;
    m_hasCustomTitleColor = true;
}

Vec4 GroupBox::getTitleColor() const {
    if (m_hasCustomTitleColor) {
        return m_titleColor;
    }
    // Get default color via UIContext instead of deprecated singleton
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    return style ? style->getDefaultTextColor() : Vec4();
}

void GroupBox::resetTitleColor() {
    m_hasCustomTitleColor = false;
    m_titleColor = Vec4();
}

void GroupBox::setBackgroundColor(const Vec4& color) {
    Validation::AssertColor(color);
    m_backgroundColor = color;
    m_hasCustomBackground = true;
}

Vec4 GroupBox::getBackgroundColor() const {
    if (m_hasCustomBackground) {
        return m_backgroundColor;
    }
    // Get default background via UIContext instead of deprecated singleton
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    return style ? style->getDefaultGroupBoxBackground() : Vec4();
}

void GroupBox::resetBackgroundColor() {
    m_hasCustomBackground = false;
    m_backgroundColor = Vec4();
}

void GroupBox::setBorderColor(const Vec4& color) {
    Validation::AssertColor(color);
    m_borderColor = color;
    m_hasCustomBorderColor = true;
}

Vec4 GroupBox::getBorderColor() const {
    if (m_hasCustomBorderColor) {
        return m_borderColor;
    }
    // Get default border via UIContext instead of deprecated singleton
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    return style ? style->getDefaultGroupBoxBorder() : Vec4();
}

void GroupBox::resetBorderColor() {
    m_hasCustomBorderColor = false;
    m_borderColor = Vec4();
}

void GroupBox::setBorderWidth(float width) {
    YUCHEN_ASSERT(width >= 0.0f);
    m_borderWidth = width;
}

void GroupBox::setCornerRadius(const CornerRadius& radius) {
    Validation::AssertCornerRadius(radius);
    m_cornerRadius = radius;
}

void GroupBox::setCornerRadius(float radius) {
    YUCHEN_ASSERT(radius >= 0.0f);
    m_cornerRadius = CornerRadius(radius);
}

bool GroupBox::isValid() const {
    return m_bounds.isValid() &&
           m_titleFontSize >= Config::Font::MIN_SIZE &&
           m_titleFontSize <= Config::Font::MAX_SIZE &&
           m_borderWidth >= 0.0f &&
           m_cornerRadius.isValid();
}

bool GroupBox::handleMouseMove(const Vec2& position, const Vec2& offset) {
    if (!m_isEnabled || !m_isVisible) return false;
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect absRect(absPos.x, absPos.y, m_bounds.width, m_bounds.height);
    
    if (!absRect.contains(position)) return false;
    
    // Get style via UIContext instead of deprecated singleton
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    YUCHEN_ASSERT(style);
    float titleBarHeight = style->getGroupBoxTitleBarHeight();
    
    Vec2 contentOffset(absPos.x, absPos.y + titleBarHeight);
    
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        if ((*it) && (*it)->isVisible() && (*it)->isEnabled()) {
            if ((*it)->handleMouseMove(position, contentOffset)) {
                return true;
            }
        }
    }
    
    return false;
}

bool GroupBox::handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset) {
    if (!m_isEnabled || !m_isVisible) return false;
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect absRect(absPos.x, absPos.y, m_bounds.width, m_bounds.height);
    
    if (!absRect.contains(position)) return false;
    
    // Get style via UIContext instead of deprecated singleton
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    YUCHEN_ASSERT(style);
    float titleBarHeight = style->getGroupBoxTitleBarHeight();
    
    Vec2 contentOffset(absPos.x, absPos.y + titleBarHeight);
    
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        if ((*it) && (*it)->isVisible()) {
            if ((*it)->handleMouseClick(position, pressed, contentOffset)) {
                return true;
            }
        }
    }
    
    return false;
}

bool GroupBox::handleMouseWheel(const Vec2& delta, const Vec2& position, const Vec2& offset) {
    if (!m_isEnabled || !m_isVisible) return false;
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect absRect(absPos.x, absPos.y, m_bounds.width, m_bounds.height);
    
    if (!absRect.contains(position)) return false;
    
    // Get style via UIContext instead of deprecated singleton
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    YUCHEN_ASSERT(style);
    float titleBarHeight = style->getGroupBoxTitleBarHeight();
    
    Vec2 contentOffset(absPos.x, absPos.y + titleBarHeight);
    
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        if ((*it) && (*it)->isVisible() && (*it)->isEnabled()) {
            if ((*it)->handleMouseWheel(delta, position, contentOffset)) {
                return true;
            }
        }
    }
    
    return false;
}

}
