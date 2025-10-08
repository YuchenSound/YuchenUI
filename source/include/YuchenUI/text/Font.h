#pragma once

#include "YuchenUI/core/Types.h"
#include "YuchenUI/core/Config.h"
#include "YuchenUI/text/TextUtils.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include <hb-ft.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <list>

namespace YuchenUI
{

class FontFile
{
public:
    FontFile();
    ~FontFile();

    bool loadFromMemory(const void* data, size_t size, const std::string& name);
    bool loadFromFile(const char* path, const std::string& name);
    
    const std::string& getName() const { return m_name; }
    const std::string& getFilePath() const { return m_filePath; }
    const std::vector<unsigned char>& getMemoryData() const { return m_memoryData; }
    
    bool isValid() const { return m_isValid; }

private:
    std::string m_name;
    std::string m_filePath;
    std::vector<unsigned char> m_memoryData;
    bool m_isValid;

    FontFile(const FontFile&) = delete;
    FontFile& operator=(const FontFile&) = delete;
};

class FontFace
{
public:
    FontFace(FT_Library library);
    ~FontFace();

    bool createFromFontFile(const FontFile& fontFile);
    void destroy();

    FT_Face getFTFace() const { return m_face; }
    bool isValid() const { return m_face != nullptr; }

    FontMetrics getMetrics(float fontSize) const;
    GlyphMetrics getGlyphMetrics(uint32_t codepoint, float fontSize) const;
    float measureText(const char* text, float fontSize) const;

    bool setCharSize(float fontSize) const;

private:
    FT_Library m_library;
    FT_Face m_face;

    FontFace(const FontFace&) = delete;
    FontFace& operator=(const FontFace&) = delete;
};

class FontCache
{
public:
    FontCache();
    ~FontCache();

    hb_font_t* getHarfBuzzFont(const FontFace& fontFace, float fontSize);
    void clearAll();

private:
    void evictLeastRecentlyUsed();
    void updateLRU(uint32_t sizeKey);
    hb_font_t* createHarfBuzzFont(const FontFace& fontFace, float fontSize);

    std::unordered_map<uint32_t, hb_font_t*> m_harfBuzzFonts;
    std::list<uint32_t> m_usageOrder;
    
    static constexpr size_t MAX_CACHED_SIZES = 8;

    FontCache(const FontCache&) = delete;
    FontCache& operator=(const FontCache&) = delete;
};

} // namespace YuchenUI


