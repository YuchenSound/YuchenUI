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
/** @file Fader.h
    
    Professional audio fader with broadcast-standard dB scaling.
    
    Large-format motorized fader control commonly used in DAW mixer channels.
    Provides non-linear dB-to-position mapping optimized for audio mixing,
    with external scale rendering and theme-aware visual states.
    
    Key features:
    - Non-linear dB mapping (-144dB to +12dB range)
    - Three color themes: Normal (gray), Red (recording), Yellow (custom)
    - Active/Inactive states based on enabled property
    - External scale rendering with major and minor tick marks
    - Unity gain at 0dB (75% position) following broadcast standards
    - Smooth vertical drag interaction with optional value change callbacks
    
    The fader uses sprite sheet backgrounds with nine-slice scaling for arbitrary heights.
    Fader cap images are fixed-size overlays positioned according to current value.
    
    Design dimensions:
    - Total width: 30px (9px scale + 21px fader track)
    - Recommended height: 246px
    - Fader cap: 21x48px
    
    Example usage:
    @code
    Fader* channelFader = new Fader(context, Rect(10, 10, 30, 246));
    channelFader->setValueDb(0.0f);  // Unity gain
    channelFader->setColorTheme(FaderColorTheme::Red);  // Recording mode
    channelFader->setOnValueChanged([](float db) {
        // Update DSP gain
    });
    @endcode
*/

#pragma once

#include "YuchenUI/widgets/UIComponent.h"
#include "YuchenUI/core/Types.h"
#include <functional>
#include <vector>
#include <string>

namespace YuchenUI {

class RenderList;

//==========================================================================================
/** Fader color theme selection. */
enum class FaderColorTheme {
    Normal,   ///< Gray theme for normal channels
    Red,      ///< Red theme for recording channels
    Yellow    ///< Yellow theme for custom/aux channels
};

//==========================================================================================
/** Single tick mark on fader scale. */
struct FaderScaleTick {
    float db;          ///< dB value at this position
    float position;    ///< Normalized vertical position [0,1], 0=bottom, 1=top
    std::string label; ///< Display text (e.g., "0", "-6")
    bool isMajor;      ///< Major tick (longer line, includes label)
    
    FaderScaleTick(float d, float pos, const std::string& lbl = "", bool major = true)
        : db(d), position(pos), label(lbl), isMajor(major) {}
};

//==========================================================================================
/**
    Non-linear dB-to-position mapping for audio faders.
    
    Implements industry-standard broadcast fader curve with dense spacing
    around unity gain (0dB) and compressed spacing for extreme values.
    
    Mapping characteristics:
    - -144dB (digital silence) at position 0.0
    - 0dB (unity gain) at position 0.747 (75% travel)
    - +12dB (maximum gain) at position 1.0
    - Non-linear interpolation between 11 control points
    
    This mapping matches the behavior of professional mixing consoles
    and DAW fader controls, providing precise control in the critical
    -20dB to +6dB range.
*/
class FaderMapping {
public:
    /** Returns the 11 control points defining the fader curve. */
    static const std::vector<std::pair<float, float>>& getControlPoints() {
        static const std::vector<std::pair<float, float>> points = {
            {1.000f, 12.0f},  {0.874f, 6.0f},   {0.747f, 0.0f},   {0.652f, -5.0f},
            {0.561f, -10.0f}, {0.465f, -15.0f}, {0.374f, -20.0f}, {0.247f, -30.0f},
            {0.131f, -40.0f}, {0.071f, -60.0f}, {0.000f, -144.0f}
        };
        return points;
    }
    
    /** Converts linear position [0,1] to dB value. */
    static float positionToDb(float position);
    
    /** Converts dB value to linear position [0,1]. */
    static float dbToPosition(float db);
};

//==========================================================================================
/**
    Professional audio fader control with broadcast-standard scaling.
    
    Large-format vertical fader implementing non-linear dB mapping commonly
    used in mixing consoles and DAW applications. Provides precise control
    over audio gain with optimized spacing around unity gain.
    
    The fader consists of:
    - Background track (nine-slice scaled sprite sheet)
    - Fader cap (fixed-size overlay at current position)
    - External scale (rendered separately, typically to the left)
    
    Visual states:
    - Color theme: Normal/Red/Yellow (user-controlled)
    - Active/Inactive: Based on enabled property
    
    Interaction:
    - Vertical drag to adjust value
    - Click to jump to position (optional)
    - Value change callbacks for DSP integration
    
    @see FaderMapping, FaderColorTheme
*/
class Fader : public UIComponent {
public:
    /** Value change callback function type. */
    using ValueChangedCallback = std::function<void(float)>;
    
    //======================================================================================
    /** Recommended component dimensions. */
    static constexpr float RECOMMENDED_WIDTH = 30.0f;
    static constexpr float RECOMMENDED_HEIGHT = 246.0f;
    static constexpr float SCALE_AREA_WIDTH = 9.0f;
    static constexpr float TRACK_AREA_WIDTH = 21.0f;
    static constexpr float CAP_WIDTH = 21.0f;
    static constexpr float CAP_HEIGHT = 48.0f;
    static constexpr float CAP_CENTER_FROM_TOP = 19.0f;
    static constexpr float CAP_CENTER_FROM_BOTTOM = CAP_HEIGHT - CAP_CENTER_FROM_TOP;

    //======================================================================================
    /**
        Creates a fader control.
        
        @param context  UI context for resource and style access
        @param bounds   Component bounds in logical pixels
    */
    explicit Fader(UIContext* context, const Rect& bounds);
    
    /** Destructor. */
    virtual ~Fader();
    
    //======================================================================================
    // Rendering and interaction
    
    void addDrawCommands(RenderList& commandList, const Vec2& offset = Vec2()) const override;
    bool handleMouseMove(const Vec2& position, const Vec2& offset = Vec2()) override;
    bool handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset = Vec2()) override;
    
    //======================================================================================
    // Value management
    
    /**
        Sets the fader value as linear position.
        
        Position is clamped to [0,1] range where 0 is bottom (-144dB)
        and 1 is top (+12dB). Triggers value changed callback.
        
        @param linearValue  Linear position [0,1]
    */
    void setValue(float linearValue);
    
    /** Returns the current linear position [0,1]. */
    float getValue() const { return m_value; }
    
    /**
        Sets the fader value in decibels.
        
        Value is clamped to [-144dB, +12dB] range and converted to
        linear position using non-linear mapping.
        
        @param dbValue  Value in decibels
    */
    void setValueDb(float dbValue);
    
    /** Returns the current value in decibels. */
    float getValueDb() const;
    
    //======================================================================================
    // Visual appearance
    
    /**
        Sets the fader color theme.
        
        Theme selection affects both background track and fader cap sprites.
        Typically used to indicate channel state (normal/recording/aux).
        
        @param theme  Color theme
    */
    void setColorTheme(FaderColorTheme theme);
    
    /** Returns the current color theme. */
    FaderColorTheme getColorTheme() const { return m_colorTheme; }
    
    /**
        Controls scale visibility.
        
        When enabled, scale tick marks and labels are rendered to the left
        of the fader track. Scale rendering uses theme-defined colors.
        
        @param visible  True to show scale
    */
    void setShowScale(bool visible);
    
    /** Returns true if scale is visible. */
    bool isScaleVisible() const { return m_showScale; }
    
    //======================================================================================
    // Scale data access
    
    /**
        Returns major tick marks for external scale rendering.
        
        Major ticks include dB labels and longer tick lines.
        Corresponds to the 11 control points in the fader mapping.
        
        @returns Vector of major tick marks
    */
    const std::vector<FaderScaleTick>& getMajorTicks() const { return m_majorTicks; }
    
    /**
        Calculates minor tick marks for a given segment.
        
        Minor ticks are subdivisions between major ticks, with density
        varying by dB range (e.g., 1dB steps from 0 to +12dB).
        
        @param startDb  Start of segment in dB
        @param endDb    End of segment in dB
        @returns Vector of minor tick marks
    */
    std::vector<FaderScaleTick> calculateMinorTicks(float startDb, float endDb) const;
    
    //======================================================================================
    // Callbacks
    
    /**
        Sets the value changed callback.
        
        Callback is invoked whenever the fader value changes through
        user interaction or programmatic setValue() calls.
        
        Callback receives the new value in decibels.
        
        @param callback  Function to call on value change
    */
    void setOnValueChanged(ValueChangedCallback callback);
    
    //======================================================================================
    // Component interface
    
    /**
        Validates fader configuration.
        
        @returns True if bounds and value are valid
    */
    bool isValid() const;

protected:
    virtual void focusInEvent(FocusReason reason) override;
    virtual void focusOutEvent(FocusReason reason) override;
    virtual CornerRadius getFocusIndicatorCornerRadius() const override { return CornerRadius(2.0f); }

private:
    /**
        Renders the fader background track.
        
        Uses nine-slice scaled sprite based on color theme and active state.
        
        @param cmdList      Render command list
        @param trackBounds  Track area bounds in window coordinates
    */
    void renderBackground(RenderList& cmdList, const Rect& trackBounds) const;
    
    /**
        Renders the fader cap at current position.
        
        Selects sprite based on color theme and active state.
        
        @param cmdList     Render command list
        @param capCenter   Cap center position in window coordinates
    */
    void renderCap(RenderList& cmdList, const Vec2& capCenter) const;
    
    /**
        Renders the fader scale with tick marks and labels.
        
        @param cmdList       Render command list
        @param scaleBounds   Scale area bounds in window coordinates
        @param faderBounds   Fader track bounds for tick alignment
    */
    void renderScale(RenderList& cmdList, const Rect& scaleBounds, const Rect& faderBounds) const;
    
    /**
        Calculates fader cap center position in local coordinates.
        
        Maps current linear value to vertical pixel position within bounds.
        Accounts for cap height to ensure cap stays within track bounds.
        
        @returns Cap center point in local coordinates
    */
    Vec2 getCapCenterPosition() const;
    
    /**
        Initializes major tick marks from control points.
        
        Called during construction.
    */
    void initializeMajorTicks();
    
    /**
        Notifies value change callback.
        
        Invokes callback with current value in dB.
    */
    void notifyValueChanged();
    
    UIContext* m_context;                   ///< UI context (not owned)
    FaderColorTheme m_colorTheme;           ///< Current color theme
    bool m_showScale;                       ///< Scale visibility flag
    
    float m_value;                          ///< Current linear position [0,1]
    
    bool m_isDragging;                      ///< Currently dragging flag
    float m_dragOffset;                     ///< Mouse Y offset from cap center
    
    std::vector<FaderScaleTick> m_majorTicks;  ///< Major tick marks
    
    ValueChangedCallback m_onValueChanged;  ///< Value change callback
};

} // namespace YuchenUI
