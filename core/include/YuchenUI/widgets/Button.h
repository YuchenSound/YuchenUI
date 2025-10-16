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
#include <string>
#include <functional>

namespace YuchenUI {

class RenderList;

using ButtonClickCallback = std::function<void()>;

/**
    Button role determines visual style and semantic meaning.
    
    Different roles map to different visual treatments in the theme system:
    - Normal: Standard secondary button (typically gray)
    - Primary: Primary action button (typically blue, more prominent)
    - Destructive: Dangerous action button (typically red, for delete operations)
*/
enum class ButtonRole {
    Normal,       ///< Standard secondary button
    Primary,      ///< Primary action button (emphasized)
    Destructive   ///< Destructive action button (delete, remove, etc.)
};

/**
    Clickable button widget with customizable text, font, and style.
    
    Button is a standard clickable UI component that displays text and responds to
    mouse clicks. It supports:
    - Text with customizable font and size
    - Multiple visual roles (Normal, Primary, Destructive)
    - Hover and pressed states
    - Click callbacks
    - Keyboard focus (Space/Enter to activate)
    
    Font system (Version 3.0):
    The button uses a simplified Qt-style font API with automatic fallback:
    - setFont(): Set primary font, automatically adds CJK fallback
    - setFontChain(): Set complete fallback chain for full control
    - resetFont(): Return to theme default
    
    Visual states:
    - Normal: Default appearance
    - Hovered: Mouse over the button
    - Pressed: Mouse button held down
    - Disabled: Non-interactive state
    - Focused: Has keyboard focus (shows focus indicator)
    
    Example usage:
    @code
    // Create button
    Button* button = parent->addChild<Button>(Rect(10, 10, 100, 30));
    button->setText("Click Me");
    button->setRole(ButtonRole::Primary);
    
    // Set custom font
    button->setFont(myFontHandle);
    button->setFontSize(14.0f);
    
    // Handle clicks
    button->setClickCallback([]() {
        std::cout << "Button clicked!" << std::endl;
    });
    @endcode
    
    @see ButtonRole, UIComponent
*/
class Button : public UIComponent {
public:
    /**
        Constructs a button with the specified bounds.
        
        The button is created with:
        - Empty text
        - Default font size (from Config::Font::DEFAULT_SIZE)
        - Normal role
        - Strong focus policy (keyboard + mouse focus)
        
        @param bounds  Initial bounding rectangle
    */
    explicit Button(const Rect& bounds);
    
    virtual ~Button();
    
    //======================================================================================
    // UIComponent Interface Implementation
    
    void addDrawCommands(RenderList& commandList, const Vec2& offset = Vec2()) const override;
    bool handleMouseMove(const Vec2& position, const Vec2& offset = Vec2()) override;
    bool handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset = Vec2()) override;
    
    //======================================================================================
    // Text API
    
    /**
        Sets the button text.
        
        @param text  Text to display on the button
    */
    void setText(const std::string& text);
    
    /**
        Sets the button text from C-style string.
        
        @param text  Null-terminated C string
    */
    void setText(const char* text);
    
    /**
        Returns the current button text.
        
        @return Reference to the text string
    */
    const std::string& getText() const { return m_text; }
    
    //======================================================================================
    // Font API (Qt-style, Version 3.0)
    
    /**
        Sets button font with automatic fallback.
        
        The system automatically adds appropriate CJK fallback fonts based on the
        context's font provider. This is the recommended method for setting fonts.
        
        Example:
        @code
        button->setFont(arialFontHandle);
        // Automatically uses: Arial → System CJK → Emoji
        @endcode
        
        @param fontHandle  Primary font handle from font provider
    */
    void setFont(FontHandle fontHandle);
    
    /**
        Sets complete font fallback chain.
        
        For full control over font fallback, including emoji and symbol fonts.
        The chain must be valid (at least one font).
        
        Example:
        @code
        FontFallbackChain chain(arialFont, cjkFont, emojiFont);
        button->setFontChain(chain);
        @endcode
        
        @param chain  Font fallback chain to use
    */
    void setFontChain(const FontFallbackChain& chain);
    
    /**
        Returns current font fallback chain.
        
        If custom font not set, returns the theme's default button font chain.
        
        @return Current font fallback chain
    */
    FontFallbackChain getFontChain() const;
    
    /**
        Resets font to theme default.
        
        Clears any custom font setting and uses the theme's default button font chain.
    */
    void resetFont();
    
    //======================================================================================
    // Text Style API
    
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
    
    /**
        Sets custom text color.
        
        Overrides the theme's default text color for this button.
        
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
    // Button Behavior API
    
    /**
        Sets the button's visual role.
        
        The role determines the visual treatment:
        - Normal: Standard gray button
        - Primary: Emphasized blue button
        - Destructive: Warning red button
        
        @param role  Button role to set
    */
    void setRole(ButtonRole role);
    
    /**
        Returns the current button role.
        
        @return Current button role
    */
    ButtonRole getRole() const { return m_role; }
    
    /**
        Sets the callback function invoked when button is clicked.
        
        The callback is called when:
        1. Mouse button is pressed inside the button
        2. Mouse button is released inside the button
        3. Or Space/Enter is pressed while button has focus
        
        @param callback  Function to call on click (can be nullptr to clear)
    */
    void setClickCallback(ButtonClickCallback callback);
    
    /**
        Validates button state.
        
        Checks that:
        - Bounds are valid
        - Font size is within allowed range
        
        @return true if valid, false otherwise
    */
    bool isValid() const;
    
protected:
    /**
        Returns corner radius for focus indicator.
        
        Buttons use slightly rounded focus indicators.
        
        @return Corner radius of 3 pixels
    */
    CornerRadius getFocusIndicatorCornerRadius() const override { return CornerRadius(3.0f); }
    
private:
    std::string m_text;                   ///< Button text
    FontFallbackChain m_fontChain;        ///< Font fallback chain (if custom)
    float m_fontSize;                     ///< Font size in points
    Vec4 m_textColor;                     ///< Text color (if custom)
    
    ButtonRole m_role;                    ///< Visual role (Normal/Primary/Destructive)
    
    bool m_isHovered;                     ///< Whether mouse is over button
    bool m_isPressed;                     ///< Whether mouse button is held down
    
    ButtonClickCallback m_clickCallback;  ///< Click event callback
    
    bool m_hasCustomFont;                 ///< Whether custom font is set
    bool m_hasCustomTextColor;            ///< Whether custom text color is set
};

} // namespace YuchenUI
