#include "YuchenUI/text/FontManager.h"
#include "YuchenUI/text/TextUtils.h"
#include "YuchenUI/resource/IResourceResolver.h"
#include "YuchenUI/core/Assert.h"
#include "YuchenUI/core/Config.h"
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <iostream>

#ifdef __APPLE__
    #include <CoreText/CoreText.h>
#endif

namespace YuchenUI {

struct MeasureTextKey
{
    std::string text;
    float fontSize;
    
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

struct MeasureTextKeyHash
{
    size_t operator()(const MeasureTextKey& key) const
    {
        std::hash<std::string> stringHasher;
        std::hash<float> floatHasher;
        return stringHasher(key.text) ^ (floatHasher(key.fontSize) << 1);
    }
};

struct FontMetricsKey
{
    FontHandle fontHandle;
    float fontSize;
    
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

struct FontMetricsKeyHash
{
    size_t operator()(const FontMetricsKey& key) const
    {
        std::hash<FontHandle> handleHasher;
        std::hash<float> floatHasher;
        return handleHasher(key.fontHandle) ^ (floatHasher(key.fontSize) << 1);
    }
};

static std::unordered_map<MeasureTextKey, Vec2, MeasureTextKeyHash> s_measureTextCache;
static std::unordered_map<FontMetricsKey, FontMetrics, FontMetricsKeyHash> s_fontMetricsCache;
static std::unordered_set<uint32_t> s_warnedMissingGlyphs;

FontManager::FontManager()
    : m_isInitialized(false)
    , m_resourceResolver(nullptr)
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

bool FontManager::initialize(IResourceResolver* resourceResolver)
{
    YUCHEN_ASSERT_MSG(!m_isInitialized, "FontManager already initialized");
    YUCHEN_ASSERT_MSG(resourceResolver != nullptr, "Resource resolver cannot be null");

    m_resourceResolver = resourceResolver;

    if (!initializeFreeType())
    {
        std::cerr << "[FontManager] ERROR: Failed to initialize FreeType" << std::endl;
        return false;
    }

    if (!m_fontDatabase.initialize(m_freeTypeLibrary, m_resourceResolver))
    {
        std::cerr << "[FontManager] ERROR: Failed to initialize font database" << std::endl;
        cleanupFreeType();
        return false;
    }

    int discoveredCount = m_fontDatabase.discoverAndRegisterFonts(m_fonts);
    
    if (discoveredCount == 0)
    {
        std::cerr << "[FontManager] WARNING: No fonts discovered!" << std::endl;
    }
    
    initializeFonts();
    loadCJKFont();
    loadSymbolFont();
    
    m_isInitialized = true;
    
    std::cout << "[FontManager] Initialized (" << m_fonts.size() << " fonts)" << std::endl;
    
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

void FontManager::initializeFonts()
{
    m_defaultRegularFont    = m_fontDatabase.getFontForRole(FontRole::DefaultRegular);
    m_defaultBoldFont       = m_fontDatabase.getFontForRole(FontRole::DefaultBold);
    m_defaultNarrowFont     = m_fontDatabase.getFontForRole(FontRole::DefaultNarrow);
    m_defaultNarrowBoldFont = m_fontDatabase.getFontForRole(FontRole::DefaultNarrow);
    if (m_defaultRegularFont == INVALID_FONT_HANDLE)
    {
        std::cerr << "[FontManager] ERROR: No default font found!" << std::endl;
        if (!m_fonts.empty() && m_fonts[0].isValid) m_defaultRegularFont = 0;
    }
    if (m_defaultBoldFont == INVALID_FONT_HANDLE)       m_defaultBoldFont = m_defaultRegularFont;
    if (m_defaultNarrowFont == INVALID_FONT_HANDLE)     m_defaultNarrowFont = m_defaultRegularFont;
    if (m_defaultNarrowBoldFont == INVALID_FONT_HANDLE) m_defaultNarrowBoldFont = m_defaultBoldFont;
    YUCHEN_ASSERT_MSG(m_defaultRegularFont != INVALID_FONT_HANDLE, "Failed to assign default regular font");
}

void FontManager::loadCJKFont()
{
    m_defaultCJKFont = m_fontDatabase.getFontForRole(FontRole::CJK);
    
    if (m_defaultCJKFont == INVALID_FONT_HANDLE)
    {
#ifdef __APPLE__
        std::string cjkPath = getCoreTextFontPath("PingFangSC-Regular");
        
        if (!cjkPath.empty())
        {
            size_t fontIndex = m_fonts.size();
            m_fonts.emplace_back();
            m_defaultCJKFont = m_fontDatabase.registerFont(
                cjkPath.c_str(),
                "PingFang SC",
                &m_fonts[fontIndex]
            );
        }
        
        if (m_defaultCJKFont == INVALID_FONT_HANDLE)
        {
            cjkPath = getCoreTextFontPath("STHeitiSC-Light");
            if (!cjkPath.empty())
            {
                size_t fontIndex = m_fonts.size();
                m_fonts.emplace_back();
                m_defaultCJKFont = m_fontDatabase.registerFont(
                    cjkPath.c_str(),
                    "Heiti SC",
                    &m_fonts[fontIndex]
                );
            }
        }
        
#elif defined(_WIN32)
        const char* cjkPath = "C:\\Windows\\Fonts\\msyh.ttc";
        std::ifstream testFile(cjkPath);
        
        if (testFile.good())
        {
            testFile.close();
            size_t fontIndex = m_fonts.size();
            m_fonts.emplace_back();
            m_defaultCJKFont = m_fontDatabase.registerFont(
                cjkPath,
                "Microsoft YaHei",
                &m_fonts[fontIndex]
            );
        }
        
        if (m_defaultCJKFont == INVALID_FONT_HANDLE)
        {
            cjkPath = "C:\\Windows\\Fonts\\simsun.ttc";
            testFile.open(cjkPath);
            if (testFile.good())
            {
                testFile.close();
                size_t fontIndex = m_fonts.size();
                m_fonts.emplace_back();
                m_defaultCJKFont = m_fontDatabase.registerFont(
                    cjkPath,
                    "SimSun",
                    &m_fonts[fontIndex]
                );
            }
        }
#endif
    }
    
    if (m_defaultCJKFont == INVALID_FONT_HANDLE)
    {
        std::cerr << "[FontManager] ERROR: Failed to load CJK font!" << std::endl;
        std::cerr << "[FontManager] Chinese text will NOT be displayed." << std::endl;
        
        m_defaultCJKFont = m_defaultRegularFont;
        
        YUCHEN_ASSERT_MSG(false, "CJK font loading failed - check system fonts");
    }

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
        size_t fontIndex = m_fonts.size();
        m_fonts.emplace_back();
        m_defaultSymbolFont = m_fontDatabase.registerFont(
            "C:\\Windows\\Fonts\\seguisym.ttf", "Segoe UI Symbol", &m_fonts[fontIndex]);
#endif
    }
    if (m_defaultSymbolFont == INVALID_FONT_HANDLE)
    { std::cerr << "[FontManager] Symbol font not available" << std::endl; }
}

#ifdef __APPLE__
std::string FontManager::getCoreTextFontPath(const char* fontName) const
{
    YUCHEN_ASSERT(fontName != nullptr);
    
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
    chain.addFont(m_defaultCJKFont);

    if (m_defaultSymbolFont != INVALID_FONT_HANDLE)
    {
        chain.addFont(m_defaultSymbolFont);
    }
    
    return chain;
}

FontFallbackChain FontManager::createTitleFallbackChain() const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "FontManager not initialized");
    
    return createBoldFallbackChain();
}

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
    
    uint32_t glyphIndex = FT_Get_Char_Index(face, codepoint);
    return glyphIndex != 0;
}

bool FontManager::hasGlyph(FontHandle handle, uint32_t codepoint) const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "FontManager not initialized");
    YUCHEN_ASSERT_MSG(handle != INVALID_FONT_HANDLE, "Invalid font handle");
    
    uint64_t cacheKey = (static_cast<uint64_t>(handle) << 32) | codepoint;
    
    auto it = m_glyphAvailabilityCache.find(cacheKey);
    if (it != m_glyphAvailabilityCache.end())
    {
        return it->second;
    }
    
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
    
    for (size_t i = 0; i < fallbackChain.fonts.size(); ++i)
    {
        FontHandle font = fallbackChain.fonts[i];
        if (font != INVALID_FONT_HANDLE && hasGlyph(font, codepoint))
        {
            return font;
        }
    }
    
    if (s_warnedMissingGlyphs.find(codepoint) == s_warnedMissingGlyphs.end())
    {
        s_warnedMissingGlyphs.insert(codepoint);
        
        bool isValidVisible = (codepoint >= 0x20 && codepoint != 0x7F) && !(codepoint >= 0xD800 && codepoint <= 0xDFFF) && (codepoint <= 0x10FFFF);
        if (isValidVisible)
        {
            const char* charType = "Unknown";
            if (TextUtils::isEmojiCharacter(codepoint))
            { charType = "Emoji"; }
            else if (TextUtils::isSymbolCharacter(codepoint))
            { charType = "Symbol"; }
            else if (TextUtils::isChineseCharacter(codepoint))
            { charType = "CJK"; }
            else if (TextUtils::isWesternCharacter(codepoint))
            { charType = "Western"; }
            std::string charStr = TextUtils::encodeUTF8(codepoint);
            std::cerr << "[FontManager] WARNING: No font supports " << charType << " character U+" << std::hex << std::uppercase << codepoint << std::dec << " ('" << charStr << "')" << std::endl;
        }
    }
    
    return fallbackChain.getPrimary();
}

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
    
    FontMetricsKey key(handle, fontSize);
    auto it = s_fontMetricsCache.find(key);
    if (it != s_fontMetricsCache.end())
    {
        return it->second;
    }
    
    const FontEntry* entry = getFontEntry(handle);
    YUCHEN_ASSERT_MSG(entry && entry->isValid, "Invalid font entry");
    
    FontMetrics metrics = entry->face->getMetrics(fontSize);
    YUCHEN_ASSERT_MSG(metrics.isValid(), "Invalid font metrics");
    
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
    
    MeasureTextKey key(text, fontSize);
    auto it = s_measureTextCache.find(key);
    if (it != s_measureTextCache.end()) return it->second;
    
    FontFallbackChain fallbackChain = createDefaultFallbackChain();
    
    std::vector<TextSegment> segments = TextUtils::segmentTextWithFallback(text, fallbackChain, const_cast<FontManager*>(this));
    YUCHEN_ASSERT(!segments.empty());
    
    float totalWidth = 0.0f;
    float maxHeight = 0.0f;
    
    hb_buffer_t* buffer = hb_buffer_create();
    
    for (const auto& segment : segments)
    {
        YUCHEN_ASSERT_MSG(isValidFont(segment.fontHandle), "Invalid font handle in text segment");
        
        const FontEntry* entry = getFontEntry(segment.fontHandle);
        if (!entry || !entry->isValid) continue;
        
        hb_font_t* hbFont = entry->cache->getHarfBuzzFont(*entry->face, fontSize);
        if (!hbFont) continue;
        
        hb_buffer_clear_contents(buffer);
        hb_buffer_set_direction(buffer, HB_DIRECTION_LTR);
        
        hb_script_t script = TextUtils::detectTextScript(segment.text.c_str());
        hb_buffer_set_script(buffer, script);
        
        const char* language = TextUtils::getLanguageForScript(script);
        hb_buffer_set_language(buffer, hb_language_from_string(language, -1));
        
        hb_buffer_add_utf8(buffer, segment.text.c_str(), -1, 0, -1);
        
        hb_feature_t features[1];
        features[0].tag = HB_TAG('k','e','r','n');
        features[0].value = 0;
        features[0].start = 0;
        features[0].end = (unsigned int)-1;
        
        hb_shape(hbFont, buffer, features, 1);
        
        unsigned int glyphCount = 0;
        hb_glyph_position_t* glyphPositions = hb_buffer_get_glyph_positions(buffer, &glyphCount);
        
        float segmentWidth = 0.0f;
        for (unsigned int i = 0; i < glyphCount; ++i)
        {
            segmentWidth += glyphPositions[i].x_advance / 64.0f;
        }
        
        totalWidth += segmentWidth;
        
        float segmentHeight = getTextHeight(segment.fontHandle, fontSize);
        maxHeight = std::max(maxHeight, segmentHeight);
    }
    
    hb_buffer_destroy(buffer);
    
    Vec2 result(totalWidth, maxHeight);
    YUCHEN_ASSERT_MSG(result.isValid(), "Invalid text measurement result");
    
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

}
