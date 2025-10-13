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
/** @file Font.h
    
    Font file loading, FreeType face management, and HarfBuzz font caching.
    
    Provides three-tier font system:
    - FontFile: Loads font data from memory or filesystem
    - FontFace: Wraps FreeType FT_Face for glyph rasterization
    - FontCache: Caches HarfBuzz fonts per size with LRU eviction
    
    Font pipeline:
    1. FontFile loads raw font data (TTF/OTF/TTC)
    2. FontFace creates FreeType face from font data
    3. FontCache creates HarfBuzz fonts from FreeType face for text shaping
    4. Glyphs rasterized via FreeType, shaped via HarfBuzz
*/

#pragma once

#include "YuchenUI/core/Types.h"
#include "YuchenUI/core/Config.h"
#include "YuchenUI/text/TextUtils.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include <hb-ft.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <list>

namespace YuchenUI
{

//==========================================================================================
/**
    Font file data container.
    
    Loads font data from filesystem or memory buffer and stores it for FreeType.
    Data copied to internal buffer to ensure lifetime independence from source.
    Supports TrueType (.ttf), OpenType (.otf), and TrueType Collection (.ttc) formats.
    
    @see FontFace
*/
class FontFile
{
public:
    /** Creates empty font file. */
    FontFile();
    
    /** Destructor. Frees font data buffer. */
    ~FontFile();

    //======================================================================================
    /** Loads font from memory buffer.
        
        Copies data to internal buffer to ensure lifetime independence.
        
        @param data  Font data buffer (TTF/OTF/TTC format)
        @param size  Data size in bytes
        @param name  Font name for identification
        @returns True if load succeeded
    */
    bool loadFromMemory(const void* data, size_t size, const std::string& name);
    
    /** Loads font from filesystem.
        
        Reads entire file into memory buffer.
        
        @param path  Path to font file
        @param name  Font name for identification
        @returns True if file read succeeded
    */
    bool loadFromFile(const char* path, const std::string& name);
    
    //======================================================================================
    /** Returns font name. */
    const std::string& getName() const { return m_name; }
    
    /** Returns original file path if loaded from file, empty otherwise. */
    const std::string& getFilePath() const { return m_filePath; }
    
    /** Returns font data buffer. */
    const std::vector<unsigned char>& getMemoryData() const { return m_memoryData; }
    
    /** Returns true if font data loaded successfully. */
    bool isValid() const { return m_isValid; }

private:
    //======================================================================================
    std::string m_name;                          ///< Font name
    std::string m_filePath;                      ///< Source file path (if loaded from file)
    std::vector<unsigned char> m_memoryData;     ///< Font data buffer
    bool m_isValid;                              ///< Load success flag

    FontFile(const FontFile&) = delete;
    FontFile& operator=(const FontFile&) = delete;
};

//==========================================================================================
/**
    FreeType font face wrapper.
    
    Wraps FT_Face for glyph rasterization and metrics queries. Provides size-independent
    interface - font size specified per operation. Face must be created from valid FontFile.
    
    Thread safety: Not thread-safe. FT_Face is not thread-safe.
    
    @see FontFile, FontCache
*/
class FontFace
{
public:
    //======================================================================================
    /** Creates font face with FreeType library context.
        
        @param library  FreeType library instance (must outlive face)
    */
    FontFace(FT_Library library);
    
    /** Destructor. Destroys FreeType face if created. */
    ~FontFace();

    //======================================================================================
    /** Creates FreeType face from font file.
        
        Selects Unicode character map automatically. Face remains valid as long
        as source FontFile remains valid (FreeType may reference its data).
        
        @param fontFile  Font file containing face data
        @returns True if face created successfully
    */
    bool createFromFontFile(const FontFile& fontFile);
    
    /** Destroys FreeType face and releases resources. */
    void destroy();

    //======================================================================================
    /** Returns underlying FreeType face handle. */
    FT_Face getFTFace() const { return m_face; }
    
    /** Returns true if face created successfully. */
    bool isValid() const { return m_face != nullptr; }

    //======================================================================================
    /** Returns font-level metrics for specified size.
        
        Metrics scaled to requested font size.
        
        @param fontSize  Font size in points
        @returns Font metrics (ascender, descender, line height)
    */
    FontMetrics getMetrics(float fontSize) const;
    
    /** Returns metrics for specific glyph at specified size.
        
        @param codepoint  Unicode code point
        @param fontSize   Font size in points
        @returns Glyph metrics (bearing, size, advance)
    */
    GlyphMetrics getGlyphMetrics(uint32_t codepoint, float fontSize) const;
    
    /** Measures horizontal extent of text string.
        
        Simple measurement without shaping. For accurate measurement with
        complex scripts, use TextRenderer::shapeText().
        
        @param text      UTF-8 text string
        @param fontSize  Font size in points
        @returns Total horizontal advance in pixels
    */
    float measureText(const char* text, float fontSize) const;

    //==========================================================================================
    /** Sets character size on FreeType face.
        
        Automatically detects and handles bitmap fonts (e.g., Apple Color Emoji).
        - For bitmap fonts: Selects closest fixed size using FT_Select_Size()
        - For vector fonts: Scales to exact size using FT_Set_Char_Size()
        
        Must be called before glyph loading operations. Uses Config::Font::FREETYPE_DPI
        for resolution.
        
        @param fontSize  Font size in points
        @returns True if size set successfully
    */
    bool setCharSize(float fontSize) const;

private:
    //======================================================================================
    FT_Library m_library;  ///< FreeType library context (not owned)
    FT_Face m_face;        ///< FreeType face handle

    FontFace(const FontFace&) = delete;
    FontFace& operator=(const FontFace&) = delete;
};

//==========================================================================================
/**
    HarfBuzz font cache with LRU eviction.
    
    Caches HarfBuzz font objects per font size to avoid repeated creation overhead.
    Uses Least Recently Used (LRU) eviction when cache reaches capacity. HarfBuzz
    fonts created from FreeType faces for text shaping operations.
    
    Cache key: Font size quantized to half-point resolution (size * 2)
    Capacity: Config::Font::MAX_CACHED_SIZES per FontFace
    
    @see FontFace, TextRenderer
*/
class FontCache
{
public:
    /** Creates empty font cache. */
    FontCache();
    
    /** Destructor. Destroys all cached HarfBuzz fonts. */
    ~FontCache();

    //======================================================================================
    /** Retrieves or creates HarfBuzz font for specified size.
        
        Returns cached font if available, creates new if not. Updates LRU order
        on access. Evicts least recently used entry when cache full.
        
        @param fontFace  FreeType font face
        @param fontSize  Font size in points
        @returns HarfBuzz font handle, or nullptr on error
    */
    hb_font_t* getHarfBuzzFont(const FontFace& fontFace, float fontSize);
    
    /** Destroys all cached HarfBuzz fonts and clears cache. */
    void clearAll();

private:
    //======================================================================================
    /** Evicts least recently used entry to make room for new entry. */
    void evictLeastRecentlyUsed();
    
    /** Updates LRU order for accessed entry.
        
        Moves entry to front of usage list.
        
        @param sizeKey  Quantized font size key
    */
    void updateLRU(uint32_t sizeKey);
    
    /** Creates new HarfBuzz font from FreeType face.
        
        Sets up HarfBuzz font with FreeType callbacks and scale factors.
        
        @param fontFace  FreeType font face
        @param fontSize  Font size in points
        @returns HarfBuzz font handle, or nullptr on error
    */
    hb_font_t* createHarfBuzzFont(const FontFace& fontFace, float fontSize);

    //======================================================================================
    std::unordered_map<uint32_t, hb_font_t*> m_harfBuzzFonts;  ///< Size key -> HarfBuzz font
    std::list<uint32_t> m_usageOrder;                          ///< LRU list (front = most recent)
    
    static constexpr size_t MAX_CACHED_SIZES = 8;              ///< Maximum cached sizes per face

    FontCache(const FontCache&) = delete;
    FontCache& operator=(const FontCache&) = delete;
};

} // namespace YuchenUI
