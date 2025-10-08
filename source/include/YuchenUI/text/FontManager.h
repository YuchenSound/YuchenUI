#pragma once

#include "YuchenUI/core/Types.h"
#include "YuchenUI/text/Font.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <vector>
#include <memory>
#include <string>

namespace YuchenUI {

struct FontEntry {
    std::unique_ptr<FontFile> file;
    std::unique_ptr<FontFace> face;
    std::unique_ptr<FontCache> cache;
    std::string name;
    bool isValid;
    
    FontEntry() : file(nullptr), face(nullptr), cache(nullptr), name(), isValid(false) {}
};

class FontManager {
public:
    static FontManager& getInstance();
    
    FontManager();
    ~FontManager();
    
    bool initialize();
    void destroy();
    bool isInitialized() const { return m_isInitialized; }
    
    FontHandle getArialRegular() const { return m_arialRegular; }
    FontHandle getArialBold() const { return m_arialBold; }
    FontHandle getArialNarrowRegular() const { return m_arialNarrowRegular; }
    FontHandle getArialNarrowBold() const { return m_arialNarrowBold; }
    FontHandle getPingFangFont() const { return m_pingFangFont; }
    
    FontHandle selectFontForCharacter(uint32_t codepoint) const;
    bool isValidFont(FontHandle handle) const;
    FontMetrics getFontMetrics(FontHandle handle, float fontSize) const;
    GlyphMetrics getGlyphMetrics(FontHandle handle, uint32_t codepoint, float fontSize) const;
    Vec2 measureText(const char* text, float fontSize) const;
    float getTextHeight(FontHandle handle, float fontSize) const;
    void* getFontFace(FontHandle handle) const;
    void* getHarfBuzzFont(FontHandle handle, float fontSize, float dpiScale);

private:
    bool initializeFreeType();
    void cleanupFreeType();
    void initializeFonts();
    
#ifdef __APPLE__
    std::string getCoreTextFontPath(const char* fontName) const;
#endif

    FontHandle loadFontFromFile(const char* path, const std::string& name);
    FontHandle loadFontFromMemory(const void* data, size_t size, const std::string& name);
    FontHandle generateFontHandle();
    FontEntry* getFontEntry(FontHandle handle);
    const FontEntry* getFontEntry(FontHandle handle) const;

    static FontManager* s_instance;
    
    bool m_isInitialized;
    FontHandle m_nextHandle;
    std::vector<FontEntry> m_fonts;
    FT_Library m_freeTypeLibrary;

    FontHandle m_arialRegular;
    FontHandle m_arialBold;
    FontHandle m_arialNarrowRegular;
    FontHandle m_arialNarrowBold;
    FontHandle m_pingFangFont;
};

}
