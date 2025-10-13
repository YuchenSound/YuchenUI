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
/** @file FontManager.cpp
    
    Implementation notes:
    - Font handles are sequential integers starting from 1
    - Font metrics and text measurements cached globally to avoid repeated queries
    - macOS uses CoreText to locate system fonts by name
    - Windows uses hardcoded paths to system fonts
    - Falls back to Arial for CJK if system font unavailable
    - Emoji fonts loaded automatically on initialization
    - Glyph availability cached for performance (per font-codepoint pair)
    - measureText() uses HarfBuzz shaping for accurate measurement with kerning
    
    Version 2.0 Changes:
    - Added hasGlyph() implementation with caching
    - Added selectFontForCodepoint() for fallback chain resolution
    - Added loadEmojiFont() and loadSymbolFont() for special character support
    - Enhanced font fallback chain builders
    - Improved performance with glyph availability cache
*/

#include "YuchenUI/text/FontManager.h"
#include "YuchenUI/text/TextUtils.h"
#include "YuchenUI/core/Assert.h"
#include "YuchenUI/core/Config.h"
#include "../generated/embedded_resources.h"
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <iostream>

#ifdef __APPLE__
    #include <CoreText/CoreText.h>
#endif

namespace YuchenUI {

//==========================================================================================
// Global Cache Structures

/** Cache key for text measurement results. */
struct MeasureTextKey
{
    std::string text;   ///< Text string
    float fontSize;     ///< Font size in points
    
    MeasureTextKey(const char* t, float fs) : text(t), fontSize(fs)
    {
        YUCHEN_ASSERT(t != nullptr);
        YUCHEN_ASSERT(fs > 0.0f);
    }
    
    bool operator==(const MeasureTextKey& other) const
    {
        return text == other.text && std::abs(fontSize - other.fontSize) < 0.01f;
    }
};

/** Hash functor for MeasureTextKey. */
struct MeasureTextKeyHash
{
    size_t operator()(const MeasureTextKey& key) const
    {
        std::hash<std::string> stringHasher;
        std::hash<float> floatHasher;
        return stringHasher(key.text) ^ (floatHasher(key.fontSize) << 1);
    }
};

/** Cache key for font metrics. */
struct FontMetricsKey
{
    FontHandle fontHandle;  ///< Font handle
    float fontSize;         ///< Font size in points
    
    FontMetricsKey(FontHandle fh, float fs) : fontHandle(fh), fontSize(fs)
    {
        YUCHEN_ASSERT(fh != INVALID_FONT_HANDLE);
        YUCHEN_ASSERT(fs > 0.0f);
    }
    
    bool operator==(const FontMetricsKey& other) const
    {
        return fontHandle == other.fontHandle &&
               std::abs(fontSize - other.fontSize) < 0.01f;
    }
};

/** Hash functor for FontMetricsKey. */
struct FontMetricsKeyHash
{
    size_t operator()(const FontMetricsKey& key) const
    {
        std::hash<FontHandle> handleHasher;
        std::hash<float> floatHasher;
        return handleHasher(key.fontHandle) ^ (floatHasher(key.fontSize) << 1);
    }
};

// Global caches (cleared on FontManager::destroy())
static std::unordered_map<MeasureTextKey, Vec2, MeasureTextKeyHash> s_measureTextCache;
static std::unordered_map<FontMetricsKey, FontMetrics, FontMetricsKeyHash> s_fontMetricsCache;
static std::unordered_set<uint32_t> s_warnedMissingGlyphs;

//==========================================================================================
// Lifecycle

FontManager::FontManager()
    : m_isInitialized(false)
    , m_fonts()
    , m_freeTypeLibrary(nullptr)
    , m_defaultRegularFont(INVALID_FONT_HANDLE)
    , m_defaultBoldFont(INVALID_FONT_HANDLE)
    , m_defaultNarrowFont(INVALID_FONT_HANDLE)
    , m_defaultNarrowBoldFont(INVALID_FONT_HANDLE)
    , m_defaultCJKFont(INVALID_FONT_HANDLE)
    , m_defaultSymbolFont(INVALID_FONT_HANDLE)
    , m_glyphAvailabilityCache()
{
    m_fonts.reserve(Config::Font::MAX_FONTS);
}

FontManager::~FontManager()
{
    destroy();
}

bool FontManager::initialize()
{
    YUCHEN_ASSERT_MSG(!m_isInitialized, "FontManager already initialized");

    if (!initializeFreeType())
    {
        std::cerr << "[FontManager] Failed to initialize FreeType" << std::endl;
        return false;
    }

    if (!m_fontDatabase.initialize(m_freeTypeLibrary))
    {
        std::cerr << "[FontManager] Failed to initialize font database" << std::endl;
        cleanupFreeType();
        return false;
    }

    int discoveredCount = m_fontDatabase.discoverAndRegisterFonts(m_fonts);
    std::cout << "[FontManager] Discovered " << discoveredCount << " fonts" << std::endl;

    initializeFonts();
    loadSymbolFont();
    
    m_isInitialized = true;
    
    std::cout << "[FontManager] Initialization complete" << std::endl;
    std::cout << "  - Total fonts: " << m_fonts.size() << std::endl;  // No need to subtract 1!
    std::cout << "  - Font families: " << m_fontDatabase.families().size() << std::endl;
    
    return true;
}

void FontManager::destroy()
{
    if (!m_isInitialized) return;

    s_measureTextCache.clear();
    s_fontMetricsCache.clear();
    s_warnedMissingGlyphs.clear();
    m_glyphAvailabilityCache.clear();

    m_fontDatabase.shutdown();

    m_fonts.clear();
    m_defaultRegularFont = INVALID_FONT_HANDLE;
    m_defaultBoldFont = INVALID_FONT_HANDLE;
    m_defaultNarrowFont = INVALID_FONT_HANDLE;
    m_defaultNarrowBoldFont = INVALID_FONT_HANDLE;
    m_defaultCJKFont = INVALID_FONT_HANDLE;
    m_defaultSymbolFont = INVALID_FONT_HANDLE;
        
    cleanupFreeType();
    m_isInitialized = false;
}

//==========================================================================================
// FreeType Initialization

bool FontManager::initializeFreeType()
{
    FT_Error error = FT_Init_FreeType(&m_freeTypeLibrary);
    if (error != FT_Err_Ok)
    {
        std::cerr << "[FontManager] FT_Init_FreeType failed with error: " << error << std::endl;
        return false;
    }
    return true;
}

void FontManager::cleanupFreeType()
{
    if (m_freeTypeLibrary)
    {
        FT_Done_FreeType(m_freeTypeLibrary);
        m_freeTypeLibrary = nullptr;
    }
}

//==========================================================================================
// Default Font Loading

void FontManager::initializeFonts()
{
    m_defaultRegularFont    = m_fontDatabase.getFontForRole(FontRole::DefaultRegular);
    m_defaultBoldFont       = m_fontDatabase.getFontForRole(FontRole::DefaultBold);
    m_defaultNarrowFont     = m_fontDatabase.getFontForRole(FontRole::DefaultNarrow);
    m_defaultNarrowBoldFont = m_fontDatabase.getFontForRole(FontRole::DefaultNarrow);
    m_defaultCJKFont        = m_fontDatabase.getFontForRole(FontRole::CJK);
    if (m_defaultRegularFont == INVALID_FONT_HANDLE)
    {
        std::cerr << "[FontManager] CRITICAL: No default regular font found!" << std::endl;
        if (!m_fonts.empty() && m_fonts[0].isValid)
        {
            m_defaultRegularFont = 0;
            std::cerr << "[FontManager] Using first available font as emergency fallback" << std::endl;
        }
    }
    
    if (m_defaultBoldFont == INVALID_FONT_HANDLE)m_defaultBoldFont              = m_defaultRegularFont;
    if (m_defaultNarrowFont == INVALID_FONT_HANDLE)m_defaultNarrowFont          = m_defaultRegularFont;
    if (m_defaultNarrowBoldFont == INVALID_FONT_HANDLE)m_defaultNarrowBoldFont  = m_defaultBoldFont;
    if (m_defaultCJKFont == INVALID_FONT_HANDLE)m_defaultCJKFont                = m_defaultRegularFont;
    YUCHEN_ASSERT_MSG(m_defaultRegularFont != INVALID_FONT_HANDLE,"Failed to assign default regular font");
    std::cout << "[FontManager] Font role assignment complete" << std::endl;
}

void FontManager::loadSymbolFont()
{
    m_defaultSymbolFont = m_fontDatabase.getFontForRole(FontRole::Symbol);
    
    if (m_defaultSymbolFont == INVALID_FONT_HANDLE)
    {
#ifdef __APPLE__
        std::string symbolPath = getCoreTextFontPath("Apple Symbols");
        if (!symbolPath.empty())
        {
            size_t fontIndex = m_fonts.size();
            m_fonts.emplace_back();
            m_defaultSymbolFont = m_fontDatabase.registerFont(
                symbolPath.c_str(), "Apple Symbols", &m_fonts[fontIndex]);
        }
#elif defined(_WIN32)
        // NEW: No need to pre-allocate, just append
        size_t fontIndex = m_fonts.size();
        m_fonts.emplace_back();
        m_defaultSymbolFont = m_fontDatabase.registerFont(
            "C:\\Windows\\Fonts\\seguisym.ttf", "Segoe UI Symbol", &m_fonts[fontIndex]);
#endif
    }
    
    if (m_defaultSymbolFont == INVALID_FONT_HANDLE)
    {
        std::cout << "[FontManager] Symbol font not available" << std::endl;
    }
    else
    {
        std::cout << "[FontManager] Symbol font loaded" << std::endl;
    }
}

#ifdef __APPLE__
std::string FontManager::getCoreTextFontPath(const char* fontName) const
{
    YUCHEN_ASSERT(fontName != nullptr);
    
    // Create CoreText font by name
    CFStringRef fontNameRef = CFStringCreateWithCString(NULL, fontName, kCFStringEncodingUTF8);
    if (!fontNameRef)
    {
        return "";
    }
    
    CTFontRef ctFont = CTFontCreateWithName(fontNameRef, 0.0, NULL);
    CFRelease(fontNameRef);
    
    if (!ctFont)
    {
        return "";
    }
    
    // Get font descriptor and extract file URL
    CTFontDescriptorRef descriptor = CTFontCopyFontDescriptor(ctFont);
    CFURLRef url = (CFURLRef)CTFontDescriptorCopyAttribute(descriptor, kCTFontURLAttribute);
    
    std::string path;
    if (url)
    {
        CFStringRef pathString = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
        if (pathString)
        {
            char pathBuffer[1024];
            if (CFStringGetCString(pathString, pathBuffer, sizeof(pathBuffer), kCFStringEncodingUTF8))
            {
                path = pathBuffer;
            }
            CFRelease(pathString);
        }
        CFRelease(url);
    }
    
    CFRelease(descriptor);
    CFRelease(ctFont);
    
    return path;
}
#endif

//==========================================================================================
// Font Fallback Chain Builders

FontFallbackChain FontManager::createDefaultFallbackChain() const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "FontManager not initialized");
    
    FontFallbackChain chain;
    chain.addFont(m_defaultRegularFont);
    chain.addFont(m_defaultCJKFont);

    if (m_defaultSymbolFont != INVALID_FONT_HANDLE)
    {
        chain.addFont(m_defaultSymbolFont);
    }
    
    return chain;
}

FontFallbackChain FontManager::createBoldFallbackChain() const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "FontManager not initialized");
    
    FontFallbackChain chain;
    chain.addFont(m_defaultBoldFont);
    chain.addFont(m_defaultCJKFont);  // PingFang doesn't have separate bold, uses same font

    if (m_defaultSymbolFont != INVALID_FONT_HANDLE)
    {
        chain.addFont(m_defaultSymbolFont);
    }
    
    return chain;
}

FontFallbackChain FontManager::createTitleFallbackChain() const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "FontManager not initialized");
    
    // For titles, use bold variants
    return createBoldFallbackChain();
}

//==========================================================================================
// Font Fallback Implementation

bool FontManager::hasGlyphImpl(FontHandle handle, uint32_t codepoint) const
{
    const FontEntry* entry = getFontEntry(handle);
    if (!entry || !entry->isValid)
    {
        return false;
    }
    
    FT_Face face = entry->face->getFTFace();
    if (!face)
    {
        return false;
    }
    
    // Query FreeType for glyph index
    // A glyph index of 0 means the character is not supported
    uint32_t glyphIndex = FT_Get_Char_Index(face, codepoint);
    return glyphIndex != 0;
}

bool FontManager::hasGlyph(FontHandle handle, uint32_t codepoint) const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "FontManager not initialized");
    YUCHEN_ASSERT_MSG(handle != INVALID_FONT_HANDLE, "Invalid font handle");
    
    // Create cache key: (FontHandle << 32) | codepoint
    uint64_t cacheKey = (static_cast<uint64_t>(handle) << 32) | codepoint;
    
    // Check cache first
    auto it = m_glyphAvailabilityCache.find(cacheKey);
    if (it != m_glyphAvailabilityCache.end())
    {
        return it->second;
    }
    
    // Query FreeType and cache result
    bool hasGlyph = hasGlyphImpl(handle, codepoint);
    m_glyphAvailabilityCache[cacheKey] = hasGlyph;
    
    return hasGlyph;
}

FontHandle FontManager::selectFontForCodepoint(
    uint32_t codepoint,
    const FontFallbackChain& fallbackChain) const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "FontManager not initialized");
    YUCHEN_ASSERT_MSG(!fallbackChain.isEmpty(), "Fallback chain is empty");
    
    // Try each font in the fallback chain
    for (size_t i = 0; i < fallbackChain.fonts.size(); ++i)
    {
        FontHandle font = fallbackChain.fonts[i];
        if (font != INVALID_FONT_HANDLE && hasGlyph(font, codepoint))
        {
            return font;
        }
    }
    
    // No font supports this character - log warning (deduplicated)
    if (s_warnedMissingGlyphs.find(codepoint) == s_warnedMissingGlyphs.end())
    {
        s_warnedMissingGlyphs.insert(codepoint);
        
        // Determine character type
        const char* charType = "Unknown";
        if (TextUtils::isEmojiCharacter(codepoint))
        {
            charType = "Emoji";
        }
        else if (TextUtils::isSymbolCharacter(codepoint))
        {
            charType = "Symbol";
        }
        else if (TextUtils::isChineseCharacter(codepoint))
        {
            charType = "CJK";
        }
        else if (TextUtils::isWesternCharacter(codepoint))
        {
            charType = "Western";
        }
        
        std::cerr << "[FontManager] Warning: No font in fallback chain supports "
                  << charType << " character U+"
                  << std::hex << std::uppercase << codepoint << std::dec
                  << " ('" << TextUtils::encodeUTF8(codepoint) << "'). "
                  << "Character will render as .notdef glyph."
                  << std::endl;
    }
    
    // Return primary font (will render as .notdef glyph)
    return fallbackChain.getPrimary();
}

//==========================================================================================
// Font Loading

FontHandle FontManager::loadFontFromFile(const char* path, const char* name)
{
    YUCHEN_ASSERT(path != nullptr);
    YUCHEN_ASSERT_MSG(m_fonts.size() < Config::Font::MAX_FONTS, "Maximum number of fonts reached");
    
    FontHandle handle = static_cast<FontHandle>(m_fonts.size());
    m_fonts.emplace_back();
    FontEntry& entry = m_fonts[handle];
    
    entry.file = std::make_unique<FontFile>();
    entry.face = std::make_unique<FontFace>(m_freeTypeLibrary);
    entry.cache = std::make_unique<FontCache>();
    entry.name = name;

    if (!entry.file->loadFromFile(path, name))
    {
        std::cerr << "[FontManager] Failed to load font file: " << path << std::endl;
        m_fonts.pop_back();
        return INVALID_FONT_HANDLE;
    }
    
    if (!entry.face->createFromFontFile(*entry.file))
    {
        std::cerr << "[FontManager] Failed to create font face: " << name << std::endl;
        m_fonts.pop_back();
        return INVALID_FONT_HANDLE;
    }

    entry.isValid = true;
    return handle;
}

FontHandle FontManager::loadFontFromMemory(const void* data, size_t size, const char* name)
{
    YUCHEN_ASSERT(data != nullptr);
    YUCHEN_ASSERT(size > 0);
    YUCHEN_ASSERT_MSG(m_fonts.size() < Config::Font::MAX_FONTS, "Maximum number of fonts reached");

    FontHandle handle = static_cast<FontHandle>(m_fonts.size());
    m_fonts.emplace_back();
    FontEntry& entry = m_fonts[handle];
    
    entry.file = std::make_unique<FontFile>();
    entry.face = std::make_unique<FontFace>(m_freeTypeLibrary);
    entry.cache = std::make_unique<FontCache>();
    entry.name = name;

    bool fileLoaded [[maybe_unused]] = entry.file->loadFromMemory(data, size, name);
    YUCHEN_ASSERT_MSG(fileLoaded, "Failed to load font from memory");

    bool faceCreated [[maybe_unused]] = entry.face->createFromFontFile(*entry.file);
    YUCHEN_ASSERT_MSG(faceCreated, "Failed to create font face from memory");

    entry.isValid = true;
    return handle;
}

//==========================================================================================
// Font Queries

bool FontManager::isValidFont(FontHandle handle) const
{
    const FontEntry* entry = getFontEntry(handle);
    return entry && entry->isValid && entry->face && entry->face->isValid();
}

FontMetrics FontManager::getFontMetrics(FontHandle handle, float fontSize) const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "FontManager not initialized");
    YUCHEN_ASSERT_MSG(handle != INVALID_FONT_HANDLE, "Invalid font handle");
    YUCHEN_ASSERT_MSG(fontSize >= Config::Font::MIN_SIZE && fontSize <= Config::Font::MAX_SIZE, "Font size out of range");
    
    // Check cache
    FontMetricsKey key(handle, fontSize);
    auto it = s_fontMetricsCache.find(key);
    if (it != s_fontMetricsCache.end())
    {
        return it->second;
    }
    
    // Query FreeType
    const FontEntry* entry = getFontEntry(handle);
    YUCHEN_ASSERT_MSG(entry && entry->isValid, "Invalid font entry");
    
    FontMetrics metrics = entry->face->getMetrics(fontSize);
    YUCHEN_ASSERT_MSG(metrics.isValid(), "Invalid font metrics");
    
    // Cache result
    s_fontMetricsCache[key] = metrics;
    return metrics;
}

GlyphMetrics FontManager::getGlyphMetrics(FontHandle handle, uint32_t codepoint, float fontSize) const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "FontManager not initialized");
    YUCHEN_ASSERT_MSG(handle != INVALID_FONT_HANDLE, "Invalid font handle");
    YUCHEN_ASSERT_MSG(fontSize >= Config::Font::MIN_SIZE && fontSize <= Config::Font::MAX_SIZE, "Font size out of range");
    
    const FontEntry* entry = getFontEntry(handle);
    YUCHEN_ASSERT_MSG(entry && entry->isValid, "Invalid font entry");
    
    return entry->face->getGlyphMetrics(codepoint, fontSize);
}

Vec2 FontManager::measureText(const char* text, float fontSize) const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "FontManager not initialized");
    YUCHEN_ASSERT(text != nullptr);
    YUCHEN_ASSERT_MSG(fontSize >= Config::Font::MIN_SIZE && fontSize <= Config::Font::MAX_SIZE, "Font size out of range");
    
    if (*text == '\0') return Vec2();
    
    // Check cache
    MeasureTextKey key(text, fontSize);
    auto it = s_measureTextCache.find(key);
    if (it != s_measureTextCache.end()) return it->second;
    
    // Use default fallback chain for measurement
    FontFallbackChain fallbackChain = createDefaultFallbackChain();
    
    // Segment text by font requirements using fallback
    std::vector<TextSegment> segments = TextUtils::segmentTextWithFallback(text, fallbackChain, const_cast<FontManager*>(this));
    YUCHEN_ASSERT(!segments.empty());
    
    float totalWidth = 0.0f;
    float maxHeight = 0.0f;
    
    // Create reusable HarfBuzz buffer
    hb_buffer_t* buffer = hb_buffer_create();
    
    // Measure each segment with HarfBuzz shaping
    for (const auto& segment : segments)
    {
        YUCHEN_ASSERT_MSG(isValidFont(segment.fontHandle), "Invalid font handle in text segment");
        
        const FontEntry* entry = getFontEntry(segment.fontHandle);
        if (!entry || !entry->isValid) continue;
        
        // Get HarfBuzz font for shaping
        hb_font_t* hbFont = entry->cache->getHarfBuzzFont(*entry->face, fontSize);
        if (!hbFont) continue;
        
        // Prepare HarfBuzz buffer
        hb_buffer_clear_contents(buffer);
        hb_buffer_set_direction(buffer, HB_DIRECTION_LTR);
        
        hb_script_t script = TextUtils::detectTextScript(segment.text.c_str());
        hb_buffer_set_script(buffer, script);
        
        const char* language = TextUtils::getLanguageForScript(script);
        hb_buffer_set_language(buffer, hb_language_from_string(language, -1));
        
        hb_buffer_add_utf8(buffer, segment.text.c_str(), -1, 0, -1);
        
        // Disable kerning for measurement consistency
        hb_feature_t features[1];
        features[0].tag = HB_TAG('k','e','r','n');
        features[0].value = 0;
        features[0].start = 0;
        features[0].end = (unsigned int)-1;
        
        // Shape text
        hb_shape(hbFont, buffer, features, 1);
        
        // Sum glyph advances
        unsigned int glyphCount = 0;
        hb_glyph_position_t* glyphPositions = hb_buffer_get_glyph_positions(buffer, &glyphCount);
        
        float segmentWidth = 0.0f;
        for (unsigned int i = 0; i < glyphCount; ++i)
        {
            segmentWidth += glyphPositions[i].x_advance / 64.0f;
        }
        
        totalWidth += segmentWidth;
        
        // Track maximum height
        float segmentHeight = getTextHeight(segment.fontHandle, fontSize);
        maxHeight = std::max(maxHeight, segmentHeight);
    }
    
    hb_buffer_destroy(buffer);
    
    Vec2 result(totalWidth, maxHeight);
    YUCHEN_ASSERT_MSG(result.isValid(), "Invalid text measurement result");
    
    // Cache result
    s_measureTextCache[key] = result;
    return result;
}

float FontManager::getTextHeight(FontHandle handle, float fontSize) const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "FontManager not initialized");
    YUCHEN_ASSERT_MSG(handle != INVALID_FONT_HANDLE, "Invalid font handle");
    YUCHEN_ASSERT_MSG(fontSize >= Config::Font::MIN_SIZE && fontSize <= Config::Font::MAX_SIZE, "Font size out of range");
    
    return getFontMetrics(handle, fontSize).lineHeight;
}

//==========================================================================================
// Internal Access

void* FontManager::getFontFace(FontHandle handle) const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "FontManager not initialized");
    
    const FontEntry* entry = getFontEntry(handle);
    YUCHEN_ASSERT_MSG(entry && entry->isValid, "Invalid font entry");
    
    return static_cast<void*>(entry->face->getFTFace());
}

void* FontManager::getHarfBuzzFont(FontHandle handle, float fontSize, float dpiScale)
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "FontManager not initialized");
    YUCHEN_ASSERT_MSG(handle != INVALID_FONT_HANDLE, "Invalid font handle");
    YUCHEN_ASSERT_MSG(fontSize >= Config::Font::MIN_SIZE && fontSize <= Config::Font::MAX_SIZE, "Font size out of range");
    YUCHEN_ASSERT_MSG(dpiScale > 0.0f, "DPI scale must be positive");
    
    FontEntry* entry = getFontEntry(handle);
    YUCHEN_ASSERT_MSG(entry && entry->isValid, "Invalid font entry");
    
    float scaledFontSize = fontSize * dpiScale;
    return static_cast<void*>(entry->cache->getHarfBuzzFont(*entry->face, scaledFontSize));
}

FontEntry* FontManager::getFontEntry(FontHandle handle)
{
    if (handle == INVALID_FONT_HANDLE) return nullptr;
    if (handle >= m_fonts.size()) return nullptr;
    return &m_fonts[handle];
}

const FontEntry* FontManager::getFontEntry(FontHandle handle) const
{
    if (handle == INVALID_FONT_HANDLE) return nullptr;
    if (handle >= m_fonts.size()) return nullptr;
    return &m_fonts[handle];
}

FontHandle FontManager::findFont(const char* familyName, FontWeight weight,
                                  FontStyle style) const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "FontManager not initialized");
    return m_fontDatabase.findFont(familyName, weight, style);
}

std::vector<std::string> FontManager::availableFontFamilies() const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "FontManager not initialized");
    return m_fontDatabase.families();
}

std::vector<FontDescriptor> FontManager::fontsForFamily(const char* familyName) const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "FontManager not initialized");
    return m_fontDatabase.fontsForFamily(familyName);
}

void FontManager::printAvailableFonts() const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "FontManager not initialized");
    
    auto families = m_fontDatabase.families();
    
    std::cout << "\n========== Available Fonts ==========" << std::endl;
    std::cout << "Total families: " << families.size() << std::endl;
    std::cout << "=====================================" << std::endl;
    
    for (const auto& family : families)
    {
        std::cout << "\nFamily: " << family << std::endl;
        
        auto fonts = m_fontDatabase.fontsForFamily(family.c_str());
        for (const auto& font : fonts)
        {
            std::cout << "  - " << font.fullName
                      << " (Weight: " << static_cast<int>(font.weight)
                      << ", Style: " << (font.style == FontStyle::Italic ? "Italic" :
                                        font.style == FontStyle::Oblique ? "Oblique" : "Normal")
                      << ", Glyphs: " << font.numGlyphs
                      << ", " << (font.hasColorGlyphs ? "Color" : "Monochrome")
                      << ")" << std::endl;
        }
    }
    
    std::cout << "\n=====================================" << std::endl;
}

} // namespace YuchenUI
