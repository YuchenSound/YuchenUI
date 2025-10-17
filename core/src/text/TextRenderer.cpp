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
/** @file TextRenderer.cpp
    
    Implementation notes:
    - Text cache key combines text, fonts, and size into 64-bit hash
    - Text segmented by font fallback chain for appropriate font selection
    - HarfBuzz buffer reused across shaping calls for efficiency
    - Kerning disabled via HarfBuzz features (kern=0)
    - Glyph positions scaled by 1/64 (HarfBuzz uses 26.6 fixed-point)
    - DPI scaling applied to font size for glyph rasterization
    - Vertex generation creates quads with texture coordinates
    - All vertices reference current atlas texture
    
    Version 2.0 Changes:
    - Added TextCacheKey constructors for fallback chain and legacy APIs
    - Implemented shapeText() with FontFallbackChain support
    - Enhanced text segmentation using TextUtils::segmentTextWithFallback()
    - Improved cache key generation for better hit rates
    - Better support for emoji and symbol rendering
*/

#include "YuchenUI/text/TextRenderer.h"
#include "YuchenUI/text/IFontProvider.h"
#include "YuchenUI/text/TextUtils.h"
#include "YuchenUI/core/Validation.h"
#include "YuchenUI/core/Config.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdexcept>
#include <functional>

namespace YuchenUI {

//==========================================================================================
// TextCacheKey Implementation

TextCacheKey::TextCacheKey(const char* text, const FontFallbackChain& fallbackChain,
                           float fontSize, float letterSpacing)
{
    std::string textStr(text);
    std::hash<std::string> stringHasher;
    std::hash<float> floatHasher;
    std::hash<int> intHasher;
    
    // Start with text hash
    hash = stringHasher(textStr);
    
    // XOR with each font in fallback chain
    for (size_t i = 0; i < fallbackChain.fonts.size(); ++i)
    {
        std::hash<FontHandle> fontHasher;
        hash ^= (fontHasher(fallbackChain.fonts[i]) << (8 + i * 8));
    }
    
    // XOR with font size
    hash ^= (floatHasher(fontSize) << 24);
    
    int quantizedSpacing = static_cast<int>(letterSpacing / 10.0f) * 10;
    hash ^= (intHasher(quantizedSpacing) << 40);
}

//==========================================================================================
// Lifecycle

TextRenderer::TextRenderer(IGraphicsBackend* backend, IFontProvider* fontProvider)
    : m_backend(backend)
    , m_fontProvider(fontProvider)
    , m_glyphCache(nullptr)
    , m_isInitialized(false)
    , m_dpiScale(1.0f)
    , m_harfBuzzBuffer(nullptr)
    , m_shapedTextCache()
{
    YUCHEN_ASSERT_MSG(backend != nullptr, "IGraphicsBackend cannot be null");
    YUCHEN_ASSERT_MSG(fontProvider != nullptr, "IFontProvider cannot be null");
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
    YUCHEN_ASSERT_MSG(m_fontProvider != nullptr, "Font provider is null");
    
    m_dpiScale = dpiScale;

    // Create glyph cache with DPI scaling
    m_glyphCache = std::make_unique<GlyphCache>(m_backend, m_dpiScale);
    YUCHEN_ASSERT_MSG(m_glyphCache->initialize(), "GlyphCache initialization failed");
    
    // Create HarfBuzz buffer
    if (!initializeResources()) return false;
    
    m_isInitialized = true;
    return true;
}

void TextRenderer::destroy()
{
    if (!m_isInitialized) return;
    
    // Cleanup HarfBuzz buffer
    cleanupResources();
    
    // Destroy glyph cache
    if (m_glyphCache)
    {
        m_glyphCache->destroy();
        m_glyphCache.reset();
    }
    
    // Clear shaped text cache
    m_shapedTextCache.clear();
    m_isInitialized = false;
}

bool TextRenderer::initializeResources()
{
    // Create reusable HarfBuzz buffer for text shaping
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

//==========================================================================================
// Frame Management

void TextRenderer::beginFrame()
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "Not initialized");
    
    // Advance glyph cache frame for expiration tracking
    if (m_glyphCache) m_glyphCache->beginFrame();
}

//==========================================================================================
// Text Shaping (New API with Font Fallback)

void TextRenderer::shapeText(const char* text,
                             const FontFallbackChain& fallbackChain,
                             float fontSize,
                             float letterSpacing,
                             ShapedText& outShapedText)
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "Not initialized");
    YUCHEN_ASSERT_MSG(text != nullptr, "Text cannot be null");
    YUCHEN_ASSERT_MSG(!fallbackChain.isEmpty(), "Fallback chain is empty");
    YUCHEN_ASSERT_MSG(fontSize >= Config::Font::MIN_SIZE && fontSize <= Config::Font::MAX_SIZE,
                      "Font size out of range");
    
    outShapedText.clear();
    
    // Validate text length
    size_t textLength = strlen(text);
    if (textLength == 0 || textLength > Config::Text::MAX_LENGTH) return;
    
    // Clamp letter spacing to reasonable range
    letterSpacing = std::max(-1000.0f, std::min(1000.0f, letterSpacing));
    
    // Check shaped text cache with letter spacing
    TextCacheKey cacheKey(text, fallbackChain, fontSize, letterSpacing);
    auto it = m_shapedTextCache.find(cacheKey);
    if (it != m_shapedTextCache.end())
    {
        outShapedText = it->second;
        return;
    }
    
    // Segment text by font fallback chain (per-character font selection)
    std::vector<TextSegment> segments = TextUtils::segmentTextWithFallback(
        text,
        fallbackChain,
        m_fontProvider
    );
    
    if (segments.empty()) return;
    
    float totalAdvance = 0.0f;
    float maxHeight = fontSize;
    
    // Shape each segment and combine results with letter spacing
    for (const auto& segment : segments)
    {
        ShapedText segmentShaped;
        if (shapeTextWithHarfBuzz(segment.text.c_str(), segment.fontHandle,
                                  fontSize, letterSpacing, segmentShaped))
        {
            // Offset segment glyphs by accumulated advance
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
    
    // Cache shaped result
    m_shapedTextCache[cacheKey] = outShapedText;
}

//==========================================================================================
// HarfBuzz Shaping

bool TextRenderer::shapeTextWithHarfBuzz(const char* text,
                                         FontHandle fontHandle,
                                         float fontSize,
                                         float letterSpacing,
                                         ShapedText& outShapedText)
{
    YUCHEN_ASSERT_MSG(text != nullptr, "Text cannot be null");
    YUCHEN_ASSERT_MSG(fontSize >= Config::Font::MIN_SIZE && fontSize <= Config::Font::MAX_SIZE,
                      "Font size out of range");
    
    // Get HarfBuzz font scaled for DPI
    float scaledFontSize = fontSize * m_dpiScale;
    hb_font_t* hbFont = static_cast<hb_font_t*>(
        m_fontProvider->getHarfBuzzFont(fontHandle, scaledFontSize, 1.0f)
    );
    
    YUCHEN_ASSERT_MSG(hbFont != nullptr, "Failed to get HarfBuzz font");
    
    // Prepare HarfBuzz buffer
    hb_buffer_clear_contents(m_harfBuzzBuffer);
    hb_buffer_add_utf8(m_harfBuzzBuffer, text, -1, 0, -1);
    
    // Detect script and language
    hb_script_t dominantScript = TextUtils::detectTextScript(text);
    const char* languageStr = TextUtils::getLanguageForScript(dominantScript);
    hb_language_t language = hb_language_from_string(languageStr, -1);
    
    // Set buffer properties
    hb_buffer_set_direction(m_harfBuzzBuffer, HB_DIRECTION_LTR);
    hb_buffer_set_script(m_harfBuzzBuffer, dominantScript);
    hb_buffer_set_language(m_harfBuzzBuffer, language);
    
    // IMPORTANT: Disable kerning (as per user requirement - kerning is unstable)
    hb_feature_t features[1];
    features[0].tag = HB_TAG('k','e','r','n');
    features[0].value = 0;
    features[0].start = 0;
    features[0].end = (unsigned int)-1;
    
    // Shape text
    hb_shape(hbFont, m_harfBuzzBuffer, features, 1);
    
    // Extract shaped glyph info
    unsigned int glyphCount = 0;
    hb_glyph_info_t* glyphInfos = hb_buffer_get_glyph_infos(m_harfBuzzBuffer, &glyphCount);
    hb_glyph_position_t* glyphPositions = hb_buffer_get_glyph_positions(m_harfBuzzBuffer, &glyphCount);
    
    YUCHEN_ASSERT_MSG(glyphInfos != nullptr && glyphPositions != nullptr,
                      "Failed to get glyph info/positions");
    YUCHEN_ASSERT_MSG(glyphCount > 0 && glyphCount <= Config::Text::MAX_GLYPHS_PER_TEXT,
                      "Invalid glyph count");
    
    outShapedText.glyphs.reserve(glyphCount);
    
    // Calculate letter spacing in pixels (em = fontSize)
    float spacingPixels = (letterSpacing / 1000.0f) * fontSize * m_dpiScale;
    
    float penX = 0.0f;
    float penY = 0.0f;
    
    // Convert HarfBuzz glyph data to ShapedGlyph format with letter spacing
    for (unsigned int i = 0; i < glyphCount; ++i)
    {
        ShapedGlyph glyph;
        glyph.glyphIndex = glyphInfos[i].codepoint;
        glyph.cluster = glyphInfos[i].cluster;
        glyph.fontHandle = fontHandle;
        
        // Extract position and advance (26.6 fixed-point format)
        float xOffset = glyphPositions[i].x_offset / 64.0f;
        float yOffset = glyphPositions[i].y_offset / 64.0f;
        float xAdvance = glyphPositions[i].x_advance / 64.0f;
        float yAdvance = glyphPositions[i].y_advance / 64.0f;
        
        if (i < glyphCount - 1) xAdvance += spacingPixels;
    
        // Scale back from DPI-scaled coordinates
        glyph.position = Vec2((penX + xOffset) / m_dpiScale, (penY + yOffset) / m_dpiScale);
        glyph.advance = xAdvance / m_dpiScale;
        
        outShapedText.glyphs.push_back(glyph);
        
        // Advance pen position
        penX += xAdvance;
        penY += yAdvance;
    }
    
    outShapedText.totalAdvance = penX / m_dpiScale;
    outShapedText.totalSize = Vec2(penX / m_dpiScale, fontSize);
    
    return true;
}

//==========================================================================================
// Vertex Generation

void TextRenderer::generateTextVertices(const ShapedText& shaped,
                                       const Vec2& position,
                                       const Vec4& color,
                                       const FontFallbackChain& fontChain,
                                       float fontSize,
                                       std::vector<TextVertex>& vertices)
{
    vertices.clear();
    vertices.reserve(shaped.glyphs.size() * 4);
    Vec2 atlasSize = m_glyphCache->getCurrentAtlasSize();
    
    for (const auto& glyph : shaped.glyphs)
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
            
            renderGlyph(actualFontHandle, glyph.glyphIndex, scaledFontSize,
                       bitmapData, glyphSize, bearing, advance);
            
            m_glyphCache->cacheGlyph(key, bitmapData, glyphSize, bearing, advance);
            entry = m_glyphCache->getGlyph(key);
        }
        if (!entry || entry->textureRect.width <= 0.0f || entry->textureRect.height <= 0.0f) continue;
        Vec2 glyphPos = Vec2(
            position.x + glyph.position.x + (entry->bearing.x / m_dpiScale),
            position.y + glyph.position.y - (entry->bearing.y / m_dpiScale)
        );
        float glyphWidth = entry->textureRect.width / m_dpiScale;
        float glyphHeight = entry->textureRect.height / m_dpiScale;
        
        Vec2 texCoordMin = Vec2(
            entry->textureRect.x / atlasSize.x,
            entry->textureRect.y / atlasSize.y
        );
        Vec2 texCoordMax = Vec2(
            (entry->textureRect.x + entry->textureRect.width) / atlasSize.x,
            (entry->textureRect.y + entry->textureRect.height) / atlasSize.y
        );
        
        TextVertex topLeft(
            Vec2(glyphPos.x, glyphPos.y),
            Vec2(texCoordMin.x, texCoordMin.y),
            color
        );
        TextVertex topRight(
            Vec2(glyphPos.x + glyphWidth, glyphPos.y),
            Vec2(texCoordMax.x, texCoordMin.y),
            color
        );
        TextVertex bottomLeft(
            Vec2(glyphPos.x, glyphPos.y + glyphHeight),
            Vec2(texCoordMin.x, texCoordMax.y),
            color
        );
        TextVertex bottomRight(
            Vec2(glyphPos.x + glyphWidth, glyphPos.y + glyphHeight),
            Vec2(texCoordMax.x, texCoordMax.y),
            color
        );
        
        vertices.push_back(topLeft);
        vertices.push_back(topRight);
        vertices.push_back(bottomLeft);
        vertices.push_back(bottomRight);
    }
}

//==========================================================================================
// Glyph Rasterization

void TextRenderer::renderGlyph(FontHandle fontHandle,
                               uint32_t glyphIndex,
                               float scaledFontSize,
                               const void*& outBitmapData,
                               Vec2& outSize,
                               Vec2& outBearing,
                               float& outAdvance)
{
    // Get FreeType face from font provider
    void* face = m_fontProvider->getFontFace(fontHandle);
    rasterizeGlyphWithFreeType(face, glyphIndex, scaledFontSize,
                                outBitmapData, outSize, outBearing, outAdvance);
}

void TextRenderer::rasterizeGlyphWithFreeType(void* face,
                                               uint32_t glyphIndex,
                                               float scaledFontSize,
                                               const void*& outBitmapData,
                                               Vec2& outSize,
                                               Vec2& outBearing,
                                               float& outAdvance)
{
    YUCHEN_ASSERT_MSG(face != nullptr, "Face cannot be null");
    
    FT_Face ftFace = static_cast<FT_Face>(face);
    
    // Set character size
    FT_Error error = FT_Set_Char_Size(
        ftFace,
        0,
        (FT_F26Dot6)(scaledFontSize * 64),
        Config::Font::FREETYPE_DPI,
        Config::Font::FREETYPE_DPI
    );
    if (error != FT_Err_Ok)
    {
        throw std::runtime_error("Failed to set char size");
    }
    
    // Load and render glyph
    error = FT_Load_Glyph(ftFace, glyphIndex, Config::Font::LOAD_FLAGS_RENDER);
    if (error != FT_Err_Ok)
    {
        throw std::runtime_error("Failed to load glyph");
    }
    
    FT_GlyphSlot slot = ftFace->glyph;
    if (slot->format != FT_GLYPH_FORMAT_BITMAP)
    {
        throw std::runtime_error("Glyph not in bitmap format");
    }
    
    // Extract bitmap data and metrics
    outBitmapData = slot->bitmap.buffer;
    outSize = Vec2(slot->bitmap.width, slot->bitmap.rows);
    outBearing = Vec2(slot->bitmap_left, slot->bitmap_top);
    outAdvance = slot->advance.x / 64.0f;
}

//==========================================================================================
// State Query

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

} // namespace YuchenUI
