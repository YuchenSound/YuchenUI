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
/** @file GlyphCache.cpp
    
    Implementation notes:
    - Atlas size scales with DPI: BASE_ATLAS_SIZE * dpiScale
    - Row-based packing: fills current row left-to-right, advances to new row when full
    - Padding added around each glyph to prevent texture bleeding
    - Empty glyphs (zero-size bitmaps) stored as metadata only
    - Frame-based expiration: glyphs unused for GLYPH_EXPIRE_FRAMES removed
    - Cleanup runs every CLEANUP_INTERVAL_FRAMES
    - No atlas defragmentation - relies on periodic cleanup and atlas creation
    - R8 texture format (single-channel grayscale) for alpha mask rendering
*/

#include "YuchenUI/text/GlyphCache.h"
#include "YuchenUI/rendering/IGraphicsBackend.h"
#include "YuchenUI/core/Validation.h"
#include "YuchenUI/core/Config.h"
#include <algorithm>

namespace YuchenUI {

//==========================================================================================
// Lifecycle

GlyphCache::GlyphCache(IGraphicsBackend* backend, float dpiScale)
    : m_backend(backend)
    , m_isInitialized(false)
    , m_dpiScale(dpiScale)
    , m_currentAtlasIndex(0)
    , m_currentFrame(0)
{
    YUCHEN_ASSERT_MSG(backend != nullptr, "IGraphicsBackend cannot be null");
    if (dpiScale <= 0.0f) m_dpiScale = 1.0f;
}

GlyphCache::~GlyphCache()
{
    destroy();
}

bool GlyphCache::initialize()
{
    if (m_isInitialized) return false;
    
    // Create first atlas
    createNewAtlas();
    if (m_atlases.empty()) return false;
    
    m_isInitialized = true;
    return true;
}

void GlyphCache::destroy()
{
    if (!m_isInitialized) return;
    
    // Clear all glyph metadata
    clearAllGlyphs();
    
    // Destroy all atlas textures
    for (auto& atlas : m_atlases)
    {
        if (atlas->textureHandle)
        {
            m_backend->destroyTexture(atlas->textureHandle);
            atlas->textureHandle = nullptr;
        }
    }
    
    m_atlases.clear();
    m_currentAtlasIndex = 0;
    m_isInitialized = false;
}

//==========================================================================================
// Atlas Size Calculation

uint32_t GlyphCache::getAtlasWidth() const
{
    return static_cast<uint32_t>(Config::GlyphCache::BASE_ATLAS_WIDTH * m_dpiScale + 0.5f);
}

uint32_t GlyphCache::getAtlasHeight() const
{
    return static_cast<uint32_t>(Config::GlyphCache::BASE_ATLAS_HEIGHT * m_dpiScale + 0.5f);
}

Vec2 GlyphCache::getCurrentAtlasSize() const
{
    if (!m_isInitialized)  return Vec2();
    if (m_atlases.empty()) return Vec2();
    
    const auto& atlas = m_atlases[m_currentAtlasIndex];
    return Vec2(static_cast<float>(atlas->width), static_cast<float>(atlas->height));
}

//==========================================================================================
// Atlas Management

void GlyphCache::createNewAtlas()
{
    // Check atlas limit
    if (m_atlases.size() >= Config::GlyphCache::MAX_ATLASES) return;
    
    uint32_t atlasWidth = getAtlasWidth();
    uint32_t atlasHeight = getAtlasHeight();
    
    // Create atlas structure
    auto atlas = std::make_unique<GlyphAtlas>(atlasWidth, atlasHeight);
    
    // Create GPU texture (R8 format for grayscale alpha mask)
    atlas->textureHandle = m_backend->createTexture2D(
        atlasWidth,
        atlasHeight,
        TextureFormat::R8_Unorm
    );
    
    YUCHEN_ASSERT_MSG(atlas->textureHandle != nullptr, "Failed to create atlas texture");
    
    m_atlases.push_back(std::move(atlas));
    
    // Set as current atlas if first one
    if (m_atlases.size() == 1) m_currentAtlasIndex = 0;
}

GlyphAtlas* GlyphCache::findAtlasWithSpace(uint32_t width, uint32_t height)
{
    // Calculate required space including padding
    uint32_t requiredWidth = width + Config::GlyphCache::GLYPH_PADDING * 2;
    uint32_t requiredHeight = height + Config::GlyphCache::GLYPH_PADDING * 2;
    
    // Search all atlases for space
    for (auto& atlas : m_atlases)
    {
        if (atlas->isFull) continue;
        
        // Check if glyph fits in current row
        bool canFitInCurrentRow = (atlas->currentX + requiredWidth <= atlas->width) &&
                                  (atlas->currentY + requiredHeight <= atlas->height);
        
        // Check if glyph fits in new row
        bool canFitInNewRow = (atlas->currentY + atlas->rowHeight + requiredHeight <= atlas->height) &&
                             (requiredWidth <= atlas->width);
        
        if (canFitInCurrentRow || canFitInNewRow)
        {
            return atlas.get();
        }
        else
        {
            // Mark atlas as full if glyph cannot fit
            atlas->isFull = true;
        }
    }
    
    return nullptr;
}

void GlyphCache::addGlyphToAtlas(GlyphAtlas* atlas, uint32_t width, uint32_t height, Rect& outRect) {
    if (!atlas || atlas->isFull) return;
    
    // Calculate required space including padding
    uint32_t requiredWidth = width + Config::GlyphCache::GLYPH_PADDING * 2;
    uint32_t requiredHeight = height + Config::GlyphCache::GLYPH_PADDING * 2;
    
    // Check if glyph fits in current row
    bool canFitInCurrentRow = (atlas->currentX + requiredWidth <= atlas->width) &&
                             (atlas->currentY + requiredHeight <= atlas->height);
    
    if (!canFitInCurrentRow)
    {
        // Advance to new row
        atlas->currentX = 0;
        atlas->currentY += atlas->rowHeight;
        atlas->rowHeight = 0;
    }
    
    // Set output rectangle (excluding padding)
    outRect.x = static_cast<float>(atlas->currentX + Config::GlyphCache::GLYPH_PADDING);
    outRect.y = static_cast<float>(atlas->currentY + Config::GlyphCache::GLYPH_PADDING);
    outRect.width = static_cast<float>(width);
    outRect.height = static_cast<float>(height);
    
    // Update packing state
    atlas->currentX += requiredWidth;
    atlas->rowHeight = std::max(atlas->rowHeight, requiredHeight);
}

void GlyphCache::uploadGlyphBitmap(GlyphAtlas* atlas, const Rect& rect, const void* bitmapData)
{
    if (!rect.isValid()) return;
    
    YUCHEN_ASSERT(atlas != nullptr);
    YUCHEN_ASSERT(atlas->textureHandle != nullptr);
    
    // Upload glyph bitmap to GPU texture
    m_backend->updateTexture2D(
        atlas->textureHandle,
        static_cast<uint32_t>(rect.x),
        static_cast<uint32_t>(rect.y),
        static_cast<uint32_t>(rect.width),
        static_cast<uint32_t>(rect.height),
        bitmapData,
        static_cast<size_t>(rect.width)  // Bytes per row (R8 = 1 byte per pixel)
    );
}

//==========================================================================================
// Glyph Caching

const GlyphCacheEntry* GlyphCache::getGlyph(const GlyphKey& key)
{
    if (!m_isInitialized) return nullptr;
    
    // Lookup in cache
    auto it = m_glyphCache.find(key);
    if (it != m_glyphCache.end())
    {
        // Mark as used in current frame for LRU tracking
        it->second.markUsed(m_currentFrame);
        return &it->second;
    }
    
    return nullptr;
}

void GlyphCache::cacheGlyph(const GlyphKey& key, const void* bitmapData, const Vec2& size,
                           const Vec2& bearing, float advance)
{
    if (!m_isInitialized) return;
    if (!size.isValid() || !bearing.isValid()) return;
    if (advance < 0.0f) return;
    
    // Check if glyph is empty (zero-size bitmap)
    bool isEmptyGlyph = (bitmapData == nullptr) || (size.x <= 0.0f) || (size.y <= 0.0f);
    
    if (isEmptyGlyph)
    {
        // Cache metadata only for empty glyphs (e.g., space character)
        GlyphCacheEntry entry;
        entry.textureRect = Rect(0, 0, 0, 0);
        entry.bearing = bearing;
        entry.advance = advance;
        entry.lastUsedFrame = m_currentFrame;
        entry.isValid = true;
        
        m_glyphCache[key] = entry;
        return;
    }
    
    // Convert size to integer dimensions
    uint32_t width = static_cast<uint32_t>(size.x + 0.5f);
    uint32_t height = static_cast<uint32_t>(size.y + 0.5f);
    
    // Validate glyph fits in atlas
    uint32_t atlasWidth = getAtlasWidth();
    uint32_t atlasHeight = getAtlasHeight();
    
    if (width > atlasWidth - Config::GlyphCache::GLYPH_PADDING * 2) return;
    if (height > atlasHeight - Config::GlyphCache::GLYPH_PADDING * 2) return;
    
    // Find atlas with space
    GlyphAtlas* atlas = findAtlasWithSpace(width, height);
    
    if (!atlas)
    {
        // Create new atlas if under limit
        if (m_atlases.size() < Config::GlyphCache::MAX_ATLASES)
        {
            createNewAtlas();
            atlas = m_atlases.back().get();
        }
        else
        {
            // Atlas limit reached - cleanup and retry
            cleanupExpiredGlyphs();
            atlas = findAtlasWithSpace(width, height);
            if (!atlas) return;  // Still no space after cleanup
        }
    }
    
    // Allocate space in atlas
    Rect textureRect;
    addGlyphToAtlas(atlas, width, height, textureRect);
    
    // Upload bitmap to GPU
    uploadGlyphBitmap(atlas, textureRect, bitmapData);
    
    // Create cache entry
    GlyphCacheEntry entry;
    entry.textureRect = textureRect;
    entry.bearing = bearing;
    entry.advance = advance;
    entry.lastUsedFrame = m_currentFrame;
    entry.isValid = true;
    
    m_glyphCache[key] = entry;
}

//==========================================================================================
// Frame Management

void GlyphCache::beginFrame()
{
    if (!m_isInitialized) return;
    
    m_currentFrame++;
    
    // Run cleanup periodically
    if (m_currentFrame % Config::GlyphCache::CLEANUP_INTERVAL_FRAMES == 0)
        cleanupExpiredGlyphs();
}

void GlyphCache::cleanupExpiredGlyphs()
{
    // Collect keys of expired glyphs
    std::vector<GlyphKey> keysToRemove;
    keysToRemove.reserve(m_glyphCache.size() / 4);
    
    for (const auto& pair : m_glyphCache) {
        if (pair.second.isExpired(m_currentFrame, Config::GlyphCache::GLYPH_EXPIRE_FRAMES))
        {
            keysToRemove.push_back(pair.first);
        }
    }
    // Remove expired glyphs
    for (const auto& key : keysToRemove)
    {
        removeGlyph(key);
    }
}

void GlyphCache::clearAllGlyphs()
{
    // Clear cache entries
    m_glyphCache.clear();
    
    // Reset all atlases to empty state (keeps textures allocated)
    for (auto& atlas : m_atlases)
    {
        atlas->reset();
    }
}

void GlyphCache::removeGlyph(const GlyphKey& key)
{
    auto it = m_glyphCache.find(key);
    if (it != m_glyphCache.end()) m_glyphCache.erase(it);
}

//==========================================================================================
// Atlas Access

void* GlyphCache::getCurrentAtlasTexture() const
{
    if (m_atlases.empty() || m_currentAtlasIndex >= m_atlases.size()) return nullptr;
    return m_atlases[m_currentAtlasIndex]->textureHandle;
}

} // namespace YuchenUI
