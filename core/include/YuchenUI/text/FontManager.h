#pragma once

#include "YuchenUI/core/Types.h"
#include "YuchenUI/text/IFontProvider.h"
#include "YuchenUI/text/FontDatabase.h"
#include "YuchenUI/text/Font.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

typedef struct FT_LibraryRec_* FT_Library;

namespace YuchenUI {

class IResourceResolver;

struct FontEntry {
    std::unique_ptr<FontFile> file;
    std::unique_ptr<FontFace> face;
    std::unique_ptr<FontCache> cache;
    std::string name;
    bool isValid;
    
    FontEntry() : file(nullptr), face(nullptr), cache(nullptr), name(), isValid(false) {}
};

class FontManager : public IFontProvider {
public:
    FontManager();
    ~FontManager();
    
    bool initialize(IResourceResolver* resourceResolver);
    void destroy();
    bool isInitialized() const { return m_isInitialized; }
    
    FontHandle loadFontFromFile(const char* path, const char* name) override;
    FontHandle loadFontFromMemory(const void* data, size_t size, const char* name) override;
    
    bool isValidFont(FontHandle handle) const override;
    FontMetrics getFontMetrics(FontHandle handle, float fontSize) const override;
    GlyphMetrics getGlyphMetrics(FontHandle handle, uint32_t codepoint, float fontSize) const override;
    Vec2 measureText(const char* text, float fontSize) const override;
    float getTextHeight(FontHandle handle, float fontSize) const override;
    
    void* getFontFace(FontHandle handle) const override;
    void* getHarfBuzzFont(FontHandle handle, float fontSize, float dpiScale) override;
    
    bool hasGlyph(FontHandle handle, uint32_t codepoint) const override;
    FontHandle selectFontForCodepoint(uint32_t codepoint, const FontFallbackChain& fallbackChain) const override;
    
    FontFallbackChain createDefaultFallbackChain() const override;
    FontFallbackChain createBoldFallbackChain() const override;
    FontFallbackChain createTitleFallbackChain() const override;
    
    FontHandle getDefaultFont() const override { return m_defaultRegularFont; }
    FontHandle getDefaultBoldFont() const override { return m_defaultBoldFont; }
    FontHandle getDefaultNarrowFont() const override { return m_defaultNarrowFont; }
    FontHandle getDefaultNarrowBoldFont() const override { return m_defaultNarrowBoldFont; }
    FontHandle getDefaultCJKFont() const override { return m_defaultCJKFont; }
    FontHandle getDefaultSymbolFont() const override { return m_defaultSymbolFont; }
    
    FontHandle findFont(const char* familyName, FontWeight weight = FontWeight::Normal, FontStyle style = FontStyle::Normal) const override;
    std::vector<std::string> availableFontFamilies() const override;
    std::vector<FontDescriptor> fontsForFamily(const char* familyName) const override;
    void printAvailableFonts() const override;
    
    FontEntry* getFontEntry(FontHandle handle);
    const FontEntry* getFontEntry(FontHandle handle) const;

private:
    bool initializeFreeType();
    void cleanupFreeType();
    
    void initializeFonts();
    void loadCJKFont();
    void loadSymbolFont();
    
    bool hasGlyphImpl(FontHandle handle, uint32_t codepoint) const;
    
#ifdef __APPLE__
    std::string getCoreTextFontPath(const char* fontName) const;
#endif
    
    bool m_isInitialized;
    IResourceResolver* m_resourceResolver;
    std::vector<FontEntry> m_fonts;
    FT_Library m_freeTypeLibrary;
    FontDatabase m_fontDatabase;
    
    FontHandle m_defaultRegularFont;
    FontHandle m_defaultBoldFont;
    FontHandle m_defaultNarrowFont;
    FontHandle m_defaultNarrowBoldFont;
    FontHandle m_defaultCJKFont;
    FontHandle m_defaultSymbolFont;
    
    mutable std::unordered_map<uint64_t, bool> m_glyphAvailabilityCache;
};

}
