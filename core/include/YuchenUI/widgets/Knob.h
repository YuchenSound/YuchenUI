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

//==========================================================================================
/** @file Knob.h
    
    Rotary knob control with theme-aware multi-frame sprite sheet visualization.
    
    A knob control that displays a rotating knob graphic using frame-based animation.
    The knob value is mapped to sprite sheet frames, providing smooth visual feedback.
    
    Features:
    - Vertical mouse drag to change value
    - Configurable value range (default 0.0 to 1.0)
    - Automatic frame mapping based on value
    - Two knob types: NoCentered (Volume) and Centered (Pan)
    - Theme-aware rendering (Dark/Classical styles)
    - Active/Inactive visual states
    - Double-click to reset to default value
    - Value change callbacks
    - Adjustable drag sensitivity
    
    The knob uses a 29-frame sprite sheet arranged vertically. The appropriate
    resource is automatically selected based on theme, knob type, and active state.
    
    Example usage:
    @code
    // Volume knob (no center point, 0 at left)
    Knob* volumeKnob = new Knob(Rect(10, 10, 34, 36));
    volumeKnob->setKnobType(KnobType::NoCentered);
    volumeKnob->setValueRange(0.0f, 100.0f);
    volumeKnob->setDefaultValue(75.0f);
    
    // Pan knob (center point, 0 at middle)
    Knob* panKnob = new Knob(Rect(60, 10, 34, 36));
    panKnob->setKnobType(KnobType::Centered);
    panKnob->setValueRange(-1.0f, 1.0f);
    panKnob->setDefaultValue(0.0f);
    @endcode
*/

#pragma once

#include "YuchenUI/widgets/Widget.h"
#include "YuchenUI/core/Types.h"
#include "YuchenUI/theme/Theme.h"
#include <functional>

namespace YuchenUI {

class RenderList;

//==========================================================================================
/**
    Rotary knob control with theme-aware sprite sheet visualization.
    
    Provides a traditional rotary knob interface where vertical mouse dragging
    changes the value. The current value is visually represented by selecting
    the appropriate frame from a multi-frame sprite sheet.
    
    The knob automatically adapts its appearance based on:
    - Current theme (Dark/Classical)
    - Knob type (NoCentered/Centered)
    - Active state (focused or being dragged)
    
    The knob supports:
    - Linear value ranges with configurable min/max
    - Mouse drag interaction with adjustable sensitivity
    - Double-click to reset to default value
    - Value change notifications via callback
    - Two behavioral types for different use cases
    
    @see KnobType, UIStyle::drawKnob
*/
class Knob : public Widget {
public:
    /** Value change callback function type. */
    using ValueChangedCallback = std::function<void(float)>;
    
    //======================================================================================
    /**
        Creates a knob control.
        
        The bounds should match the logical pixel size of a single frame in the
        sprite sheet (typically 34x36 pixels for standard knobs).
        
        @param bounds  Component bounds in logical pixels
    */
    explicit Knob(const Rect& bounds);
    
    /** Destructor. */
    virtual ~Knob();
    
    //======================================================================================
    // Rendering and interaction
    
    void addDrawCommands(RenderList& commandList, const Vec2& offset = Vec2()) const override;
    bool handleMouseMove(const Vec2& position, const Vec2& offset = Vec2()) override;
    bool handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset = Vec2()) override;
    
    //======================================================================================
    // Value management
    
    /**
        Sets the knob value.
        
        Value is clamped to the range [minValue, maxValue].
        Triggers the value changed callback if the value actually changes.
        
        @param value  New value
    */
    void setValue(float value);
    
    /** Returns the current knob value. */
    float getValue() const { return m_value; }
    
    /**
        Sets the value range.
        
        @param minValue  Minimum value (inclusive)
        @param maxValue  Maximum value (inclusive)
    */
    void setValueRange(float minValue, float maxValue);
    
    /** Returns the minimum value. */
    float getMinValue() const { return m_minValue; }
    
    /** Returns the maximum value. */
    float getMaxValue() const { return m_maxValue; }
    
    /**
        Sets the default value for double-click reset.
        
        @param value  Default value
    */
    void setDefaultValue(float value);
    
    /** Returns the default value. */
    float getDefaultValue() const { return m_defaultValue; }
    
    /**
        Resets the knob to its default value.
    */
    void resetToDefault();
    
    //======================================================================================
    // Knob type and appearance
    
    /**
        Sets the knob type (center behavior).
        
        NoCentered: No center point, minimum value at left rotation
                    Example: Volume (0% to 100%)
        
        Centered: Center point at middle rotation, zero at center
                  Example: Pan (Left to Center to Right)
        
        @param type  Knob type
    */
    void setKnobType(KnobType type);
    
    /** Returns the current knob type. */
    KnobType getKnobType() const { return m_knobType; }
    
    //======================================================================================
    // Interaction settings
    
    /**
        Sets the drag sensitivity.
        
        Higher values make the knob more sensitive to mouse movement.
        Default is 1.0.
        
        @param sensitivity  Sensitivity multiplier (positive value)
    */
    void setSensitivity(float sensitivity);
    
    /** Returns the current sensitivity. */
    float getSensitivity() const { return m_sensitivity; }
    
    /**
        Enables or disables double-click reset.
        
        @param enabled  True to enable double-click reset
    */
    void setDoubleClickResetEnabled(bool enabled);
    
    /** Returns true if double-click reset is enabled. */
    bool isDoubleClickResetEnabled() const { return m_doubleClickResetEnabled; }
    
    //======================================================================================
    // Callbacks
    
    /**
        Sets the value changed callback.
        
        The callback is invoked whenever the knob value changes through
        user interaction or programmatic setValue() calls.
        
        @param callback  Function to call on value change
    */
    void setOnValueChanged(ValueChangedCallback callback);
    
    //======================================================================================
    // Component interface
    

    /**
        Validates knob configuration.
        
        @returns True if value range and configuration are valid
    */
    bool isValid() const;

protected:
    virtual void focusInEvent(FocusReason reason) override;
    virtual void focusOutEvent(FocusReason reason) override;

private:
    /**
        Maps value to frame index.
        
        Converts the current value to a frame index based on the value range
        and total number of frames.
        
        @returns Frame index [0, frameCount-1]
    */
    int valueToFrame() const;
    
    /**
        Notifies value change.
        
        Invokes the callback if registered.
    */
    void notifyValueChanged();
    
    KnobType m_knobType;                    ///< Knob center behavior type
    
    float m_value;                          ///< Current value
    float m_minValue;                       ///< Minimum value
    float m_maxValue;                       ///< Maximum value
    float m_defaultValue;                   ///< Default value for reset
    
    float m_sensitivity;                    ///< Drag sensitivity multiplier
    bool m_doubleClickResetEnabled;         ///< Enable double-click reset
    
    bool m_isDragging;                      ///< Currently dragging
    float m_dragStartY;                     ///< Mouse Y position at drag start
    float m_dragStartValue;                 ///< Value at drag start
    
    ValueChangedCallback m_onValueChanged;  ///< Value change callback
    
    static constexpr int DEFAULT_FRAME_COUNT = 29;
    static constexpr float DEFAULT_FRAME_WIDTH = 34.0f;
    static constexpr float DEFAULT_FRAME_HEIGHT = 36.0f;
};

} // namespace YuchenUI
