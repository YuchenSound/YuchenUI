#pragma once

#include "YuchenUI/core/Types.h"
#include <vector>
#include <string>

namespace YuchenUI {

struct FontDescriptor;
enum class FontWeight;
enum class FontStyle;

class IFontProvider {
public:
    virtual ~IFontProvider() = default;
    
    virtual FontHandle loadFontFromMemory(const void* data, size_t size, const char* name) = 0;
    virtual FontHandle loadFontFromFile(const char* path, const char* name) = 0;
    
    virtual bool isValidFont(FontHandle handle) const = 0;
    virtual FontMetrics getFontMetrics(FontHandle handle, float fontSize) const = 0;
    virtual GlyphMetrics getGlyphMetrics(FontHandle handle, uint32_t codepoint, float fontSize) const = 0;
    virtual Vec2 measureText(const char* text, float fontSize) const = 0;
    virtual float getTextHeight(FontHandle handle, float fontSize) const = 0;
    
    virtual bool hasGlyph(FontHandle handle, uint32_t codepoint) const = 0;
    virtual FontHandle selectFontForCodepoint(uint32_t codepoint, const FontFallbackChain& fallbackChain) const = 0;
    
    virtual FontHandle getDefaultFont() const = 0;
    virtual FontHandle getDefaultBoldFont() const = 0;
    virtual FontHandle getDefaultNarrowFont() const = 0;
    virtual FontHandle getDefaultNarrowBoldFont() const = 0;
    virtual FontHandle getDefaultCJKFont() const = 0;
    virtual FontHandle getDefaultSymbolFont() const = 0;
    
    virtual FontFallbackChain createDefaultFallbackChain() const = 0;
    virtual FontFallbackChain createBoldFallbackChain() const = 0;
    virtual FontFallbackChain createTitleFallbackChain() const = 0;
    
    virtual FontHandle findFont(const char* familyName, FontWeight weight, FontStyle style) const = 0;
    virtual std::vector<std::string> availableFontFamilies() const = 0;
    virtual std::vector<FontDescriptor> fontsForFamily(const char* familyName) const = 0;
    virtual void printAvailableFonts() const = 0;
    
    virtual void* getFontFace(FontHandle handle) const = 0;
    virtual void* getHarfBuzzFont(FontHandle handle, float fontSize, float dpiScale) = 0;
};

}
