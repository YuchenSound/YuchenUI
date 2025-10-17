/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Widgets module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

#pragma once

#include "YuchenUI/widgets/Widget.h"
#include "YuchenUI/platform/IInputMethodSupport.h"
#include "YuchenUI/core/Types.h"
#include <string>
#include <functional>

namespace YuchenUI {

class RenderList;

using SpinBoxValueChangedCallback = std::function<void(double)>;

/**
    Numeric input widget with increment/decrement controls.
    
    SpinBox provides a compact widget for entering numeric values with optional suffix,
    precision control, and min/max bounds. It supports multiple interaction methods:
    keyboard input, mouse wheel scrolling, and drag-to-adjust.
    
    Features:
    - Numeric value input with range validation
    - Configurable precision and step size
    - Optional suffix for units (e.g., "Hz", "dB", "ms")
    - Mouse drag to adjust value
    - Mouse wheel support for fine-tuning
    - Keyboard input with validation
    - Text alignment control (horizontal and vertical)
    - Read-only mode for display purposes
    - Customizable font and colors via theme
    
    Interaction modes:
    - Click to enter edit mode and type value directly
    - Drag horizontally/vertically to adjust value by steps
    - Scroll mouse wheel to increment/decrement
    - Press Up/Down arrow keys to adjust by step
    - Press Enter to apply, Escape to cancel
    
    Text alignment (Version 3.1):
    SpinBox supports full text alignment control:
    - Horizontal: Left, Center, Right
    - Vertical: Top, Middle, Bottom
    - Default: Center + Middle (centered both ways)
    
    Font system (Version 3.0):
    Similar to TextLabel, SpinBox uses Qt-style font API:
    - setFont(): Set primary font with automatic CJK fallback
    - setFontChain(): Set complete fallback chain
    - resetFont(): Return to theme default
    
    Example usage:
    @code
    // Create frequency spinbox with suffix
    SpinBox* freqBox = parent->addChild<SpinBox>(Rect(10, 10, 80, 20));
    freqBox->setValue(440.0);
    freqBox->setMinValue(20.0);
    freqBox->setMaxValue(20000.0);
    freqBox->setStep(1.0);
    freqBox->setPrecision(1);
    freqBox->setSuffix("Hz");
    freqBox->setAlignment(TextAlignment::Center, VerticalAlignment::Middle);
    
    // Set callback for value changes
    freqBox->setValueChangedCallback([](double value) {
        std::cout << "New frequency: " << value << " Hz" << std::endl;
    });
    
    // Create read-only display spinbox
    SpinBox* display = parent->addChild<SpinBox>(Rect(100, 10, 60, 17));
    display->setValue(-12.5);
    display->setSuffix("dB");
    display->setReadOnly(true);
    display->setHasBackground(false);
    display->setAlignment(TextAlignment::Left, VerticalAlignment::Middle);
    @endcode
    
    Theme integration:
    If custom settings are not provided, SpinBox uses theme defaults:
    - Font: theme->getDefaultLabelFontChain()
    - Text color: Defined by theme style (e.g., green for dark theme)
    - Background: Defined by theme style
    
    @see TextLabel, TextInput
*/
class SpinBox : public Widget, public IInputMethodSupport {
public:
    /**
        Constructs a spinbox with the specified bounds.
        
        The spinbox is created with:
        - Initial value: 0.0
        - Range: [-max_double, +max_double]
        - Step: 1.0
        - Precision: 2 decimal places
        - Default font size
        - Center + Middle alignment
        - Background enabled
        - Edit mode enabled
        
        @param bounds  Initial bounding rectangle
    */
    explicit SpinBox(const Rect& bounds);
    
    virtual ~SpinBox();
    
    //======================================================================================
    // Value Management
    
    /**
        Sets the current numeric value.
        
        The value will be clamped to [minValue, maxValue] range. If in edit mode,
        the input buffer will be updated to reflect the new value.
        
        @param value  New numeric value
    */
    void setValue(double value);
    
    /**
        Returns the current numeric value.
        
        @return Current value
    */
    double getValue() const { return m_value; }
    
    /**
        Sets the minimum allowed value.
        
        If current value is below the new minimum, it will be clamped.
        
        @param min  Minimum value
    */
    void setMinValue(double min);
    
    /**
        Returns the minimum allowed value.
        
        @return Minimum value
    */
    double getMinValue() const { return m_minValue; }
    
    /**
        Sets the maximum allowed value.
        
        If current value exceeds the new maximum, it will be clamped.
        
        @param max  Maximum value
    */
    void setMaxValue(double max);
    
    /**
        Returns the maximum allowed value.
        
        @return Maximum value
    */
    double getMaxValue() const { return m_maxValue; }
    
    /**
        Sets the step size for increment/decrement operations.
        
        Used when adjusting value via arrow keys, mouse wheel, or drag.
        The step value is always stored as positive.
        
        @param step  Step size (absolute value will be used)
    */
    void setStep(double step);
    
    /**
        Returns the step size.
        
        @return Step size (always positive)
    */
    double getStep() const { return m_step; }
    
    /**
        Sets the number of decimal places to display.
        
        Precision is clamped to [0, 10] range. Trailing zeros are automatically
        removed from display.
        
        @param precision  Number of decimal places (0-10)
    */
    void setPrecision(int precision);
    
    /**
        Returns the number of decimal places.
        
        @return Decimal precision
    */
    int getPrecision() const { return m_precision; }
    
    /**
        Sets the suffix text displayed after the value.
        
        Common suffixes include units like "Hz", "dB", "ms", "%", etc.
        The suffix is separated from the value by a space.
        
        @param suffix  Suffix string (e.g., "Hz", "dB")
    */
    void setSuffix(const std::string& suffix);
    
    /**
        Returns the current suffix text.
        
        @return Reference to suffix string
    */
    const std::string& getSuffix() const { return m_suffix; }
    
    /**
        Sets callback function for value changes.
        
        The callback is invoked whenever the value changes through user interaction
        (drag, wheel, keyboard input, or arrow keys). It is not invoked when setValue()
        is called programmatically.
        
        @param callback  Function to call with new value
    */
    void setValueChangedCallback(SpinBoxValueChangedCallback callback);
    
    //======================================================================================
    // Text Style
    
    /**
        Sets the font size.
        
        Size is clamped to [Config::Font::MIN_SIZE, Config::Font::MAX_SIZE].
        
        @param fontSize  Font size in points
    */
    void setFontSize(float fontSize);
    
    /**
        Returns the current font size.
        
        @return Font size in points
    */
    float getFontSize() const { return m_fontSize; }
    
    //======================================================================================
    // Font API (Qt-style, Version 3.0)
    
    /**
        Sets spinbox font with automatic fallback.
        
        The system automatically adds appropriate CJK fallback fonts.
        
        @param fontHandle  Primary font handle
    */
    void setFont(FontHandle fontHandle);
    
    /**
        Sets complete font fallback chain.
        
        For full control over font fallback.
        
        @param chain  Font fallback chain
    */
    void setFontChain(const FontFallbackChain& chain);
    
    /**
        Returns current font fallback chain.
        
        If custom font not set, returns theme's default label font chain.
        
        @return Current font fallback chain
    */
    FontFallbackChain getFontChain() const;
    
    /**
        Resets font to theme default.
        
        Clears any custom font setting.
    */
    void resetFont();
    
    //======================================================================================
    // Text Alignment API (Version 3.1)
    
    /**
        Sets both horizontal and vertical text alignment.
        
        Controls how the displayed value is positioned within the spinbox bounds.
        This is particularly useful for read-only display spinboxes where alignment
        affects visual consistency in layouts.
        
        @param horizontal  Horizontal alignment (Left, Center, Right)
        @param vertical    Vertical alignment (Top, Middle, Bottom)
    */
    void setAlignment(TextAlignment horizontal, VerticalAlignment vertical);
    
    /**
        Sets horizontal text alignment only.
        
        @param alignment  Horizontal alignment (Left, Center, Right)
    */
    void setHorizontalAlignment(TextAlignment alignment);
    
    /**
        Sets vertical text alignment only.
        
        @param alignment  Vertical alignment (Top, Middle, Bottom)
    */
    void setVerticalAlignment(VerticalAlignment alignment);
    
    /**
        Returns current horizontal text alignment.
        
        @return Current horizontal alignment
    */
    TextAlignment getHorizontalAlignment() const { return m_horizontalAlignment; }
    
    /**
        Returns current vertical text alignment.
        
        @return Current vertical alignment
    */
    VerticalAlignment getVerticalAlignment() const { return m_verticalAlignment; }
    
    //======================================================================================
    // Visual Configuration
    
    /**
        Sets whether to draw background.
        
        When disabled, only the text is rendered (useful for embedding in other widgets
        like meter displays).
        
        @param hasBackground  true to draw background, false for transparent
    */
    void setHasBackground(bool hasBackground);
    
    /**
        Returns whether background is drawn.
        
        @return true if background is drawn, false otherwise
    */
    bool hasBackground() const { return m_hasBackground; }
    
    /**
        Sets read-only mode.
        
        In read-only mode, the spinbox displays a value but cannot be edited.
        User interactions (click, drag, wheel) are disabled.
        
        @param readOnly  true for read-only, false for editable
    */
    void setReadOnly(bool readOnly);
    
    /**
        Returns whether spinbox is in read-only mode.
        
        @return true if read-only, false if editable
    */
    bool isReadOnly() const { return m_isReadOnly; }
    
    //======================================================================================
    // Widget Interface
    
    void addDrawCommands(RenderList& commandList, const Vec2& offset = Vec2()) const override;
    bool handleMouseMove(const Vec2& position, const Vec2& offset = Vec2()) override;
    bool handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset = Vec2()) override;
    bool handleMouseWheel(const Vec2& delta, const Vec2& position, const Vec2& offset = Vec2()) override;
    bool handleKeyPress(const Event& event) override;
    bool handleTextInput(uint32_t codepoint) override;
    void update(float deltaTime) override;
    
    /**
        Sets whether spinbox can receive keyboard focus.
        
        This is a convenience method that sets focus policy.
        
        @param focusable  true to enable focus, false to disable
    */
    void setFocusable(bool focusable);
    
    /**
        Returns cursor rectangle for input method support.
        
        @return Cursor rectangle in window coordinates
    */
    Rect getInputMethodCursorRect() const override;
    
    /**
        Validates spinbox state.
        
        Checks that:
        - Bounds are valid
        - Font size is within allowed range
        
        @return true if valid, false otherwise
    */
    bool isValid() const;

protected:
    void focusInEvent(FocusReason reason) override;
    void focusOutEvent(FocusReason reason) override;
    
private:
    void enterEditMode();
    void exitEditMode();
    void applyValue();
    void clampValue();
    std::string formatValue() const;
    std::string formatValueWithSuffix() const;
    bool isValidChar(char c) const;
    bool canInsertChar(char c) const;
    void insertCharAtCursor(char c);
    void deleteCharBeforeCursor();
    void deleteCharAfterCursor();
    void moveCursor(int delta);
    void moveCursorToStart();
    void moveCursorToEnd();
    void selectAll();
    void adjustValueByDrag(const Vec2& currentPos);
    void adjustValueByStep(double multiplier);
    double parseInputBuffer() const;
    
    double m_value;                       ///< Current numeric value
    double m_minValue;                    ///< Minimum allowed value
    double m_maxValue;                    ///< Maximum allowed value
    double m_step;                        ///< Step size for adjustments
    int m_precision;                      ///< Number of decimal places
    std::string m_suffix;                 ///< Suffix text (e.g., "Hz", "dB")
    float m_fontSize;                     ///< Font size in points
    FontFallbackChain m_fontChain;        ///< Font fallback chain (if custom)
    bool m_hasCustomFont;                 ///< Whether custom font is set
    
    TextAlignment m_horizontalAlignment;  ///< Horizontal text alignment
    VerticalAlignment m_verticalAlignment; ///< Vertical text alignment
    
    bool m_isReadOnly;                    ///< Whether in read-only mode
    bool m_isEditing;                     ///< Whether in edit mode
    bool m_isHovered;                     ///< Whether mouse is hovering
    bool m_isDragging;                    ///< Whether dragging to adjust
    Vec2 m_dragStartPos;                  ///< Mouse position when drag started
    double m_dragStartValue;              ///< Value when drag started
    
    std::string m_inputBuffer;            ///< Text buffer during editing
    size_t m_cursorPosition;              ///< Cursor position in buffer
    bool m_showCursor;                    ///< Whether to show cursor
    float m_cursorBlinkTimer;             ///< Cursor blink animation timer
    
    SpinBoxValueChangedCallback m_valueChangedCallback; ///< Value change callback
    
    float m_paddingLeft;                  ///< Left padding in pixels
    float m_paddingTop;                   ///< Top padding in pixels
    float m_paddingRight;                 ///< Right padding in pixels
    float m_paddingBottom;                ///< Bottom padding in pixels
    
    bool m_hasBackground;                 ///< Whether to draw background
    
    static constexpr float CURSOR_BLINK_INTERVAL = 0.53f; ///< Cursor blink interval in seconds
    static constexpr float DRAG_SENSITIVITY = 0.1f;       ///< Drag adjustment sensitivity
};

} // namespace YuchenUI
