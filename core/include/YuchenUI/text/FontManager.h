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

//==========================================================================================
/** @file FontManager.h
    
    Singleton font manager with FreeType integration.
    
    Manages lifecycle of all fonts in application. Initializes FreeType library,
    loads embedded and system fonts, and provides font selection based on character
    sets. Pre-loads embedded Arial fonts and platform-specific CJK fonts.
    
    Font handles:
    - Opaque size_t values identifying fonts
    - INVALID_FONT_HANDLE (SIZE_MAX) represents invalid font
    - Handles valid for lifetime of FontManager
    
    Platform differences:
    - macOS: Loads PingFang SC via CoreText
    - Windows: Loads Microsoft YaHei from system fonts
    - Fallback: Uses Arial for CJK if system font unavailable
*/

#pragma once

#include "YuchenUI/core/Types.h"
#include "YuchenUI/text/Font.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <vector>
#include <memory>
#include <string>

namespace YuchenUI {

//==========================================================================================
/** Font registry entry.
    
    Contains complete font data: file, FreeType face, and HarfBuzz cache.
    All components must remain valid together.
*/
struct FontEntry {
    std::unique_ptr<FontFile> file;    ///< Font file data
    std::unique_ptr<FontFace> face;    ///< FreeType face wrapper
    std::unique_ptr<FontCache> cache;  ///< HarfBuzz font cache
    std::string name;                  ///< Font name
    bool isValid;                      ///< Entry validity flag
    
    FontEntry() : file(nullptr), face(nullptr), cache(nullptr), name(), isValid(false) {}
};

//==========================================================================================
/**
    Singleton font manager.
    
    FontManager provides centralized font management with FreeType integration.
    Automatically loads embedded fonts and platform-specific system fonts on
    initialization. Provides character-based font selection for mixed Western/CJK text.
    
    Lifecycle:
    1. getInstance() creates and initializes singleton
    2. Loads embedded Arial fonts from resources
    3. Loads system CJK fonts (PingFang/Microsoft YaHei)
    4. destroy() cleans up FreeType and all fonts
    
    Character selection:
    - Western characters (Latin, Greek, Cyrillic): Arial
    - CJK characters (Chinese, Japanese, Korean): PingFang/YaHei
    - Fallback: Arial for unknown characters
    
    Thread safety: Not thread-safe. Use from single thread.
    
    @see FontFile, FontFace, FontCache
*/
class FontManager {
public:
    //======================================================================================
    /** Returns singleton instance.
        
        Creates instance on first call. Does not auto-initialize - caller must
        call initialize() explicitly.
        
        @returns Reference to singleton instance
    */
    static FontManager& getInstance();
    
    //======================================================================================
    /** Creates font manager without initialization. */
    FontManager();
    
    /** Destructor. Calls destroy() if still initialized. */
    ~FontManager();
    
    //======================================================================================
    /** Initializes font manager and loads default fonts.
        
        Initializes FreeType library and loads:
        - Embedded Arial Regular, Bold, Narrow Regular, Narrow Bold
        - Platform CJK font: PingFang SC (macOS) or Microsoft YaHei (Windows)
        - Falls back to Arial for CJK if system font unavailable
        
        @returns True if initialization succeeded
    */
    bool initialize();
    
    /** Destroys all fonts and FreeType library.
        
        Invalidates all font handles. Clears measurement caches.
    */
    void destroy();
    
    /** Returns true if font manager initialized. */
    bool isInitialized() const { return m_isInitialized; }
    
    //======================================================================================
    /** Returns handle to embedded Arial Regular font. */
    FontHandle getArialRegular() const { return m_arialRegular; }
    
    /** Returns handle to embedded Arial Bold font. */
    FontHandle getArialBold() const { return m_arialBold; }
    
    /** Returns handle to embedded Arial Narrow Regular font. */
    FontHandle getArialNarrowRegular() const { return m_arialNarrowRegular; }
    
    /** Returns handle to embedded Arial Narrow Bold font. */
    FontHandle getArialNarrowBold() const { return m_arialNarrowBold; }
    
    /** Returns handle to platform CJK font (PingFang/YaHei).
        
        Falls back to Arial if system font unavailable.
    */
    FontHandle getPingFangFont() const { return m_pingFangFont; }
    
    //======================================================================================
    /** Selects appropriate font for character.
        
        Selection rules:
        - Western characters: Arial Regular
        - CJK characters: PingFang/YaHei
        - Other: Arial Regular
        
        @param codepoint  Unicode code point
        @returns Font handle for character
    */
    FontHandle selectFontForCharacter(uint32_t codepoint) const;
    
    /** Validates font handle.
        
        @param handle  Font handle to validate
        @returns True if handle references valid loaded font
    */
    bool isValidFont(FontHandle handle) const;
    
    //======================================================================================
    /** Returns font metrics for specified size.
        
        Results cached to avoid repeated FreeType queries.
        
        @param handle    Font handle
        @param fontSize  Font size in points
        @returns Font metrics (ascender, descender, line height)
    */
    FontMetrics getFontMetrics(FontHandle handle, float fontSize) const;
    
    /** Returns glyph metrics for character at specified size.
        
        @param handle     Font handle
        @param codepoint  Unicode code point
        @param fontSize   Font size in points
        @returns Glyph metrics (bearing, size, advance)
    */
    GlyphMetrics getGlyphMetrics(FontHandle handle, uint32_t codepoint, float fontSize) const;
    
    /** Measures text dimensions with proper font selection.
        
        Segments text by character set, measures each segment with appropriate font,
        and combines results. Uses HarfBuzz shaping for accurate measurement.
        Results cached to avoid repeated measurement.
        
        @param text      UTF-8 text string
        @param fontSize  Font size in points
        @returns Text bounding box (width, height)
    */
    Vec2 measureText(const char* text, float fontSize) const;
    
    /** Returns line height for font at specified size.
        
        @param handle    Font handle
        @param fontSize  Font size in points
        @returns Line height in pixels
    */
    float getTextHeight(FontHandle handle, float fontSize) const;
    
    //======================================================================================
    /** Returns opaque FreeType face handle for font.
        
        Cast to FT_Face in caller code.
        
        @param handle  Font handle
        @returns Opaque FreeType face pointer
    */
    void* getFontFace(FontHandle handle) const;
    
    /** Returns HarfBuzz font for specified size and DPI.
        
        Retrieves from cache or creates new. Cast to hb_font_t* in caller code.
        
        @param handle    Font handle
        @param fontSize  Font size in points
        @param dpiScale  DPI scale factor
        @returns Opaque HarfBuzz font pointer
    */
    void* getHarfBuzzFont(FontHandle handle, float fontSize, float dpiScale);

private:
    //======================================================================================
    /** Initializes FreeType library.
        
        @returns True if FreeType initialization succeeded
    */
    bool initializeFreeType();
    
    /** Destroys FreeType library and clears context. */
    void cleanupFreeType();
    
    /** Loads default embedded and system fonts.
        
        Loads embedded Arial fonts and platform CJK font.
    */
    void initializeFonts();
    
#ifdef __APPLE__
    /** Retrieves filesystem path for CoreText font by name.
        
        Uses CoreText API to locate system font file.
        
        @param fontName  Font name (e.g., "PingFang SC Regular")
        @returns Font file path, or empty string if not found
    */
    std::string getCoreTextFontPath(const char* fontName) const;
#endif

    //======================================================================================
    /** Loads font from filesystem.
        
        Creates FontEntry with file, face, and cache.
        
        @param path  Path to font file
        @param name  Font name for identification
        @returns Font handle, or INVALID_FONT_HANDLE on error
    */
    FontHandle loadFontFromFile(const char* path, const std::string& name);
    
    /** Loads font from memory buffer.
        
        Creates FontEntry with file, face, and cache.
        
        @param data  Font data buffer
        @param size  Data size in bytes
        @param name  Font name for identification
        @returns Font handle, or INVALID_FONT_HANDLE on error
    */
    FontHandle loadFontFromMemory(const void* data, size_t size, const std::string& name);
    
    /** Generates new unique font handle.
        
        Increments handle counter to ensure uniqueness.
        
        @returns New font handle
    */
    FontHandle generateFontHandle();
    
    /** Retrieves font entry by handle.
        
        @param handle  Font handle
        @returns Pointer to font entry, or nullptr if invalid
    */
    FontEntry* getFontEntry(FontHandle handle);
    
    /** Retrieves font entry by handle (const version).
        
        @param handle  Font handle
        @returns Const pointer to font entry, or nullptr if invalid
    */
    const FontEntry* getFontEntry(FontHandle handle) const;

    //======================================================================================
    static FontManager* s_instance;          ///< Singleton instance
    
    bool m_isInitialized;                    ///< Initialization state
    FontHandle m_nextHandle;                 ///< Next font handle to allocate
    std::vector<FontEntry> m_fonts;          ///< Font registry
    FT_Library m_freeTypeLibrary;            ///< FreeType library context

    FontHandle m_arialRegular;               ///< Embedded Arial Regular
    FontHandle m_arialBold;                  ///< Embedded Arial Bold
    FontHandle m_arialNarrowRegular;         ///< Embedded Arial Narrow Regular
    FontHandle m_arialNarrowBold;            ///< Embedded Arial Narrow Bold
    FontHandle m_pingFangFont;               ///< Platform CJK font (PingFang/YaHei)
};

} // namespace YuchenUI
