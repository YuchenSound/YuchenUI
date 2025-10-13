/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Text module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

#pragma once

#include "YuchenUI/core/Types.h"
#include "YuchenUI/text/IFontProvider.h"
#include "YuchenUI/text/FontDatabase.h"
#include "YuchenUI/text/Font.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

namespace YuchenUI {

//==========================================================================================
/** Font registry entry. */
struct FontEntry {
    std::unique_ptr<FontFile> file;
    std::unique_ptr<FontFace> face;
    std::unique_ptr<FontCache> cache;
    std::string name;
    bool isValid;
    
    FontEntry() : file(nullptr), face(nullptr), cache(nullptr), name(), isValid(false) {}
};

//==========================================================================================
/**
    Font manager with FreeType integration and font fallback support.
    
    FontManager implements IFontProvider interface and provides complete font
    management for Desktop applications. It handles font loading, FreeType
    integration, system font enumeration, and font fallback chain resolution.
    
    Version 2.0 Changes:
    - Added hasGlyph() for checking glyph availability
    - Added selectFontForCodepoint() for font fallback
    - Added glyph availability caching for performance
    - Enhanced emoji and symbol font support
    
    Architecture:
    - Core layer code should use IFontProvider interface methods only
    - Desktop-specific code can use direct font access methods for specific fonts
    - Application layer injects FontManager instance into UIContext as IFontProvider
    
    Font Mapping on Desktop (macOS/Windows):
    - getDefaultFont() -> Arial Regular
    - getDefaultBoldFont() -> Arial Bold
    - getDefaultCJKFont() -> PingFang SC (macOS) / Microsoft YaHei (Windows)
    - getDefaultEmojiFont() -> Apple Color Emoji (macOS) / Segoe UI Emoji (Windows)
    
    Lifecycle:
    - Create instance via constructor
    - Call initialize() to load fonts
    - Inject into Application/UIContext as IFontProvider
    
    Usage Example:
    @code
    // In Application
    FontManager fontManager;
    fontManager.initialize();
    
    // Build fallback chain
    FontFallbackChain chain = fontManager.createDefaultFallbackChain();
    
    UIContext context(&fontManager);
    
    // In IUIContent implementation
    IFontProvider* fonts = m_context->getFontProvider();
    FontFallbackChain chain = fonts->createDefaultFallbackChain();
    cmdList.drawText("Hello世界😊", pos, chain, 14.0f, color);
    @endcode
    
    @see IFontProvider, Application
*/
class FontManager : public IFontProvider {
public:
    //======================================================================================
    // Instance-based API
    
    /**
        Creates font manager without initialization.
        
        Call initialize() after construction to load fonts.
        Typically created and managed by Application class.
    */
    FontManager();
    
    /**
        Destructor. Calls destroy() if still initialized.
    */
    ~FontManager();
    
    /**
        Initializes font manager and loads default fonts.
        
        Loads system fonts (Arial variants, CJK fonts, and emoji fonts).
        Must be called before using any font methods.
        
        @returns True if initialization succeeded
    */
    bool initialize();
    
    /**
        Destroys all fonts and FreeType library.
        
        Automatically called by destructor if needed.
    */
    void destroy();
    
    /**
        Returns true if font manager initialized.
    */
    bool isInitialized() const { return m_isInitialized; }
    
    //======================================================================================
    // IFontProvider Interface Implementation
    //
    // These methods should be used by Core layer code (IUIContent implementations)
    // to maintain platform independence. Do not use Desktop-specific methods below
    // in Core layer code.
    
    /**
        Loads font from memory buffer.
        
        @param data  Font data buffer (TTF/OTF/TTC format)
        @param size  Data size in bytes
        @param name  Font name for identification
        @returns Font handle, or INVALID_FONT_HANDLE on failure
    */
    FontHandle loadFontFromMemory(const void* data, size_t size, const char* name) override;
    
    /**
        Loads font from filesystem.
        
        @param path  Path to font file
        @param name  Font name for identification
        @returns Font handle, or INVALID_FONT_HANDLE on failure
    */
    FontHandle loadFontFromFile(const char* path, const char* name) override;
    
    /**
        Validates font handle.
        
        @param handle  Font handle to validate
        @returns True if handle references a valid loaded font
    */
    bool isValidFont(FontHandle handle) const override;
    
    /**
        Returns font metrics for specified size.
        
        @param handle    Font handle
        @param fontSize  Font size in points
        @returns Font metrics (ascender, descender, line height)
    */
    FontMetrics getFontMetrics(FontHandle handle, float fontSize) const override;
    
    /**
        Returns glyph metrics for character at specified size.
        
        @param handle     Font handle
        @param codepoint  Unicode code point
        @param fontSize   Font size in points
        @returns Glyph metrics (bearing, size, advance)
    */
    GlyphMetrics getGlyphMetrics(FontHandle handle, uint32_t codepoint, float fontSize) const override;
    
    /**
        Measures text dimensions.
        
        @param text      UTF-8 text string
        @param fontSize  Font size in points
        @returns Text bounding box (width, height)
    */
    Vec2 measureText(const char* text, float fontSize) const override;
    
    /**
        Returns line height for font at specified size.
        
        @param handle    Font handle
        @param fontSize  Font size in points
        @returns Line height in pixels
    */
    float getTextHeight(FontHandle handle, float fontSize) const override;
    
    /**
        Checks if a font has a glyph for specific Unicode code point.
        
        This method queries the font's character map using FreeType's FT_Get_Char_Index.
        A glyph index of 0 indicates the character is not supported.
        
        Performance: Results are cached in m_glyphAvailabilityCache to avoid
        repeated FreeType queries.
        
        @param handle     Font handle
        @param codepoint  Unicode code point to check
        @returns True if font has glyph for this character
    */
    bool hasGlyph(FontHandle handle, uint32_t codepoint) const override;
    
    /**
        Selects best font from fallback chain for specific character.
        
        Iterates through fallback chain in order, checking each font with hasGlyph().
        Returns first font that supports the character, or primary font if none do.
        
        Algorithm:
        1. For each font in fallback chain:
           - Check if font has glyph for codepoint
           - If yes, return this font immediately
        2. If no font supports character, return primary font (will render .notdef)
        
        Example:
        @code
        // Chain: [Arial, PingFang, Apple Color Emoji]
        selectFontForCodepoint('A', chain)    -> Arial
        selectFontForCodepoint('中', chain)    -> PingFang
        selectFontForCodepoint('😊', chain)   -> Apple Color Emoji
        selectFontForCodepoint('☺', chain)    -> Apple Color Emoji
        @endcode
        
        @param codepoint      Unicode code point
        @param fallbackChain  Ordered list of fonts to try
        @returns Font handle that can render character, or primary font
    */
    FontHandle selectFontForCodepoint(
        uint32_t codepoint,
        const FontFallbackChain& fallbackChain
    ) const override;

    FontHandle getDefaultFont()           const override { return m_defaultRegularFont;    }
    FontHandle getDefaultBoldFont()       const override { return m_defaultBoldFont;       }
    FontHandle getDefaultCJKFont()        const override { return m_defaultCJKFont;        }
    FontHandle getDefaultNarrowFont()     const override { return m_defaultNarrowFont;     }
    FontHandle getDefaultNarrowBoldFont() const override { return m_defaultNarrowBoldFont; }
    
    void* getFontFace(FontHandle handle) const override;
    
    /**
        Returns HarfBuzz font for specified size and DPI.
        
        Internal use only. Cast to hb_font_t* in implementation.
        
        @param handle    Font handle
        @param fontSize  Font size in points
        @param dpiScale  DPI scale factor
        @returns Opaque HarfBuzz font pointer
    */
    void* getHarfBuzzFont(FontHandle handle, float fontSize, float dpiScale) override;
    
    //======================================================================================
    // Font Fallback Chain Builders
    
    /**
        Creates default font fallback chain with all available fonts.
        
        Builds chain in priority order:
        1. Default regular font (Arial)
        2. Default CJK font (PingFang/YaHei)
        3. Default emoji font (if available)
        4. Default symbol font (if available)
        
        This is the recommended fallback chain for general text rendering.
        
        @returns Complete fallback chain
    */
    FontFallbackChain createDefaultFallbackChain() const;
    
    /**
        Creates bold font fallback chain.
        
        Similar to createDefaultFallbackChain() but uses bold variants where available.
        
        @returns Bold fallback chain
    */
    FontFallbackChain createBoldFallbackChain() const;
    
    /**
        Creates title font fallback chain.
        
        Uses larger, more prominent fonts suitable for titles.
        
        @returns Title fallback chain
    */
    FontFallbackChain createTitleFallbackChain() const;

    //======================================================================================
    // Font Database Integration (New in v3.0)

    /**
        Returns reference to internal font database.
        
        Provides access to advanced font query and metadata APIs.
        
        @returns Font database reference
    */
    FontDatabase& getFontDatabase() { return m_fontDatabase; }
    const FontDatabase& getFontDatabase() const { return m_fontDatabase; }

    /**
        Finds font by family name and properties.
        
        Uses intelligent matching with fallback logic.
        
        @param familyName  Font family name (case-insensitive)
        @param weight      Desired weight (default: Normal)
        @param style       Desired style (default: Normal)
        @returns Font handle, or INVALID_FONT_HANDLE if no match
    */
    FontHandle findFont(const char* familyName,
                        FontWeight weight = FontWeight::Normal,
                        FontStyle style = FontStyle::Normal) const;

    /**
        Lists all registered font families.
        
        @returns Vector of family names
    */
    std::vector<std::string> availableFontFamilies() const;

    /**
        Returns all font variants for a family.
        
        @param familyName  Font family name
        @returns Vector of font descriptors
    */
    std::vector<FontDescriptor> fontsForFamily(const char* familyName) const;

    /**
        Prints all registered fonts to console.
        
        Useful for debugging font discovery.
    */
    void printAvailableFonts() const;
private:
    //======================================================================================
    bool initializeFreeType();
    void cleanupFreeType();
    void initializeFonts();
    void loadSymbolFont();
    
#ifdef __APPLE__
    std::string getCoreTextFontPath(const char* fontName) const;
#endif

    FontEntry* getFontEntry(FontHandle handle);
    const FontEntry* getFontEntry(FontHandle handle) const;
    
    /**
        Checks glyph availability with caching.
        
        Internal method that implements hasGlyph() with caching layer.
        Cache key: (FontHandle << 32) | codepoint
        
        @param handle     Font handle
        @param codepoint  Unicode code point
        @returns True if font has glyph
    */
    bool hasGlyphImpl(FontHandle handle, uint32_t codepoint) const;
    
    //======================================================================================
    bool m_isInitialized;
    std::vector<FontEntry> m_fonts;
    FT_Library m_freeTypeLibrary;

    // Desktop platform font handles
    FontHandle m_defaultRegularFont;
    FontHandle m_defaultBoldFont;
    FontHandle m_defaultNarrowFont;
    FontHandle m_defaultNarrowBoldFont;
    FontHandle m_defaultCJKFont;
    FontHandle m_defaultSymbolFont;
    FontDatabase m_fontDatabase;  ///< Font database for metadata and role assignment

    // Glyph availability cache for performance
    // Key: (FontHandle << 32) | codepoint, Value: has glyph
    mutable std::unordered_map<uint64_t, bool> m_glyphAvailabilityCache;
};

} // namespace YuchenUI
