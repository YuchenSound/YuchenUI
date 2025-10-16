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

#include "YuchenUI/widgets/UIComponent.h"
#include "YuchenUI/core/Types.h"

namespace YuchenUI {

class RenderList;

/**
    Container widget with customizable background, border, and rounded corners.
    
    Frame is a versatile container component that provides:
    - Solid background color with optional customization
    - Border with customizable color and width
    - Rounded corners with per-corner radius control
    - Child component management (inherited from UIComponent)
    - Padding support for content area
    
    Frames are typically used to:
    - Group related UI elements together
    - Create visual boundaries and sections
    - Provide decorative backgrounds
    - Build complex layouts with nested containers
    
    Visual properties:
    - Background: Filled rectangle with optional transparency
    - Border: Outline with configurable width and color
    - Corners: Per-corner radius for rounded appearance
    
    The frame automatically renders its background and border, then renders
    all child components on top.
    
    Example usage:
    @code
    // Create a rounded frame with custom colors
    Frame* frame = parent->addChild<Frame>(Rect(10, 10, 300, 200));
    frame->setBackgroundColor(Vec4::FromRGBA(240, 240, 240, 255));
    frame->setBorderColor(Vec4::FromRGBA(200, 200, 200, 255));
    frame->setBorderWidth(1.0f);
    frame->setCornerRadius(8.0f);
    frame->setPadding(10.0f);
    
    // Add child components
    Button* button = frame->addChild<Button>(Rect(10, 10, 100, 30));
    CheckBox* checkbox = frame->addChild<CheckBox>(Rect(10, 50, 150, 20));
    @endcode
    
    Theme integration:
    If custom colors are not set, Frame uses theme defaults:
    - Background: theme->getDefaultFrameBackground()
    - Border: theme->getDefaultFrameBorder()
    
    @see UIComponent, CornerRadius
*/
class Frame : public UIComponent {
public:
    /**
        Constructs a frame with the specified bounds.
        
        The frame is created with:
        - Theme default background color
        - Theme default border color
        - No border (width = 0)
        - No corner rounding
        - No padding
        
        @param bounds  Initial bounding rectangle
    */
    explicit Frame(const Rect& bounds);
    
    virtual ~Frame();
    
    //======================================================================================
    // UIComponent Interface Implementation
    
    void addDrawCommands(RenderList& commandList, const Vec2& offset = Vec2()) const override;
    bool handleMouseMove(const Vec2& position, const Vec2& offset = Vec2()) override;
    bool handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset = Vec2()) override;
    bool handleMouseWheel(const Vec2& delta, const Vec2& position, const Vec2& offset = Vec2()) override;
    
    //======================================================================================
    // Appearance API
    
    /**
        Sets custom background color.
        
        Overrides the theme's default frame background color.
        
        @param color  RGBA color vector (components in [0, 1])
    */
    void setBackgroundColor(const Vec4& color);
    
    /**
        Returns current background color.
        
        If custom color is set, returns that. Otherwise returns theme default.
        
        @return RGBA color vector
    */
    Vec4 getBackgroundColor() const;
    
    /**
        Resets background color to theme default.
        
        Clears any custom background color setting.
    */
    void resetBackgroundColor();
    
    /**
        Sets custom border color.
        
        Overrides the theme's default frame border color.
        
        @param color  RGBA color vector (components in [0, 1])
    */
    void setBorderColor(const Vec4& color);
    
    /**
        Returns current border color.
        
        If custom color is set, returns that. Otherwise returns theme default.
        
        @return RGBA color vector
    */
    Vec4 getBorderColor() const;
    
    /**
        Resets border color to theme default.
        
        Clears any custom border color setting.
    */
    void resetBorderColor();
    
    /**
        Sets border width in pixels.
        
        Set to 0 to disable border rendering.
        
        @param width  Border width in pixels (must be >= 0)
    */
    void setBorderWidth(float width);
    
    /**
        Returns current border width.
        
        @return Border width in pixels
    */
    float getBorderWidth() const { return m_borderWidth; }
    
    /**
        Sets corner radius with per-corner control.
        
        Allows different radius values for each corner.
        
        @param radius  Corner radius structure
    */
    void setCornerRadius(const CornerRadius& radius);
    
    /**
        Sets uniform corner radius for all corners.
        
        Convenience method for equal rounding on all corners.
        
        @param radius  Radius in pixels (must be >= 0)
    */
    void setCornerRadius(float radius);
    
    /**
        Returns current corner radius.
        
        @return Corner radius structure
    */
    const CornerRadius& getCornerRadius() const { return m_cornerRadius; }
    
    /**
        Validates frame state.
        
        Checks that:
        - Bounds are valid
        - Border width is non-negative
        - Corner radius is valid
        
        @return true if valid, false otherwise
    */
    bool isValid() const;

private:
    Vec4 m_backgroundColor;          ///< Background color (if custom)
    Vec4 m_borderColor;              ///< Border color (if custom)
    float m_borderWidth;             ///< Border width in pixels
    CornerRadius m_cornerRadius;     ///< Corner radius for rounding
    
    bool m_hasCustomBackground;      ///< Whether custom background is set
    bool m_hasCustomBorderColor;     ///< Whether custom border color is set
};

} // namespace YuchenUI
