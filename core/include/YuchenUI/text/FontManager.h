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
#include "YuchenUI/text/Font.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <vector>
#include <memory>
#include <string>

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
    Font manager with FreeType integration.
    
    FontManager implements IFontProvider interface and provides complete font
    management for Desktop applications. It handles font loading, FreeType
    integration, and system font enumeration.
    
    Architecture:
    - Core layer code should use IFontProvider interface methods only
    - Desktop-specific code can use direct font access methods for specific fonts
    - Application layer injects FontManager instance into UIContext as IFontProvider
    
    Font Mapping on Desktop (macOS/Windows):
    - getDefaultFont() -> Arial Regular
    - getDefaultBoldFont() -> Arial Bold
    - getDefaultCJKFont() -> PingFang SC (macOS) / Microsoft YaHei (Windows)
    
    Lifecycle:
    - Create instance via constructor
    - Call initialize() to load fonts
    - Inject into Application/UIContext as IFontProvider
    
    Usage Example:
    @code
    // In Application
    FontManager fontManager;
    fontManager.initialize();
    
    UIContext context(&fontManager);  // Inject as IFontProvider
    
    // In IUIContent implementation
    IFontProvider* fonts = m_context->getFontProvider();
    FontHandle boldFont = fonts->getDefaultBoldFont();  // Use interface method
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
        
        Loads system fonts (Arial variants and CJK fonts).
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
        Returns default regular font handle.
        
        Desktop Implementation: Returns Arial Regular
        
        Use this method in Core layer code instead of getArialRegular()
        to maintain platform independence.
        
        @returns Default regular font handle
    */
    FontHandle getDefaultFont() const override;
    
    /**
        Returns default bold font handle.
        
        Desktop Implementation: Returns Arial Bold
        
        Use this method in Core layer code instead of getArialBold()
        to maintain platform independence.
        
        @returns Default bold font handle
    */
    FontHandle getDefaultBoldFont() const override;
    
    /**
        Returns default CJK font handle.
        
        Desktop Implementation:
        - macOS: PingFang SC
        - Windows: Microsoft YaHei
        
        Use this method in Core layer code instead of getPingFangFont()
        to maintain platform independence.
        
        @returns Default CJK font handle
    */
    FontHandle getDefaultCJKFont() const override;
    
    /**
        Returns opaque FreeType face handle for font.
        
        Internal use only. Cast to FT_Face in implementation.
        
        @param handle  Font handle
        @returns Opaque FreeType face pointer
    */
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
    // Desktop-specific Font Access
    //
    // WARNING: These methods are Desktop platform specific and should NOT be used in
    // Core layer code (IUIContent implementations). They exist for:
    // 1. Desktop-specific application code that needs direct font access
    // 2. Internal implementation of IFontProvider interface methods
    //
    // For Core layer code, use the IFontProvider interface methods above:
    // - getDefaultFont() instead of getArialRegular()
    // - getDefaultBoldFont() instead of getArialBold()
    // - getDefaultCJKFont() instead of getPingFangFont()
    
    /**
        Returns Arial Regular font handle.
        
        Direct access to Arial Regular font. Desktop-specific method.
        Core layer code should use getDefaultFont() instead.
        
        @returns Arial Regular font handle
    */
    FontHandle getArialRegular() const { return m_arialRegular; }
    
    /**
        Returns Arial Bold font handle.
        
        Direct access to Arial Bold font. Desktop-specific method.
        Core layer code should use getDefaultBoldFont() instead.
        
        @returns Arial Bold font handle
    */
    FontHandle getArialBold() const { return m_arialBold; }
    
    /**
        Returns Arial Narrow Regular font handle.
        
        Direct access to Arial Narrow Regular font. Desktop-specific method.
        
        @returns Arial Narrow Regular font handle
    */
    FontHandle getArialNarrowRegular() const { return m_arialNarrowRegular; }
    
    /**
        Returns Arial Narrow Bold font handle.
        
        Direct access to Arial Narrow Bold font. Desktop-specific method.
        
        @returns Arial Narrow Bold font handle
    */
    FontHandle getArialNarrowBold() const { return m_arialNarrowBold; }
    
    /**
        Returns PingFang/YaHei font handle.
        
        Direct access to CJK font. Desktop-specific method.
        Core layer code should use getDefaultCJKFont() instead.
        
        Platform: PingFang SC (macOS), Microsoft YaHei (Windows)
        
        @returns CJK font handle
    */
    FontHandle getPingFangFont() const { return m_pingFangFont; }
    
    /**
        Selects appropriate font for given character.
        
        Chooses between Western (Arial) and CJK fonts based on Unicode range.
        
        @param codepoint  Unicode code point
        @returns Font handle for character
    */
    FontHandle selectFontForCharacter(uint32_t codepoint) const;

private:
    //======================================================================================
    bool initializeFreeType();
    void cleanupFreeType();
    void initializeFonts();
    
#ifdef __APPLE__
    std::string getCoreTextFontPath(const char* fontName) const;
#endif

    FontHandle generateFontHandle();
    FontEntry* getFontEntry(FontHandle handle);
    const FontEntry* getFontEntry(FontHandle handle) const;
    
    //======================================================================================
    bool m_isInitialized;
    FontHandle m_nextHandle;
    std::vector<FontEntry> m_fonts;
    FT_Library m_freeTypeLibrary;

    // Desktop platform font handles
    FontHandle m_arialRegular;
    FontHandle m_arialBold;
    FontHandle m_arialNarrowRegular;
    FontHandle m_arialNarrowBold;
    FontHandle m_pingFangFont;
};

} // namespace YuchenUI
