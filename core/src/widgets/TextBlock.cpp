#include "YuchenUI/widgets/TextBlock.h"
#include "YuchenUI/text/IFontProvider.h"
#include "YuchenUI/theme/IThemeProvider.h"
#include "YuchenUI/theme/Theme.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/core/UIContext.h"
#include "YuchenUI/core/Validation.h"
#include "YuchenUI/core/Assert.h"

namespace YuchenUI {

TextBlock::TextBlock(const Rect& bounds)
    : m_text()
    , m_fontChain()              // ✅ 新：统一的字体链
    , m_fontSize(Config::Font::DEFAULT_SIZE)
    , m_textColor()
    , m_bounds(bounds)
    , m_horizontalAlignment(TextAlignment::Left)
    , m_verticalAlignment(VerticalAlignment::Top)
    , m_paddingLeft(Config::Text::DEFAULT_PADDING)
    , m_paddingTop(Config::Text::DEFAULT_PADDING)
    , m_paddingRight(Config::Text::DEFAULT_PADDING)
    , m_paddingBottom(Config::Text::DEFAULT_PADDING)
    , m_lineHeightMultiplier(1.2f)
    , m_paragraphSpacing(0.0f)
    , m_hasCustomFont(false)     // ✅ 新：统一的标志
    , m_hasCustomTextColor(false)
    , m_cachedLines()
    , m_needsLayout(true)
{
    Validation::AssertRect(bounds);
    YUCHEN_ASSERT(bounds.width >= 0.0f && bounds.height >= 0.0f);
}

TextBlock::~TextBlock() {
}

void TextBlock::addDrawCommands(RenderList& commandList, const Vec2& offset) const {
    if (!isVisible() || m_text.empty()) return;
    
    if (m_needsLayout) {
        layoutText();
        m_needsLayout = false;
    }
    
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    IFontProvider* fontProvider = m_ownerContext ? m_ownerContext->getFontProvider() : nullptr;
    YUCHEN_ASSERT(style);
    YUCHEN_ASSERT(fontProvider);
    
    FontFallbackChain fallbackChain = m_hasCustomFont
        ? m_fontChain
        : style->getDefaultLabelFontChain();
    
    Vec4 textColor = m_hasCustomTextColor ? m_textColor : style->getDefaultTextColor();
    
    Rect absoluteBounds(
        m_bounds.x + offset.x,
        m_bounds.y + offset.y,
        m_bounds.width,
        m_bounds.height
    );
    
    commandList.pushClipRect(absoluteBounds);
    
    for (const auto& line : m_cachedLines) {
        if (line.text.empty()) continue;
        
        Vec2 position(
            absoluteBounds.x + line.position.x,
            absoluteBounds.y + line.position.y
        );
        commandList.drawText(line.text.c_str(), position, fallbackChain, m_fontSize, textColor);
    }
    
    commandList.popClipRect();
}

void TextBlock::setText(const std::string& text) {
    if (m_text != text) {
        m_text = text;
        m_needsLayout = true;
    }
}

void TextBlock::setText(const char* text) {
    setText(std::string(text));
}

void TextBlock::setFont(FontHandle fontHandle) {
    IFontProvider* fontProvider = m_ownerContext ? m_ownerContext->getFontProvider() : nullptr;
    
    if (!fontProvider || !fontProvider->isValidFont(fontHandle)) {
        return;
    }
    
    // 自动构建带 CJK fallback 的链
    FontHandle cjkFont = fontProvider->getDefaultCJKFont();
    m_fontChain = FontFallbackChain(fontHandle, cjkFont);
    m_hasCustomFont = true;
    m_needsLayout = true;
}

void TextBlock::setFontChain(const FontFallbackChain& chain) {
    if (!chain.isValid()) {
        return;
    }
    
    m_fontChain = chain;
    m_hasCustomFont = true;
    m_needsLayout = true;
}

FontFallbackChain TextBlock::getFontChain() const {
    if (m_hasCustomFont) {
        return m_fontChain;
    }
    
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    return style ? style->getDefaultLabelFontChain() : FontFallbackChain();
}

void TextBlock::resetFont() {
    m_fontChain.clear();
    m_hasCustomFont = false;
    m_needsLayout = true;
}

void TextBlock::setFontSize(float fontSize) {
    if (fontSize >= Config::Font::MIN_SIZE && fontSize <= Config::Font::MAX_SIZE) {
        if (m_fontSize != fontSize) {
            m_fontSize = fontSize;
            m_needsLayout = true;
        }
    }
}

void TextBlock::setTextColor(const Vec4& color) {
    Validation::AssertColor(color);
    m_textColor = color;
    m_hasCustomTextColor = true;
}

Vec4 TextBlock::getTextColor() const {
    if (m_hasCustomTextColor) {
        return m_textColor;
    }
    // Get default color via UIContext instead of deprecated singleton
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    return style ? style->getDefaultTextColor() : Vec4();
}

void TextBlock::resetTextColor() {
    m_hasCustomTextColor = false;
    m_textColor = Vec4();
}

void TextBlock::setBounds(const Rect& bounds) {
    if (Validation::ValidateRect(bounds) && bounds.width >= 0.0f && bounds.height >= 0.0f) {
        if (m_bounds.width != bounds.width || m_bounds.height != bounds.height) {
            m_needsLayout = true;
        }
        m_bounds = bounds;
    }
}

void TextBlock::setAlignment(TextAlignment horizontal, VerticalAlignment vertical) {
    if (m_horizontalAlignment != horizontal || m_verticalAlignment != vertical) {
        m_horizontalAlignment = horizontal;
        m_verticalAlignment = vertical;
        m_needsLayout = true;
    }
}

void TextBlock::setHorizontalAlignment(TextAlignment alignment) {
    if (m_horizontalAlignment != alignment) {
        m_horizontalAlignment = alignment;
        m_needsLayout = true;
    }
}

void TextBlock::setVerticalAlignment(VerticalAlignment alignment) {
    if (m_verticalAlignment != alignment) {
        m_verticalAlignment = alignment;
        m_needsLayout = true;
    }
}

void TextBlock::setPadding(float left, float top, float right, float bottom) {
    if (m_paddingLeft != left || m_paddingTop != top ||
        m_paddingRight != right || m_paddingBottom != bottom) {
        m_paddingLeft = left;
        m_paddingTop = top;
        m_paddingRight = right;
        m_paddingBottom = bottom;
        m_needsLayout = true;
    }
}

void TextBlock::setPadding(float padding) {
    setPadding(padding, padding, padding, padding);
}

void TextBlock::getPadding(float& left, float& top, float& right, float& bottom) const {
    left = m_paddingLeft;
    top = m_paddingTop;
    right = m_paddingRight;
    bottom = m_paddingBottom;
}

void TextBlock::setLineHeightMultiplier(float multiplier) {
    YUCHEN_ASSERT(multiplier > 0.0f);
    if (m_lineHeightMultiplier != multiplier) {
        m_lineHeightMultiplier = multiplier;
        m_needsLayout = true;
    }
}

void TextBlock::setParagraphSpacing(float spacing) {
    YUCHEN_ASSERT(spacing >= 0.0f);
    if (m_paragraphSpacing != spacing) {
        m_paragraphSpacing = spacing;
        m_needsLayout = true;
    }
}

Vec2 TextBlock::calculateContentSize() const {
    if (m_needsLayout) {
        layoutText();
        m_needsLayout = false;
    }
    
    if (m_cachedLines.empty()) {
        return Vec2();
    }
    
    float maxWidth = 0.0f;
    float totalHeight = 0.0f;
    
    for (const auto& line : m_cachedLines) {
        maxWidth = std::max(maxWidth, line.width);
        totalHeight = line.position.y + line.height;
    }
    
    return Vec2(maxWidth + m_paddingLeft + m_paddingRight,
                totalHeight + m_paddingTop + m_paddingBottom);
}

bool TextBlock::isValid() const {
    if (!Validation::ValidateRect(m_bounds)) return false;
    if (m_fontSize < Config::Font::MIN_SIZE || m_fontSize > Config::Font::MAX_SIZE) return false;
    if (m_lineHeightMultiplier <= 0.0f) return false;
    if (m_paragraphSpacing < 0.0f) return false;
    return true;
}

void TextBlock::layoutText() const {
    m_cachedLines.clear();
    
    if (m_text.empty()) return;
    
    // Get font provider and style via UIContext instead of deprecated singletons
    IFontProvider* fontProvider = m_ownerContext ? m_ownerContext->getFontProvider() : nullptr;
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    YUCHEN_ASSERT(fontProvider);
    YUCHEN_ASSERT(style);
    
    FontFallbackChain fallbackChain = m_hasCustomFont
        ? m_fontChain
        : style->getDefaultLabelFontChain();
    FontHandle westernFont = fallbackChain.getPrimary();
    FontMetrics metrics = fontProvider->getFontMetrics(westernFont, m_fontSize);
    
    float lineHeight[[maybe_unused]] = metrics.lineHeight * m_lineHeightMultiplier;
    float contentWidth = m_bounds.width - m_paddingLeft - m_paddingRight;
    
    if (contentWidth <= 0.0f) return;
    
    std::vector<std::string> paragraphs = splitIntoParagraphs(m_text);
    float currentY = m_paddingTop;
    
    for (size_t i = 0; i < paragraphs.size(); ++i) {
        layoutParagraph(paragraphs[i], currentY, m_cachedLines);
        
        if (!m_cachedLines.empty()) {
            const TextLine& lastLine = m_cachedLines.back();
            currentY = lastLine.position.y + lastLine.height;
            
            if (i < paragraphs.size() - 1) {
                currentY += m_paragraphSpacing;
            }
        }
    }
    
    if (m_verticalAlignment != VerticalAlignment::Top && !m_cachedLines.empty()) {
        float contentHeight = m_bounds.height - m_paddingTop - m_paddingBottom;
        float totalTextHeight = m_cachedLines.back().position.y + m_cachedLines.back().height - m_paddingTop;
        
        float offsetY = 0.0f;
        switch (m_verticalAlignment) {
            case VerticalAlignment::Middle:
                offsetY = (contentHeight - totalTextHeight) * 0.5f;
                break;
            case VerticalAlignment::Bottom:
                offsetY = contentHeight - totalTextHeight;
                break;
            default:
                break;
        }
        
        if (offsetY > 0.0f) {
            for (auto& line : m_cachedLines) {
                line.position.y += offsetY;
            }
        }
    }
}

std::vector<std::string> TextBlock::splitIntoParagraphs(const std::string& text) const {
    std::vector<std::string> paragraphs;
    size_t start = 0;
    size_t pos = 0;
    
    while (pos < text.length()) {
        if (text[pos] == '\n') {
            paragraphs.push_back(text.substr(start, pos - start));
            start = pos + 1;
        }
        ++pos;
    }
    
    if (start < text.length()) {
        paragraphs.push_back(text.substr(start));
    }
    
    if (paragraphs.empty()) {
        paragraphs.push_back("");
    }
    
    return paragraphs;
}

void TextBlock::layoutParagraph(const std::string& paragraph, float startY, std::vector<TextLine>& lines) const {
    // Get font provider and style via UIContext
    IFontProvider* fontProvider = m_ownerContext ? m_ownerContext->getFontProvider() : nullptr;
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    YUCHEN_ASSERT(fontProvider);
    YUCHEN_ASSERT(style);
    
    if (paragraph.empty()) {
        FontFallbackChain fallbackChain = m_hasCustomFont
            ? m_fontChain
            : style->getDefaultLabelFontChain();
        FontHandle westernFont = fallbackChain.getPrimary();
        FontMetrics metrics = fontProvider->getFontMetrics(westernFont, m_fontSize);
        float lineHeight = metrics.lineHeight * m_lineHeightMultiplier;
        
        TextLine emptyLine;
        emptyLine.text = "";
        emptyLine.width = 0.0f;
        emptyLine.height = lineHeight;
        emptyLine.position = Vec2(m_paddingLeft, startY);
        lines.push_back(emptyLine);
        return;
    }
    
    FontFallbackChain fallbackChain = m_hasCustomFont
        ? m_fontChain
        : style->getDefaultLabelFontChain();
    FontHandle westernFont = fallbackChain.getPrimary();
    FontMetrics metrics = fontProvider->getFontMetrics(westernFont, m_fontSize);
    
    float lineHeight = metrics.lineHeight * m_lineHeightMultiplier;
    float contentWidth = m_bounds.width - m_paddingLeft - m_paddingRight;
    float currentY = startY;
    
    size_t offset = 0;
    while (offset < paragraph.length()) {
        size_t consumed = 0;
        std::string lineText = wrapLine(paragraph.substr(offset), contentWidth, consumed);
        
        if (consumed == 0) break;
        
        float textWidth = measureTextWidth(lineText);
        
        TextLine line;
        line.text = lineText;
        line.width = textWidth;
        line.height = lineHeight;
        
        float xPos = m_paddingLeft;
        switch (m_horizontalAlignment) {
            case TextAlignment::Center:
                xPos = m_paddingLeft + (contentWidth - textWidth) * 0.5f;
                break;
            case TextAlignment::Right:
                xPos = m_paddingLeft + contentWidth - textWidth;
                break;
            case TextAlignment::Justify:
                xPos = m_paddingLeft;
                break;
            default:
                xPos = m_paddingLeft;
                break;
        }
        
        line.position = Vec2(xPos, currentY + metrics.ascender);
        lines.push_back(line);
        
        currentY += lineHeight;
        offset += consumed;
    }
}

std::string TextBlock::wrapLine(const std::string& text, float maxWidth, size_t& outConsumed) const {
    if (text.empty()) {
        outConsumed = 0;
        return "";
    }
    
    float currentWidth = 0.0f;
    size_t lastBreakPos = 0;
    size_t currentPos = 0;
    bool hasContent = false;
    
    while (currentPos < text.length()) {
        size_t nextPos = currentPos + 1;
        
        unsigned char c = static_cast<unsigned char>(text[currentPos]);
        if ((c & 0x80) != 0) {
            if ((c & 0xE0) == 0xC0) nextPos = currentPos + 2;
            else if ((c & 0xF0) == 0xE0) nextPos = currentPos + 3;
            else if ((c & 0xF8) == 0xF0) nextPos = currentPos + 4;
        }
        
        if (nextPos > text.length()) break;
        
        std::string charStr = text.substr(currentPos, nextPos - currentPos);
        float charWidth = measureTextWidth(charStr);
        
        if (currentWidth + charWidth > maxWidth) {
            if (hasContent) {
                if (lastBreakPos > 0) {
                    outConsumed = lastBreakPos;
                    return text.substr(0, lastBreakPos);
                }
                outConsumed = currentPos;
                return text.substr(0, currentPos);
            }
            
            outConsumed = nextPos;
            return charStr;
        }
        
        currentWidth += charWidth;
        hasContent = true;
        
        if (text[currentPos] == ' ' || text[currentPos] == '\t') {
            lastBreakPos = nextPos;
        } else if ((c & 0x80) != 0) {
            lastBreakPos = nextPos;
        }
        
        currentPos = nextPos;
    }
    
    outConsumed = text.length();
    return text;
}

float TextBlock::measureTextWidth(const std::string& text) const {
    if (text.empty()) return 0.0f;
    
    // Get font provider via UIContext instead of deprecated singleton
    IFontProvider* fontProvider = m_ownerContext ? m_ownerContext->getFontProvider() : nullptr;
    if (!fontProvider) return 0.0f;
    
    Vec2 size = fontProvider->measureText(text.c_str(), m_fontSize);
    return size.x;
}

}
