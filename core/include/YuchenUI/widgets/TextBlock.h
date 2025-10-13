#pragma once

#include "YuchenUI/widgets/UIComponent.h"
#include "YuchenUI/core/Types.h"
#include "YuchenUI/core/Config.h"
#include <string>
#include <vector>

namespace YuchenUI {

class RenderList;

struct TextLine {
    std::string text;
    float width;
    float height;
    Vec2 position;
    
    TextLine() : text(), width(0.0f), height(0.0f), position() {}
};

/**
    Multi-line text block with automatic wrapping.
    
    Version 3.0 Changes:
    - Qt-style font API with automatic fallback
    - Simplified font management
*/
class TextBlock : public UIComponent {
public:
    explicit TextBlock(const Rect& bounds);
    virtual ~TextBlock();
    
    void addDrawCommands(RenderList& commandList, const Vec2& offset = Vec2()) const override;
    bool handleMouseMove(const Vec2& position, const Vec2& offset = Vec2()) override { return false; }
    bool handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset = Vec2()) override { return false; }
    
    //======================================================================================
    // Text API
    
    void setText(const std::string& text);
    void setText(const char* text);
    const std::string& getText() const { return m_text; }
    
    //======================================================================================
    // Font API (Qt-style, v3.0)
    
    void setFont(FontHandle fontHandle);
    void setFontChain(const FontFallbackChain& chain);
    FontFallbackChain getFontChain() const;
    void resetFont();
    
    //======================================================================================
    // Text Style API
    
    void setFontSize(float fontSize);
    float getFontSize() const { return m_fontSize; }
    
    void setTextColor(const Vec4& color);
    Vec4 getTextColor() const;
    void resetTextColor();
    
    //======================================================================================
    // Layout API
    
    void setBounds(const Rect& bounds);
    const Rect& getBounds() const override { return m_bounds; }
    
    void setAlignment(TextAlignment horizontal, VerticalAlignment vertical);
    void setHorizontalAlignment(TextAlignment alignment);
    void setVerticalAlignment(VerticalAlignment alignment);
    TextAlignment getHorizontalAlignment() const { return m_horizontalAlignment; }
    VerticalAlignment getVerticalAlignment() const { return m_verticalAlignment; }
    
    void setPadding(float left, float top, float right, float bottom);
    void setPadding(float padding);
    void getPadding(float& left, float& top, float& right, float& bottom) const;
    
    void setLineHeightMultiplier(float multiplier);
    float getLineHeightMultiplier() const { return m_lineHeightMultiplier; }
    
    void setParagraphSpacing(float spacing);
    float getParagraphSpacing() const { return m_paragraphSpacing; }
    
    Vec2 calculateContentSize() const;
    bool isValid() const;
    
private:
    void layoutText() const;
    std::vector<std::string> splitIntoParagraphs(const std::string& text) const;
    void layoutParagraph(const std::string& paragraph, float startY, std::vector<TextLine>& lines) const;
    std::string wrapLine(const std::string& text, float maxWidth, size_t& outConsumed) const;
    float measureTextWidth(const std::string& text) const;
    
    std::string m_text;
    FontFallbackChain m_fontChain;
    float m_fontSize;
    Vec4 m_textColor;
    Rect m_bounds;
    TextAlignment m_horizontalAlignment;
    VerticalAlignment m_verticalAlignment;
    float m_paddingLeft;
    float m_paddingTop;
    float m_paddingRight;
    float m_paddingBottom;
    float m_lineHeightMultiplier;
    float m_paragraphSpacing;
    
    bool m_hasCustomFont;
    bool m_hasCustomTextColor;
    
    mutable std::vector<TextLine> m_cachedLines;
    mutable bool m_needsLayout;
};

}
