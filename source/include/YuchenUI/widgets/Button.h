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

class Button : public UIComponent {
public:
    explicit Button(const Rect& bounds);
    virtual ~Button();
    
    void addDrawCommands(RenderList& commandList, const Vec2& offset = Vec2()) const override;
    bool handleMouseMove(const Vec2& position, const Vec2& offset = Vec2()) override;
    bool handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset = Vec2()) override;
    
    void setText(const std::string& text);
    void setText(const char* text);
    const std::string& getText() const { return m_text; }
    
    void setWesternFont(FontHandle fontHandle);
    FontHandle getWesternFont() const;
    void resetWesternFont();
    
    void setChineseFont(FontHandle fontHandle);
    FontHandle getChineseFont() const;
    void resetChineseFont();
    
    void setFontSize(float fontSize);
    float getFontSize() const { return m_fontSize; }
    
    void setTextColor(const Vec4& color);
    Vec4 getTextColor() const;
    void resetTextColor();
    
    void setBounds(const Rect& bounds);
    const Rect& getBounds() const override { return m_bounds; }
    
    void setRole(ButtonRole role);
    ButtonRole getRole() const { return m_role; }
    
    void setClickCallback(ButtonClickCallback callback);
    
    bool isValid() const;
    
    bool canReceiveFocus() const override { return m_isEnabled && m_isVisible; }

protected:
    CornerRadius getFocusIndicatorCornerRadius() const override { return CornerRadius(2.0f); }
    
private:
    std::string m_text;
    FontHandle m_westernFontHandle;
    FontHandle m_chineseFontHandle;
    float m_fontSize;
    Vec4 m_textColor;
    
    Rect m_bounds;
    ButtonRole m_role;
    
    bool m_isHovered;
    bool m_isPressed;
    
    ButtonClickCallback m_clickCallback;
    
    bool m_hasCustomWesternFont;
    bool m_hasCustomChineseFont;
    bool m_hasCustomTextColor;
};

}
