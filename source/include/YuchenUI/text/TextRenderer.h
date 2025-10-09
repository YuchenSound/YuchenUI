#pragma once

#include "YuchenUI/core/Types.h"
#include "YuchenUI/text/GlyphCache.h"
#include <hb.h>
#include <vector>
#include <unordered_map>

namespace YuchenUI {

class GraphicsContext;
class FontManager;

struct TextCacheKey {
    uint64_t hash;
    
    TextCacheKey(const char* text, FontHandle westernFont, FontHandle chineseFont, float fontSize);
    bool operator==(const TextCacheKey& other) const { return hash == other.hash; }
};

struct TextCacheKeyHash {
    size_t operator()(const TextCacheKey& key) const { return key.hash; }
};

class TextRenderer {
public:
    explicit TextRenderer(GraphicsContext* context);
    ~TextRenderer();
    
    bool initialize(float dpiScale = 1.0f);
    void destroy();
    bool isInitialized() const;
    
    void beginFrame();
    void shapeText(const char* text, FontHandle westernFont, FontHandle chineseFont, float fontSize, ShapedText& outShapedText);
    void generateTextVertices(const ShapedText& shapedText, const Vec2& basePosition, const Vec4& color, FontHandle fontHandle, float fontSize, std::vector<TextVertex>& outVertices);
    
    void* getCurrentAtlasTexture() const;
    float getDPIScale() const;
    
private:
    bool initializeResources();
    void cleanupResources();
    bool shapeTextWithHarfBuzz(const char* text, FontHandle fontHandle, float fontSize, ShapedText& outShapedText);
    void renderGlyph(FontHandle fontHandle, uint32_t glyphIndex, float fontSize, const void*& outBitmapData, Vec2& outSize, Vec2& outBearing, float& outAdvance);
    void rasterizeGlyphWithFreeType(void* face, uint32_t glyphIndex, float fontSize, const void*& outBitmapData, Vec2& outSize, Vec2& outBearing, float& outAdvance);
    
    GraphicsContext* m_context;
    std::unique_ptr<GlyphCache> m_glyphCache;
    bool m_isInitialized;
    float m_dpiScale;
    hb_buffer_t* m_harfBuzzBuffer;
    
    std::unordered_map<TextCacheKey, ShapedText, TextCacheKeyHash> m_shapedTextCache;
};

}
