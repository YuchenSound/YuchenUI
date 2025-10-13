#pragma once

#include "YuchenUI/widgets/Widget.h"
#include "YuchenUI/core/Types.h"
#include <string>

namespace YuchenUI {

class RenderList;

/**
    GroupBox widget with Qt-style font API.
    
    Version 3.0 Changes:
    - Replaced setTitleFont() → setTitleFont()/setTitleFontChain()
    - Automatic font fallback for title text
*/
class GroupBox : public Widget {
public:
    explicit GroupBox(const Rect& bounds);
    virtual ~GroupBox();
    
    void addDrawCommands(RenderList& commandList, const Vec2& offset = Vec2()) const override;
    
    bool handleMouseMove(const Vec2& position, const Vec2& offset) override;
    bool handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset) override;
    bool handleMouseWheel(const Vec2& delta, const Vec2& position, const Vec2& offset) override;
    
    //======================================================================================
    // Title API
    
    void setTitle(const std::string& title);
    void setTitle(const char* title);
    const std::string& getTitle() const { return m_title; }
    
    //======================================================================================
    // Title Font API (Qt-style, v3.0)
    
    /**
        Sets title font with automatic fallback.
        
        @param fontHandle  Primary font handle for title
    */
    void setTitleFont(FontHandle fontHandle);
    
    /**
        Sets complete font fallback chain for title.
        
        @param chain  Font fallback chain
    */
    void setTitleFontChain(const FontFallbackChain& chain);
    
    /**
        Returns current title font fallback chain.
        
        @returns Current font fallback chain
    */
    FontFallbackChain getTitleFontChain() const;
    
    /**
        Resets title font to style default.
    */
    void resetTitleFont();
    
    //======================================================================================
    // Title Style API
    
    void setTitleFontSize(float fontSize);
    float getTitleFontSize() const { return m_titleFontSize; }
    
    void setTitleColor(const Vec4& color);
    Vec4 getTitleColor() const;
    void resetTitleColor();
    
    //======================================================================================
    // Appearance API
    
    void setBackgroundColor(const Vec4& color);
    Vec4 getBackgroundColor() const;
    void resetBackgroundColor();
    
    void setBorderColor(const Vec4& color);
    Vec4 getBorderColor() const;
    void resetBorderColor();
    
    void setBorderWidth(float width);
    float getBorderWidth() const { return m_borderWidth; }
    
    void setCornerRadius(const CornerRadius& radius);
    void setCornerRadius(float radius);
    const CornerRadius& getCornerRadius() const { return m_cornerRadius; }
    
    bool isValid() const;

private:
    std::string m_title;
    FontFallbackChain m_titleFontChain;
    float m_titleFontSize;
    Vec4 m_titleColor;
    Vec4 m_backgroundColor;
    Vec4 m_borderColor;
    float m_borderWidth;
    CornerRadius m_cornerRadius;
    
    bool m_hasCustomTitleFont;
    bool m_hasCustomTitleColor;
    bool m_hasCustomBackground;
    bool m_hasCustomBorderColor;
};

}
