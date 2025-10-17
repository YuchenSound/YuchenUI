/*******************************************************************************************
**
** YuchenUI - MetalRenderer Comprehensive Test Suite
**
** Copyright (C) 2025 Yuchen Wei
**
** 完整的 MetalRenderer 单元测试和性能测试
** 使用 Google Test 和 Google Mock 框架
**
********************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/QuartzCore.h>
#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>

#include "MetalRenderer.h"

#include "YuchenUI/text/FontManager.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/text/TextRenderer.h"
#include "YuchenUI/text/IFontProvider.h"
#include "YuchenUI/image/TextureCache.h"

#include <vector>
#include <chrono>
#include <random>

using namespace YuchenUI;
using namespace testing;
//==========================================================================================
// Mock Objects
//==========================================================================================

/**
 * Mock Metal Device
 *
 * 模拟 Metal 设备，记录所有 API 调用以验证渲染行为
 */
class MockMetalDevice {
public:
    MOCK_METHOD(id<MTLCommandQueue>, newCommandQueue, ());
    MOCK_METHOD(id<MTLLibrary>, newLibraryWithSource, (NSString*, MTLCompileOptions*, NSError**));
    MOCK_METHOD(id<MTLRenderPipelineState>, newRenderPipelineStateWithDescriptor, (MTLRenderPipelineDescriptor*, NSError**));
    MOCK_METHOD(id<MTLBuffer>, newBufferWithBytes, (const void*, NSUInteger, MTLResourceOptions));
    MOCK_METHOD(id<MTLBuffer>, newBufferWithLength, (NSUInteger, MTLResourceOptions));
    MOCK_METHOD(id<MTLTexture>, newTextureWithDescriptor, (MTLTextureDescriptor*));
    MOCK_METHOD(id<MTLSamplerState>, newSamplerStateWithDescriptor, (MTLSamplerDescriptor*));
    
    // 统计信息
    size_t bufferCreationCount = 0;
    size_t textureCreationCount = 0;
    size_t pipelineCreationCount = 0;
};

/**
 * Mock Font Provider
 *
 * 模拟字体提供者，用于文本渲染测试
 */
class MockFontProvider : public IFontProvider {
public:
    MOCK_METHOD(FontHandle, loadFontFromMemory, (const void*, size_t, const char*), (override));
    MOCK_METHOD(FontHandle, loadFontFromFile, (const char*, const char*), (override));
    MOCK_METHOD(bool, isValidFont, (FontHandle), (const, override));
    MOCK_METHOD(FontMetrics, getFontMetrics, (FontHandle, float), (const, override));
    MOCK_METHOD(GlyphMetrics, getGlyphMetrics, (FontHandle, uint32_t, float), (const, override));
    MOCK_METHOD(Vec2, measureText, (const char*, float), (const, override));
    MOCK_METHOD(float, getTextHeight, (FontHandle, float), (const, override));
    MOCK_METHOD(bool, hasGlyph, (FontHandle, uint32_t), (const, override));
    MOCK_METHOD(FontHandle, selectFontForCodepoint, (uint32_t, const FontFallbackChain&), (const, override));
    MOCK_METHOD(FontHandle, getDefaultFont, (), (const, override));
    MOCK_METHOD(FontHandle, getDefaultBoldFont, (), (const, override));
    MOCK_METHOD(FontHandle, getDefaultNarrowFont, (), (const, override));
    MOCK_METHOD(FontHandle, getDefaultNarrowBoldFont, (), (const, override));
    MOCK_METHOD(FontHandle, getDefaultCJKFont, (), (const, override));
    MOCK_METHOD(void*, getFontFace, (FontHandle), (const, override));
    MOCK_METHOD(void*, getHarfBuzzFont, (FontHandle, float, float), (override));
};

/**
 * Mock Text Renderer
 *
 * 模拟文本渲染器，用于测试文本渲染流程
 */
class MockTextRenderer : public TextRenderer {
public:
    MockTextRenderer(IGraphicsBackend* backend, IFontProvider* fontProvider)
        : TextRenderer(backend, fontProvider) {}
    
    MOCK_METHOD(void, shapeText, (const char*, const FontFallbackChain&, float, float, ShapedText&));
    MOCK_METHOD(void, generateTextVertices, (const ShapedText&, const Vec2&, const Vec4&, const FontFallbackChain&, float, std::vector<TextVertex>&));
    MOCK_METHOD(void*, getCurrentAtlasTexture, (), (const));
    MOCK_METHOD(void, beginFrame, ());
};

/**
 * Mock Texture Cache
 *
 * 模拟纹理缓存，用于测试图像渲染
 */
class MockTextureCache : public TextureCache {
public:
    MockTextureCache(IGraphicsBackend* backend) : TextureCache(backend) {}
    
    MOCK_METHOD(void*, getTexture, (const char*, uint32_t&, uint32_t&, float*));
    MOCK_METHOD(void, setCurrentDPI, (float));
    MOCK_METHOD(void, clearAll, ());
};

/**
 * Mock Metal Layer
 *
 * 模拟 CAMetalLayer，用于测试渲染目标管理
 */
@interface MockMetalLayer : NSObject
@property (nonatomic, strong) id<MTLDevice> device;
@property (nonatomic, assign) MTLPixelFormat pixelFormat;
@property (nonatomic, assign) CGSize drawableSize;
@property (nonatomic, assign) BOOL framebufferOnly;
@property (nonatomic, assign) BOOL presentsWithTransaction;
@property (nonatomic, assign) BOOL allowsNextDrawableTimeout;
@property (nonatomic, assign) BOOL displaySyncEnabled API_AVAILABLE(macos(10.15));
@property (nonatomic, strong) id<CAMetalDrawable> nextDrawable;

- (id<CAMetalDrawable>)nextDrawable;
@end

@implementation MockMetalLayer
- (id<CAMetalDrawable>)nextDrawable {
    return _nextDrawable;
}
@end

//==========================================================================================
// Test Fixture
//==========================================================================================

/**
 * MetalRenderer Test Fixture
 *
 * 提供完整的测试环境，使用真实的 FontProvider 而不是 Mock
 */
class MetalRendererTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建真实的 FontManager
        realFontProvider = std::make_unique<FontManager>();
        
        bool initSuccess = realFontProvider->initialize();
        ASSERT_TRUE(initSuccess) << "Failed to initialize FontManager";
        
        FontHandle defaultFont = realFontProvider->getDefaultFont();
        ASSERT_NE(defaultFont, INVALID_FONT_HANDLE) << "No default font available after initialization";
        
        realDevice = MTLCreateSystemDefaultDevice();
        ASSERT_NE(realDevice, nullptr) << "Failed to create Metal device";
        
        // 创建 Mock Metal Layer
        mockLayer = [[MockMetalLayer alloc] init];
        mockLayer.device = realDevice;
        
        // 创建渲染器
        renderer = std::make_unique<MetalRenderer>();
    }
    
    void TearDown() override {
        renderer.reset();
        mockLayer = nil;
        realDevice = nil;
        realFontProvider.reset();
    }
    
    /**
     * 初始化渲染器
     */
    bool initializeRenderer(int width = 1920, int height = 1080, float dpiScale = 2.0f) {
        return renderer->initialize((__bridge void*)mockLayer, width, height, dpiScale, realFontProvider.get());
    }
    
    /**
     * 创建测试用的 RenderList
     */
    RenderList createTestRenderList(size_t commandCount) {
        RenderList list;
        list.clear(Vec4(0.0f, 0.0f, 0.0f, 1.0f));
        
        for (size_t i = 0; i < commandCount; ++i) {
            float x = static_cast<float>(i * 10);
            float y = static_cast<float>(i * 10);
            YuchenUI::Rect rect(x, y, 100.0f, 50.0f);
            Vec4 color = Vec4::FromRGBA(255, 0, 0, 255);
            list.fillRect(rect, color, CornerRadius(5.0f));
        }
        
        return list;
    }
    
    /**
     * 创建包含多种命令类型的 RenderList
     */
    RenderList createMixedRenderList() {
        RenderList list;
        list.clear(Vec4(0.2f, 0.2f, 0.2f, 1.0f));
        
        // 矩形
        list.fillRect(YuchenUI::Rect(10, 10, 100, 50), Vec4::FromRGBA(255, 0, 0, 255), CornerRadius(5.0f));
        list.drawRect(YuchenUI::Rect(120, 10, 100, 50), Vec4::FromRGBA(0, 255, 0, 255), 2.0f, CornerRadius(5.0f));
        
        // 文本（使用真实字体）
        FontHandle defaultFont = realFontProvider->getDefaultFont();
        FontFallbackChain chain(defaultFont);
        list.drawText("Hello World", Vec2(10, 70), chain, 14.0f, Vec4::FromRGBA(255, 255, 255, 255));
        
        // 图像
        list.drawImage("test.png", YuchenUI::Rect(10, 90, 100, 100), ScaleMode::Stretch);
        
        // 形状
        list.drawLine(Vec2(10, 200), Vec2(110, 200), Vec4::FromRGBA(255, 255, 0, 255), 2.0f);
        list.fillTriangle(Vec2(10, 220), Vec2(60, 220), Vec2(35, 270), Vec4::FromRGBA(0, 255, 255, 255));
        list.fillCircle(Vec2(60, 300), 30.0f, Vec4::FromRGBA(255, 0, 255, 255));
        
        return list;
    }
    
    /**
     * 性能测试辅助：测量渲染时间
     */
    template<typename Func>
    double measureRenderTime(Func&& func, int iterations = 1000) {
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            func();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        return duration.count() / static_cast<double>(iterations);
    }
    
    // 测试数据成员
    std::unique_ptr<MetalRenderer> renderer;
    std::unique_ptr<FontManager> realFontProvider;
    id<MTLDevice> realDevice;
    MockMetalLayer* mockLayer;
};


//==========================================================================================
// 功能测试 - 初始化和生命周期
//==========================================================================================

/**
 * 测试：基本初始化成功
 */
TEST_F(MetalRendererTest, InitializationSuccess) {
    EXPECT_TRUE(initializeRenderer(1920, 1080, 2.0f));
    EXPECT_TRUE(renderer->isInitialized());
    EXPECT_EQ(renderer->getRenderSize(), Vec2(1920.0f, 1080.0f));
    EXPECT_FLOAT_EQ(renderer->getDPIScale(), 2.0f);
}

/**
 * 测试：边界尺寸初始化
 */
TEST_F(MetalRendererTest, InitializationWithBoundaryDimensions) {
    // 最小尺寸
    EXPECT_TRUE(renderer->initialize((__bridge void*)mockLayer,
                                     Config::Window::MIN_SIZE, Config::Window::MIN_SIZE, 1.0f, realFontProvider.get()));
    EXPECT_TRUE(renderer->isInitialized());
    
    // 最大尺寸
    renderer = std::make_unique<MetalRenderer>();
    EXPECT_TRUE(renderer->initialize((__bridge void*)mockLayer,
                                     Config::Window::MAX_SIZE, Config::Window::MAX_SIZE, 1.0f, realFontProvider.get()));
    EXPECT_TRUE(renderer->isInitialized());
}

/**
 * 测试：重复初始化应该失败
 */
TEST_F(MetalRendererTest, DoubleInitializationFails) {
    EXPECT_TRUE(initializeRenderer());
    
    // 第二次初始化应该失败（通过断言）
    // 在生产代码中，这会触发 YUCHEN_ASSERT
    // 在测试中，我们可以捕获这个行为
#ifdef YUCHEN_DEBUG
    EXPECT_DEATH(
                 renderer->initialize((__bridge void*)mockLayer, 1920, 1080, 2.0f, realFontProvider.get()),
        "Already initialized"
    );
#endif
}

/**
 * 测试：窗口大小调整
 */
TEST_F(MetalRendererTest, ResizeWindow) {
    ASSERT_TRUE(initializeRenderer(1920, 1080, 2.0f));
    
    // 调整到新尺寸
    renderer->resize(2560, 1440);
    EXPECT_EQ(renderer->getRenderSize(), Vec2(2560.0f, 1440.0f));
    
    // 调整到最小尺寸
    renderer->resize(Config::Window::MIN_SIZE, Config::Window::MIN_SIZE);
    EXPECT_EQ(renderer->getRenderSize(), Vec2(
        static_cast<float>(Config::Window::MIN_SIZE),
        static_cast<float>(Config::Window::MIN_SIZE)
    ));
    
    // 调整到最大尺寸
    renderer->resize(Config::Window::MAX_SIZE, Config::Window::MAX_SIZE);
    EXPECT_EQ(renderer->getRenderSize(), Vec2(
        static_cast<float>(Config::Window::MAX_SIZE),
        static_cast<float>(Config::Window::MAX_SIZE)
    ));
}

/**
 * 测试：无效的窗口大小调整
 */
TEST_F(MetalRendererTest, ResizeWithInvalidDimensions) {
    ASSERT_TRUE(initializeRenderer());
    
#ifdef YUCHEN_DEBUG
    // 无效宽度
    EXPECT_DEATH(renderer->resize(0, 1080), "Invalid width");
    
    // 无效高度
    EXPECT_DEATH(renderer->resize(1920, 0), "Invalid height");
    
    // 超出范围
    EXPECT_DEATH(renderer->resize(Config::Window::MAX_SIZE + 1, 1080), "Invalid width");
#endif
}

//==========================================================================================
// 功能测试 - 纹理管理
//==========================================================================================

/**
 * 测试：创建纹理
 */
TEST_F(MetalRendererTest, CreateTexture) {
    ASSERT_TRUE(initializeRenderer());
    
    // 创建 R8 纹理
    void* textureR8 = renderer->createTexture2D(256, 256, TextureFormat::R8_Unorm);
    EXPECT_NE(textureR8, nullptr);
    
    // 创建 RGBA8 纹理
    void* textureRGBA8 = renderer->createTexture2D(512, 512, TextureFormat::RGBA8_Unorm);
    EXPECT_NE(textureRGBA8, nullptr);
    
    // 清理
    renderer->destroyTexture(textureR8);
    renderer->destroyTexture(textureRGBA8);
}

/**
 * 测试：更新纹理数据
 */
TEST_F(MetalRendererTest, UpdateTextureData) {
    ASSERT_TRUE(initializeRenderer());
    
    const uint32_t width = 256;
    const uint32_t height = 256;
    void* texture = renderer->createTexture2D(width, height, TextureFormat::RGBA8_Unorm);
    ASSERT_NE(texture, nullptr);
    
    // 创建测试数据
    std::vector<uint8_t> testData(width * height * 4, 128);
    
    // 更新整个纹理
    EXPECT_NO_THROW(
        renderer->updateTexture2D(texture, 0, 0, width, height, testData.data(), width * 4)
    );
    
    // 更新部分区域
    EXPECT_NO_THROW(
        renderer->updateTexture2D(texture, 64, 64, 128, 128, testData.data(), 128 * 4)
    );
    
    renderer->destroyTexture(texture);
}

/**
 * 测试：纹理创建边界情况
 */
TEST_F(MetalRendererTest, TextureCreationBoundaryCases) {
    ASSERT_TRUE(initializeRenderer());
    
    // 最小尺寸纹理
    void* minTexture = renderer->createTexture2D(1, 1, TextureFormat::RGBA8_Unorm);
    EXPECT_NE(minTexture, nullptr);
    renderer->destroyTexture(minTexture);
    
    // 大尺寸纹理（Metal 通常支持 16384x16384）
    void* largeTexture = renderer->createTexture2D(8192, 8192, TextureFormat::RGBA8_Unorm);
    EXPECT_NE(largeTexture, nullptr);
    renderer->destroyTexture(largeTexture);
    
    // 非正方形纹理
    void* rectTexture = renderer->createTexture2D(1024, 256, TextureFormat::R8_Unorm);
    EXPECT_NE(rectTexture, nullptr);
    renderer->destroyTexture(rectTexture);
}

/**
 * 测试：销毁空纹理句柄
 */
TEST_F(MetalRendererTest, DestroyNullTexture) {
    ASSERT_TRUE(initializeRenderer());
    
    // 销毁 nullptr 应该安全
    EXPECT_NO_THROW(renderer->destroyTexture(nullptr));
}

//==========================================================================================
// 功能测试 - 渲染命令执行
//==========================================================================================

/**
 * 测试：基本帧渲染流程
 */
TEST_F(MetalRendererTest, BasicFrameRenderingFlow) {
    ASSERT_TRUE(initializeRenderer());
    
    // 开始帧
    EXPECT_NO_THROW(renderer->beginFrame());
    
    // 执行空命令列表
    RenderList emptyList;
    EXPECT_NO_THROW(renderer->executeRenderCommands(emptyList));
    
    // 结束帧
    EXPECT_NO_THROW(renderer->endFrame());
}

/**
 * 测试：渲染矩形
 */
TEST_F(MetalRendererTest, RenderRectangles) {
    ASSERT_TRUE(initializeRenderer());
    
    RenderList list;
    list.clear(Vec4(0.2f, 0.2f, 0.2f, 1.0f));
    
    // 填充矩形
    list.fillRect(YuchenUI::Rect(10, 10, 100, 50), Vec4::FromRGBA(255, 0, 0, 255), CornerRadius(5.0f));
    
    // 边框矩形
    list.drawRect(YuchenUI::Rect(120, 10, 100, 50), Vec4::FromRGBA(0, 255, 0, 255), 2.0f, CornerRadius(5.0f));
    
    // 无圆角矩形
    list.fillRect(YuchenUI::Rect(230, 10, 100, 50), Vec4::FromRGBA(0, 0, 255, 255), CornerRadius());
    
    renderer->beginFrame();
    EXPECT_NO_THROW(renderer->executeRenderCommands(list));
    renderer->endFrame();
}

/**
 * 测试：渲染文本
 */
TEST_F(MetalRendererTest, RenderText) {
    ASSERT_TRUE(initializeRenderer());
    
    RenderList list;
    FontFallbackChain chain(1);
    
    // 简单文本
    list.drawText("Hello World", Vec2(10, 10), chain, 14.0f, Vec4::FromRGBA(255, 255, 255, 255));
    
    // 不同大小的文本
    list.drawText("Large Text", Vec2(10, 30), chain, 24.0f, Vec4::FromRGBA(255, 0, 0, 255));
    list.drawText("Small Text", Vec2(10, 60), chain, 10.0f, Vec4::FromRGBA(0, 255, 0, 255));
    
    // 带字间距的文本
    list.drawText("Spaced Text", Vec2(10, 80), chain, 14.0f, Vec4::FromRGBA(255, 255, 255, 255), 100.0f);
    
    renderer->beginFrame();
    EXPECT_NO_THROW(renderer->executeRenderCommands(list));
    renderer->endFrame();
}

/**
 * 测试：渲染图像
 */
TEST_F(MetalRendererTest, RenderImages) {
    ASSERT_TRUE(initializeRenderer());
    
    RenderList list;
    
    // Stretch 模式
    list.drawImage("test.png", YuchenUI::Rect(10, 10, 100, 100), ScaleMode::Stretch);
    
    // Fill 模式
    list.drawImage("test.png", YuchenUI::Rect(120, 10, 100, 100), ScaleMode::Fill);
    
    // Original 模式
    list.drawImage("test.png", YuchenUI::Rect(230, 10, 100, 100), ScaleMode::Original);
    
    // NineSlice 模式
    NineSliceMargins margins(10.0f, 10.0f, 10.0f, 10.0f);
    list.drawImage("test.png", YuchenUI::Rect(340, 10, 100, 100), ScaleMode::NineSlice, margins);
    
    renderer->beginFrame();
    EXPECT_NO_THROW(renderer->executeRenderCommands(list));
    renderer->endFrame();
}

/**
 * 测试：渲染形状
 */
TEST_F(MetalRendererTest, RenderShapes) {
    ASSERT_TRUE(initializeRenderer());
    
    RenderList list;
    
    // 线段
    list.drawLine(Vec2(10, 10), Vec2(110, 10), Vec4::FromRGBA(255, 0, 0, 255), 2.0f);
    list.drawLine(Vec2(10, 20), Vec2(110, 20), Vec4::FromRGBA(0, 255, 0, 255), 5.0f);
    
    // 三角形
    list.fillTriangle(Vec2(10, 40), Vec2(60, 40), Vec2(35, 90), Vec4::FromRGBA(0, 0, 255, 255));
    list.drawTriangle(Vec2(70, 40), Vec2(120, 40), Vec2(95, 90), Vec4::FromRGBA(255, 255, 0, 255), 2.0f);
    
    // 圆形
    list.fillCircle(Vec2(50, 130), 30.0f, Vec4::FromRGBA(255, 0, 255, 255));
    list.drawCircle(Vec2(130, 130), 30.0f, Vec4::FromRGBA(0, 255, 255, 255), 3.0f);
    
    renderer->beginFrame();
    EXPECT_NO_THROW(renderer->executeRenderCommands(list));
    renderer->endFrame();
}

/**
 * 测试：混合命令类型渲染
 */
TEST_F(MetalRendererTest, RenderMixedCommands) {
    ASSERT_TRUE(initializeRenderer());
    
    RenderList list = createMixedRenderList();
    
    renderer->beginFrame();
    EXPECT_NO_THROW(renderer->executeRenderCommands(list));
    renderer->endFrame();
}

//==========================================================================================
// 功能测试 - 裁剪
//==========================================================================================

/**
 * 测试：基本裁剪功能
 */
TEST_F(MetalRendererTest, BasicClipping) {
    ASSERT_TRUE(initializeRenderer());
    
    RenderList list;
    
    // 设置裁剪区域
    list.pushClipRect(YuchenUI::Rect(50, 50, 200, 200));
    
    // 在裁剪区域内绘制
    list.fillRect(YuchenUI::Rect(60, 60, 100, 50), Vec4::FromRGBA(255, 0, 0, 255), CornerRadius());
    
    // 部分在裁剪区域外
    list.fillRect(YuchenUI::Rect(150, 150, 100, 100), Vec4::FromRGBA(0, 255, 0, 255), CornerRadius());
    
    // 恢复裁剪
    list.popClipRect();
    
    // 在裁剪区域外绘制
    list.fillRect(YuchenUI::Rect(300, 300, 100, 50), Vec4::FromRGBA(0, 0, 255, 255), CornerRadius());
    
    renderer->beginFrame();
    EXPECT_NO_THROW(renderer->executeRenderCommands(list));
    renderer->endFrame();
}

/**
 * 测试：嵌套裁剪
 */
TEST_F(MetalRendererTest, NestedClipping) {
    ASSERT_TRUE(initializeRenderer());
    
    RenderList list;
    
    // 第一层裁剪
    list.pushClipRect(YuchenUI::Rect(50, 50, 300, 300));
    list.fillRect(YuchenUI::Rect(60, 60, 100, 100), Vec4::FromRGBA(255, 0, 0, 128), CornerRadius());
    
    // 第二层裁剪（与第一层相交）
    list.pushClipRect(YuchenUI::Rect(100, 100, 200, 200));
    list.fillRect(YuchenUI::Rect(110, 110, 100, 100), Vec4::FromRGBA(0, 255, 0, 128), CornerRadius());
    
    // 第三层裁剪
    list.pushClipRect(YuchenUI::Rect(150, 150, 100, 100));
    list.fillRect(YuchenUI::Rect(160, 160, 50, 50), Vec4::FromRGBA(0, 0, 255, 128), CornerRadius());
    
    // 逐层恢复
    list.popClipRect();
    list.fillRect(YuchenUI::Rect(120, 220, 50, 50), Vec4::FromRGBA(255, 255, 0, 128), CornerRadius());
    
    list.popClipRect();
    list.fillRect(YuchenUI::Rect(70, 280, 50, 50), Vec4::FromRGBA(255, 0, 255, 128), CornerRadius());
    
    list.popClipRect();
    list.fillRect(YuchenUI::Rect(360, 360, 50, 50), Vec4::FromRGBA(0, 255, 255, 128), CornerRadius());
    
    renderer->beginFrame();
    EXPECT_NO_THROW(renderer->executeRenderCommands(list));
    renderer->endFrame();
}

/**
 * 测试：无交集的裁剪区域
 */
TEST_F(MetalRendererTest, NonIntersectingClipRegions) {
    ASSERT_TRUE(initializeRenderer());
    
    RenderList list;
    
    // 第一个裁剪区域
    list.pushClipRect(YuchenUI::Rect(50, 50, 100, 100));
    
    // 第二个不相交的裁剪区域
    list.pushClipRect(YuchenUI::Rect(200, 200, 100, 100));
    
    // 这个矩形不应该被渲染（在空的交集内）
    list.fillRect(YuchenUI::Rect(220, 220, 50, 50), Vec4::FromRGBA(255, 0, 0, 255), CornerRadius());
    
    list.popClipRect();
    list.popClipRect();
    
    renderer->beginFrame();
    EXPECT_NO_THROW(renderer->executeRenderCommands(list));
    renderer->endFrame();
}

//==========================================================================================
// 性能测试 - 批处理效率
//==========================================================================================

/**
 * 性能测试：矩形批处理 - 10个矩形
 */
TEST_F(MetalRendererTest, PerformanceRectBatching10) {
    ASSERT_TRUE(initializeRenderer());
    
    auto renderFunc = [this]() {
        RenderList list = createTestRenderList(10);
        renderer->beginFrame();
        renderer->executeRenderCommands(list);
        renderer->endFrame();
    };
    
    double avgTime = measureRenderTime(renderFunc, 1000);
    
    std::cout << "\n[Performance] 10 Rectangles: " << avgTime << " μs per frame" << std::endl;
    
    // 基准：10个矩形应该在 100μs 内完成（根据实际硬件调整）
    EXPECT_LT(avgTime, 100.0);
}

/**
 * 性能测试：矩形批处理 - 100个矩形
 */
TEST_F(MetalRendererTest, PerformanceRectBatching100) {
    ASSERT_TRUE(initializeRenderer());
    
    auto renderFunc = [this]() {
        RenderList list = createTestRenderList(100);
        renderer->beginFrame();
        renderer->executeRenderCommands(list);
        renderer->endFrame();
    };
    
    double avgTime = measureRenderTime(renderFunc, 1000);
    
    std::cout << "[Performance] 100 Rectangles: " << avgTime << " μs per frame" << std::endl;
    
    // 基准：100个矩形应该在 500μs 内完成
    EXPECT_LT(avgTime, 500.0);
}

/**
 * 性能测试：矩形批处理 - 1000个矩形
 */
TEST_F(MetalRendererTest, PerformanceRectBatching1000) {
    ASSERT_TRUE(initializeRenderer());
    
    auto renderFunc = [this]() {
        RenderList list = createTestRenderList(1000);
        renderer->beginFrame();
        renderer->executeRenderCommands(list);
        renderer->endFrame();
    };
    
    double avgTime = measureRenderTime(renderFunc, 100);
    
    std::cout << "[Performance] 1000 Rectangles: " << avgTime << " μs per frame" << std::endl;
    
    // 基准：1000个矩形应该在 3000μs 内完成
    EXPECT_LT(avgTime, 3000.0);
}

/**
 * 性能测试：矩形批处理 - 10000个矩形（压力测试）
 */
TEST_F(MetalRendererTest, PerformanceRectBatching9999_StressTest) {
    ASSERT_TRUE(initializeRenderer());
    
    auto renderFunc = [this]() {
        RenderList list = createTestRenderList(9999);
        renderer->beginFrame();
        renderer->executeRenderCommands(list);
        renderer->endFrame();
    };
    
    double avgTime = measureRenderTime(renderFunc, 10);
    
    std::cout << "[Performance] 9999 Rectangles (Stress): " << avgTime << " μs per frame" << std::endl;
    std::cout << "           Equivalent FPS: " << (1000000.0 / avgTime) << " fps" << std::endl;
    
    // 基准：9999个矩形应该在 20000μs (20ms) 内完成，保证 50fps
    EXPECT_LT(avgTime, 20000.0);
}

/**
 * 性能测试：批处理效率比较
 *
 * 比较批处理与逐个绘制的性能差异
 */
TEST_F(MetalRendererTest, PerformanceBatchingEfficiency) {
    ASSERT_TRUE(initializeRenderer());
    
    const int rectCount = 1000;
    
    // 批处理模式（应该更快）
    auto batchedFunc = [this, rectCount]() {
        RenderList list = createTestRenderList(rectCount);
        renderer->beginFrame();
        renderer->executeRenderCommands(list);
        renderer->endFrame();
    };
    
    double batchedTime = measureRenderTime(batchedFunc, 100);
    
    std::cout << "\n[Performance] Batching Efficiency Test:" << std::endl;
    std::cout << "  Batched rendering: " << batchedTime << " μs" << std::endl;
    std::cout << "  Rectangles per batch: " << rectCount << std::endl;
    std::cout << "  Time per rectangle: " << (batchedTime / rectCount) << " μs" << std::endl;
    
    // 批处理应该能够高效处理大量矩形
    EXPECT_LT(batchedTime / rectCount, 5.0); // 每个矩形平均时间应该小于 5μs
}

/**
 * 性能测试：混合命令批处理
 */
TEST_F(MetalRendererTest, PerformanceMixedCommandBatching) {
    ASSERT_TRUE(initializeRenderer());
    
    auto renderFunc = [this]() {
        RenderList list = createMixedRenderList();
        renderer->beginFrame();
        renderer->executeRenderCommands(list);
        renderer->endFrame();
    };
    
    double avgTime = measureRenderTime(renderFunc, 1000);
    
    std::cout << "\n[Performance] Mixed Commands: " << avgTime << " μs per frame" << std::endl;
    
    // 混合命令应该在合理时间内完成
    EXPECT_LT(avgTime, 500.0);
}

/**
 * 性能测试：裁剪开销
 */
TEST_F(MetalRendererTest, PerformanceClippingOverhead) {
    ASSERT_TRUE(initializeRenderer());
    
    // 无裁剪
    auto noClipFunc = [this]() {
        RenderList list;
        for (int i = 0; i < 100; ++i) {
            list.fillRect(YuchenUI::Rect(i * 10.0f, i * 10.0f, 50, 50), Vec4::FromRGBA(255, 0, 0, 255), CornerRadius());
        }
        renderer->beginFrame();
        renderer->executeRenderCommands(list);
        renderer->endFrame();
    };
    
    // 有裁剪
    auto withClipFunc = [this]() {
        RenderList list;
        list.pushClipRect(YuchenUI::Rect(100, 100, 500, 500));
        for (int i = 0; i < 100; ++i) {
            list.fillRect(YuchenUI::Rect(i * 10.0f, i * 10.0f, 50, 50), Vec4::FromRGBA(255, 0, 0, 255), CornerRadius());
        }
        list.popClipRect();
        renderer->beginFrame();
        renderer->executeRenderCommands(list);
        renderer->endFrame();
    };
    
    double noClipTime = measureRenderTime(noClipFunc, 1000);
    double withClipTime = measureRenderTime(withClipFunc, 1000);
    double overhead = withClipTime - noClipTime;
    double overheadPercent = (overhead / noClipTime) * 100.0;
    
    std::cout << "\n[Performance] Clipping Overhead:" << std::endl;
    std::cout << "  Without clipping: " << noClipTime << " μs" << std::endl;
    std::cout << "  With clipping: " << withClipTime << " μs" << std::endl;
    std::cout << "  Overhead: " << overhead << " μs (" << overheadPercent << "%)" << std::endl;
    
    // 裁剪开销应该小于 20%
    EXPECT_LT(overheadPercent, 20.0);
}

//==========================================================================================
// 性能测试 - 帧率和稳定性
//==========================================================================================

/**
 * 性能测试：60 FPS 目标帧率测试
 */
TEST_F(MetalRendererTest, PerformanceTargetFrameRate60FPS) {
    ASSERT_TRUE(initializeRenderer());
    
    const double targetFrameTime = 16666.67; // 60 FPS = 16.67ms per frame
    const int frameCount = 1000;
    
    std::vector<double> frameTimes;
    frameTimes.reserve(frameCount);
    
    for (int i = 0; i < frameCount; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        
        RenderList list = createTestRenderList(100);
        renderer->beginFrame();
        renderer->executeRenderCommands(list);
        renderer->endFrame();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        frameTimes.push_back(static_cast<double>(duration));
    }
    
    // 计算统计数据
    double avgFrameTime = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0) / frameCount;
    double minFrameTime = *std::min_element(frameTimes.begin(), frameTimes.end());
    double maxFrameTime = *std::max_element(frameTimes.begin(), frameTimes.end());
    
    // 计算标准差
    double variance = 0.0;
    for (double time : frameTimes) {
        variance += (time - avgFrameTime) * (time - avgFrameTime);
    }
    double stdDev = std::sqrt(variance / frameCount);
    
    std::cout << "\n[Performance] 60 FPS Target Test (" << frameCount << " frames):" << std::endl;
    std::cout << "  Target frame time: " << targetFrameTime << " μs (60 fps)" << std::endl;
    std::cout << "  Average frame time: " << avgFrameTime << " μs (" << (1000000.0 / avgFrameTime) << " fps)" << std::endl;
    std::cout << "  Min frame time: " << minFrameTime << " μs" << std::endl;
    std::cout << "  Max frame time: " << maxFrameTime << " μs" << std::endl;
    std::cout << "  Standard deviation: " << stdDev << " μs" << std::endl;
    std::cout << "  Frame time stability: " << (stdDev / avgFrameTime * 100.0) << "%" << std::endl;
    
    // 平均帧时间应该小于目标
    EXPECT_LT(avgFrameTime, targetFrameTime);
    
    // 帧时间稳定性应该好于 20%
    EXPECT_LT(stdDev / avgFrameTime, 0.20);
}

/**
 * 性能测试：240 FPS 高帧率测试
 */
TEST_F(MetalRendererTest, PerformanceHighFrameRate240FPS) {
    ASSERT_TRUE(initializeRenderer());
    
    const double targetFrameTime = 4166.67; // 240 FPS = 4.17ms per frame
    const int frameCount = 1000;
    
    std::vector<double> frameTimes;
    frameTimes.reserve(frameCount);
    
    // 使用较少的绘制命令来达到高帧率
    for (int i = 0; i < frameCount; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        
        RenderList list = createTestRenderList(10);
        renderer->beginFrame();
        renderer->executeRenderCommands(list);
        renderer->endFrame();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        frameTimes.push_back(static_cast<double>(duration));
    }
    
    double avgFrameTime = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0) / frameCount;
    
    std::cout << "\n[Performance] 240 FPS High Frame Rate Test:" << std::endl;
    std::cout << "  Target frame time: " << targetFrameTime << " μs (240 fps)" << std::endl;
    std::cout << "  Average frame time: " << avgFrameTime << " μs (" << (1000000.0 / avgFrameTime) << " fps)" << std::endl;
    
    // 应该能达到 240 FPS（对于轻量级渲染）
    EXPECT_LT(avgFrameTime, targetFrameTime * 1.2); // 允许 20% 误差
}

/**
 * 性能测试：帧时间一致性测试
 *
 * 验证连续帧之间的时间变化是否平滑
 */
TEST_F(MetalRendererTest, PerformanceFrameTimeConsistency) {
    ASSERT_TRUE(initializeRenderer());
    
    const int frameCount = 500;
    std::vector<double> frameTimes;
    std::vector<double> frameDeltas;
    frameTimes.reserve(frameCount);
    frameDeltas.reserve(frameCount - 1);
    
    for (int i = 0; i < frameCount; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        
        RenderList list = createTestRenderList(50);
        renderer->beginFrame();
        renderer->executeRenderCommands(list);
        renderer->endFrame();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        frameTimes.push_back(static_cast<double>(duration));
        
        if (i > 0) {
            frameDeltas.push_back(std::abs(frameTimes[i] - frameTimes[i-1]));
        }
    }
    
    double avgDelta = std::accumulate(frameDeltas.begin(), frameDeltas.end(), 0.0) / frameDeltas.size();
    double maxDelta = *std::max_element(frameDeltas.begin(), frameDeltas.end());
    
    std::cout << "\n[Performance] Frame Time Consistency:" << std::endl;
    std::cout << "  Average frame-to-frame delta: " << avgDelta << " μs" << std::endl;
    std::cout << "  Maximum frame-to-frame delta: " << maxDelta << " μs" << std::endl;
    
    // 帧间差异应该相对较小
    double avgFrameTime = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0) / frameCount;
    EXPECT_LT(avgDelta, avgFrameTime * 0.30); // 平均差异小于 30%
    EXPECT_LT(maxDelta, avgFrameTime * 2.0);  // 最大差异小于 200%
}

//==========================================================================================
// 性能测试 - 内存和资源
//==========================================================================================

/**
 * 性能测试：纹理创建和销毁开销
 */
TEST_F(MetalRendererTest, PerformanceTextureCreationDestruction) {
    ASSERT_TRUE(initializeRenderer());
    
    const int iterationCount = 100;
    
    // 测试纹理创建
    auto createFunc = [this]() {
        void* texture = renderer->createTexture2D(256, 256, TextureFormat::RGBA8_Unorm);
        return texture;
    };
    
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<void*> textures;
    for (int i = 0; i < iterationCount; ++i) {
        textures.push_back(createFunc());
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto createTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    // 测试纹理销毁
    start = std::chrono::high_resolution_clock::now();
    for (void* texture : textures) {
        renderer->destroyTexture(texture);
    }
    end = std::chrono::high_resolution_clock::now();
    auto destroyTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    double avgCreateTime = static_cast<double>(createTime) / iterationCount;
    double avgDestroyTime = static_cast<double>(destroyTime) / iterationCount;
    
    std::cout << "\n[Performance] Texture Creation/Destruction (256x256 RGBA):" << std::endl;
    std::cout << "  Average creation time: " << avgCreateTime << " μs" << std::endl;
    std::cout << "  Average destruction time: " << avgDestroyTime << " μs" << std::endl;
    std::cout << "  Total time per texture: " << (avgCreateTime + avgDestroyTime) << " μs" << std::endl;
    
    // 纹理操作应该相对快速
    EXPECT_LT(avgCreateTime, 1000.0);  // 创建应小于 1ms
    EXPECT_LT(avgDestroyTime, 100.0);  // 销毁应小于 0.1ms
}

/**
 * 性能测试：纹理更新开销
 */
TEST_F(MetalRendererTest, PerformanceTextureUpdate) {
    ASSERT_TRUE(initializeRenderer());
    
    const uint32_t width = 512;
    const uint32_t height = 512;
    void* texture = renderer->createTexture2D(width, height, TextureFormat::RGBA8_Unorm);
    ASSERT_NE(texture, nullptr);
    
    std::vector<uint8_t> testData(width * height * 4, 128);
    
    // 测试全量更新
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; ++i) {
        renderer->updateTexture2D(texture, 0, 0, width, height, testData.data(), width * 4);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto fullUpdateTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    // 测试部分更新
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; ++i) {
        renderer->updateTexture2D(texture, 128, 128, 256, 256, testData.data(), 256 * 4);
    }
    end = std::chrono::high_resolution_clock::now();
    auto partialUpdateTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    std::cout << "\n[Performance] Texture Update (512x512):" << std::endl;
    std::cout << "  Full update: " << (fullUpdateTime / 100.0) << " μs per update" << std::endl;
    std::cout << "  Partial update (256x256): " << (partialUpdateTime / 100.0) << " μs per update" << std::endl;
    
    renderer->destroyTexture(texture);
    
    // 部分更新应该比全量更新快
    EXPECT_LT(partialUpdateTime, fullUpdateTime);
}

/**
 * 性能测试：大量小纹理 vs 少量大纹理
 */
TEST_F(MetalRendererTest, PerformanceTextureGranularity) {
    ASSERT_TRUE(initializeRenderer());
    
    // 大量小纹理（128个 64x64）
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<void*> smallTextures;
    for (int i = 0; i < 128; ++i) {
        smallTextures.push_back(renderer->createTexture2D(64, 64, TextureFormat::RGBA8_Unorm));
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto smallTexturesTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    // 少量大纹理（2个 512x512）
    start = std::chrono::high_resolution_clock::now();
    std::vector<void*> largeTextures;
    for (int i = 0; i < 2; ++i) {
        largeTextures.push_back(renderer->createTexture2D(512, 512, TextureFormat::RGBA8_Unorm));
    }
    end = std::chrono::high_resolution_clock::now();
    auto largeTexturesTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    std::cout << "\n[Performance] Texture Granularity (same total memory):" << std::endl;
    std::cout << "  128 small textures (64x64): " << smallTexturesTime << " μs" << std::endl;
    std::cout << "  2 large textures (512x512): " << largeTexturesTime << " μs" << std::endl;
    
    // 清理
    for (void* texture : smallTextures) renderer->destroyTexture(texture);
    for (void* texture : largeTextures) renderer->destroyTexture(texture);
}

//==========================================================================================
// 边界测试 - 极端输入
//==========================================================================================

/**
 * 边界测试：空命令列表
 */
TEST_F(MetalRendererTest, BoundaryEmptyCommandList) {
    ASSERT_TRUE(initializeRenderer());
    
    RenderList emptyList;
    
    renderer->beginFrame();
    EXPECT_NO_THROW(renderer->executeRenderCommands(emptyList));
    renderer->endFrame();
}

/**
 * 边界测试：最大命令数量
 */
TEST_F(MetalRendererTest, BoundaryMaximumCommands) {
    ASSERT_TRUE(initializeRenderer());
    
    RenderList list = createTestRenderList(Config::Rendering::MAX_COMMANDS_PER_LIST - 1);
    
    renderer->beginFrame();
    EXPECT_NO_THROW(renderer->executeRenderCommands(list));
    renderer->endFrame();
}

/**
 * 边界测试：极小矩形
 */
TEST_F(MetalRendererTest, BoundaryTinyRectangle) {
    ASSERT_TRUE(initializeRenderer());
    
    RenderList list;
    
    // 1x1 像素矩形
    list.fillRect(YuchenUI::Rect(100, 100, 1, 1), Vec4::FromRGBA(255, 0, 0, 255), CornerRadius());
    
    // 0.1x0.1 像素矩形（子像素）
    list.fillRect(YuchenUI::Rect(100, 100, 0.1f, 0.1f), Vec4::FromRGBA(0, 255, 0, 255), CornerRadius());
    
    renderer->beginFrame();
    EXPECT_NO_THROW(renderer->executeRenderCommands(list));
    renderer->endFrame();
}

/**
 * 边界测试：超大矩形
 */
TEST_F(MetalRendererTest, BoundaryHugeRectangle) {
    ASSERT_TRUE(initializeRenderer());
    
    RenderList list;
    
    // 超出屏幕的巨大矩形
    list.fillRect(YuchenUI::Rect(-1000, -1000, 10000, 10000), Vec4::FromRGBA(255, 0, 0, 255), CornerRadius());
    
    renderer->beginFrame();
    EXPECT_NO_THROW(renderer->executeRenderCommands(list));
    renderer->endFrame();
}

/**
 * 边界测试：极大圆角
 */
TEST_F(MetalRendererTest, BoundaryExtremeCornerRadius) {
    ASSERT_TRUE(initializeRenderer());
    
    RenderList list;
    
    // 圆角等于矩形一半（完全圆形）
    list.fillRect(YuchenUI::Rect(100, 100, 100, 100), Vec4::FromRGBA(255, 0, 0, 255), CornerRadius(50.0f));
        
    renderer->beginFrame();
    EXPECT_NO_THROW(renderer->executeRenderCommands(list));
    renderer->endFrame();
}

/**
 * 边界测试：负坐标
 */
TEST_F(MetalRendererTest, BoundaryNegativeCoordinates) {
    ASSERT_TRUE(initializeRenderer());
    
    RenderList list;
    
    // 负坐标矩形
    list.fillRect(YuchenUI::Rect(-50, -50, 100, 100), Vec4::FromRGBA(255, 0, 0, 255), CornerRadius());
    
    // 负坐标线段
    list.drawLine(Vec2(-100, 50), Vec2(100, 50), Vec4::FromRGBA(0, 255, 0, 255), 2.0f);
    
    // 负坐标圆形
    list.fillCircle(Vec2(-50, 150), 40.0f, Vec4::FromRGBA(0, 0, 255, 255));
    
    renderer->beginFrame();
    EXPECT_NO_THROW(renderer->executeRenderCommands(list));
    renderer->endFrame();
}

/**
 * 边界测试：透明度边界
 */
TEST_F(MetalRendererTest, BoundaryAlphaValues) {
    ASSERT_TRUE(initializeRenderer());
    
    RenderList list;
    
    // 完全透明
    list.fillRect(YuchenUI::Rect(10, 10, 100, 50), Vec4::FromRGBA(255, 0, 0, 0), CornerRadius());
    
    // 几乎透明
    list.fillRect(YuchenUI::Rect(120, 10, 100, 50), Vec4::FromRGBA(255, 0, 0, 1), CornerRadius());
    
    // 半透明
    list.fillRect(YuchenUI::Rect(230, 10, 100, 50), Vec4::FromRGBA(255, 0, 0, 128), CornerRadius());
    
    // 几乎不透明
    list.fillRect(YuchenUI::Rect(340, 10, 100, 50), Vec4::FromRGBA(255, 0, 0, 254), CornerRadius());
    
    // 完全不透明
    list.fillRect(YuchenUI::Rect(450, 10, 100, 50), Vec4::FromRGBA(255, 0, 0, 255), CornerRadius());
    
    renderer->beginFrame();
    EXPECT_NO_THROW(renderer->executeRenderCommands(list));
    renderer->endFrame();
}

/**
 * 边界测试：深度嵌套裁剪
 */
TEST_F(MetalRendererTest, BoundaryDeepNestedClipping) {
    ASSERT_TRUE(initializeRenderer());
    
    RenderList list;
    
    // 创建 20 层嵌套裁剪
    for (int i = 0; i < 20; ++i) {
        float margin = i * 10.0f;
        list.pushClipRect(YuchenUI::Rect(margin, margin, 500 - margin * 2, 500 - margin * 2));
        list.fillRect(YuchenUI::Rect(margin + 5, margin + 5, 50, 50), Vec4::FromRGBA(255, 0, 0, 128), CornerRadius());
    }
    
    // 恢复所有裁剪
    for (int i = 0; i < 20; ++i) {
        list.popClipRect();
    }
    
    renderer->beginFrame();
    EXPECT_NO_THROW(renderer->executeRenderCommands(list));
    renderer->endFrame();
}

/**
 * 边界测试：裁剪栈不平衡
 */
TEST_F(MetalRendererTest, BoundaryUnbalancedClipStack) {
    ASSERT_TRUE(initializeRenderer());
    
    RenderList list;
    
    // Push 但不 Pop
    list.pushClipRect(YuchenUI::Rect(50, 50, 200, 200));
    list.fillRect(YuchenUI::Rect(60, 60, 100, 50), Vec4::FromRGBA(255, 0, 0, 255), CornerRadius());
    // 故意不调用 popClipRect()
    
    // 渲染器应该能够处理这种情况
    renderer->beginFrame();
    EXPECT_NO_THROW(renderer->executeRenderCommands(list));
    renderer->endFrame();
}

//==========================================================================================
// 压力测试 - 极限场景
//==========================================================================================

/**
 * 压力测试：连续多帧渲染
 */
TEST_F(MetalRendererTest, StressContinuousFrameRendering) {
    ASSERT_TRUE(initializeRenderer());
    
    const int frameCount = 10000;
    
    for (int i = 0; i < frameCount; ++i) {
        RenderList list;
        list.fillRect(YuchenUI::Rect(10, 10, 100, 50), Vec4::FromRGBA(255, 0, 0, 255), CornerRadius());
        
        renderer->beginFrame();
        renderer->executeRenderCommands(list);
        renderer->endFrame();
    }
    
    // 应该能够稳定渲染大量帧
    SUCCEED();
}

/**
 * 压力测试：随机渲染命令
 */
TEST_F(MetalRendererTest, StressRandomCommands) {
    ASSERT_TRUE(initializeRenderer());
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> typeDist(0, 6);
    std::uniform_real_distribution<> posDist(0.0f, 1000.0f);
    std::uniform_real_distribution<> sizeDist(10.0f, 200.0f);
    std::uniform_int_distribution<> colorDist(0, 255);
    
    RenderList list;
    int clipDepth = 0;
    
    for (int i = 0; i < 1000; ++i) {
        int type = typeDist(gen);
        float x = posDist(gen);
        float y = posDist(gen);
        float w = sizeDist(gen);
        float h = sizeDist(gen);
        Vec4 color = Vec4::FromRGBA(colorDist(gen), colorDist(gen),
                                     colorDist(gen), colorDist(gen));
        
        switch (type) {
            case 0:
                list.fillRect(YuchenUI::Rect(x, y, w, h), color, CornerRadius(5.0f));
                break;
            case 1:
                list.drawRect(YuchenUI::Rect(x, y, w, h), color, 2.0f, CornerRadius(5.0f));
                break;
            case 2:
                list.drawLine(Vec2(x, y), Vec2(x + w, y + h), color, 2.0f);
                break;
            case 3:
                list.fillTriangle(Vec2(x, y), Vec2(x + w, y),
                                 Vec2(x + w/2, y + h), color);
                break;
            case 4:
                list.fillCircle(Vec2(x, y), w / 2, color);
                break;
            case 5:
                list.pushClipRect(YuchenUI::Rect(x, y, w, h));
                clipDepth++;
                break;
            case 6:
                if (clipDepth > 0) {
                    list.popClipRect();
                    clipDepth--;
                }
                break;
        }
    }
    
    while (clipDepth > 0) {
        list.popClipRect();
        clipDepth--;
    }
    
    renderer->beginFrame();
    EXPECT_NO_THROW(renderer->executeRenderCommands(list));
    renderer->endFrame();
}

/**
 * 压力测试：内存泄漏检测
 *
 * 通过重复创建和销毁纹理来检测内存泄漏
 */
TEST_F(MetalRendererTest, StressMemoryLeakDetection) {
    ASSERT_TRUE(initializeRenderer());
    
    const int iterations = 1000;
    
    for (int i = 0; i < iterations; ++i) {
        // 创建纹理
        void* texture = renderer->createTexture2D(256, 256, TextureFormat::RGBA8_Unorm);
        ASSERT_NE(texture, nullptr);
        
        // 更新纹理
        std::vector<uint8_t> data(256 * 256 * 4, 128);
        renderer->updateTexture2D(texture, 0, 0, 256, 256, data.data(), 256 * 4);
        
        // 销毁纹理
        renderer->destroyTexture(texture);
    }
    
    // 如果有内存泄漏，这个测试会导致内存增长
    // 需要配合内存分析工具（如 Instruments）来验证
    SUCCEED();
}

//==========================================================================================
// 集成测试 - 真实场景模拟
//==========================================================================================

/**
 * 集成测试：UI 界面渲染模拟
 *
 * 模拟真实的 UI 界面渲染场景
 */
TEST_F(MetalRendererTest, IntegrationUISceneSimulation) {
    ASSERT_TRUE(initializeRenderer());
    
    RenderList list;
    
    // 背景
    list.clear(Vec4(0.95f, 0.95f, 0.95f, 1.0f));
    
    // 标题栏
    list.fillRect(YuchenUI::Rect(0, 0, 1920, 60), Vec4::FromRGBA(50, 50, 50, 255), CornerRadius());
    
    FontFallbackChain chain(1);
    list.drawText("YuchenUI Application", Vec2(20, 20), chain, 18.0f, Vec4::FromRGBA(255, 255, 255, 255));
    
    // 侧边栏
    list.fillRect(YuchenUI::Rect(0, 60, 250, 1020), Vec4::FromRGBA(240, 240, 240, 255), CornerRadius());
    
    // 侧边栏项目
    for (int i = 0; i < 10; ++i) {
        float y = 80 + i * 50;
        list.fillRect(YuchenUI::Rect(10, y, 230, 40), Vec4::FromRGBA(255, 255, 255, 255), CornerRadius(5.0f));
        list.drawText("Menu Item", Vec2(20, y + 12), chain, 14.0f, Vec4::FromRGBA(50, 50, 50, 255));
    }
    
    // 主内容区域
    list.fillRect(YuchenUI::Rect(250, 60, 1670, 1020), Vec4::FromRGBA(255, 255, 255, 255), CornerRadius());
    
    // 内容卡片
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 4; ++col) {
            float x = 270 + col * 400;
            float y = 80 + row * 320;
            list.fillRect(YuchenUI::Rect(x, y, 380, 300), Vec4::FromRGBA(250, 250, 250, 255), CornerRadius(8.0f));
            list.drawRect(YuchenUI::Rect(x, y, 380, 300), Vec4::FromRGBA(200, 200, 200, 255), 1.0f, CornerRadius(8.0f));
        }
    }
    
    renderer->beginFrame();
    EXPECT_NO_THROW(renderer->executeRenderCommands(list));
    renderer->endFrame();
}

/**
 * 集成测试：游戏 HUD 渲染模拟
 */
TEST_F(MetalRendererTest, IntegrationGameHUDSimulation) {
    ASSERT_TRUE(initializeRenderer());
    
    RenderList list;
    
    // 半透明背景
    list.clear(Vec4(0.0f, 0.0f, 0.0f, 0.5f));
    
    // 生命值条
    list.fillRect(YuchenUI::Rect(20, 20, 300, 30), Vec4::FromRGBA(100, 100, 100, 200), CornerRadius(15.0f));
    list.fillRect(YuchenUI::Rect(25, 25, 200, 20), Vec4::FromRGBA(255, 50, 50, 255), CornerRadius(10.0f));
    
    // 能量条
    list.fillRect(YuchenUI::Rect(20, 60, 300, 30), Vec4::FromRGBA(100, 100, 100, 200), CornerRadius(15.0f));
    list.fillRect(YuchenUI::Rect(25, 65, 150, 20), Vec4::FromRGBA(50, 150, 255, 255), CornerRadius(10.0f));
    
    // 小地图
    list.pushClipRect(YuchenUI::Rect(1720, 20, 180, 180));
    list.fillRect(YuchenUI::Rect(1720, 20, 180, 180), Vec4::FromRGBA(50, 50, 50, 200), CornerRadius(5.0f));
    
    // 小地图上的点
    for (int i = 0; i < 20; ++i) {
        float x = 1730 + (i % 5) * 35;
        float y = 30 + (i / 5) * 35;
        list.fillCircle(Vec2(x, y), 3.0f, Vec4::FromRGBA(255, 255, 0, 255));
    }
    list.popClipRect();
    
    // 准星
    list.drawLine(Vec2(950, 530), Vec2(970, 550), Vec4::FromRGBA(255, 255, 255, 200), 2.0f);
    list.drawLine(Vec2(970, 530), Vec2(950, 550), Vec4::FromRGBA(255, 255, 255, 200), 2.0f);
    list.drawCircle(Vec2(960, 540), 20.0f, Vec4::FromRGBA(255, 255, 255, 150), 2.0f);
    
    renderer->beginFrame();
    EXPECT_NO_THROW(renderer->executeRenderCommands(list));
    renderer->endFrame();
}
