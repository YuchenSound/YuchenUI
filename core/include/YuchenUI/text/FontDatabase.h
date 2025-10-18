#pragma once

#include "YuchenUI/core/Types.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace YuchenUI {

class IResourceResolver;

enum class FontWeight {
    Thin = 100,
    ExtraLight = 200,
    Light = 300,
    Normal = 400,
    Medium = 500,
    SemiBold = 600,
    Bold = 700,
    ExtraBold = 800,
    Black = 900
};

enum class FontStyle {
    Normal,
    Italic,
    Oblique
};

enum class FontStretch {
    UltraCondensed,
    ExtraCondensed,
    Condensed,
    SemiCondensed,
    Normal,
    SemiExpanded,
    Expanded,
    ExtraExpanded,
    UltraExpanded
};

enum class FontRole {
    Unknown = 0,
    DefaultRegular,
    DefaultBold,
    DefaultItalic,
    DefaultNarrow,
    CJK,
    Arabic,
    Hebrew,
    Emoji,
    Symbol,
    Monospace
};

struct FontDescriptor {
    std::string familyName;
    std::string styleName;
    std::string fullName;
    std::string postScriptName;
    
    FontWeight weight;
    FontStyle style;
    FontStretch stretch;
    
    bool isFixedPitch;
    bool isScalable;
    bool hasColorGlyphs;
    
    int numGlyphs;
    int unitsPerEM;
    
    std::vector<std::pair<uint32_t, uint32_t>> unicodeRanges;
    
    FontHandle handle;
    std::string sourcePath;
    
    FontDescriptor()
        : weight(FontWeight::Normal)
        , style(FontStyle::Normal)
        , stretch(FontStretch::Normal)
        , isFixedPitch(false)
        , isScalable(true)
        , hasColorGlyphs(false)
        , numGlyphs(0)
        , unitsPerEM(0)
        , handle(INVALID_FONT_HANDLE)
    {}
};

struct FontQuery {
    std::string familyName;
    FontWeight weight;
    FontStyle style;
    FontStretch stretch;
    
    FontQuery()
        : weight(FontWeight::Normal)
        , style(FontStyle::Normal)
        , stretch(FontStretch::Normal)
    {}
    
    FontQuery(const char* family, FontWeight w = FontWeight::Normal,
              FontStyle s = FontStyle::Normal)
        : familyName(family ? family : "")
        , weight(w)
        , style(s)
        , stretch(FontStretch::Normal)
    {}
};

class FontFile;
class FontFace;
class FontCache;
struct FontEntry;

class FontDatabase {
public:
    FontDatabase();
    ~FontDatabase();
    
    bool initialize(FT_Library library, IResourceResolver* resourceResolver);
    void shutdown();
    bool isInitialized() const { return m_isInitialized; }
    
    FontHandle registerFont(const char* path, const char* name, FontEntry* entry);
    FontHandle registerFontFromMemory(const void* data, size_t size,
                                      const char* name, FontEntry* entry);
    
    int discoverAndRegisterFonts(std::vector<FontEntry>& fontEntries);
    
    void assignFontRoles();
    FontHandle getFontForRole(FontRole role) const;
    
    FontHandle findFont(const char* familyName,
                        FontWeight weight = FontWeight::Normal,
                        FontStyle style = FontStyle::Normal) const;
    
    FontHandle findFont(const FontQuery& query) const;
    
    std::vector<std::string> families() const;
    std::vector<FontDescriptor> fontsForFamily(const char* familyName) const;
    const FontDescriptor* getFontDescriptor(FontHandle handle) const;
    
    FontHandle selectBestMatch(const FontQuery& query) const;
    
    bool supportsCharacter(FontHandle handle, uint32_t codepoint) const;
    std::vector<std::pair<uint32_t, uint32_t>> getUnicodeCoverage(FontHandle handle) const;

private:
    bool extractFontMetadata(FT_Face face, FontDescriptor& descriptor);
    FontWeight detectFontWeight(const char* styleName, long styleFlags) const;
    FontStyle detectFontStyle(const char* styleName, long styleFlags) const;
    FontStretch detectFontStretch(const char* styleName) const;
    void analyzeUnicodeCoverage(FT_Face face, FontDescriptor& descriptor);
    
    bool hasLatinCoverage(const FontDescriptor& desc) const;
    bool hasCJKCoverage(const FontDescriptor& desc) const;
    bool hasArabicCoverage(const FontDescriptor& desc) const;
    bool hasHebrewCoverage(const FontDescriptor& desc) const;
    FontHandle findBestFontForRole(FontRole role) const;
    
    int calculateWeightDistance(FontWeight w1, FontWeight w2) const;
    bool stringEqualsCaseInsensitive(const std::string& s1, const std::string& s2) const;
    
    FT_Library m_library;
    IResourceResolver* m_resourceResolver;
    bool m_isInitialized;
    std::unordered_map<FontHandle, FontDescriptor> m_descriptors;
    std::unordered_map<std::string, std::vector<FontHandle>> m_familyMap;
    std::unordered_map<FontRole, FontHandle> m_roleAssignments;
    
    FontDatabase(const FontDatabase&) = delete;
    FontDatabase& operator=(const FontDatabase&) = delete;
};

}
