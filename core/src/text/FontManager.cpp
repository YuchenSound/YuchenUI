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

/// @Warning This file write by Claude! Therefore, the code quality of this file is currently unknown.
///                                                                                      ———— Yuchen WEI

//==========================================================================================
/** @file FontManager.cpp
    
    Implementation notes:
    - Singleton auto-initialized on first getInstance() call
    - Font handles are sequential integers starting from 1
    - Font metrics and text measurements cached globally to avoid repeated queries
    - macOS uses CoreText to locate system fonts by name
    - Windows uses hardcoded path to Microsoft YaHei
    - Falls back to Arial for CJK if system font unavailable
    - measureText() uses HarfBuzz shaping for accurate measurement with kerning
*/

#include "YuchenUI/text/FontManager.h"
#include "YuchenUI/text/TextUtils.h"
#include "YuchenUI/core/Assert.h"
#include "YuchenUI/core/Config.h"
#include "../generated/embedded_resources.h"
#include <unordered_map>
#include <functional>

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

//==========================================================================================
// Lifecycle

FontManager::FontManager()
    : m_isInitialized(false)
    , m_nextHandle(1)
    , m_fonts()
    , m_freeTypeLibrary(nullptr)
    , m_arialRegular(INVALID_FONT_HANDLE)
    , m_arialBold(INVALID_FONT_HANDLE)
    , m_arialNarrowRegular(INVALID_FONT_HANDLE)
    , m_arialNarrowBold(INVALID_FONT_HANDLE)
    , m_pingFangFont(INVALID_FONT_HANDLE)
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

    if (!initializeFreeType()) return false;

    initializeFonts();
    m_isInitialized = true;
    return true;
}

void FontManager::destroy()
{
    if (!m_isInitialized) return;

    // Clear global caches
    s_measureTextCache.clear();
    s_fontMetricsCache.clear();

    // Clear fonts (destroys FontFile, FontFace, FontCache)
    m_fonts.clear();
    m_arialRegular = m_arialBold = m_arialNarrowRegular = m_arialNarrowBold = m_pingFangFont = INVALID_FONT_HANDLE;
    
    // Reset counter
    m_nextHandle = 1;
    
    cleanupFreeType();
    m_isInitialized = false;
}

// Implement IFontProvider interface methods (already exist, just ensure they're public)

FontHandle FontManager::getDefaultFont() const
{
    return m_arialRegular;
}

FontHandle FontManager::getDefaultBoldFont() const
{
    return m_arialBold;
}

FontHandle FontManager::getDefaultCJKFont() const
{
    return m_pingFangFont;
}

//==========================================================================================
// FreeType Initialization

bool FontManager::initializeFreeType()
{
    FT_Error error = FT_Init_FreeType(&m_freeTypeLibrary);
    YUCHEN_ASSERT_MSG(error == FT_Err_Ok, "Failed to initialize FreeType");
    return error == FT_Err_Ok;
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
    // Load embedded Arial fonts from resources
    const Resources::ResourceData* arialRegularRes = Resources::findResource("fonts/Arial_Regular.ttf");
    const Resources::ResourceData* arialBoldRes = Resources::findResource("fonts/Arial_Bold.ttf");
    const Resources::ResourceData* arialNarrowRegularRes = Resources::findResource("fonts/ArialNarrow_Regular.ttf");
    const Resources::ResourceData* arialNarrowBoldRes = Resources::findResource("fonts/ArialNarrow_Bold.ttf");
    
    YUCHEN_ASSERT(arialRegularRes != nullptr);
    YUCHEN_ASSERT(arialBoldRes != nullptr);
    YUCHEN_ASSERT(arialNarrowRegularRes != nullptr);
    YUCHEN_ASSERT(arialNarrowBoldRes != nullptr);
    
    m_arialRegular = loadFontFromMemory(arialRegularRes->data, arialRegularRes->size, "Arial_Regular");
    m_arialBold = loadFontFromMemory(arialBoldRes->data, arialBoldRes->size, "Arial_Bold");
    m_arialNarrowRegular = loadFontFromMemory(arialNarrowRegularRes->data, arialNarrowRegularRes->size, "ArialNarrow_Regular");
    m_arialNarrowBold = loadFontFromMemory(arialNarrowBoldRes->data, arialNarrowBoldRes->size, "ArialNarrow_Bold");
    
    YUCHEN_ASSERT(m_arialRegular != INVALID_FONT_HANDLE);
    YUCHEN_ASSERT(m_arialBold != INVALID_FONT_HANDLE);
    YUCHEN_ASSERT(m_arialNarrowRegular != INVALID_FONT_HANDLE);
    YUCHEN_ASSERT(m_arialNarrowBold != INVALID_FONT_HANDLE);
    
    // Load platform-specific CJK font
#ifdef __APPLE__
    std::string pingFangPath = getCoreTextFontPath("PingFang SC Regular");
    if (!pingFangPath.empty()) m_pingFangFont = loadFontFromFile(pingFangPath.c_str(), "PingFang SC Regular");
#elif defined(_WIN32)
    const char* msyhPath = "C:\\Windows\\Fonts\\msyh.ttc";
    m_pingFangFont = loadFontFromFile(msyhPath, "Microsoft YaHei");
#endif
    
    // Fallback to Arial if system font unavailable
    if (m_pingFangFont == INVALID_FONT_HANDLE) m_pingFangFont = m_arialRegular;
}

#ifdef __APPLE__
std::string FontManager::getCoreTextFontPath(const char* fontName) const
{
    YUCHEN_ASSERT(fontName != nullptr);
    
    // Create CoreText font by name
    CTFontRef ctFont = CTFontCreateWithName(CFStringCreateWithCString(NULL, fontName, kCFStringEncodingUTF8), 0.0, NULL);
    if (!ctFont) return "";
    
    // Get font descriptor and extract file URL
    CTFontDescriptorRef descriptor = CTFontCopyFontDescriptor(ctFont);
    CFURLRef url = (CFURLRef)CTFontDescriptorCopyAttribute(descriptor, kCTFontURLAttribute);
    CFStringRef pathString = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);

    // Convert CFString to std::string
    char pathBuffer[1024];
    std::string path = (CFStringGetCString(pathString, pathBuffer, sizeof(pathBuffer), kCFStringEncodingUTF8)) ? pathBuffer : "";

    // Release CoreFoundation objects
    CFRelease(pathString);
    CFRelease(url);
    CFRelease(descriptor);
    CFRelease(ctFont);

    return path;
}
#endif

//==========================================================================================
// Font Selection

FontHandle FontManager::selectFontForCharacter(uint32_t codepoint) const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "FontManager not initialized");
    
    if (TextUtils::isWesternCharacter(codepoint))
    {
        return m_arialRegular;
    }
    else
    {
        return m_pingFangFont;
    }
}

//==========================================================================================
// Font Loading

FontHandle FontManager::loadFontFromFile(const char* path, const char* name)
{
    YUCHEN_ASSERT(path != nullptr);
    YUCHEN_ASSERT(m_fonts.size() < Config::Font::MAX_FONTS);

    FontHandle handle = generateFontHandle();
    
    // Expand font vector if necessary
    while (m_fonts.size() <= handle)
    {
        m_fonts.emplace_back();
    }

    FontEntry& entry = m_fonts[handle];
    entry.file = std::make_unique<FontFile>();
    entry.face = std::make_unique<FontFace>(m_freeTypeLibrary);
    entry.cache = std::make_unique<FontCache>();
    entry.name = name;

    if (!entry.file->loadFromFile(path, name)) return INVALID_FONT_HANDLE;
    if (!entry.face->createFromFontFile(*entry.file)) return INVALID_FONT_HANDLE;

    entry.isValid = true;
    return handle;
}

FontHandle FontManager::loadFontFromMemory(const void* data, size_t size, const char* name)
{
    YUCHEN_ASSERT(data != nullptr);
    YUCHEN_ASSERT(size > 0);
    YUCHEN_ASSERT_MSG(m_fonts.size() < Config::Font::MAX_FONTS, "Maximum number of fonts reached");

    FontHandle handle = generateFontHandle();
    
    // Expand font vector if necessary
    while (m_fonts.size() <= handle) { m_fonts.emplace_back(); }

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
    
    // Segment text by font requirements
    std::vector<TextSegment> segments = TextUtils::segmentText(text, m_arialRegular, m_pingFangFont);
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

FontHandle FontManager::generateFontHandle()
{
    FontHandle handle = m_nextHandle++;
    YUCHEN_ASSERT_MSG(handle != INVALID_FONT_HANDLE, "Font handle overflow");
    return handle;
}

FontEntry* FontManager::getFontEntry(FontHandle handle)
{
    if (handle >= m_fonts.size() || handle == INVALID_FONT_HANDLE) return nullptr;
    return &m_fonts[handle];
}

const FontEntry* FontManager::getFontEntry(FontHandle handle) const
{
    if (handle >= m_fonts.size() || handle == INVALID_FONT_HANDLE) return nullptr;
    return &m_fonts[handle];
}

} // namespace YuchenUI
