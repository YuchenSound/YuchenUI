#include "YuchenUI/text/TextRenderer.h"
#include "YuchenUI/text/FontManager.h"
#include "YuchenUI/text/TextUtils.h"
#include "YuchenUI/core/Validation.h"
#include "YuchenUI/core/Config.h"
#include <stdexcept>
#include <functional>

namespace YuchenUI {

TextCacheKey::TextCacheKey(const char* text, FontHandle westernFont, FontHandle chineseFont, float fontSize)
{
    std::string textStr(text);
    std::hash<std::string> stringHasher;
    std::hash<FontHandle> fontHasher;
    std::hash<float> floatHasher;
    
    hash = stringHasher(textStr) ^
           (fontHasher(westernFont) << 8) ^
           (fontHasher(chineseFont) << 16) ^
           (floatHasher(fontSize) << 24);
}

TextRenderer::TextRenderer(IGraphicsBackend* backend)
    : m_backend(backend)
    , m_glyphCache(nullptr)
    , m_isInitialized(false)
    , m_dpiScale(1.0f)
    , m_harfBuzzBuffer(nullptr)
    , m_shapedTextCache()
{
    YUCHEN_ASSERT_MSG(backend != nullptr, "IGraphicsBackend cannot be null");
}

TextRenderer::~TextRenderer()
{
    destroy();
}

bool TextRenderer::initialize(float dpiScale)
{
    YUCHEN_ASSERT_MSG(!m_isInitialized, "Already initialized");
    YUCHEN_ASSERT_MSG(dpiScale > 0.0f, "DPI scale must be positive");
    YUCHEN_ASSERT_MSG(m_backend != nullptr, "GraphicsContext is null");
    
    m_dpiScale = dpiScale;
    
    YUCHEN_ASSERT_MSG(FontManager::getInstance().isInitialized(), "FontManager not initialized");
    
    m_glyphCache = std::make_unique<GlyphCache>(m_backend, m_dpiScale);
    YUCHEN_ASSERT_MSG(m_glyphCache->initialize(), "GlyphCache initialization failed");
    
    if (!initializeResources()) return false;
    
    m_isInitialized = true;
    return true;
}

void TextRenderer::destroy()
{
    if (!m_isInitialized) return;
    
    cleanupResources();
    
    if (m_glyphCache)
    {
        m_glyphCache->destroy();
        m_glyphCache.reset();
    }
    
    m_shapedTextCache.clear();
    m_isInitialized = false;
}

bool TextRenderer::initializeResources()
{
    m_harfBuzzBuffer = hb_buffer_create();
    return m_harfBuzzBuffer != nullptr;
}

void TextRenderer::cleanupResources()
{
    if (m_harfBuzzBuffer)
    {
        hb_buffer_destroy(m_harfBuzzBuffer);
        m_harfBuzzBuffer = nullptr;
    }
}

void TextRenderer::beginFrame()
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "Not initialized");
    
    if (m_glyphCache) m_glyphCache->beginFrame();
}

void TextRenderer::shapeText(const char* text, FontHandle westernFont, FontHandle chineseFont, float fontSize, ShapedText& outShapedText)
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "Not initialized");
    YUCHEN_ASSERT_MSG(text != nullptr, "Text cannot be null");
    YUCHEN_ASSERT_MSG(fontSize >= Config::Font::MIN_SIZE && fontSize <= Config::Font::MAX_SIZE,"Font size out of range");
    
    outShapedText.clear();
    
    size_t textLength = strlen(text);
    if (textLength == 0 || textLength > Config::Text::MAX_LENGTH) return;
    
    TextCacheKey cacheKey(text, westernFont, chineseFont, fontSize);
    auto it = m_shapedTextCache.find(cacheKey);
    if (it != m_shapedTextCache.end())
    {
        outShapedText = it->second;
        return;
    }
    
    std::vector<TextSegment> segments = TextUtils::segmentText(text, westernFont, chineseFont);
    if (segments.empty()) return;
    
    float totalAdvance = 0.0f;
    float maxHeight = fontSize;
    
    for (const auto& segment : segments)
    {
        ShapedText segmentShaped;
        if (shapeTextWithHarfBuzz(segment.text.c_str(), segment.fontHandle, fontSize, segmentShaped))
        {
            for (auto& glyph : segmentShaped.glyphs)
            {
                glyph.position.x += totalAdvance;
                outShapedText.glyphs.push_back(glyph);
            }
            totalAdvance += segmentShaped.totalAdvance;
            maxHeight = std::max(maxHeight, segmentShaped.totalSize.y);
        }
    }
    
    outShapedText.totalAdvance = totalAdvance;
    outShapedText.totalSize = Vec2(totalAdvance, maxHeight);
    
    m_shapedTextCache[cacheKey] = outShapedText;
}

bool TextRenderer::shapeTextWithHarfBuzz(const char* text, FontHandle fontHandle, float fontSize, ShapedText& outShapedText)
{
    YUCHEN_ASSERT_MSG(text != nullptr, "Text cannot be null");
    YUCHEN_ASSERT_MSG(fontSize >= Config::Font::MIN_SIZE && fontSize <= Config::Font::MAX_SIZE,"Font size out of range");
    
    float scaledFontSize = fontSize * m_dpiScale;
    hb_font_t* hbFont = static_cast<hb_font_t*>(FontManager::getInstance().getHarfBuzzFont(fontHandle, scaledFontSize, 1.0f));
    YUCHEN_ASSERT_MSG(hbFont != nullptr, "Failed to get HarfBuzz font");
    
    hb_buffer_clear_contents(m_harfBuzzBuffer);
    hb_buffer_add_utf8(m_harfBuzzBuffer, text, -1, 0, -1);
    
    hb_script_t dominantScript = TextUtils::detectTextScript(text);
    const char* languageStr = TextUtils::getLanguageForScript(dominantScript);
    hb_language_t language = hb_language_from_string(languageStr, -1);
    
    hb_buffer_set_direction(m_harfBuzzBuffer, HB_DIRECTION_LTR);
    hb_buffer_set_script(m_harfBuzzBuffer, dominantScript);
    hb_buffer_set_language(m_harfBuzzBuffer, language);
    
    hb_feature_t features[1];
    features[0].tag = HB_TAG('k','e','r','n');
    features[0].value = 0;
    features[0].start = 0;
    features[0].end = (unsigned int)-1;
    
    hb_shape(hbFont, m_harfBuzzBuffer, features, 1);
    
    unsigned int glyphCount = 0;
    hb_glyph_info_t* glyphInfos = hb_buffer_get_glyph_infos(m_harfBuzzBuffer, &glyphCount);
    hb_glyph_position_t* glyphPositions = hb_buffer_get_glyph_positions(m_harfBuzzBuffer, &glyphCount);
    
    YUCHEN_ASSERT_MSG(glyphInfos != nullptr && glyphPositions != nullptr,"Failed to get glyph info/positions");
    YUCHEN_ASSERT_MSG(glyphCount > 0 && glyphCount <= Config::Text::MAX_GLYPHS_PER_TEXT,"Invalid glyph count");
    
    outShapedText.glyphs.reserve(glyphCount);
    
    float penX = 0.0f;
    float penY = 0.0f;
    
    for (unsigned int i = 0; i < glyphCount; ++i)
    {
        ShapedGlyph glyph;
        glyph.glyphIndex = glyphInfos[i].codepoint;
        glyph.cluster = glyphInfos[i].cluster;
        glyph.fontHandle = fontHandle;
        
        float xOffset = glyphPositions[i].x_offset / 64.0f;
        float yOffset = glyphPositions[i].y_offset / 64.0f;
        float xAdvance = glyphPositions[i].x_advance / 64.0f;
        float yAdvance = glyphPositions[i].y_advance / 64.0f;
        
        glyph.position = Vec2((penX + xOffset) / m_dpiScale, (penY + yOffset) / m_dpiScale);
        glyph.advance = xAdvance / m_dpiScale;
        
        outShapedText.glyphs.push_back(glyph);
        
        penX += xAdvance;
        penY += yAdvance;
    }
    
    outShapedText.totalAdvance = penX / m_dpiScale;
    outShapedText.totalSize = Vec2(penX / m_dpiScale, fontSize);
    
    return true;
}

void TextRenderer::generateTextVertices(const ShapedText& shapedText, const Vec2& basePosition,
                                       const Vec4& color, FontHandle fontHandle, float fontSize,
                                       std::vector<TextVertex>& outVertices)
{
    outVertices.clear();
    outVertices.reserve(shapedText.glyphs.size() * 4);
    
    Vec2 atlasSize = m_glyphCache->getCurrentAtlasSize();
    
    for (const auto& glyph : shapedText.glyphs)
    {
        if (glyph.glyphIndex == 0) continue;
        
        FontHandle actualFontHandle = glyph.fontHandle;
        
        float scaledFontSize = fontSize * m_dpiScale;
        GlyphKey key(actualFontHandle, glyph.glyphIndex, scaledFontSize);
        const GlyphCacheEntry* entry = m_glyphCache->getGlyph(key);
        
        if (!entry)
        {
            const void* bitmapData = nullptr;
            Vec2 glyphSize, bearing;
            float advance;
            
            renderGlyph(actualFontHandle, glyph.glyphIndex, scaledFontSize, bitmapData, glyphSize, bearing, advance);
            
            m_glyphCache->cacheGlyph(key, bitmapData, glyphSize, bearing, advance);
            entry = m_glyphCache->getGlyph(key);
        }
        
        if (!entry || entry->textureRect.width <= 0.0f || entry->textureRect.height <= 0.0f) continue;
        
        Vec2 glyphPos = Vec2(
            basePosition.x + glyph.position.x + (entry->bearing.x / m_dpiScale),
            basePosition.y + glyph.position.y - (entry->bearing.y / m_dpiScale)
        );
        
        float glyphWidth = entry->textureRect.width / m_dpiScale;
        float glyphHeight = entry->textureRect.height / m_dpiScale;
        
        Vec2 texCoordMin = Vec2(entry->textureRect.x / atlasSize.x, entry->textureRect.y / atlasSize.y);
        Vec2 texCoordMax = Vec2(
            (entry->textureRect.x + entry->textureRect.width) / atlasSize.x,
            (entry->textureRect.y + entry->textureRect.height) / atlasSize.y
        );
        
        TextVertex topLeft(Vec2(glyphPos.x, glyphPos.y), Vec2(texCoordMin.x, texCoordMin.y), color);
        TextVertex topRight(Vec2(glyphPos.x + glyphWidth, glyphPos.y), Vec2(texCoordMax.x, texCoordMin.y), color);
        TextVertex bottomLeft(Vec2(glyphPos.x, glyphPos.y + glyphHeight), Vec2(texCoordMin.x, texCoordMax.y), color);
        TextVertex bottomRight(Vec2(glyphPos.x + glyphWidth, glyphPos.y + glyphHeight), Vec2(texCoordMax.x, texCoordMax.y), color);
        
        outVertices.push_back(topLeft);
        outVertices.push_back(topRight);
        outVertices.push_back(bottomLeft);
        outVertices.push_back(bottomRight);
    }
}

void TextRenderer::renderGlyph(FontHandle fontHandle, uint32_t glyphIndex, float scaledFontSize,
                              const void*& outBitmapData, Vec2& outSize, Vec2& outBearing, float& outAdvance)
{
    void* face = FontManager::getInstance().getFontFace(fontHandle);
    rasterizeGlyphWithFreeType(face, glyphIndex, scaledFontSize, outBitmapData, outSize, outBearing, outAdvance);
}

void TextRenderer::rasterizeGlyphWithFreeType(void* face, uint32_t glyphIndex, float scaledFontSize,
                                             const void*& outBitmapData, Vec2& outSize, Vec2& outBearing, float& outAdvance)
{
    YUCHEN_ASSERT_MSG(face != nullptr, "Face cannot be null");
    
    FT_Face ftFace = static_cast<FT_Face>(face);
    
    FT_Error error = FT_Set_Char_Size(ftFace, 0, (FT_F26Dot6)(scaledFontSize * 64), Config::Font::FREETYPE_DPI, Config::Font::FREETYPE_DPI);
    if (error != FT_Err_Ok) throw std::runtime_error("Failed to set char size");
    
    error = FT_Load_Glyph(ftFace, glyphIndex, FT_LOAD_RENDER);
    if (error != FT_Err_Ok) throw std::runtime_error("Failed to load glyph");
    
    FT_GlyphSlot slot = ftFace->glyph;
    if (slot->format != FT_GLYPH_FORMAT_BITMAP) throw std::runtime_error("Glyph not in bitmap format");
    
    outBitmapData = slot->bitmap.buffer;
    outSize = Vec2(slot->bitmap.width, slot->bitmap.rows);
    outBearing = Vec2(slot->bitmap_left, slot->bitmap_top);
    outAdvance = slot->advance.x / 64.0f;
}

bool TextRenderer::isInitialized() const
{
    return m_isInitialized;
}

float TextRenderer::getDPIScale() const
{
    return m_dpiScale;
}

void* TextRenderer::getCurrentAtlasTexture() const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "Not initialized");
    
    if (!m_glyphCache) return nullptr;
    return m_glyphCache->getCurrentAtlasTexture();
}

}
