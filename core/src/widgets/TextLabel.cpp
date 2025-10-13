#include "YuchenUI/widgets/TextLabel.h"
#include "YuchenUI/text/IFontProvider.h"
#include "YuchenUI/theme/IThemeProvider.h"
#include "YuchenUI/theme/Theme.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/core/UIContext.h"
#include "YuchenUI/core/Validation.h"

namespace YuchenUI {

TextLabel::TextLabel(const Rect& bounds)
    : m_text()
    , m_fontChain()
    , m_fontSize(Config::Font::DEFAULT_SIZE)
    , m_textColor()
    , m_bounds(bounds)
    , m_horizontalAlignment(TextAlignment::Left)
    , m_verticalAlignment(VerticalAlignment::Top)
    , m_paddingLeft(Config::Text::DEFAULT_PADDING)
    , m_paddingTop(Config::Text::DEFAULT_PADDING)
    , m_paddingRight(Config::Text::DEFAULT_PADDING)
    , m_paddingBottom(Config::Text::DEFAULT_PADDING)
    , m_hasCustomFont(false)
    , m_hasCustomTextColor(false)
{
    Validation::AssertRect(bounds);
    YUCHEN_ASSERT(bounds.width >= 0.0f && bounds.height >= 0.0f);
}

TextLabel::~TextLabel() {
}

void TextLabel::addDrawCommands(RenderList& commandList, const Vec2& offset) const {
    if (!isVisible() || m_text.empty()) return;
    
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    IFontProvider* fontProvider = m_ownerContext ? m_ownerContext->getFontProvider() : nullptr;
    YUCHEN_ASSERT(style);
    YUCHEN_ASSERT(fontProvider);
    
    FontFallbackChain fallbackChain = m_hasCustomFont
        ? m_fontChain
        : style->getDefaultLabelFontChain();
    
    Vec4 textColor = m_hasCustomTextColor ? m_textColor : style->getDefaultTextColor();
    
    Vec2 textSize = fontProvider->measureText(m_text.c_str(), m_fontSize);
    FontMetrics metrics = fontProvider->getFontMetrics(fallbackChain.getPrimary(), m_fontSize);
    
    Rect contentRect(
        m_paddingLeft,
        m_paddingTop,
        m_bounds.width - m_paddingLeft - m_paddingRight,
        m_bounds.height - m_paddingTop - m_paddingBottom
    );
    
    Rect absoluteBounds(
        m_bounds.x + offset.x,
        m_bounds.y + offset.y,
        m_bounds.width,
        m_bounds.height
    );
    
    Vec2 position;
    
    switch (m_horizontalAlignment) {
        case TextAlignment::Left:
            position.x = absoluteBounds.x + contentRect.x;
            break;
        case TextAlignment::Center:
            position.x = absoluteBounds.x + contentRect.x + (contentRect.width - textSize.x) * 0.5f;
            break;
        case TextAlignment::Right:
            position.x = absoluteBounds.x + contentRect.x + contentRect.width - textSize.x;
            break;
        case TextAlignment::Justify:
            position.x = absoluteBounds.x + contentRect.x;
            break;
    }
    
    switch (m_verticalAlignment) {
        case VerticalAlignment::Top:
            position.y = absoluteBounds.y + contentRect.y + metrics.ascender;
            break;
        case VerticalAlignment::Middle:
            position.y = absoluteBounds.y + contentRect.y + (contentRect.height - metrics.lineHeight) * 0.5f + metrics.ascender;
            break;
        case VerticalAlignment::Bottom:
            position.y = absoluteBounds.y + contentRect.y + contentRect.height - (metrics.lineHeight - metrics.ascender);
            break;
        case VerticalAlignment::Baseline:
            position.y = absoluteBounds.y + contentRect.y + metrics.ascender;
            break;
    }

    commandList.pushClipRect(absoluteBounds);
    commandList.drawText(m_text.c_str(), position, fallbackChain, m_fontSize, textColor);
    commandList.popClipRect();
}

void TextLabel::setText(const std::string& text) {
    m_text = text;
}

void TextLabel::setText(const char* text) {
    setText(std::string(text));
}

void TextLabel::setFont(FontHandle fontHandle) {
    IFontProvider* fontProvider = m_ownerContext ? m_ownerContext->getFontProvider() : nullptr;
    
    if (!fontProvider || !fontProvider->isValidFont(fontHandle)) {
        return;
    }
    
    // 自动构建带 CJK fallback 的链
    FontHandle cjkFont = fontProvider->getDefaultCJKFont();
    m_fontChain = FontFallbackChain(fontHandle, cjkFont);
    m_hasCustomFont = true;
}

void TextLabel::setFontChain(const FontFallbackChain& chain) {
    if (!chain.isValid()) {
        return;
    }
    
    m_fontChain = chain;
    m_hasCustomFont = true;
}

FontFallbackChain TextLabel::getFontChain() const {
    if (m_hasCustomFont) {
        return m_fontChain;
    }
    
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    return style ? style->getDefaultLabelFontChain() : FontFallbackChain();
}

void TextLabel::resetFont() {
    m_fontChain.clear();
    m_hasCustomFont = false;
}

void TextLabel::setFontSize(float fontSize) {
    if (fontSize >= Config::Font::MIN_SIZE && fontSize <= Config::Font::MAX_SIZE) {
        m_fontSize = fontSize;
    }
}

void TextLabel::setTextColor(const Vec4& color) {
    Validation::AssertColor(color);
    m_textColor = color;
    m_hasCustomTextColor = true;
}

Vec4 TextLabel::getTextColor() const {
    if (m_hasCustomTextColor) {
        return m_textColor;
    }
    // Get default color via UIContext instead of deprecated singleton
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    return style ? style->getDefaultTextColor() : Vec4();
}

void TextLabel::resetTextColor() {
    m_hasCustomTextColor = false;
    m_textColor = Vec4();
}

void TextLabel::setBounds(const Rect& bounds) {
    if (Validation::ValidateRect(bounds) && bounds.width >= 0.0f && bounds.height >= 0.0f) {
        m_bounds = bounds;
    }
}

void TextLabel::setAlignment(TextAlignment horizontal, VerticalAlignment vertical) {
    m_horizontalAlignment = horizontal;
    m_verticalAlignment = vertical;
}

void TextLabel::setHorizontalAlignment(TextAlignment alignment) {
    m_horizontalAlignment = alignment;
}

void TextLabel::setVerticalAlignment(VerticalAlignment alignment) {
    m_verticalAlignment = alignment;
}

void TextLabel::setPadding(float left, float top, float right, float bottom) {
    m_paddingLeft = left;
    m_paddingTop = top;
    m_paddingRight = right;
    m_paddingBottom = bottom;
}

void TextLabel::setPadding(float padding) {
    setPadding(padding, padding, padding, padding);
}

void TextLabel::getPadding(float& left, float& top, float& right, float& bottom) const {
    left = m_paddingLeft;
    top = m_paddingTop;
    right = m_paddingRight;
    bottom = m_paddingBottom;
}

Vec2 TextLabel::measureText() const {
    if (m_text.empty()) return Vec2();
    
    // Get font provider via UIContext instead of deprecated singleton
    IFontProvider* fontProvider = m_ownerContext ? m_ownerContext->getFontProvider() : nullptr;
    
    return fontProvider->measureText(m_text.c_str(), m_fontSize);
}

bool TextLabel::isValid() const {
    if (!Validation::ValidateRect(m_bounds)) return false;
    if (m_fontSize < Config::Font::MIN_SIZE || m_fontSize > Config::Font::MAX_SIZE) return false;
    return true;
}

}
