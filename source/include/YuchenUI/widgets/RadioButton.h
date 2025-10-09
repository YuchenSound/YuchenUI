#pragma once

#include "YuchenUI/widgets/UIComponent.h"
#include "YuchenUI/core/Types.h"
#include <string>
#include <functional>

namespace YuchenUI {

class RenderList;
class RadioButtonGroup;

using RadioButtonCheckedCallback = std::function<void(bool)>;

class RadioButton : public UIComponent {
public:
    explicit RadioButton(const Rect& bounds);
    virtual ~RadioButton();
    
    void addDrawCommands(RenderList& commandList, const Vec2& offset = Vec2()) const override;
    bool handleMouseMove(const Vec2& position, const Vec2& offset = Vec2()) override;
    bool handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset = Vec2()) override;
    bool handleKeyPress(const Event& event) override;
    
    void setChecked(bool checked);
    bool isChecked() const { return m_isChecked; }
    
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
    
    void setCheckedCallback(RadioButtonCheckedCallback callback);
    
    void setGroup(RadioButtonGroup* group);
    RadioButtonGroup* getGroup() const { return m_group; }
    
    bool isValid() const;
    bool canReceiveFocus() const override { return m_isEnabled && m_isVisible; }

protected:
    CornerRadius getFocusIndicatorCornerRadius() const override { return CornerRadius(7.0f); }

private:
    void internalSetChecked(bool checked);
    
    static constexpr float RADIO_SIZE = 14.0f;
    static constexpr float TEXT_SPACING = 6.0f;
    
    Rect m_bounds;
    bool m_isChecked;
    std::string m_text;
    float m_fontSize;
    Vec4 m_textColor;
    bool m_hasCustomTextColor;
    
    bool m_isHovered;
    bool m_isPressed;
    
    RadioButtonGroup* m_group;
    RadioButtonCheckedCallback m_checkedCallback;
    
    friend class RadioButtonGroup;
};

using RadioButtonGroupSelectionCallback = std::function<void(int, RadioButton*)>;

class RadioButtonGroup {
public:
    RadioButtonGroup();
    ~RadioButtonGroup();
    
    void addButton(RadioButton* button);
    void removeButton(RadioButton* button);
    void clearButtons();
    
    void setCheckedButton(RadioButton* button);
    void setCheckedIndex(int index);
    
    RadioButton* getCheckedButton() const;
    int getCheckedIndex() const;
    
    size_t getButtonCount() const { return m_buttons.size(); }
    RadioButton* getButton(size_t index) const;
    
    void setSelectionCallback(RadioButtonGroupSelectionCallback callback);
    
    void selectNext(RadioButton* current);
    void selectPrevious(RadioButton* current);

private:
    std::vector<RadioButton*> m_buttons;
    RadioButtonGroupSelectionCallback m_selectionCallback;
};
}
