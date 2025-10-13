#pragma once

#include "YuchenUI/widgets/UIComponent.h"
#include "YuchenUI/core/Types.h"
#include "YuchenUI/core/Config.h"
#include <string>

namespace YuchenUI {

class RenderList;

/**
    Text label widget with Qt-style font API.
    
    Version 3.0 Changes:
    - Replaced setWesternFont()/setChineseFont() → setFont()/setFontChain()
    - Simplified font management
    - Automatic CJK and emoji fallback
*/
class TextLabel : public UIComponent {
public:
    explicit TextLabel(const Rect& bounds);
    virtual ~TextLabel();
    
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
    
    /**
        Sets label font with automatic fallback.
        
        @param fontHandle  Primary font handle
    */
    void setFont(FontHandle fontHandle);
    
    /**
        Sets complete font fallback chain.
        
        @param chain  Font fallback chain
    */
    void setFontChain(const FontFallbackChain& chain);
    
    /**
        Returns current font fallback chain.
        
        @returns Current font fallback chain
    */
    FontFallbackChain getFontChain() const;
    
    /**
        Resets font to style default.
    */
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
    
    Vec2 measureText() const;
    bool isValid() const;
    
private:
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
    
    bool m_hasCustomFont;
    bool m_hasCustomTextColor;
};

}
