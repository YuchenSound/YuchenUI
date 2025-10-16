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

namespace YuchenUI {

class RenderList;

/**
    Container with titled border for grouping related controls.
    
    GroupBox provides a visual grouping mechanism with a title bar and optional border.
    It is commonly used to organize related UI elements together with a descriptive label.
    
    Features:
    - Title text with customizable font and color
    - Background fill with customizable color
    - Border with adjustable width and color
    - Rounded corners support
    - Child component management (inherited from UIComponent)
    - Automatic title bar height calculation
    
    Visual structure:
    ┌─ Title Text ──────────┐
    │                       │
    │  [Child Components]   │
    │                       │
    └───────────────────────┘
    
    The title bar occupies a fixed height at the top, and children are positioned
    below the title bar. The offset is automatically applied when rendering and
    handling events for child components.
    
    Font system (Version 3.0):
    Similar to Button, GroupBox uses simplified Qt-style font API:
    - setTitleFont(): Set primary font with automatic CJK fallback
    - setTitleFontChain(): Set complete fallback chain
    - resetTitleFont(): Return to theme default
    
    Example usage:
    @code
    // Create group box
    GroupBox* group = parent->addChild<GroupBox>(Rect(10, 10, 300, 200));
    group->setTitle("Audio Settings");
    group->setBorderWidth(1.0f);
    group->setCornerRadius(4.0f);
    
    // Add grouped controls
    CheckBox* checkbox1 = group->addChild<CheckBox>(Rect(10, 10, 150, 20));
    checkbox1->setText("Enable reverb");
    
    CheckBox* checkbox2 = group->addChild<CheckBox>(Rect(10, 40, 150, 20));
    checkbox2->setText("Enable delay");
    @endcode
    
    Theme integration:
    If custom colors/fonts are not set, GroupBox uses theme defaults:
    - Title font: theme->getDefaultTitleFontChain()
    - Title color: theme->getDefaultTextColor()
    - Background: theme->getDefaultGroupBoxBackground()
    - Border: theme->getDefaultGroupBoxBorder()
    
    @see UIComponent, Frame
*/
class GroupBox : public Widget {
public:
    /**
        Constructs a group box with the specified bounds.
        
        The group box is created with:
        - Empty title
        - Default title font size (from Config::Font::DEFAULT_SIZE)
        - Theme default colors
        - 1 pixel border
        - No corner rounding
        
        @param bounds  Initial bounding rectangle
    */
    explicit GroupBox(const Rect& bounds);
    
    virtual ~GroupBox();
    
    //======================================================================================
    // UIComponent Interface Implementation
    
    void addDrawCommands(RenderList& commandList, const Vec2& offset = Vec2()) const override;
    bool handleMouseMove(const Vec2& position, const Vec2& offset = Vec2()) override;
    bool handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset = Vec2()) override;
    bool handleMouseWheel(const Vec2& delta, const Vec2& position, const Vec2& offset = Vec2()) override;
    
    //======================================================================================
    // Title API
    
    /**
        Sets the title text.
        
        @param title  Title to display in the title bar
    */
    void setTitle(const std::string& title);
    
    /**
        Sets the title text from C-style string.
        
        @param title  Null-terminated C string
    */
    void setTitle(const char* title);
    
    /**
        Returns the current title text.
        
        @return Reference to the title string
    */
    const std::string& getTitle() const { return m_title; }
    
    //======================================================================================
    // Title Font API (Qt-style, Version 3.0)
    
    /**
        Sets title font with automatic fallback.
        
        The system automatically adds appropriate CJK fallback fonts.
        
        @param fontHandle  Primary font handle for title
    */
    void setTitleFont(FontHandle fontHandle);
    
    /**
        Sets complete font fallback chain for title.
        
        For full control over title font fallback.
        
        @param chain  Font fallback chain
    */
    void setTitleFontChain(const FontFallbackChain& chain);
    
    /**
        Returns current title font fallback chain.
        
        If custom font not set, returns theme's default title font chain.
        
        @return Current font fallback chain
    */
    FontFallbackChain getTitleFontChain() const;
    
    /**
        Resets title font to theme default.
        
        Clears any custom font setting.
    */
    void resetTitleFont();
    
    //======================================================================================
    // Title Style API
    
    /**
        Sets the title font size.
        
        Size is clamped to [Config::Font::MIN_SIZE, Config::Font::MAX_SIZE].
        
        @param fontSize  Font size in points
    */
    void setTitleFontSize(float fontSize);
    
    /**
        Returns the current title font size.
        
        @return Font size in points
    */
    float getTitleFontSize() const { return m_titleFontSize; }
    
    /**
        Sets custom title text color.
        
        Overrides the theme's default text color.
        
        @param color  RGBA color vector (components in [0, 1])
    */
    void setTitleColor(const Vec4& color);
    
    /**
        Returns current title color.
        
        If custom color is set, returns that. Otherwise returns theme default.
        
        @return RGBA color vector
    */
    Vec4 getTitleColor() const;
    
    /**
        Resets title color to theme default.
        
        Clears any custom color setting.
    */
    void resetTitleColor();
    
    //======================================================================================
    // Appearance API
    
    /**
        Sets custom background color.
        
        Overrides the theme's default group box background color.
        
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
        
        Overrides the theme's default group box border color.
        
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
        
        @param radius  Corner radius structure
    */
    void setCornerRadius(const CornerRadius& radius);
    
    /**
        Sets uniform corner radius for all corners.
        
        @param radius  Radius in pixels (must be >= 0)
    */
    void setCornerRadius(float radius);
    
    /**
        Returns current corner radius.
        
        @return Corner radius structure
    */
    const CornerRadius& getCornerRadius() const { return m_cornerRadius; }
    
    /**
        Validates group box state.
        
        Checks that:
        - Bounds are valid
        - Title font size is within allowed range
        - Border width is non-negative
        - Corner radius is valid
        
        @return true if valid, false otherwise
    */
    bool isValid() const;

private:
    std::string m_title;                  ///< Title text
    FontFallbackChain m_titleFontChain;   ///< Title font chain (if custom)
    float m_titleFontSize;                ///< Title font size in points
    Vec4 m_titleColor;                    ///< Title text color (if custom)
    Vec4 m_backgroundColor;               ///< Background color (if custom)
    Vec4 m_borderColor;                   ///< Border color (if custom)
    float m_borderWidth;                  ///< Border width in pixels
    CornerRadius m_cornerRadius;          ///< Corner radius for rounding
    
    bool m_hasCustomTitleFont;            ///< Whether custom title font is set
    bool m_hasCustomTitleColor;           ///< Whether custom title color is set
    bool m_hasCustomBackground;           ///< Whether custom background is set
    bool m_hasCustomBorderColor;          ///< Whether custom border color is set
};

} // namespace YuchenUI
