#include "YuchenUI/text/Font.h"
#include "YuchenUI/core/Assert.h"
#include <cstring>
#include <fstream>

namespace YuchenUI {

// MARK: - FontFile
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
    
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;
    
    size_t size = file.tellg();
    if (size == 0) return false;
    
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

// MARK: - FontFace
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
    
    if (!fontFile.getMemoryData().empty())
    {
        const auto& memoryData = fontFile.getMemoryData();
        error = FT_New_Memory_Face(m_library,
                                   static_cast<const FT_Byte*>(memoryData.data()),
                                   static_cast<FT_Long>(memoryData.size()),
                                   0,
                                   &m_face);
    }
    else
    {
        error = FT_New_Face(m_library, fontFile.getFilePath().c_str(), 0, &m_face);
    }
    if (error != FT_Err_Ok || !m_face || m_face->num_charmaps == 0)
    {
        if (m_face)
        {
            FT_Done_Face(m_face);
            m_face = nullptr;
        }
        return false;
    }
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
    
    uint32_t glyphIndex = FT_Get_Char_Index(m_face, codepoint);
    if (glyphIndex == 0) return GlyphMetrics();
    
    FT_Error error = FT_Load_Glyph(m_face, glyphIndex, FT_LOAD_DEFAULT);
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
    
    while (*p)
    {
        uint32_t codepoint = TextUtils::decodeUTF8(p);
        if (codepoint == 0) break;
        
        if (codepoint != 0xFFFD)
        {
            uint32_t glyphIndex = FT_Get_Char_Index(m_face, codepoint);
            if (glyphIndex != 0)
            {
                FT_Error error = FT_Load_Glyph(m_face, glyphIndex, FT_LOAD_DEFAULT);
                if (error == FT_Err_Ok) totalWidth += m_face->glyph->advance.x / 64.0f;
            }
            else if (codepoint == ' ')
            {
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
    FT_Error error = FT_Set_Char_Size(m_face, 0, (FT_F26Dot6)(fontSize * 64), Config::Font::FREETYPE_DPI, Config::Font::FREETYPE_DPI);
    return error == FT_Err_Ok;
}

// MARK: - FontCache
FontCache::FontCache() : m_harfBuzzFonts(), m_usageOrder(){}
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
    
    uint32_t sizeKey = static_cast<uint32_t>(fontSize * 2.0f);
    
    auto it = m_harfBuzzFonts.find(sizeKey);
    if (it != m_harfBuzzFonts.end() && it->second != nullptr)
    {
        updateLRU(sizeKey);
        return it->second;
    }
    
    if (m_harfBuzzFonts.size() >= MAX_CACHED_SIZES) evictLeastRecentlyUsed();
    
    hb_font_t* hbFont = createHarfBuzzFont(fontFace, fontSize);
    if (!hbFont)
    {
        std::cerr << "[FontCache] Failed to create HarfBuzz font" << std::endl;
        return nullptr;
    }
    
    m_harfBuzzFonts[sizeKey] = hbFont;
    updateLRU(sizeKey);
    
    return hbFont;
}

void FontCache::clearAll()
{
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
    
    FT_Error error = FT_Set_Char_Size(ftFace, 0, (FT_F26Dot6)(fontSize * 64), Config::Font::FREETYPE_DPI, Config::Font::FREETYPE_DPI);
    if (error != FT_Err_Ok)
    {
        std::cerr << "[FontCache] Failed to set char size" << std::endl;
        return nullptr;
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
