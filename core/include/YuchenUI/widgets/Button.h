#pragma once

#include "YuchenUI/widgets/UIComponent.h"
#include "YuchenUI/core/Types.h"
#include <string>
#include <functional>

namespace YuchenUI {

class RenderList;

using ButtonClickCallback = std::function<void()>;

enum class ButtonRole {
    Normal,
    Confirm,
    Cancel
};

/**
    Button widget with Qt-style font API.
    
    Version 3.0 Changes:
    - Replaced setWesternFont()/setChineseFont() → setFont()/setFontChain()
    - Simplified from 4 font fields → 2 font fields
    - Font fallback handled automatically
    
    Migration:
    @code
    // Old API
    button->setWesternFont(arialFont);
    button->setChineseFont(cjkFont);
    
    // New API (simpler)
    button->setFont(arialFont);  // Automatically adds CJK fallback
    
    // Or explicit fallback chain
    button->setFontChain(FontFallbackChain(arialFont, cjkFont));
    @endcode
*/
class Button : public UIComponent {
public:
    explicit Button(const Rect& bounds);
    virtual ~Button();
    
    void addDrawCommands(RenderList& commandList, const Vec2& offset = Vec2()) const override;
    bool handleMouseMove(const Vec2& position, const Vec2& offset = Vec2()) override;
    bool handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset = Vec2()) override;
    
    //======================================================================================
    // Text API
    
    void setText(const std::string& text);
    void setText(const char* text);
    const std::string& getText() const { return m_text; }
    
    //======================================================================================
    // Font API (Qt-style, v3.0)
    
    /**
        Sets button font with automatic fallback.
        
        The system automatically adds appropriate CJK fallback fonts.
        
        @param fontHandle  Primary font handle
    */
    void setFont(FontHandle fontHandle);
    
    /**
        Sets complete font fallback chain.
        
        For full control over font fallback, including emoji and symbol fonts.
        
        Example:
        @code
        FontFallbackChain chain(arialFont, cjkFont, emojiFont);
        button->setFontChain(chain);
        @endcode
        
        @param chain  Font fallback chain
    */
    void setFontChain(const FontFallbackChain& chain);
    
    /**
        Returns current font fallback chain.
        
        If custom font not set, returns style's default button font chain.
        
        @returns Current font fallback chain
    */
    FontFallbackChain getFontChain() const;
    
    /**
        Resets font to style default.
        
        Clears any custom font and uses style's default button font chain.
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
    // Geometry API
    
    void setBounds(const Rect& bounds);
    const Rect& getBounds() const override { return m_bounds; }
    
    //======================================================================================
    // Button Behavior API
    
    void setRole(ButtonRole role);
    ButtonRole getRole() const { return m_role; }
    
    void setClickCallback(ButtonClickCallback callback);
    
    bool isValid() const;
    
protected:
    CornerRadius getFocusIndicatorCornerRadius() const override { return CornerRadius(2.0f); }
    
private:
    std::string m_text;
    FontFallbackChain m_fontChain;  // ✅ 新：统一的字体链
    float m_fontSize;
    Vec4 m_textColor;
    
    Rect m_bounds;
    ButtonRole m_role;
    
    bool m_isHovered;
    bool m_isPressed;
    
    ButtonClickCallback m_clickCallback;
    
    bool m_hasCustomFont;        // ✅ 新：简化为一个标志
    bool m_hasCustomTextColor;
};

}
