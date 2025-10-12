/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Core module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

#pragma once

#include "YuchenUI/core/Types.h"

namespace YuchenUI {

//==========================================================================================
/**
    Abstract interface for font resource providers.
    
    IFontProvider defines the interface for accessing font resources and metrics.
    This abstraction allows the Core rendering layer to remain independent of
    specific font management implementations.
    
    The interface is implemented by FontManager in the Desktop layer, but can
    also be implemented by custom font providers for embedded scenarios (e.g., GLFW).
    
    Thread safety: Implementation-dependent. FontManager implementation is not thread-safe.
    
    @see FontManager
*/
class IFontProvider {
public:
    virtual ~IFontProvider() = default;
    
    //======================================================================================
    // Font Loading
    
    /**
        Loads a font from memory buffer.
        
        @param data  Font data buffer (TTF/OTF/TTC format)
        @param size  Data size in bytes
        @param name  Font name for identification
        @returns Font handle, or INVALID_FONT_HANDLE on failure
    */
    virtual FontHandle loadFontFromMemory(const void* data, size_t size, const char* name) = 0;
    
    /**
        Loads a font from filesystem.
        
        @param path  Path to font file
        @param name  Font name for identification
        @returns Font handle, or INVALID_FONT_HANDLE on failure
    */
    virtual FontHandle loadFontFromFile(const char* path, const char* name) = 0;
    
    //======================================================================================
    // Font Queries
    
    /**
        Validates a font handle.
        
        @param handle  Font handle to validate
        @returns True if handle references a valid loaded font
    */
    virtual bool isValidFont(FontHandle handle) const = 0;
    
    /**
        Returns font metrics for specified size.
        
        @param handle    Font handle
        @param fontSize  Font size in points
        @returns Font metrics (ascender, descender, line height)
    */
    virtual FontMetrics getFontMetrics(FontHandle handle, float fontSize) const = 0;
    
    /**
        Returns glyph metrics for character at specified size.
        
        @param handle     Font handle
        @param codepoint  Unicode code point
        @param fontSize   Font size in points
        @returns Glyph metrics (bearing, size, advance)
    */
    virtual GlyphMetrics getGlyphMetrics(FontHandle handle, uint32_t codepoint, float fontSize) const = 0;
    
    /**
        Measures text dimensions with proper font selection.
        
        @param text      UTF-8 text string
        @param fontSize  Font size in points
        @returns Text bounding box (width, height)
    */
    virtual Vec2 measureText(const char* text, float fontSize) const = 0;
    
    /**
        Returns line height for font at specified size.
        
        @param handle    Font handle
        @param fontSize  Font size in points
        @returns Line height in pixels
    */
    virtual float getTextHeight(FontHandle handle, float fontSize) const = 0;
    
    //======================================================================================
    // Font Fallback Support (New in v2.0)
    
    /**
        Checks if a font has a glyph for a specific Unicode code point.
        
        This method is critical for font fallback. It queries the font's character
        map to determine if it can render the specified character.
        
        Performance note: This method is frequently called during text layout.
        Implementations should cache results when possible.
        
        Example usage:
        @code
        if (fontProvider->hasGlyph(arialFont, 0x1F600)) {
            // Arial can render 😀 (it can't in reality)
        } else {
            // Need to try emoji font
        }
        @endcode
        
        @param handle     Font handle
        @param codepoint  Unicode code point to check
        @returns True if font has a glyph for this character, false otherwise
    */
    virtual bool hasGlyph(FontHandle handle, uint32_t codepoint) const = 0;
    
    /**
        Selects the best font from a fallback chain for a specific character.
        
        Iterates through the fallback chain and returns the first font that
        has a glyph for the specified character. If no font in the chain supports
        the character, returns the primary font (first in chain).
        
        This is the core of the font fallback system. It enables proper rendering
        of mixed-script text like "Hello世界😊" where different characters need
        different fonts.
        
        Example usage:
        @code
        FontFallbackChain chain(arialFont, cjkFont, emojiFont);
        FontHandle font = fontProvider->selectFontForCodepoint(0x1F600, chain);
        // Returns emojiFont because Arial and CJK fonts don't have emoji
        @endcode
        
        @param codepoint      Unicode code point
        @param fallbackChain  Ordered list of fonts to try
        @returns Font handle that can render this character, or primary font if none
    */
    virtual FontHandle selectFontForCodepoint(
        uint32_t codepoint,
        const FontFallbackChain& fallbackChain
    ) const = 0;
    
    //======================================================================================
    // Default Font Access
    
    /**
        Returns default regular font handle.
        
        @returns Default regular font (e.g., Arial Regular)
    */
    virtual FontHandle getDefaultFont() const = 0;
    
    /**
        Returns default bold font handle.
        
        @returns Default bold font (e.g., Arial Bold)
    */
    virtual FontHandle getDefaultBoldFont() const = 0;
    
    /**
        Returns default CJK font handle.
        
        @returns Default CJK font (e.g., PingFang SC or Microsoft YaHei)
    */
    virtual FontHandle getDefaultCJKFont() const = 0;
    
    //======================================================================================
    // Internal Access (used by rendering backend)
    
    /**
        Returns opaque FreeType face handle for font.
        
        Internal use only. Cast to FT_Face in implementation.
        
        @param handle  Font handle
        @returns Opaque FreeType face pointer
    */
    virtual void* getFontFace(FontHandle handle) const = 0;
    
    /**
        Returns HarfBuzz font for specified size and DPI.
        
        Internal use only. Cast to hb_font_t* in implementation.
        
        @param handle    Font handle
        @param fontSize  Font size in points
        @param dpiScale  DPI scale factor
        @returns Opaque HarfBuzz font pointer
    */
    virtual void* getHarfBuzzFont(FontHandle handle, float fontSize, float dpiScale) = 0;
};

} // namespace YuchenUI
