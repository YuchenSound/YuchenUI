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
    
    High-level text rendering with HarfBuzz shaping, glyph caching, and font fallback.
    
    Version 2.0 Changes:
    - Added shapeText() overload with FontFallbackChain support
    - Enhanced text segmentation with fallback chain resolution
    - Improved multi-script text rendering
    - Better emoji and symbol support
    
    TextRenderer provides complete text rendering pipeline:
    1. Text segmentation by font fallback chain (per-character font selection)
    2. HarfBuzz text shaping per segment
    3. FreeType glyph rasterization on demand
    4. Glyph caching in GPU texture atlases
    5. Vertex generation for GPU rendering
    
    Shaping pipeline:
    - Segment text by font fallback (Western/CJK/Emoji/Symbol)
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
    uint64_t hash;
    
    /** Creates cache key from text rendering parameters.
        
        Letter spacing is quantized to integer values to improve cache hit rate.
        
        @param text              UTF-8 text string
        @param fallbackChain     Font fallback chain
        @param fontSize          Font size in points
        @param letterSpacing     Letter spacing in thousandths of em
    */
    TextCacheKey(const char* text, const FontFallbackChain& fallbackChain,
                 float fontSize, float letterSpacing = 0.0f);
    
    bool operator==(const TextCacheKey& other) const { return hash == other.hash; }
};

/** Hash functor for TextCacheKey. */
struct TextCacheKeyHash {
    size_t operator()(const TextCacheKey& key) const { return key.hash; }
};

//==========================================================================================
/**
    Text rendering with shaping, glyph caching, and font fallback.
    
    TextRenderer manages complete text rendering pipeline from text string to GPU
    vertices. Uses HarfBuzz for complex script shaping, FreeType for glyph
    rasterization, and GPU texture atlases for caching. Shaped text cached to
    avoid repeated shaping overhead.
    
    Version 2.0 Changes:
    - Font fallback chain support for mixed scripts
    - Per-character font selection
    - Enhanced emoji and symbol rendering
    - Improved cache key generation
    
    Key features:
    - Multi-font text support via fallback chains
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
    
    //======================================================================================
    // Text Shaping (New API with Font Fallback)
    
    /**
        Shapes text string with font fallback chain and letter spacing.
        
        Letter spacing is applied after HarfBuzz shaping by adjusting glyph
        advances. The spacing value is in thousandths of em:
        - 0 = normal spacing
        - 100 = add 0.1em between characters
        - -100 = reduce spacing by 0.1em
        
        Example:
        @code
        ShapedText shaped;
        renderer.shapeText("Hello", chain, 14.0f, 100.0f, shaped);  // +0.1em spacing
        @endcode
        
        @param text              UTF-8 text string
        @param fallbackChain     Font fallback chain
        @param fontSize          Font size in points
        @param letterSpacing     Letter spacing in thousandths of em (-1000 to 1000)
        @param outShapedText     Output shaped text result
    */
    void shapeText(const char* text,
                   const FontFallbackChain& fallbackChain,
                   float fontSize,
                   float letterSpacing,
                   ShapedText& outShapedText);
    
    //======================================================================================
    /** Generates GPU vertices for shaped text.
    */
    void generateTextVertices(const ShapedText& shaped,
                             const Vec2& position,
                             const Vec4& color,
                             const FontFallbackChain& fontChain,
                             float fontSize,
                             std::vector<TextVertex>& vertices);
    
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
    
    /**
        Shapes text segment with HarfBuzz and applies letter spacing.
        
        @param text              UTF-8 text segment
        @param fontHandle        Font handle for segment
        @param fontSize          Font size in points
        @param letterSpacing     Letter spacing in thousandths of em
        @param outShapedText     Output shaped glyphs
        @returns True if shaping succeeded
    */
    bool shapeTextWithHarfBuzz(const char* text,
                               FontHandle fontHandle,
                               float fontSize,
                               float letterSpacing,
                               ShapedText& outShapedText);
    
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
    void renderGlyph(FontHandle fontHandle,
                     uint32_t glyphIndex,
                     float fontSize,
                     const void*& outBitmapData,
                     Vec2& outSize,
                     Vec2& outBearing,
                     float& outAdvance);
    
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
    void rasterizeGlyphWithFreeType(void* face,
                                     uint32_t glyphIndex,
                                     float fontSize,
                                     const void*& outBitmapData,
                                     Vec2& outSize,
                                     Vec2& outBearing,
                                     float& outAdvance);
    
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
