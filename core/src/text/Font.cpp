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
/** @file Font.cpp
    
    Implementation notes:
    - FontFile copies data to internal buffer for lifetime independence
    - FontFace wraps FT_Face with automatic cleanup
    - FontCache uses LRU eviction with quantized size keys (size * 2)
    - HarfBuzz fonts created with FT callbacks and scale factors
    - setCharSize() must be called before glyph operations on FT_Face
    - measureText() is simple advance sum without shaping (for basic estimation)
*/

#include "YuchenUI/text/Font.h"
#include "YuchenUI/core/Assert.h"
#include <cstring>
#include <fstream>

namespace YuchenUI {

//==========================================================================================
// FontFile Implementation

FontFile::FontFile()
    : m_name()
    , m_filePath()
    , m_memoryData()
    , m_isValid(false)
{
}

FontFile::~FontFile()
{
}

bool FontFile::loadFromMemory(const void* data, size_t size, const std::string& name)
{
    YUCHEN_ASSERT_MSG(data != nullptr, "Data cannot be null");
    YUCHEN_ASSERT_MSG(size > 0, "Size must be positive");
    YUCHEN_ASSERT_MSG(!name.empty(), "Font name cannot be empty");
    
    // Copy data to internal buffer
    m_memoryData.resize(size);
    memcpy(m_memoryData.data(), data, size);
    m_name = name;
    m_filePath.clear();
    m_isValid = true;
    
    return true;
}

bool FontFile::loadFromFile(const char* path, const std::string& name)
{
    YUCHEN_ASSERT_MSG(path != nullptr, "Path cannot be null");
    YUCHEN_ASSERT_MSG(!name.empty(), "Font name cannot be empty");
    
    // Open file in binary mode, seek to end for size
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;
    
    size_t size = file.tellg();
    if (size == 0) return false;
    
    // Read entire file into buffer
    file.seekg(0, std::ios::beg);
    m_memoryData.resize(size);
    
    if (!file.read(reinterpret_cast<char*>(m_memoryData.data()), size))
    {
        m_memoryData.clear();
        return false;
    }
    
    m_name = name;
    m_filePath = path;
    m_isValid = true;
    
    return true;
}

//==========================================================================================
// FontFace Implementation

FontFace::FontFace(FT_Library library)
    : m_library(library)
    , m_face(nullptr)
{
    if (!library) m_library = nullptr;
}

FontFace::~FontFace()
{
    destroy();
}

bool FontFace::createFromFontFile(const FontFile& fontFile)
{
    if (!fontFile.isValid()) return false;
    if (m_face != nullptr) return false;
    if (!m_library) return false;

    FT_Error error;
    
    // Create face from memory or file path
    if (!fontFile.getMemoryData().empty())
    {
        const auto& memoryData = fontFile.getMemoryData();
        error = FT_New_Memory_Face(m_library,
                                   static_cast<const FT_Byte*>(memoryData.data()),
                                   static_cast<FT_Long>(memoryData.size()),
                                   0,  // Face index (0 for single-face fonts)
                                   &m_face);
    }
    else
    {
        error = FT_New_Face(m_library, fontFile.getFilePath().c_str(), 0, &m_face);
    }
    
    // Validate face creation
    if (error != FT_Err_Ok || !m_face || m_face->num_charmaps == 0)
    {
        if (m_face)
        {
            FT_Done_Face(m_face);
            m_face = nullptr;
        }
        return false;
    }
    
    // Select Unicode character map
    if (FT_Select_Charmap(m_face, FT_ENCODING_UNICODE) != FT_Err_Ok)
    {
        FT_Done_Face(m_face);
        m_face = nullptr;
        return false;
    }
    
    return true;
}

void FontFace::destroy()
{
    if (m_face)
    {
        FT_Done_Face(m_face);
        m_face = nullptr;
    }
}

FontMetrics FontFace::getMetrics(float fontSize) const
{
    if (!isValid()) return FontMetrics();
    if (fontSize < Config::Font::MIN_SIZE || fontSize > Config::Font::MAX_SIZE) return FontMetrics();
    if (!setCharSize(fontSize)) return FontMetrics();
    
    FontMetrics metrics;
    
    // Scale font units to pixels
    float scale = fontSize / m_face->units_per_EM;

    metrics.ascender = m_face->ascender * scale;
    metrics.descender = m_face->descender * scale;
    metrics.lineHeight = m_face->height * scale;
    metrics.maxAdvance = m_face->max_advance_width / 64.0f;
    
    return metrics;
}

GlyphMetrics FontFace::getGlyphMetrics(uint32_t codepoint, float fontSize) const
{
    if (!isValid()) return GlyphMetrics();
    if (fontSize < Config::Font::MIN_SIZE || fontSize > Config::Font::MAX_SIZE) return GlyphMetrics();
    if (!setCharSize(fontSize)) return GlyphMetrics();
    
    // Get glyph index for code point
    uint32_t glyphIndex = FT_Get_Char_Index(m_face, codepoint);
    if (glyphIndex == 0) return GlyphMetrics();
    
    // Load glyph to get metrics
    FT_Error error = FT_Load_Glyph(m_face, glyphIndex, Config::Font::LOAD_FLAGS_METRICS);
    if (error != FT_Err_Ok) return GlyphMetrics();
    
    GlyphMetrics metrics;
    metrics.glyphIndex = glyphIndex;
    metrics.bearing = Vec2(m_face->glyph->bitmap_left, m_face->glyph->bitmap_top);
    metrics.size = Vec2(m_face->glyph->bitmap.width, m_face->glyph->bitmap.rows);
    metrics.advance = m_face->glyph->advance.x / 64.0f;
    
    return metrics;
}

float FontFace::measureText(const char* text, float fontSize) const {
    if (!text) return 0.0f;
    if (!isValid()) return 0.0f;
    if (fontSize < Config::Font::MIN_SIZE || fontSize > Config::Font::MAX_SIZE) return 0.0f;
    
    if (*text == '\0' || !setCharSize(fontSize)) return 0.0f;
    
    float totalWidth = 0.0f;
    const char* p = text;
    
    // Sum advances for each character (simple measurement without shaping)
    while (*p)
    {
        uint32_t codepoint = TextUtils::decodeUTF8(p);
        if (codepoint == 0) break;
        
        if (codepoint != 0xFFFD)  // Skip replacement character
        {
            uint32_t glyphIndex = FT_Get_Char_Index(m_face, codepoint);
            if (glyphIndex != 0)
            {
                FT_Error error = FT_Load_Glyph(m_face, glyphIndex, Config::Font::LOAD_FLAGS_METRICS);
                if (error == FT_Err_Ok) totalWidth += m_face->glyph->advance.x / 64.0f;
            }
            else if (codepoint == ' ')
            {
                // Estimate space width as 25% of font size
                totalWidth += fontSize * 0.25f;
            }
        }
    }
    
    return totalWidth;
}

bool FontFace::setCharSize(float fontSize) const
{
    if (!isValid()) return false;
    if (fontSize < Config::Font::MIN_SIZE || fontSize > Config::Font::MAX_SIZE) return false;
    
    bool isBitmapFont = FT_HAS_FIXED_SIZES(m_face);
    
    if (isBitmapFont && m_face->num_fixed_sizes > 0)
    {
        int targetSize = static_cast<int>(fontSize + 0.5f);
        int bestIndex = 0;
        int bestDiff = std::abs(m_face->available_sizes[0].height - targetSize);
        
        for (int i = 1; i < m_face->num_fixed_sizes; ++i)
        {
            int currentHeight = m_face->available_sizes[i].height;
            int diff = std::abs(currentHeight - targetSize);
            
            if (diff < bestDiff)
            {
                bestDiff = diff;
                bestIndex = i;
            }
        }
        
        FT_Error error = FT_Select_Size(m_face, bestIndex);
        
        if (error != FT_Err_Ok)
        {
            std::cerr << "[FontFace] Failed to select bitmap size " << bestIndex
                      << " (target: " << targetSize << "px)" << std::endl;
            return false;
        }
        
        return true;
    }
    
    FT_Error error = FT_Set_Char_Size(
        m_face,
        0,
        (FT_F26Dot6)(fontSize * 64),
        Config::Font::FREETYPE_DPI,
        Config::Font::FREETYPE_DPI
    );
    
    if (error != FT_Err_Ok)
    {
        std::cerr << "[FontFace] Failed to set char size " << fontSize
                  << "pt (error: " << error << ")" << std::endl;
        return false;
    }
    
    return true;
}

//==========================================================================================
// FontCache Implementation

FontCache::FontCache()
    : m_harfBuzzFonts()
    , m_usageOrder()
{
}

FontCache::~FontCache()
{
    clearAll();
}

hb_font_t* FontCache::getHarfBuzzFont(const FontFace& fontFace, float fontSize)
{
    if (!fontFace.isValid())
    {
        std::cerr << "[FontCache] Invalid font face" << std::endl;
        return nullptr;
    }
    
    if (fontSize < Config::Font::MIN_SIZE || fontSize > Config::Font::MAX_SIZE)
    {
        std::cerr << "[FontCache] Font size out of range: " << fontSize << std::endl;
        return nullptr;
    }
    
    // Quantize size to half-point resolution (multiply by 2)
    uint32_t sizeKey = static_cast<uint32_t>(fontSize * 2.0f);
    
    // Check cache for existing font
    auto it = m_harfBuzzFonts.find(sizeKey);
    if (it != m_harfBuzzFonts.end() && it->second != nullptr)
    {
        updateLRU(sizeKey);
        return it->second;
    }
    
    // Evict LRU entry if cache full
    if (m_harfBuzzFonts.size() >= MAX_CACHED_SIZES) evictLeastRecentlyUsed();
    
    // Create new HarfBuzz font
    hb_font_t* hbFont = createHarfBuzzFont(fontFace, fontSize);
    if (!hbFont)
    {
        std::cerr << "[FontCache] Failed to create HarfBuzz font" << std::endl;
        return nullptr;
    }
    
    // Cache and mark as most recently used
    m_harfBuzzFonts[sizeKey] = hbFont;
    updateLRU(sizeKey);
    
    return hbFont;
}

void FontCache::clearAll()
{
    // Destroy all HarfBuzz fonts
    for (auto& pair : m_harfBuzzFonts)
    {
        if (pair.second) hb_font_destroy(pair.second);
    }
    m_harfBuzzFonts.clear();
    m_usageOrder.clear();
}

void FontCache::evictLeastRecentlyUsed()
{
    if (m_usageOrder.empty()) return;
    
    // Remove oldest entry (back of list)
    uint32_t oldestKey = m_usageOrder.back();
    m_usageOrder.pop_back();
    
    auto it = m_harfBuzzFonts.find(oldestKey);
    if (it != m_harfBuzzFonts.end())
    {
        if (it->second) hb_font_destroy(it->second);
        m_harfBuzzFonts.erase(it);
    }
}

void FontCache::updateLRU(uint32_t sizeKey)
{
    // Move to front (most recently used)
    m_usageOrder.remove(sizeKey);
    m_usageOrder.push_front(sizeKey);
}

hb_font_t* FontCache::createHarfBuzzFont(const FontFace& fontFace, float fontSize)
{
    if (!fontFace.isValid())
    {
        std::cerr << "[FontCache] Invalid font face" << std::endl;
        return nullptr;
    }
    
    if (fontSize < Config::Font::MIN_SIZE || fontSize > Config::Font::MAX_SIZE)
    {
        std::cerr << "[FontCache] Font size out of range: " << fontSize << std::endl;
        return nullptr;
    }
    
    FT_Face ftFace = fontFace.getFTFace();
    if (!ftFace)
    {
        std::cerr << "[FontCache] FT_Face is null" << std::endl;
        return nullptr;
    }
    
    bool isBitmapFont = FT_HAS_FIXED_SIZES(ftFace);
    
    FT_Error error;
    
    if (isBitmapFont && ftFace->num_fixed_sizes > 0)
    {
        int targetSize = static_cast<int>(fontSize + 0.5f);
        int bestIndex = 0;
        int bestDiff = std::abs(ftFace->available_sizes[0].height - targetSize);
        
        for (int i = 1; i < ftFace->num_fixed_sizes; ++i)
        {
            int currentHeight = ftFace->available_sizes[i].height;
            int diff = std::abs(currentHeight - targetSize);
            
            if (diff < bestDiff)
            {
                bestDiff = diff;
                bestIndex = i;
            }
        }
        
        error = FT_Select_Size(ftFace, bestIndex);
        
        if (error != FT_Err_Ok)
        {
            std::cerr << "[FontCache] Failed to select bitmap size " << bestIndex
                      << " (target: " << targetSize << "px, error: " << error << ")" << std::endl;
            return nullptr;
        }
    }
    else
    {
        error = FT_Set_Char_Size(
            ftFace,
            0,
            (FT_F26Dot6)(fontSize * 64),
            Config::Font::FREETYPE_DPI,
            Config::Font::FREETYPE_DPI
        );
        
        if (error != FT_Err_Ok)
        {
            std::cerr << "[FontCache] Failed to set char size " << fontSize
                      << "pt (error: " << error << ")" << std::endl;
            return nullptr;
        }
    }
    
    hb_font_t* hbFont = hb_ft_font_create(ftFace, nullptr);
    if (!hbFont)
    {
        std::cerr << "[FontCache] Failed to create HarfBuzz font" << std::endl;
        return nullptr;
    }
    
    hb_ft_font_set_funcs(hbFont);
    
    int x_scale = static_cast<int>(ftFace->size->metrics.x_scale);
    int y_scale = static_cast<int>(ftFace->size->metrics.y_scale);
    hb_font_set_scale(hbFont, x_scale, y_scale);
    
    unsigned int x_ppem = ftFace->size->metrics.x_ppem;
    unsigned int y_ppem = ftFace->size->metrics.y_ppem;
    hb_font_set_ppem(hbFont, x_ppem, y_ppem);
    
    return hbFont;
}

} // namespace YuchenUI
