#pragma once

#include "YuchenUI/widgets/UIComponent.h"
#include "YuchenUI/core/Types.h"
#include <string>
#include <functional>

namespace YuchenUI {

class RenderList;

enum class CheckBoxState {
    Unchecked,
    Checked,
    Indeterminate
};

using CheckBoxStateChangedCallback = std::function<void(CheckBoxState)>;

class CheckBox : public UIComponent {
public:
    explicit CheckBox(const Rect& bounds);
    virtual ~CheckBox();
    
    void addDrawCommands(RenderList& commandList, const Vec2& offset = Vec2()) const override;
    bool handleMouseMove(const Vec2& position, const Vec2& offset = Vec2()) override;
    bool handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset = Vec2()) override;
    bool handleKeyPress(const Event& event) override;
    
    void setState(CheckBoxState state);
    CheckBoxState getState() const { return m_state; }
    
    bool isChecked() const { return m_state == CheckBoxState::Checked; }
    void setChecked(bool checked);
    
    void setText(const std::string& text);
    void setText(const char* text);
    const std::string& getText() const { return m_text; }
    
    void setFontSize(float fontSize);
    float getFontSize() const { return m_fontSize; }
    
    void setTextColor(const Vec4& color);
    Vec4 getTextColor() const;
    void resetTextColor();
    
    void setBounds(const Rect& bounds);
    const Rect& getBounds() const override { return m_bounds; }
    
    void setStateChangedCallback(CheckBoxStateChangedCallback callback);
    
    bool isValid() const;
    bool canReceiveFocus() const override { return m_isEnabled && m_isVisible; }

protected:
    CornerRadius getFocusIndicatorCornerRadius() const override { return CornerRadius(2.0f); }

private:
    void toggleState();
    Rect getCheckBoxRect(const Vec2& absPos) const;
    Rect getTextRect(const Vec2& absPos) const;
    
    static constexpr float CHECKBOX_SIZE = 14.0f;
    static constexpr float TEXT_SPACING = 6.0f;
    
    Rect m_bounds;
    CheckBoxState m_state;
    std::string m_text;
    float m_fontSize;
    Vec4 m_textColor;
    bool m_hasCustomTextColor;
    
    bool m_isHovered;
    bool m_isPressed;
    
    CheckBoxStateChangedCallback m_stateChangedCallback;
};

}
