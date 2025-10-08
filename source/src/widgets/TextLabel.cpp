#include "YuchenUI/widgets/TextLabel.h"
#include "YuchenUI/text/FontManager.h"
#include "YuchenUI/theme/ThemeManager.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/core/Validation.h"

namespace YuchenUI {

TextLabel::TextLabel(const Rect& bounds)
    : m_text()
    , m_westernFontHandle(INVALID_FONT_HANDLE)
    , m_chineseFontHandle(INVALID_FONT_HANDLE)
    , m_fontSize(Config::Font::DEFAULT_SIZE)
    , m_textColor()
    , m_bounds(bounds)
    , m_horizontalAlignment(TextAlignment::Left)
    , m_verticalAlignment(VerticalAlignment::Top)
    , m_paddingLeft(Config::Text::DEFAULT_PADDING)
    , m_paddingTop(Config::Text::DEFAULT_PADDING)
    , m_paddingRight(Config::Text::DEFAULT_PADDING)
    , m_paddingBottom(Config::Text::DEFAULT_PADDING)
    , m_hasCustomWesternFont(false)
    , m_hasCustomChineseFont(false)
    , m_hasCustomTextColor(false)
{
    Validation::AssertRect(bounds);
    YUCHEN_ASSERT(bounds.width >= 0.0f && bounds.height >= 0.0f);
}

TextLabel::~TextLabel() {
}

void TextLabel::addDrawCommands(RenderList& commandList, const Vec2& offset) const {
    if (!isVisible() || m_text.empty()) return;
    
    UIStyle* style = ThemeManager::getInstance().getCurrentStyle();
    FontManager& fontManager = FontManager::getInstance();
    
    FontHandle westernFont = m_hasCustomWesternFont ? m_westernFontHandle : style->getDefaultLabelFont();
    FontHandle chineseFont = m_hasCustomChineseFont ? m_chineseFontHandle : fontManager.getPingFangFont();
    Vec4 textColor = m_hasCustomTextColor ? m_textColor : style->getDefaultTextColor();
    
    Vec2 textSize = fontManager.measureText(m_text.c_str(), m_fontSize);
    FontMetrics metrics = fontManager.getFontMetrics(westernFont, m_fontSize);
    
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
    commandList.drawText(m_text.c_str(), position, westernFont, chineseFont, m_fontSize, textColor);
    commandList.popClipRect();
}

void TextLabel::setText(const std::string& text) {
    m_text = text;
}

void TextLabel::setText(const char* text) {
    setText(std::string(text));
}

void TextLabel::setWesternFont(FontHandle fontHandle) {
    if (FontManager::getInstance().isValidFont(fontHandle)) {
        m_westernFontHandle = fontHandle;
        m_hasCustomWesternFont = true;
    }
}

FontHandle TextLabel::getWesternFont() const {
    if (m_hasCustomWesternFont) {
        return m_westernFontHandle;
    }
    return ThemeManager::getInstance().getCurrentStyle()->getDefaultLabelFont();
}

void TextLabel::resetWesternFont() {
    m_hasCustomWesternFont = false;
    m_westernFontHandle = INVALID_FONT_HANDLE;
}

void TextLabel::setChineseFont(FontHandle fontHandle) {
    if (FontManager::getInstance().isValidFont(fontHandle)) {
        m_chineseFontHandle = fontHandle;
        m_hasCustomChineseFont = true;
    }
}

FontHandle TextLabel::getChineseFont() const {
    if (m_hasCustomChineseFont) {
        return m_chineseFontHandle;
    }
    return FontManager::getInstance().getPingFangFont();
}

void TextLabel::resetChineseFont() {
    m_hasCustomChineseFont = false;
    m_chineseFontHandle = INVALID_FONT_HANDLE;
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
    return ThemeManager::getInstance().getCurrentStyle()->getDefaultTextColor();
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
    
    FontManager& fontManager = FontManager::getInstance();
    if (!fontManager.isInitialized()) return Vec2();
    
    return fontManager.measureText(m_text.c_str(), m_fontSize);
}

bool TextLabel::isValid() const {
    if (!Validation::ValidateRect(m_bounds)) return false;
    if (m_fontSize < Config::Font::MIN_SIZE || m_fontSize > Config::Font::MAX_SIZE) return false;
    return true;
}

}
