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
/** @file GlyphCache.h
    
    GPU texture atlas cache for rasterized glyphs.
    
    Manages dynamic texture atlases for glyph bitmap storage. Rasterized glyphs packed
    into GPU textures using simple row-based packing algorithm. Supports multiple atlases
    when single atlas fills up. Implements frame-based LRU expiration for unused glyphs.
    
    Packing algorithm:
    - Row-based left-to-right packing with configurable padding
    - Advances to new row when current row full
    - Creates new atlas when current atlas full (up to MAX_ATLASES)
    - No defragmentation - relies on periodic cleanup
    
    Lifecycle:
    1. Cache glyphs on demand during text rendering
    2. Mark glyphs used each frame via getGlyph()
    3. Expire unused glyphs after GLYPH_EXPIRE_FRAMES
    4. Cleanup runs every CLEANUP_INTERVAL_FRAMES
    
    Atlas size scales with DPI: BASE_ATLAS_SIZE * dpiScale
*/

#pragma once

#include "YuchenUI/core/Types.h"
#include "YuchenUI/core/Config.h"
#include <vector>
#include <unordered_map>
#include <memory>

namespace YuchenUI {

class IGraphicsBackend;

//==========================================================================================
/**
    GPU texture atlas cache for glyphs.
    
    GlyphCache manages dynamic GPU texture atlases for storing rasterized glyph bitmaps.
    Uses simple row-based packing and frame-based expiration. Each atlas is R8 grayscale
    texture used as alpha mask for text rendering.
    
    Key features:
    - Dynamic atlas creation up to MAX_ATLASES limit
    - Row-based packing with configurable padding
    - Frame-based LRU expiration
    - Periodic cleanup of expired glyphs
    - DPI-aware atlas sizing
    
    Cache key: (FontHandle, GlyphIndex, FontSize * 64)
    Expiration: Unused glyphs removed after GLYPH_EXPIRE_FRAMES
    Cleanup: Runs every CLEANUP_INTERVAL_FRAMES
    
    @see GlyphKey, GlyphCacheEntry, TextRenderer
*/
class GlyphCache {
public:
    //======================================================================================
    /** Creates glyph cache with graphics backend and DPI scale.
        
        @param backend   Graphics backend for texture operations (not owned)
        @param dpiScale  DPI scale factor for atlas sizing
    */
    GlyphCache(IGraphicsBackend* backend, float dpiScale);
    
    /** Destructor. Destroys all atlas textures. */
    ~GlyphCache();
    
    //======================================================================================
    /** Initializes glyph cache and creates first atlas.
        
        @returns True if first atlas created successfully
    */
    bool initialize();
    
    /** Destroys all atlas textures and clears cache. */
    void destroy();
    
    //======================================================================================
    /** Retrieves cached glyph entry.
        
        Marks glyph as used in current frame for LRU tracking.
        
        @param key  Glyph cache key
        @returns Pointer to cache entry, or nullptr if not cached
    */
    const GlyphCacheEntry* getGlyph(const GlyphKey& key);
    
    /** Caches rasterized glyph in atlas.
        
        Allocates space in atlas, uploads bitmap to GPU, and stores cache entry.
        Handles empty glyphs (zero-size bitmaps) by storing metadata only.
        Creates new atlas if current atlas full.
        
        @param key          Glyph cache key
        @param bitmapData   Glyph bitmap buffer (R8 format), or nullptr for empty glyph
        @param size         Bitmap dimensions in pixels
        @param bearing      Glyph bearing (offset from baseline)
        @param advance      Horizontal advance for next glyph
    */
    void cacheGlyph(const GlyphKey& key, const void* bitmapData, const Vec2& size, const Vec2& bearing, float advance);
    
    /** Advances frame counter and triggers periodic cleanup.
        
        Call at start of each frame before text rendering. Runs cleanup every
        CLEANUP_INTERVAL_FRAMES to remove expired glyphs.
    */
    void beginFrame();
    
    //======================================================================================
    /** Returns dimensions of current atlas texture.
        
        @returns Atlas size in pixels (width, height)
    */
    Vec2 getCurrentAtlasSize() const;
    
    /** Returns opaque handle to current atlas texture.
        
        @returns GPU texture handle for current atlas
    */
    void* getCurrentAtlasTexture() const;
    
private:
    //======================================================================================
    /** Returns scaled atlas width based on DPI.
        
        @returns BASE_ATLAS_WIDTH * dpiScale
    */
    uint32_t getAtlasWidth() const;
    
    /** Returns scaled atlas height based on DPI.
        
        @returns BASE_ATLAS_HEIGHT * dpiScale
    */
    uint32_t getAtlasHeight() const;
    
    /** Creates new atlas texture.
        
        Creates R8 texture at scaled dimensions. Marks first atlas as current.
        Fails silently if MAX_ATLASES limit reached.
    */
    void createNewAtlas();
    
    /** Finds atlas with space for glyph of specified size.
        
        Checks current row and potential new row for space. Marks atlas full
        if glyph cannot fit.
        
        @param width   Glyph width in pixels
        @param height  Glyph height in pixels
        @returns Pointer to atlas with space, or nullptr if none available
    */
    GlyphAtlas* findAtlasWithSpace(uint32_t width, uint32_t height);
    
    /** Allocates space in atlas for glyph.
        
        Updates atlas packing state (currentX, currentY, rowHeight). Advances
        to new row if glyph doesn't fit in current row.
        
        @param atlas     Atlas to allocate from
        @param width     Glyph width in pixels (excluding padding)
        @param height    Glyph height in pixels (excluding padding)
        @param outRect   Output rectangle with padding applied
    */
    void addGlyphToAtlas(GlyphAtlas* atlas, uint32_t width, uint32_t height, Rect& outRect);
    
    /** Uploads glyph bitmap to atlas texture.
        
        Uses graphics backend to update texture region.
        
        @param atlas        Target atlas
        @param rect         Texture region to update
        @param bitmapData   Glyph bitmap data (R8 format)
    */
    void uploadGlyphBitmap(GlyphAtlas* atlas, const Rect& rect, const void* bitmapData);
    
    /** Removes glyphs unused for GLYPH_EXPIRE_FRAMES.
        
        Iterates cache and removes expired entries. Does not defragment atlases.
    */
    void cleanupExpiredGlyphs();
    
    /** Clears all cached glyphs and resets all atlases.
        
        Keeps atlas textures allocated but resets packing state.
    */
    void clearAllGlyphs();
    
    /** Removes specific glyph from cache.
        
        @param key  Glyph cache key
    */
    void removeGlyph(const GlyphKey& key);
    
    //======================================================================================
    IGraphicsBackend* m_backend;                                              ///< Graphics backend (not owned)
    bool m_isInitialized;                                                     ///< Initialization state
    float m_dpiScale;                                                         ///< DPI scale factor
    std::vector<std::unique_ptr<GlyphAtlas>> m_atlases;                       ///< Atlas textures
    size_t m_currentAtlasIndex;                                               ///< Current atlas for rendering
    std::unordered_map<GlyphKey, GlyphCacheEntry, GlyphKeyHash> m_glyphCache; ///< Glyph cache entries
    uint32_t m_currentFrame;                                                  ///< Frame counter for LRU
};

} // namespace YuchenUI
