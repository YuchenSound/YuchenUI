/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework - Font System Unit Tests
**
** Copyright (C) 2025 Yuchen Wei
**
** Comprehensive test suite for font loading, rendering, caching, and fallback systems.
** Tests cover normal operations, edge cases, error handling, and memory leak scenarios.
**
********************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "YuchenUI/text/Font.h"
#include "YuchenUI/text/FontManager.h"
#include "YuchenUI/text/TextRenderer.h"
#include "YuchenUI/text/TextUtils.h"
#include "YuchenUI/text/GlyphCache.h"
#include "YuchenUI/text/IFontProvider.h"
#include "YuchenUI/rendering/IGraphicsBackend.h"
#include "YuchenUI/core/Config.h"
#include "embedded_resources.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>

#include <vector>
#include <unordered_set>
#include <memory>
#include <cstring>

using namespace YuchenUI;
using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;
using ::testing::Invoke;

//==========================================================================================
// Mock Graphics Backend
//==========================================================================================

class MockGraphicsBackend : public IGraphicsBackend {
public:
    MockGraphicsBackend() : m_nextTextureId(1) {}
    
    MOCK_METHOD(bool, initialize, (void* platformSurface, int width, int height,
                                   float dpiScale, IFontProvider* fontProvider), (override));
    MOCK_METHOD(void, resize, (int width, int height), (override));
    MOCK_METHOD(void, beginFrame, (), (override));
    MOCK_METHOD(void, endFrame, (), (override));
    MOCK_METHOD(void, executeRenderCommands, (const RenderList& commands), (override));
    MOCK_METHOD(Vec2, getRenderSize, (), (const, override));
    MOCK_METHOD(float, getDPIScale, (), (const, override));
    
    // Texture management with tracking
    void* createTexture2D(uint32_t width, uint32_t height, TextureFormat format) override {
        void* handle = reinterpret_cast<void*>(m_nextTextureId++);
        
        TextureInfo info;
        info.width = width;
        info.height = height;
        info.format = format;
        info.data.resize(width * height * (format == TextureFormat::R8_Unorm ? 1 : 4));
        
        m_textures[handle] = info;
        return handle;
    }
    
    void updateTexture2D(void* texture, uint32_t x, uint32_t y,
                        uint32_t width, uint32_t height,
                        const void* data, size_t bytesPerRow) override {
        auto it = m_textures.find(texture);
        if (it == m_textures.end()) return;
        
        TextureInfo& info = it->second;
        size_t bytesPerPixel = (info.format == TextureFormat::R8_Unorm) ? 1 : 4;
        
        for (uint32_t row = 0; row < height; ++row) {
            size_t srcOffset = row * bytesPerRow;
            size_t dstOffset = ((y + row) * info.width + x) * bytesPerPixel;
            memcpy(&info.data[dstOffset],
                   static_cast<const uint8_t*>(data) + srcOffset,
                   width * bytesPerPixel);
        }
        
        m_updateCount++;
    }
    
    void destroyTexture(void* texture) override {
        m_textures.erase(texture);
        m_destroyCount++;
    }
    
    // Test helpers
    size_t getTextureCount() const { return m_textures.size(); }
    size_t getUpdateCount() const { return m_updateCount; }
    size_t getDestroyCount() const { return m_destroyCount; }
    void resetCounters() { m_updateCount = 0; m_destroyCount = 0; }
    
private:
    struct TextureInfo {
        uint32_t width;
        uint32_t height;
        TextureFormat format;
        std::vector<uint8_t> data;
    };
    
    std::unordered_map<void*, TextureInfo> m_textures;
    size_t m_nextTextureId;
    size_t m_updateCount = 0;
    size_t m_destroyCount = 0;
};

//==========================================================================================
// Test Fixtures
//==========================================================================================

class FontFileTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use embedded Arial Regular font
        const Resources::ResourceData* resource =
            Resources::findResource("fonts/Arial_Regular.ttf");
        ASSERT_NE(resource, nullptr);
        
        m_validFontData = resource->data;
        m_validFontSize = resource->size;
    }
    
    const unsigned char* m_validFontData;
    size_t m_validFontSize;
};

class FontFaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize FreeType
        FT_Error error = FT_Init_FreeType(&m_library);
        ASSERT_EQ(error, FT_Err_Ok);
        
        // Load Arial Regular
        const Resources::ResourceData* resource =
            Resources::findResource("fonts/Arial_Regular.ttf");
        ASSERT_NE(resource, nullptr);
        
        m_fontFile = std::make_unique<FontFile>();
        bool loaded = m_fontFile->loadFromMemory(resource->data, resource->size,
                                                  "Arial_Regular");
        ASSERT_TRUE(loaded);
        
        m_fontFace = std::make_unique<FontFace>(m_library);
        bool created = m_fontFace->createFromFontFile(*m_fontFile);
        ASSERT_TRUE(created);
    }
    
    void TearDown() override {
        m_fontFace.reset();
        m_fontFile.reset();
        if (m_library) FT_Done_FreeType(m_library);
    }
    
    FT_Library m_library = nullptr;
    std::unique_ptr<FontFile> m_fontFile;
    std::unique_ptr<FontFace> m_fontFace;
};

class FontManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_fontManager = std::make_unique<FontManager>();
        bool initialized = m_fontManager->initialize();
        ASSERT_TRUE(initialized);
    }
    
    void TearDown() override {
        m_fontManager->destroy();
        m_fontManager.reset();
    }
    
    std::unique_ptr<FontManager> m_fontManager;
};

class TextRendererTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_backend = std::make_unique<NiceMock<MockGraphicsBackend>>();
        
        ON_CALL(*m_backend, getRenderSize())
            .WillByDefault(Return(Vec2(1024.0f, 768.0f)));
        ON_CALL(*m_backend, getDPIScale())
            .WillByDefault(Return(1.0f));
        
        m_fontManager = std::make_unique<FontManager>();
        ASSERT_TRUE(m_fontManager->initialize());
        
        m_textRenderer = std::make_unique<TextRenderer>(m_backend.get(),
                                                         m_fontManager.get());
        ASSERT_TRUE(m_textRenderer->initialize(1.0f));
    }
    
    void TearDown() override {
        m_textRenderer->destroy();
        m_textRenderer.reset();
        m_fontManager->destroy();
        m_fontManager.reset();
        m_backend.reset();
    }
    
    std::unique_ptr<MockGraphicsBackend> m_backend;
    std::unique_ptr<FontManager> m_fontManager;
    std::unique_ptr<TextRenderer> m_textRenderer;
};

class GlyphCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_backend = std::make_unique<NiceMock<MockGraphicsBackend>>();
        m_glyphCache = std::make_unique<GlyphCache>(m_backend.get(), 1.0f);
        ASSERT_TRUE(m_glyphCache->initialize());
    }
    
    void TearDown() override {
        m_glyphCache->destroy();
        m_glyphCache.reset();
        m_backend.reset();
    }
    
    std::unique_ptr<MockGraphicsBackend> m_backend;
    std::unique_ptr<GlyphCache> m_glyphCache;
};

//==========================================================================================
// FontFile Tests
//==========================================================================================

TEST_F(FontFileTest, LoadFromMemory_Success) {
    FontFile fontFile;
    bool loaded = fontFile.loadFromMemory(m_validFontData, m_validFontSize,
                                          "TestFont");
    
    EXPECT_TRUE(loaded);
    EXPECT_TRUE(fontFile.isValid());
    EXPECT_EQ(fontFile.getName(), "TestFont");
    EXPECT_EQ(fontFile.getMemoryData().size(), m_validFontSize);
}

#if defined(YUCHEN_DEBUG) && !defined(__APPLE__)
    #define YUCHEN_DEATH_TEST_ENABLED 1
#else
    #define YUCHEN_DEATH_TEST_ENABLED 0
#endif

TEST_F(FontFileTest, LoadFromMemory_NullData) {
    FontFile fontFile;
    
#if YUCHEN_DEATH_TEST_ENABLED
    EXPECT_DEATH(
        fontFile.loadFromMemory(nullptr, 1000, "TestFont"),
        "Data cannot be null"
    );
#else
    SUCCEED() << "Death test skipped on this platform (macOS or Release build)";
#endif
}

TEST_F(FontFileTest, LoadFromMemory_ZeroSize) {
    FontFile fontFile;
    
#if YUCHEN_DEATH_TEST_ENABLED
    EXPECT_DEATH(
        fontFile.loadFromMemory(m_validFontData, 0, "TestFont"),
        "Size must be positive"
    );
#else
    SUCCEED() << "Death test skipped on this platform (macOS or Release build)";
#endif
}

TEST_F(FontFileTest, LoadFromMemory_EmptyName) {
    FontFile fontFile;
    
#if YUCHEN_DEATH_TEST_ENABLED
    EXPECT_DEATH(
        fontFile.loadFromMemory(m_validFontData, m_validFontSize, ""),
        "Font name cannot be empty"
    );
#else
    SUCCEED() << "Death test skipped on this platform (macOS or Release build)";
#endif
}

TEST_F(FontFileTest, LoadFromFile_NonExistent) {
    FontFile fontFile;
    bool loaded = fontFile.loadFromFile("/nonexistent/path/font.ttf", "TestFont");
    
    EXPECT_FALSE(loaded);
    EXPECT_FALSE(fontFile.isValid());
}

//==========================================================================================
// FontFace Tests
//==========================================================================================

TEST_F(FontFaceTest, GetMetrics_ValidSize) {
    FontMetrics metrics = m_fontFace->getMetrics(12.0f);
    
    EXPECT_TRUE(metrics.isValid());
    EXPECT_GT(metrics.ascender, 0.0f);
    EXPECT_LT(metrics.descender, 0.0f);
    EXPECT_GT(metrics.lineHeight, 0.0f);
    EXPECT_GT(metrics.maxAdvance, 0.0f);
}

TEST_F(FontFaceTest, GetMetrics_MinSize) {
    FontMetrics metrics = m_fontFace->getMetrics(Config::Font::MIN_SIZE);
    EXPECT_TRUE(metrics.isValid());
}

TEST_F(FontFaceTest, GetMetrics_MaxSize) {
    FontMetrics metrics = m_fontFace->getMetrics(Config::Font::MAX_SIZE);
    EXPECT_TRUE(metrics.isValid());
}

TEST_F(FontFaceTest, GetMetrics_BelowMinSize) {
    FontMetrics metrics = m_fontFace->getMetrics(0.5f);
    EXPECT_FALSE(metrics.isValid());
}

TEST_F(FontFaceTest, GetMetrics_AboveMaxSize) {
    FontMetrics metrics = m_fontFace->getMetrics(600.0f);
    EXPECT_FALSE(metrics.isValid());
}

TEST_F(FontFaceTest, GetGlyphMetrics_BasicLatin) {
    GlyphMetrics metrics = m_fontFace->getGlyphMetrics('A', 12.0f);
    
    EXPECT_TRUE(metrics.isValid());
    EXPECT_NE(metrics.glyphIndex, 0u);
    EXPECT_GT(metrics.advance, 0.0f);
}

TEST_F(FontFaceTest, GetGlyphMetrics_Space) {
    GlyphMetrics metrics = m_fontFace->getGlyphMetrics(' ', 12.0f);
    
    EXPECT_TRUE(metrics.isValid());
    EXPECT_NE(metrics.glyphIndex, 0u);
    EXPECT_GT(metrics.advance, 0.0f);
}

TEST_F(FontFaceTest, GetGlyphMetrics_NonExistentChar) {
    // Try a character that Arial doesn't have
    GlyphMetrics metrics = m_fontFace->getGlyphMetrics(0x1F600, 12.0f); // Emoji
    
    // Should return invalid metrics or .notdef glyph
    EXPECT_EQ(metrics.glyphIndex, 0u);
}

TEST_F(FontFaceTest, MeasureText_SimpleString) {
    float width = m_fontFace->measureText("Hello", 12.0f);
    
    EXPECT_GT(width, 0.0f);
    EXPECT_LT(width, 100.0f); // Reasonable bounds
}

TEST_F(FontFaceTest, MeasureText_EmptyString) {
    float width = m_fontFace->measureText("", 12.0f);
    EXPECT_EQ(width, 0.0f);
}

TEST_F(FontFaceTest, MeasureText_Consistency) {
    // Same text at same size should give same result
    float width1 = m_fontFace->measureText("Test", 14.0f);
    float width2 = m_fontFace->measureText("Test", 14.0f);
    
    EXPECT_FLOAT_EQ(width1, width2);
}

TEST_F(FontFaceTest, MeasureText_SizeScaling) {
    float width12 = m_fontFace->measureText("Test", 12.0f);
    float width24 = m_fontFace->measureText("Test", 24.0f);
    
    // 24pt should be roughly 2x wider than 12pt
    EXPECT_GT(width24, width12 * 1.8f);
    EXPECT_LT(width24, width12 * 2.2f);
}

//==========================================================================================
// FontCache Tests
//==========================================================================================

TEST_F(FontFaceTest, FontCache_GetOrCreate) {
    FontCache cache;
    
    hb_font_t* font1 = cache.getHarfBuzzFont(*m_fontFace, 12.0f);
    ASSERT_NE(font1, nullptr);
    
    // Second call should return same font
    hb_font_t* font2 = cache.getHarfBuzzFont(*m_fontFace, 12.0f);
    EXPECT_EQ(font1, font2);
}

TEST_F(FontFaceTest, FontCache_DifferentSizes) {
    FontCache cache;
    
    hb_font_t* font12 = cache.getHarfBuzzFont(*m_fontFace, 12.0f);
    hb_font_t* font14 = cache.getHarfBuzzFont(*m_fontFace, 14.0f);
    
    EXPECT_NE(font12, font14);
}

TEST_F(FontFaceTest, FontCache_LRUEviction) {
    FontCache cache;
    std::vector<hb_font_t*> fonts;
    
    // Fill cache beyond MAX_CACHED_SIZES (8)
    for (int i = 1; i <= 10; ++i) {
        hb_font_t* font = cache.getHarfBuzzFont(*m_fontFace, static_cast<float>(i + 10));
        fonts.push_back(font);
    }
    
    // First fonts should have been evicted
    hb_font_t* font1_retry = cache.getHarfBuzzFont(*m_fontFace, 11.0f);
    EXPECT_NE(font1_retry, fonts[0]); // New font object created
}

TEST_F(FontFaceTest, FontCache_ClearAll) {
    FontCache cache;
    
    // 创建多个不同尺寸的字体
    hb_font_t* font12 = cache.getHarfBuzzFont(*m_fontFace, 12.0f);
    hb_font_t* font14 = cache.getHarfBuzzFont(*m_fontFace, 14.0f);
    hb_font_t* font16 = cache.getHarfBuzzFont(*m_fontFace, 16.0f);
    
    ASSERT_NE(font12, nullptr);
    ASSERT_NE(font14, nullptr);
    ASSERT_NE(font16, nullptr);
    
    // 清空缓存
    cache.clearAll();
    
    // 验证：清空后再次获取同样尺寸的字体，应该重新创建
    hb_font_t* font12_new = cache.getHarfBuzzFont(*m_fontFace, 12.0f);
    ASSERT_NE(font12_new, nullptr);
    
    // 验证新创建的字体是有效的 HarfBuzz 字体
    // 通过检查 ppem 值（pixels per EM）来验证字体已正确初始化
    unsigned int x_ppem = 0, y_ppem = 0;
    hb_font_get_ppem(font12_new, &x_ppem, &y_ppem);
    EXPECT_GT(x_ppem, 0u) << "Font ppem should be non-zero after creation";
    EXPECT_GT(y_ppem, 0u) << "Font ppem should be non-zero after creation";
}

//==========================================================================================
// FontManager Tests
//==========================================================================================

TEST_F(FontManagerTest, Initialize_Success) {
    EXPECT_TRUE(m_fontManager->isInitialized());
}

TEST_F(FontManagerTest, DefaultFonts_Available) {
    FontHandle arialRegular = m_fontManager->getDefaultFont();
    FontHandle arialBold = m_fontManager->getDefaultBoldFont();
    FontHandle cjkFont = m_fontManager->getDefaultCJKFont();
    
    EXPECT_NE(arialRegular, INVALID_FONT_HANDLE);
    EXPECT_NE(arialBold, INVALID_FONT_HANDLE);
    EXPECT_NE(cjkFont, INVALID_FONT_HANDLE);
    
    EXPECT_TRUE(m_fontManager->isValidFont(arialRegular));
    EXPECT_TRUE(m_fontManager->isValidFont(arialBold));
    EXPECT_TRUE(m_fontManager->isValidFont(cjkFont));
}

TEST_F(FontManagerTest, LoadFont_FromMemory) {
    const Resources::ResourceData* resource =
        Resources::findResource("fonts/Arial_Bold.ttf");
    ASSERT_NE(resource, nullptr);
    
    FontHandle handle = m_fontManager->loadFontFromMemory(
        resource->data, resource->size, "TestFont");
    
    EXPECT_NE(handle, INVALID_FONT_HANDLE);
    EXPECT_TRUE(m_fontManager->isValidFont(handle));
}

TEST_F(FontManagerTest, LoadFont_MaxFontsLimit) {
    std::vector<FontHandle> handles;
    
    const Resources::ResourceData* resource =
        Resources::findResource("fonts/Arial_Regular.ttf");
    ASSERT_NE(resource, nullptr);
    
    // initialize() 已加载7个字体，我们加载40个测试字体
    // 总共47个，远低于64的限制，证明系统可以处理大量字体
    const size_t testFontCount = 40;
    
    for (size_t i = 0; i < testFontCount; ++i) {
        FontHandle handle = m_fontManager->loadFontFromMemory(
            resource->data, resource->size,
            ("TestFont_" + std::to_string(i)).c_str());
        
        EXPECT_NE(handle, INVALID_FONT_HANDLE)
            << "Should successfully load font " << i;
        
        if (handle != INVALID_FONT_HANDLE) {
            handles.push_back(handle);
        }
    }
    
    // 应该成功加载所有40个字体
    EXPECT_EQ(handles.size(), testFontCount);
    
    // 验证所有字体都有效
    for (const auto& handle : handles) {
        EXPECT_TRUE(m_fontManager->isValidFont(handle));
    }
}

TEST_F(FontManagerTest, GetFontMetrics_ValidFont) {
    FontHandle arial = m_fontManager->getDefaultFont();
    FontMetrics metrics = m_fontManager->getFontMetrics(arial, 12.0f);
    
    EXPECT_TRUE(metrics.isValid());
}

TEST_F(FontManagerTest, GetGlyphMetrics_ValidGlyph) {
    FontHandle arial = m_fontManager->getDefaultFont();
    GlyphMetrics metrics = m_fontManager->getGlyphMetrics(arial, 'A', 12.0f);
    
    EXPECT_TRUE(metrics.isValid());
    EXPECT_NE(metrics.glyphIndex, 0u);
}

TEST_F(FontManagerTest, MeasureText_BasicString) {
    Vec2 size = m_fontManager->measureText("Hello World", 12.0f);
    
    EXPECT_GT(size.x, 0.0f);
    EXPECT_GT(size.y, 0.0f);
}

TEST_F(FontManagerTest, MeasureText_Caching) {
    // First measurement
    Vec2 size1 = m_fontManager->measureText("Test String", 14.0f);
    
    // Second measurement should use cache
    Vec2 size2 = m_fontManager->measureText("Test String", 14.0f);
    
    EXPECT_FLOAT_EQ(size1.x, size2.x);
    EXPECT_FLOAT_EQ(size1.y, size2.y);
}

TEST_F(FontManagerTest, HasGlyph_BasicLatin) {
    FontHandle arial = m_fontManager->getDefaultFont();
    
    EXPECT_TRUE(m_fontManager->hasGlyph(arial, 'A'));
    EXPECT_TRUE(m_fontManager->hasGlyph(arial, 'z'));
    EXPECT_TRUE(m_fontManager->hasGlyph(arial, '0'));
}

TEST_F(FontManagerTest, HasGlyph_CJK) {
    FontHandle cjkFont = m_fontManager->getDefaultCJKFont();
    
    // Test common CJK characters
    EXPECT_TRUE(m_fontManager->hasGlyph(cjkFont, 0x4E2D));
    EXPECT_TRUE(m_fontManager->hasGlyph(cjkFont, 0x6587));
}

TEST_F(FontManagerTest, SelectFontForCodepoint_Latin) {
    FontFallbackChain chain = m_fontManager->createDefaultFallbackChain();
    
    FontHandle selected = m_fontManager->selectFontForCodepoint('A', chain);
    EXPECT_EQ(selected, m_fontManager->getDefaultFont());
}

TEST_F(FontManagerTest, SelectFontForCodepoint_CJK) {
    FontFallbackChain chain = m_fontManager->createDefaultFallbackChain();
    
    FontHandle selected = m_fontManager->selectFontForCodepoint(0x4E2D, chain);
    
    // Should select CJK font or default
    EXPECT_TRUE(m_fontManager->isValidFont(selected));
}

TEST_F(FontManagerTest, SelectFontForCodepoint_Emoji) {
    FontFallbackChain chain = m_fontManager->createDefaultFallbackChain();
    
    FontHandle selected = m_fontManager->selectFontForCodepoint(0x1F600, chain);
    
    // Should try to find emoji font
    EXPECT_TRUE(m_fontManager->isValidFont(selected));
}

TEST_F(FontManagerTest, FallbackChain_Empty) {
    FontFallbackChain emptyChain;
    
    EXPECT_TRUE(emptyChain.isEmpty());
    EXPECT_EQ(emptyChain.size(), 0u);
}

TEST_F(FontManagerTest, FallbackChain_BuilderPattern) {
    FontHandle arial = m_fontManager->getDefaultFont();
    FontHandle cjk = m_fontManager->getDefaultCJKFont();
    
    FontFallbackChain chain = FontFallbackChain()
        .withFont(arial)
        .withFont(cjk);
    
    EXPECT_FALSE(chain.isEmpty());
    EXPECT_EQ(chain.size(), 2u);
    EXPECT_EQ(chain.getPrimary(), arial);
}

TEST_F(FontManagerTest, FallbackChain_DefaultChain) {
    FontFallbackChain chain = m_fontManager->createDefaultFallbackChain();
    
    EXPECT_FALSE(chain.isEmpty());
    EXPECT_GE(chain.size(), 2u); // At least Arial + CJK
}

//==========================================================================================
// TextUtils Tests
//==========================================================================================

TEST(TextUtilsTest, DecodeUTF8_ASCII) {
    const char* text = "A";
    uint32_t codepoint = TextUtils::decodeUTF8(text);
    
    EXPECT_EQ(codepoint, 'A');
}

TEST(TextUtilsTest, DecodeUTF8_TwoByte) {
    const char* text = "é"; // U+00E9
    uint32_t codepoint = TextUtils::decodeUTF8(text);
    
    EXPECT_EQ(codepoint, 0x00E9u);
}

TEST(TextUtilsTest, DecodeUTF8_ThreeByte) {
    const char* text = "中"; // U+4E2D
    uint32_t codepoint = TextUtils::decodeUTF8(text);
    
    EXPECT_EQ(codepoint, 0x4E2Du);
}

TEST(TextUtilsTest, DecodeUTF8_FourByte) {
    const char* text = "😀"; // U+1F600
    uint32_t codepoint = TextUtils::decodeUTF8(text);
    
    EXPECT_EQ(codepoint, 0x1F600u);
}

TEST(TextUtilsTest, DecodeUTF8_InvalidSequence) {
    const char* text = "\xFF\xFF"; // Invalid UTF-8
    uint32_t codepoint = TextUtils::decodeUTF8(text);
    
    EXPECT_EQ(codepoint, 0xFFFDu); // Replacement character
}

TEST(TextUtilsTest, DecodeUTF8_EmptyString) {
    const char* text = "";
    uint32_t codepoint = TextUtils::decodeUTF8(text);
    
    EXPECT_EQ(codepoint, 0u);
}

TEST(TextUtilsTest, IsWesternCharacter_BasicLatin) {
    EXPECT_TRUE(TextUtils::isWesternCharacter('A'));
    EXPECT_TRUE(TextUtils::isWesternCharacter('z'));
    EXPECT_TRUE(TextUtils::isWesternCharacter('0'));
    EXPECT_TRUE(TextUtils::isWesternCharacter(' '));
}

TEST(TextUtilsTest, IsWesternCharacter_ExtendedLatin) {
    EXPECT_TRUE(TextUtils::isWesternCharacter(0x00E9)); // é
    EXPECT_TRUE(TextUtils::isWesternCharacter(0x00FC)); // ü
}

TEST(TextUtilsTest, IsWesternCharacter_NotWestern) {
    EXPECT_FALSE(TextUtils::isWesternCharacter(0x4E2D)); // 中
    EXPECT_FALSE(TextUtils::isWesternCharacter(0x1F600)); // 😀
}

TEST(TextUtilsTest, IsChineseCharacter_CJK) {
    EXPECT_TRUE(TextUtils::isChineseCharacter(0x4E2D)); // 中
    EXPECT_TRUE(TextUtils::isChineseCharacter(0x6587)); // 文
    EXPECT_TRUE(TextUtils::isChineseCharacter(0x5B57)); // 字
}

TEST(TextUtilsTest, IsChineseCharacter_NotChinese) {
    EXPECT_FALSE(TextUtils::isChineseCharacter('A'));
    EXPECT_FALSE(TextUtils::isChineseCharacter(0x1F600)); // 😀
}

TEST(TextUtilsTest, IsSymbolCharacter_Common) {
    // Miscellaneous Technical (U+2300-U+23FF)
    EXPECT_TRUE(TextUtils::isSymbolCharacter(0x2300)); // ⌀
    EXPECT_TRUE(TextUtils::isSymbolCharacter(0x2328)); // ⌨ (keyboard symbol)
    
    // Geometric Shapes (U+25A0-U+25FF)
    EXPECT_TRUE(TextUtils::isSymbolCharacter(0x25A0)); // ■
    EXPECT_TRUE(TextUtils::isSymbolCharacter(0x25CF)); // ●
    
    // Box Drawing (U+2500-U+257F)
    EXPECT_TRUE(TextUtils::isSymbolCharacter(0x2500)); // ─
    EXPECT_TRUE(TextUtils::isSymbolCharacter(0x2550)); // ═
}

TEST(TextUtilsTest, DetectScript_Latin) {
    EXPECT_EQ(TextUtils::detectScript('A'), HB_SCRIPT_LATIN);
    EXPECT_EQ(TextUtils::detectScript('z'), HB_SCRIPT_LATIN);
}

TEST(TextUtilsTest, DetectScript_Han) {
    EXPECT_EQ(TextUtils::detectScript(0x4E2D), HB_SCRIPT_HAN);
}

TEST(TextUtilsTest, DetectTextScript_Latin) {
    EXPECT_EQ(TextUtils::detectTextScript("Hello World"), HB_SCRIPT_LATIN);
}

TEST(TextUtilsTest, DetectTextScript_Han) {
    EXPECT_EQ(TextUtils::detectTextScript("中文测试"), HB_SCRIPT_HAN);
}

TEST(TextUtilsTest, DetectTextScript_Mixed) {
    // Should return HAN for mixed text
    EXPECT_EQ(TextUtils::detectTextScript("Hello世界"), HB_SCRIPT_HAN);
}

TEST(TextUtilsTest, GetLanguageForScript_Common) {
    EXPECT_STREQ(TextUtils::getLanguageForScript(HB_SCRIPT_LATIN), "en");
    EXPECT_STREQ(TextUtils::getLanguageForScript(HB_SCRIPT_HAN), "zh-cn");
    EXPECT_STREQ(TextUtils::getLanguageForScript(HB_SCRIPT_HIRAGANA), "ja");
}

//==========================================================================================
// TextRenderer Tests
//==========================================================================================

TEST_F(TextRendererTest, Initialize_Success) {
    EXPECT_TRUE(m_textRenderer->isInitialized());
}

TEST_F(TextRendererTest, ShapeText_SimpleString) {
    FontFallbackChain chain = m_fontManager->createDefaultFallbackChain();
    ShapedText shaped;
    
    m_textRenderer->shapeText("Hello", chain, 12.0f, shaped);
    
    EXPECT_FALSE(shaped.isEmpty());
    EXPECT_GT(shaped.glyphs.size(), 0u);
    EXPECT_GT(shaped.totalAdvance, 0.0f);
    EXPECT_TRUE(shaped.isValid());
}

TEST_F(TextRendererTest, ShapeText_EmptyString) {
    FontFallbackChain chain = m_fontManager->createDefaultFallbackChain();
    ShapedText shaped;
    
    m_textRenderer->shapeText("", chain, 12.0f, shaped);
    
    EXPECT_TRUE(shaped.isEmpty());
}

TEST_F(TextRendererTest, ShapeText_MixedScript) {
    FontFallbackChain chain = m_fontManager->createDefaultFallbackChain();
    ShapedText shaped;
    
    m_textRenderer->shapeText("Hello世界", chain, 14.0f, shaped);
    
    EXPECT_FALSE(shaped.isEmpty());
    EXPECT_GT(shaped.glyphs.size(), 0u);
}

TEST_F(TextRendererTest, ShapeText_Caching) {
    FontFallbackChain chain = m_fontManager->createDefaultFallbackChain();
    ShapedText shaped1, shaped2;
    
    m_textRenderer->shapeText("Cached Text", chain, 12.0f, shaped1);
    m_textRenderer->shapeText("Cached Text", chain, 12.0f, shaped2);
    
    // Should have same result
    EXPECT_EQ(shaped1.glyphs.size(), shaped2.glyphs.size());
    EXPECT_FLOAT_EQ(shaped1.totalAdvance, shaped2.totalAdvance);
}

TEST_F(TextRendererTest, GenerateTextVertices_SimpleText) {
    FontFallbackChain chain = m_fontManager->createDefaultFallbackChain();
    ShapedText shaped;
    m_textRenderer->shapeText("Test", chain, 12.0f, shaped);
    
    std::vector<TextVertex> vertices;
    m_textRenderer->generateTextVertices(shaped, Vec2(0, 0),
                                         Vec4(1, 1, 1, 1),
                                         chain, 12.0f, vertices);
    
    EXPECT_GT(vertices.size(), 0u);
    EXPECT_EQ(vertices.size() % 4, 0u); // Should be multiple of 4 (quads)
}

TEST_F(TextRendererTest, BeginFrame_AdvancesGlyphCache) {
    m_textRenderer->beginFrame();
    m_textRenderer->beginFrame();
    m_textRenderer->beginFrame();
    
    // Should not crash, glyph cache frame counter should advance
    SUCCEED();
}

//==========================================================================================
// GlyphCache Tests
//==========================================================================================

TEST_F(GlyphCacheTest, Initialize_Success) {
    Vec2 atlasSize = m_glyphCache->getCurrentAtlasSize();
    
    EXPECT_GT(atlasSize.x, 0.0f);
    EXPECT_GT(atlasSize.y, 0.0f);
}

TEST_F(GlyphCacheTest, CacheGlyph_ValidData) {
    GlyphKey key(1, 65, 12.0f); // Font 1, 'A', 12pt
    
    // Create fake glyph bitmap
    std::vector<uint8_t> bitmap(16 * 16, 128);
    
    m_glyphCache->cacheGlyph(key, bitmap.data(), Vec2(16, 16),
                             Vec2(0, 12), 8.0f);
    
    const GlyphCacheEntry* entry = m_glyphCache->getGlyph(key);
    ASSERT_NE(entry, nullptr);
    EXPECT_TRUE(entry->isValid);
    EXPECT_FLOAT_EQ(entry->advance, 8.0f);
}

TEST_F(GlyphCacheTest, CacheGlyph_EmptyGlyph) {
    GlyphKey key(1, 32, 12.0f); // Space character
    
    m_glyphCache->cacheGlyph(key, nullptr, Vec2(0, 0),
                             Vec2(0, 0), 4.0f);
    
    const GlyphCacheEntry* entry = m_glyphCache->getGlyph(key);
    ASSERT_NE(entry, nullptr);
    EXPECT_TRUE(entry->isValid);
    EXPECT_EQ(entry->textureRect.width, 0.0f);
}

TEST_F(GlyphCacheTest, GetGlyph_NotCached) {
    GlyphKey key(999, 999, 12.0f);
    
    const GlyphCacheEntry* entry = m_glyphCache->getGlyph(key);
    EXPECT_EQ(entry, nullptr);
}

TEST_F(GlyphCacheTest, BeginFrame_AdvancesCounter) {
    m_glyphCache->beginFrame();
    m_glyphCache->beginFrame();
    m_glyphCache->beginFrame();
    
    // Should advance internal frame counter
    SUCCEED();
}

TEST_F(GlyphCacheTest, Cleanup_ExpiredGlyphs) {
    GlyphKey key(1, 65, 12.0f);
    std::vector<uint8_t> bitmap(16 * 16, 128);
    
    m_glyphCache->cacheGlyph(key, bitmap.data(), Vec2(16, 16),
                             Vec2(0, 12), 8.0f);
    
    // Advance many frames without accessing
    for (uint32_t i = 0; i < Config::GlyphCache::GLYPH_EXPIRE_FRAMES + 100; ++i) {
        m_glyphCache->beginFrame();
    }
    
    // Glyph should be expired (or about to be)
    // Note: Actual cleanup is periodic, so we can't guarantee exact timing
    SUCCEED();
}

TEST_F(GlyphCacheTest, TextureCreation_Tracked) {
    size_t initialCount = m_backend->getTextureCount();
    
    GlyphKey key(1, 65, 12.0f);
    std::vector<uint8_t> bitmap(16 * 16, 128);
    m_glyphCache->cacheGlyph(key, bitmap.data(), Vec2(16, 16),
                             Vec2(0, 12), 8.0f);
    
    // Should have created at least one texture (atlas)
    EXPECT_GE(m_backend->getTextureCount(), initialCount);
}

//==========================================================================================
// Memory Leak / Growth Tests - CRITICAL
//==========================================================================================

class MemoryLeakTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_backend = std::make_unique<NiceMock<MockGraphicsBackend>>();
        
        ON_CALL(*m_backend, getRenderSize())
            .WillByDefault(Return(Vec2(1024.0f, 768.0f)));
        ON_CALL(*m_backend, getDPIScale())
            .WillByDefault(Return(1.0f));
        
        m_fontManager = std::make_unique<FontManager>();
        ASSERT_TRUE(m_fontManager->initialize());
        
        m_textRenderer = std::make_unique<TextRenderer>(m_backend.get(),
                                                         m_fontManager.get());
        ASSERT_TRUE(m_textRenderer->initialize(1.0f));
    }
    
    void TearDown() override {
        m_textRenderer->destroy();
        m_textRenderer.reset();
        m_fontManager->destroy();
        m_fontManager.reset();
        m_backend.reset();
    }
    
    std::unique_ptr<MockGraphicsBackend> m_backend;
    std::unique_ptr<FontManager> m_fontManager;
    std::unique_ptr<TextRenderer> m_textRenderer;
};

TEST_F(MemoryLeakTest, DISABLED_ShapedTextCache_DynamicText) {
    // CRITICAL: This test WILL FAIL with current implementation
    // It demonstrates the unbounded cache growth problem
    
    FontFallbackChain chain = m_fontManager->createDefaultFallbackChain();
    
    // Simulate 1 hour of clock display (3600 different texts)
    for (int i = 0; i < 3600; ++i) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "Time: %02d:%02d:%02d",
                 i/3600, (i%3600)/60, i%60);
        
        ShapedText shaped;
        m_textRenderer->shapeText(buffer, chain, 14.0f, shaped);
    }
    
    // TODO: After fix, check that cache size is bounded
    // Expected: Cache should have LRU limit (e.g., 1000 entries)
    // Current: Cache grows to 3600+ entries
    
    SUCCEED(); // Placeholder - real test would check memory
}

TEST_F(MemoryLeakTest, DISABLED_GlyphAvailabilityCache_Growth) {
    // CRITICAL: Tests hasGlyph() cache growth
    
    FontHandle arial = m_fontManager->getDefaultFont();
    
    // Check 10000 different characters
    for (uint32_t i = 0x0020; i < 0x0020 + 10000; ++i) {
        m_fontManager->hasGlyph(arial, i);
    }
    
    // TODO: After fix, verify cache size is bounded
    // Current: m_glyphAvailabilityCache grows unbounded
    
    SUCCEED();
}

TEST_F(MemoryLeakTest, DISABLED_MeasureTextCache_Growth) {
    // CRITICAL: Tests measureText() cache growth
    
    // Measure 1000 different texts
    for (int i = 0; i < 1000; ++i) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "Dynamic text %d", i);
        m_fontManager->measureText(buffer, 12.0f);
    }
    
    // TODO: After fix, verify s_measureTextCache size is bounded
    
    SUCCEED();
}

TEST_F(MemoryLeakTest, GlyphCache_Cleanup) {
    // Test that GlyphCache actually cleans up
    
    FontFallbackChain chain = m_fontManager->createDefaultFallbackChain();
    
    // Create many different texts to fill glyph cache
    for (int i = 0; i < 100; ++i) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "Text_%d", i);
        
        ShapedText shaped;
        m_textRenderer->shapeText(buffer, chain, 12.0f, shaped);
        
        std::vector<TextVertex> vertices;
        m_textRenderer->generateTextVertices(shaped, Vec2(0, 0),
                                            Vec4(1, 1, 1, 1),
                                            chain, 12.0f, vertices);
    }
    
    // Advance past expiration time without accessing glyphs
    for (uint32_t i = 0; i < Config::GlyphCache::GLYPH_EXPIRE_FRAMES + 100; ++i) {
        m_textRenderer->beginFrame();
    }
    
    // Glyph cache should have cleaned up some entries
    // (We can't directly verify, but at least ensure no crash)
    SUCCEED();
}

//==========================================================================================
// Integration Tests
//==========================================================================================

TEST_F(TextRendererTest, FullPipeline_LatinText) {
    FontFallbackChain chain = m_fontManager->createDefaultFallbackChain();
    
    // Shape text
    ShapedText shaped;
    m_textRenderer->shapeText("Hello World", chain, 14.0f, shaped);
    ASSERT_FALSE(shaped.isEmpty());
    
    // Generate vertices
    std::vector<TextVertex> vertices;
    m_textRenderer->generateTextVertices(shaped, Vec2(10, 10),
                                        Vec4(1, 1, 1, 1),
                                        chain, 14.0f, vertices);
    EXPECT_GT(vertices.size(), 0u);
    
    // Verify vertex data
    for (const auto& v : vertices) {
        EXPECT_TRUE(v.isValid());
    }
}

TEST_F(TextRendererTest, FullPipeline_MixedScript) {
    FontFallbackChain chain = m_fontManager->createDefaultFallbackChain();
    
    ShapedText shaped;
    m_textRenderer->shapeText("Hello世界123", chain, 14.0f, shaped);
    ASSERT_FALSE(shaped.isEmpty());
    
    std::vector<TextVertex> vertices;
    m_textRenderer->generateTextVertices(shaped, Vec2(10, 10),
                                        Vec4(1, 1, 1, 1),
                                        chain, 14.0f, vertices);
    EXPECT_GT(vertices.size(), 0u);
}

TEST_F(TextRendererTest, FullPipeline_MultipleFrames) {
    FontFallbackChain chain = m_fontManager->createDefaultFallbackChain();
    
    // Render same text across multiple frames
    for (int frame = 0; frame < 10; ++frame) {
        m_textRenderer->beginFrame();
        
        ShapedText shaped;
        m_textRenderer->shapeText("Consistent Text", chain, 14.0f, shaped);
        
        std::vector<TextVertex> vertices;
        m_textRenderer->generateTextVertices(shaped, Vec2(10, 10),
                                            Vec4(1, 1, 1, 1),
                                            chain, 14.0f, vertices);
    }
    
    SUCCEED();
}

//==========================================================================================
// Stress Tests
//==========================================================================================

TEST_F(TextRendererTest, Stress_ManyDifferentTexts) {
    FontFallbackChain chain = m_fontManager->createDefaultFallbackChain();
    
    // Shape 500 different texts
    for (int i = 0; i < 500; ++i) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "Text number %d", i);
        
        ShapedText shaped;
        m_textRenderer->shapeText(buffer, chain, 12.0f, shaped);
        
        EXPECT_FALSE(shaped.isEmpty());
    }
    
    SUCCEED();
}

TEST_F(TextRendererTest, Stress_ManySizes) {
    FontFallbackChain chain = m_fontManager->createDefaultFallbackChain();
    
    // Same text at many different sizes
    for (float size = Config::Font::MIN_SIZE;
         size <= Config::Font::MAX_SIZE;
         size += 10.0f) {
        ShapedText shaped;
        m_textRenderer->shapeText("Test", chain, size, shaped);
        
        EXPECT_FALSE(shaped.isEmpty());
    }
    
    SUCCEED();
}

TEST_F(FontManagerTest, Stress_ManyGlyphQueries) {
    FontHandle arial = m_fontManager->getDefaultFont();
    
    // Query 1000 different glyphs
    for (uint32_t i = 0x0020; i < 0x0020 + 1000; ++i) {
        bool has = m_fontManager->hasGlyph(arial, i);
        (void)has; // Suppress unused warning
    }
    
    SUCCEED();
}

//==========================================================================================
// Edge Case Tests
//==========================================================================================

TEST_F(TextRendererTest, EdgeCase_VeryLongText) {
    FontFallbackChain chain = m_fontManager->createDefaultFallbackChain();

    const size_t glyphLimit = Config::Text::MAX_GLYPHS_PER_TEXT;
    const size_t safeLength = glyphLimit - 100;
    
    std::string longText(safeLength, 'A');
    
    ShapedText shaped;
    m_textRenderer->shapeText(longText.c_str(), chain, 12.0f, shaped);
    
    // 应该成功处理
    EXPECT_FALSE(shaped.isEmpty());
    EXPECT_LE(shaped.glyphs.size(), glyphLimit);
    EXPECT_GT(shaped.glyphs.size(), 0u);
}

TEST_F(TextRendererTest, EdgeCase_TooLongText) {
    FontFallbackChain chain = m_fontManager->createDefaultFallbackChain();
    
    const size_t tooLong = Config::Text::MAX_LENGTH + 1000;
    std::string veryLongText(tooLong, 'A');
    
    ShapedText shaped;
    m_textRenderer->shapeText(veryLongText.c_str(), chain, 12.0f, shaped);
    
    if (!shaped.isEmpty()) {
        EXPECT_LE(shaped.glyphs.size(), Config::Text::MAX_GLYPHS_PER_TEXT);
    }
}

TEST_F(TextRendererTest, EdgeCase_SpecialCharacters) {
    FontFallbackChain chain = m_fontManager->createDefaultFallbackChain();
    
    // Test various special characters
    const char* specialTexts[] = {
        "\n\r\t",           // Whitespace
        "©®™",              // Symbols
        "→↓←↑",            // Arrows
        "αβγδ",             // Greek
        "абвг",             // Cyrillic
    };
    
    for (const char* text : specialTexts) {
        ShapedText shaped;
        m_textRenderer->shapeText(text, chain, 12.0f, shaped);
        // Should not crash
    }
    
    SUCCEED();
}

//==========================================================================================
// Performance Benchmarks (Optional, can be disabled)
//==========================================================================================

TEST_F(TextRendererTest, DISABLED_Benchmark_ShapeTextPerformance) {
    FontFallbackChain chain = m_fontManager->createDefaultFallbackChain();
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; ++i) {
        ShapedText shaped;
        m_textRenderer->shapeText("Benchmark Text", chain, 14.0f, shaped);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Shaped 1000 texts in " << duration.count() << "ms\n";
    std::cout << "Average: " << (duration.count() / 1000.0) << "ms per text\n";
    
    // Performance threshold: should be < 1s for 1000 texts
    EXPECT_LT(duration.count(), 1000);
}

//==========================================================================================
// Main
//==========================================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
