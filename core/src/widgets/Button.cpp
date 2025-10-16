#include "YuchenUI/widgets/Button.h"
#include "YuchenUI/text/IFontProvider.h"
#include "YuchenUI/theme/IThemeProvider.h"
#include "YuchenUI/theme/Theme.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/core/Validation.h"
#include "YuchenUI/core/Config.h"
#include "YuchenUI/core/UIContext.h"

namespace YuchenUI {

Button::Button(const Rect& bounds)
    : Widget()
    , m_text()
    , m_fontChain()
    , m_fontSize(Config::Font::DEFAULT_SIZE)
    , m_textColor()
    , m_role(ButtonRole::Normal)
    , m_isHovered(false)
    , m_isPressed(false)
    , m_clickCallback(nullptr)
    , m_hasCustomFont(false)
    , m_hasCustomTextColor(false)
{
    Validation::AssertRect(bounds);
    setBounds(bounds);
    setFocusPolicy(FocusPolicy::StrongFocus);
}

Button::~Button()
{
}

void Button::addDrawCommands(RenderList& commandList, const Vec2& offset) const
{
    YUCHEN_ASSERT(isValid());
    if (!m_isVisible) return;
    
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    
    ButtonDrawInfo info;
    info.bounds = Rect(m_bounds.x + offset.x, m_bounds.y + offset.y, m_bounds.width, m_bounds.height);
    info.text = m_text;
    info.fallbackChain = m_hasCustomFont ? m_fontChain : style->getDefaultButtonFontChain();
    info.textColor = m_hasCustomTextColor ? m_textColor : style->getDefaultTextColor();
    info.fontSize = m_fontSize;
    info.isHovered = m_isHovered;
    info.isPressed = m_isPressed;
    info.isEnabled = m_isEnabled;
    
    switch (m_role)
    {
        case ButtonRole::Primary:
            style->drawPrimaryButton(info, commandList);
            break;
        case ButtonRole::Destructive:
            // TODO: This is a Red Button, Need to expand the resource library!
            break;
        default:
            style->drawNormalButton(info, commandList);
            break;
    }
    
    drawFocusIndicator(commandList, offset);
}

void Button::setText(const std::string& text)
{
    m_text = text;
}

void Button::setText(const char* text)
{
    m_text = text;
}

void Button::setFont(FontHandle fontHandle)
{
    IFontProvider* fontProvider = m_ownerContext ? m_ownerContext->getFontProvider() : nullptr;
    if (!fontProvider || !fontProvider->isValidFont(fontHandle)) return;
    
    FontHandle cjkFont = fontProvider->getDefaultCJKFont();
    m_fontChain = FontFallbackChain(fontHandle, cjkFont);
    m_hasCustomFont = true;
}

void Button::setFontChain(const FontFallbackChain& chain)
{
    if (!chain.isValid()) return;
    
    m_fontChain = chain;
    m_hasCustomFont = true;
}

FontFallbackChain Button::getFontChain() const
{
    if (m_hasCustomFont) return m_fontChain;
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    return style ? style->getDefaultButtonFontChain() : FontFallbackChain();
}

void Button::resetFont()
{
    m_fontChain.clear();
    m_hasCustomFont = false;
}

void Button::setFontSize(float fontSize)
{
    if (fontSize >= Config::Font::MIN_SIZE && fontSize <= Config::Font::MAX_SIZE)
        m_fontSize = fontSize;
}

void Button::setTextColor(const Vec4& color)
{
    Validation::AssertColor(color);
    m_textColor = color;
    m_hasCustomTextColor = true;
}

Vec4 Button::getTextColor() const
{
    if (m_hasCustomTextColor) return m_textColor;
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    return style ? style->getDefaultTextColor() : Vec4();
}

void Button::resetTextColor()
{
    m_hasCustomTextColor = false;
    m_textColor = Vec4();
}

void Button::setRole(ButtonRole role)
{
    m_role = role;
}

void Button::setClickCallback(ButtonClickCallback callback)
{
    m_clickCallback = callback;
}

bool Button::handleMouseMove(const Vec2& position, const Vec2& offset)
{
    if (!m_isEnabled || !m_isVisible) return false;
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect absRect(absPos.x, absPos.y, m_bounds.width, m_bounds.height);
    
    bool wasHovered = m_isHovered;
    m_isHovered = absRect.contains(position);
    
    return wasHovered != m_isHovered;
}

bool Button::handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset)
{
    if (!m_isEnabled || !m_isVisible) return false;
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect absRect(absPos.x, absPos.y, m_bounds.width, m_bounds.height);
    
    bool isInBounds = absRect.contains(position);
    
    if (pressed && isInBounds)
    {
        m_isPressed = true;
        return true;
    }
    else if (!pressed && m_isPressed)
    {
        m_isPressed = false;
        if (isInBounds && m_clickCallback)
            m_clickCallback();
        return true;
    }
    
    return false;
}

bool Button::isValid() const
{
    return m_bounds.isValid() &&
           m_fontSize >= Config::Font::MIN_SIZE &&
           m_fontSize <= Config::Font::MAX_SIZE;
}

} // namespace YuchenUI
