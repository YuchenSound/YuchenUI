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
/** @file TextRenderer.h
    
    High-level text rendering with HarfBuzz shaping and glyph caching.
    
    TextRenderer provides complete text rendering pipeline:
    1. Text segmentation by character set (Western/CJK)
    2. HarfBuzz text shaping per segment
    3. FreeType glyph rasterization on demand
    4. Glyph caching in GPU texture atlases
    5. Vertex generation for GPU rendering
    
    Shaping pipeline:
    - Segment text by Western/CJK boundaries
    - Shape each segment with appropriate font
    - Combine shaped segments with proper positioning
    - Cache shaped results for repeated text
    
    Rendering pipeline:
    - Lookup glyphs in cache (rasterize if not cached)
    - Generate quad vertices with texture coordinates
    - Vertices reference current atlas texture
*/

#pragma once

#include "YuchenUI/core/Types.h"
#include "YuchenUI/text/GlyphCache.h"
#include <hb.h>
#include <vector>
#include <unordered_map>

namespace YuchenUI {

class IGraphicsBackend;
class IFontProvider;

//==========================================================================================
/** Cache key for shaped text results.
    
    Combines text content, fonts, and size into 64-bit hash for fast lookup.
*/
struct TextCacheKey {
    uint64_t hash;  ///< Combined hash of text, fonts, and size
    
    /** Creates cache key from text rendering parameters.
        
        @param text          UTF-8 text string
        @param westernFont   Font handle for Western characters
        @param chineseFont   Font handle for CJK characters
        @param fontSize      Font size in points
    */
    TextCacheKey(const char* text, FontHandle westernFont, FontHandle chineseFont, float fontSize);
    
    bool operator==(const TextCacheKey& other) const { return hash == other.hash; }
};

/** Hash functor for TextCacheKey. */
struct TextCacheKeyHash {
    size_t operator()(const TextCacheKey& key) const { return key.hash; }
};

//==========================================================================================
/**
    Text rendering with shaping and glyph caching.
    
    TextRenderer manages complete text rendering pipeline from text string to GPU
    vertices. Uses HarfBuzz for complex script shaping, FreeType for glyph
    rasterization, and GPU texture atlases for caching. Shaped text cached to
    avoid repeated shaping overhead.
    
    Key features:
    - Multi-font text support (Western + CJK mixing)
    - Complex script shaping via HarfBuzz
    - On-demand glyph rasterization and caching
    - Shaped text caching for performance
    - DPI-aware rendering
    
    Thread safety: Not thread-safe. Use from single thread.
    
    @see GlyphCache, IFontProvider, TextUtils
*/
class TextRenderer {
public:
    //======================================================================================
    /** Creates text renderer with graphics backend and font provider.
        
        @param backend        Graphics backend for texture operations (not owned)
        @param fontProvider   Font provider for font access (not owned)
    */
    TextRenderer(IGraphicsBackend* backend, IFontProvider* fontProvider);
    
    /** Destructor. Destroys glyph cache and HarfBuzz buffer. */
    ~TextRenderer();
    
    //======================================================================================
    /** Initializes text renderer with DPI scale.
        
        Creates glyph cache and HarfBuzz buffer. Font provider must be set before
        calling this method.
        
        @param dpiScale  DPI scale factor for glyph rendering
        @returns True if initialization succeeded
    */
    bool initialize(float dpiScale = 1.0f);
    
    /** Destroys text renderer and releases all resources.
        
        Destroys glyph cache, HarfBuzz buffer, and clears shaped text cache.
    */
    void destroy();
    
    /** Returns true if text renderer initialized. */
    bool isInitialized() const;
    
    //======================================================================================
    /** Advances frame counter for glyph cache expiration.
        
        Call at start of each frame before text rendering. Triggers periodic
        cleanup of unused glyphs.
    */
    void beginFrame();
    
    /** Shapes text string with appropriate fonts.
        
        Segments text by character set (Western/CJK), shapes each segment with
        appropriate font, and combines results. Shaped results cached for reuse.
        
        Process:
        1. Check shaped text cache
        2. Segment text by Western/CJK boundaries
        3. Shape each segment with HarfBuzz
        4. Combine segments with proper positioning
        5. Cache result for future use
        
        @param text          UTF-8 text string
        @param westernFont   Font handle for Western characters
        @param chineseFont   Font handle for CJK characters
        @param fontSize      Font size in points
        @param outShapedText Output shaped text result
    */
    void shapeText(const char* text, FontHandle westernFont, FontHandle chineseFont, float fontSize, ShapedText& outShapedText);
    
    /** Generates GPU vertices for shaped text.
        
        For each glyph in shaped text:
        1. Lookup in cache (rasterize if not cached)
        2. Calculate screen position with bearing
        3. Generate quad vertices with texture coordinates
        
        All vertices reference current glyph atlas texture. Must upload vertices
        and bind atlas texture before rendering.
        
        @param shapedText    Shaped text from shapeText()
        @param basePosition  Text baseline position in pixels
        @param color         Text color (RGBA)
        @param fontHandle    Font handle (currently unused, uses shaped glyph fonts)
        @param fontSize      Font size in points
        @param outVertices   Output vertex buffer (cleared and filled)
    */
    void generateTextVertices(const ShapedText& shapedText, const Vec2& basePosition, const Vec4& color, FontHandle fontHandle, float fontSize, std::vector<TextVertex>& outVertices);
    
    //======================================================================================
    /** Returns opaque handle to current glyph atlas texture.
        
        Bind this texture before rendering text vertices.
        
        @returns GPU texture handle for current atlas
    */
    void* getCurrentAtlasTexture() const;
    
    /** Returns DPI scale factor.
        
        @returns DPI scale used for glyph rasterization
    */
    float getDPIScale() const;
    
private:
    //======================================================================================
    /** Initializes HarfBuzz buffer for text shaping.
        
        @returns True if buffer created successfully
    */
    bool initializeResources();
    
    /** Destroys HarfBuzz buffer and releases resources. */
    void cleanupResources();
    
    /** Shapes text segment with HarfBuzz.
        
        Performs Unicode normalization, script detection, and bidi analysis
        automatically via HarfBuzz. Applies kerning features.
        
        @param text          UTF-8 text segment
        @param fontHandle    Font handle for segment
        @param fontSize      Font size in points
        @param outShapedText Output shaped glyphs
        @returns True if shaping succeeded
    */
    bool shapeTextWithHarfBuzz(const char* text, FontHandle fontHandle, float fontSize, ShapedText& outShapedText);
    
    /** Rasterizes glyph with FreeType.
        
        Loads and renders glyph bitmap at specified size. Bitmap remains valid
        until next glyph load on same face.
        
        @param fontHandle      Font handle
        @param glyphIndex      Glyph index in font
        @param fontSize        Font size in points (DPI-scaled)
        @param outBitmapData   Output bitmap buffer pointer (R8 format)
        @param outSize         Output bitmap dimensions
        @param outBearing      Output glyph bearing
        @param outAdvance      Output horizontal advance
    */
    void renderGlyph(FontHandle fontHandle, uint32_t glyphIndex, float fontSize, const void*& outBitmapData, Vec2& outSize, Vec2& outBearing, float& outAdvance);
    
    /** Rasterizes glyph using FreeType face directly.
        
        Helper function wrapping FT_Load_Glyph with FT_LOAD_RENDER.
        
        @param face            Opaque FT_Face pointer
        @param glyphIndex      Glyph index in font
        @param fontSize        Font size in points (DPI-scaled)
        @param outBitmapData   Output bitmap buffer pointer (R8 format)
        @param outSize         Output bitmap dimensions
        @param outBearing      Output glyph bearing
        @param outAdvance      Output horizontal advance
    */
    void rasterizeGlyphWithFreeType(void* face, uint32_t glyphIndex, float fontSize, const void*& outBitmapData, Vec2& outSize, Vec2& outBearing, float& outAdvance);
    
    //======================================================================================
    IGraphicsBackend* m_backend;                                                    ///< Graphics backend (not owned)
    IFontProvider* m_fontProvider;                                                  ///< Font provider (not owned)
    std::unique_ptr<GlyphCache> m_glyphCache;                                       ///< Glyph atlas cache
    bool m_isInitialized;                                                           ///< Initialization state
    float m_dpiScale;                                                               ///< DPI scale factor
    hb_buffer_t* m_harfBuzzBuffer;                                                  ///< Reusable HarfBuzz buffer
    std::unordered_map<TextCacheKey, ShapedText, TextCacheKeyHash> m_shapedTextCache; ///< Shaped text cache
};

} // namespace YuchenUI
