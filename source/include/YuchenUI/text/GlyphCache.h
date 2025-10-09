#pragma once

#include "YuchenUI/core/Types.h"
#include "YuchenUI/core/Config.h"
#include <vector>
#include <unordered_map>
#include <memory>

namespace YuchenUI {

class GraphicsContext;

class GlyphCache {
public:
    GlyphCache(GraphicsContext* context, float dpiScale);
    ~GlyphCache();
    
    bool initialize();
    void destroy();
    
    const GlyphCacheEntry* getGlyph(const GlyphKey& key);
    void cacheGlyph(const GlyphKey& key, const void* bitmapData, const Vec2& size, const Vec2& bearing, float advance);
    void beginFrame();
    
    Vec2 getCurrentAtlasSize() const;
    void* getCurrentAtlasTexture() const;
    
private:
    GraphicsContext* m_context;
    uint32_t getAtlasWidth() const;
    uint32_t getAtlasHeight() const;
    void createNewAtlas();
    GlyphAtlas* findAtlasWithSpace(uint32_t width, uint32_t height);
    void addGlyphToAtlas(GlyphAtlas* atlas, uint32_t width, uint32_t height, Rect& outRect);
    void uploadGlyphBitmap(GlyphAtlas* atlas, const Rect& rect, const void* bitmapData);
    void cleanupExpiredGlyphs();
    void clearAllGlyphs();
    void removeGlyph(const GlyphKey& key);
    
    bool m_isInitialized;
    float m_dpiScale;
    std::vector<std::unique_ptr<GlyphAtlas>> m_atlases;
    size_t m_currentAtlasIndex;
    std::unordered_map<GlyphKey, GlyphCacheEntry, GlyphKeyHash> m_glyphCache;
    uint32_t m_currentFrame;
};

}
