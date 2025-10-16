/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

#pragma once

#include "YuchenUI/widgets/Widget.h"
#include "YuchenUI/core/Types.h"
#include <string>
#include <functional>

namespace YuchenUI {

class RenderList;

/**
    Checkbox state enumeration.
    
    Represents the three possible states of a checkbox:
    - Unchecked: Empty checkbox
    - Checked: Checkbox with checkmark
    - Indeterminate: Checkbox with minus/dash (for partial selection)
*/
enum class CheckBoxState {
    Unchecked,      ///< Checkbox is not checked
    Checked,        ///< Checkbox is checked
    Indeterminate   ///< Checkbox is in indeterminate state (partial)
};

using CheckBoxStateChangedCallback = std::function<void(CheckBoxState)>;

/**
    Checkbox widget with optional text label.
    
    CheckBox is a standard toggle control that allows users to select or deselect
    an option. It supports:
    - Three states: Unchecked, Checked, Indeterminate
    - Optional text label to the right of the checkbox
    - Mouse and keyboard interaction (Space key to toggle)
    - State change callbacks
    - Customizable text appearance
    
    Visual layout:
    [☐] Label Text
    ^   ^
    |   └─ Optional text label (customizable font/color)
    └───── 14x14 pixel checkbox
    
    The checkbox itself is always 14x14 pixels, while the text extends to the right
    with 6 pixels of spacing.
    
    Keyboard support:
    - Space: Toggle between checked/unchecked
    - Tab: Focus navigation
    
    State transitions:
    - From Unchecked → Space → Checked
    - From Checked → Space → Unchecked
    - From Indeterminate → Space → Checked
    
    Example usage:
    @code
    // Create checkbox with label
    CheckBox* checkbox = parent->addChild<CheckBox>(Rect(10, 10, 150, 20));
    checkbox->setText("Enable feature");
    
    // Handle state changes
    checkbox->setStateChangedCallback([](CheckBoxState state) {
        if (state == CheckBoxState::Checked) {
            std::cout << "Checkbox checked" << std::endl;
        }
    });
    
    // Set initial state
    checkbox->setChecked(true);
    @endcode
    
    @see CheckBoxState, UIComponent
*/
class CheckBox : public Widget {
public:
    /**
        Constructs a checkbox with the specified bounds.
        
        The checkbox is created with:
        - Unchecked state
        - Empty text label
        - Default font size (from Config::Font::DEFAULT_SIZE)
        - Strong focus policy (keyboard + mouse focus)
        
        @param bounds  Initial bounding rectangle (includes label area)
    */
    explicit CheckBox(const Rect& bounds);
    
    virtual ~CheckBox();
    
    //======================================================================================
    // UIComponent Interface Implementation
    
    void addDrawCommands(RenderList& commandList, const Vec2& offset = Vec2()) const override;
    bool handleMouseMove(const Vec2& position, const Vec2& offset = Vec2()) override;
    bool handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset = Vec2()) override;
    bool handleKeyPress(const Event& event) override;
    
    //======================================================================================
    // State API
    
    /**
        Sets the checkbox state.
        
        If the state changes, the state changed callback is invoked.
        
        @param state  New state to set
    */
    void setState(CheckBoxState state);
    
    /**
        Returns the current checkbox state.
        
        @return Current state
    */
    CheckBoxState getState() const { return m_state; }
    
    /**
        Returns whether the checkbox is checked.
        
        Convenience method that checks if state == CheckBoxState::Checked.
        
        @return true if checked, false otherwise
    */
    bool isChecked() const { return m_state == CheckBoxState::Checked; }
    
    /**
        Sets checked/unchecked state.
        
        Convenience method for binary checked state.
        
        @param checked  true to check, false to uncheck
    */
    void setChecked(bool checked);
    
    //======================================================================================
    // Text Label API
    
    /**
        Sets the text label.
        
        @param text  Label text to display
    */
    void setText(const std::string& text);
    
    /**
        Sets the text label from C-style string.
        
        @param text  Null-terminated C string
    */
    void setText(const char* text);
    
    /**
        Returns the current text label.
        
        @return Reference to the label text
    */
    const std::string& getText() const { return m_text; }
    
    //======================================================================================
    // Text Style API
    
    /**
        Sets the font size for the label.
        
        Size is clamped to [Config::Font::MIN_SIZE, Config::Font::MAX_SIZE].
        
        @param fontSize  Font size in points
    */
    void setFontSize(float fontSize);
    
    /**
        Returns the current font size.
        
        @return Font size in points
    */
    float getFontSize() const { return m_fontSize; }
    
    /**
        Sets custom text color for the label.
        
        Overrides the theme's default text color.
        
        @param color  RGBA color vector (components in [0, 1])
    */
    void setTextColor(const Vec4& color);
    
    /**
        Returns current text color.
        
        If custom color is set, returns that. Otherwise returns theme default.
        
        @return RGBA color vector
    */
    Vec4 getTextColor() const;
    
    /**
        Resets text color to theme default.
        
        Clears any custom text color setting.
    */
    void resetTextColor();
    
    //======================================================================================
    // Callback API
    
    /**
        Sets the callback invoked when checkbox state changes.
        
        The callback receives the new state as a parameter.
        
        @param callback  Function to call on state change (can be nullptr to clear)
    */
    void setStateChangedCallback(CheckBoxStateChangedCallback callback);
    
    /**
        Validates checkbox state.
        
        Checks that:
        - Bounds are valid
        - Font size is within allowed range
        
        @return true if valid, false otherwise
    */
    bool isValid() const;

protected:
    /**
        Returns corner radius for focus indicator.
        
        Checkboxes use slightly rounded focus indicators.
        
        @return Corner radius of 2 pixels
    */
    CornerRadius getFocusIndicatorCornerRadius() const override { return CornerRadius(2.0f); }

private:
    /**
        Toggles between checked and unchecked states.
        
        Called when Space key is pressed or checkbox is clicked.
        Handles state transitions from all three states.
    */
    void toggleState();
    
    /**
        Returns the rectangle for the checkbox itself.
        
        @param absPos  Absolute position of the component
        @return Rectangle for the 14x14 checkbox
    */
    Rect getCheckBoxRect(const Vec2& absPos) const;
    
    /**
        Returns the rectangle for the text label area.
        
        @param absPos  Absolute position of the component
        @return Rectangle for the label text
    */
    Rect getTextRect(const Vec2& absPos) const;
    
    static constexpr float CHECKBOX_SIZE = 14.0f;  ///< Size of checkbox square in pixels
    static constexpr float TEXT_SPACING = 6.0f;    ///< Space between checkbox and label
    
    CheckBoxState m_state;                         ///< Current checkbox state
    std::string m_text;                            ///< Label text
    float m_fontSize;                              ///< Label font size
    Vec4 m_textColor;                              ///< Label text color (if custom)
    bool m_hasCustomTextColor;                     ///< Whether custom text color is set
    
    bool m_isHovered;                              ///< Whether mouse is over checkbox
    bool m_isPressed;                              ///< Whether mouse button is held down
    
    CheckBoxStateChangedCallback m_stateChangedCallback;  ///< State change callback
};

} // namespace YuchenUI
