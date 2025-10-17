/*******************************************************************************************
**
** LevelMeter_test.cpp - Comprehensive Unit Tests with Performance Analysis
**
** 这个测试文件将揭露LevelMeter的严重性能问题
**
********************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "YuchenUI/widgets_expand/LevelMeter.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/core/UIContext.h"
#include "YuchenUI/theme/Theme.h"
#include "YuchenUI/theme/IThemeProvider.h"
#include "YuchenUI/text/IFontProvider.h"
#include <chrono>
#include <vector>
#include <unordered_map>
#include <iomanip>

using namespace YuchenUI;
using namespace testing;

//==========================================================================================
// Mock Classes
//==========================================================================================

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

class MockUIStyle : public UIStyle {
public:
    LevelMeterColors getLevelMeterColors() const override {
        LevelMeterColors colors;
        colors.levelNormal = Vec4::FromRGBA(0, 255, 0, 255);
        colors.levelWarning = Vec4::FromRGBA(255, 255, 0, 255);
        colors.levelPeak = Vec4::FromRGBA(255, 0, 0, 255);
        colors.bgNormal = Vec4::FromRGBA(50, 50, 50, 255);
        colors.bgWarning = Vec4::FromRGBA(80, 80, 0, 255);
        colors.bgPeak = Vec4::FromRGBA(80, 0, 0, 255);
        colors.border = Vec4::FromRGBA(100, 100, 100, 255);
        colors.peakIndicatorActive = Vec4::FromRGBA(255, 0, 0, 255);
        colors.peakIndicatorInactive = Vec4::FromRGBA(50, 0, 0, 255);
        colors.scaleColor = Vec4::FromRGBA(200, 200, 200, 255);
        colors.internalScaleNormalActive = Vec4::FromRGBA(0, 255, 0, 255);
        colors.internalScaleNormalInactive = Vec4::FromRGBA(0, 100, 0, 255);
        colors.internalScaleWarningActive = Vec4::FromRGBA(255, 255, 0, 255);
        colors.internalScaleWarningInactive = Vec4::FromRGBA(100, 100, 0, 255);
        colors.internalScalePeakActive = Vec4::FromRGBA(255, 0, 0, 255);
        colors.internalScalePeakInactive = Vec4::FromRGBA(100, 0, 0, 255);
        return colors;
    }

    void drawNormalButton(const ButtonDrawInfo&, RenderList&) override {}
    void drawPrimaryButton(const ButtonDrawInfo&, RenderList&) override {}
    void drawDestructiveButton(const ButtonDrawInfo&, RenderList&) override {}
    void drawFrame(const FrameDrawInfo&, RenderList&) override {}
    void drawGroupBox(const GroupBoxDrawInfo&, RenderList&) override {}
    void drawScrollbarTrack(const ScrollbarTrackDrawInfo&, RenderList&) override {}
    void drawScrollbarThumb(const ScrollbarThumbDrawInfo&, RenderList&) override {}
    void drawScrollbarButton(const ScrollbarButtonDrawInfo&, RenderList&) override {}
    void drawTextInput(const TextInputDrawInfo&, RenderList&) override {}
    void drawSpinBox(const SpinBoxDrawInfo&, RenderList&) override {}
    void drawComboBox(const ComboBoxDrawInfo&, RenderList&) override {}
    void drawFocusIndicator(const FocusIndicatorDrawInfo&, RenderList&) override {}
    void drawCheckBox(const CheckBoxDrawInfo&, RenderList&) override {}
    void drawRadioButton(const RadioButtonDrawInfo&, RenderList&) override {}
    void drawKnob(const KnobDrawInfo&, RenderList&) override {}
    
    Vec4 getWindowBackground(WindowType) const override { return Vec4(); }
    Vec4 getDefaultTextColor() const override { return Vec4::FromRGBA(255, 255, 255, 255); }
    FontFallbackChain getDefaultButtonFontChain() const override { return FontFallbackChain(1); }
    FontFallbackChain getDefaultLabelFontChain() const override { return FontFallbackChain(1); }
    FontFallbackChain getDefaultTitleFontChain() const override { return FontFallbackChain(1); }
    Vec4 getDefaultFrameBackground() const override { return Vec4(); }
    Vec4 getDefaultFrameBorder() const override { return Vec4(); }
    Vec4 getDefaultGroupBoxBackground() const override { return Vec4(); }
    Vec4 getDefaultGroupBoxBorder() const override { return Vec4(); }
    Vec4 getDefaultScrollAreaBackground() const override { return Vec4(); }
    float getGroupBoxTitleBarHeight() const override { return 20.0f; }
    FaderColors getFaderColors() const override { return FaderColors(); }
};

class MockThemeProvider : public IThemeProvider {
public:
    MockThemeProvider() : style_(std::make_unique<MockUIStyle>()) {}
    
    UIStyle* getCurrentStyle() const override { return style_.get(); }
    
    void setStyle(std::unique_ptr<UIStyle> style) override {
        if (style) {
            style_ = std::move(style);
        }
    }
    
    void setFontProvider(IFontProvider* provider) override {
        fontProvider_ = provider;
        if (style_) {
            style_->setFontProvider(provider);
        }
    }
    
private:
    std::unique_ptr<UIStyle> style_;
    IFontProvider* fontProvider_ = nullptr;
};

// 性能统计用的RenderList包装器
class InstrumentedRenderList : public RenderList {
public:
    struct CallStats {
        size_t fillRectCalls = 0;
        size_t drawRectCalls = 0;
        size_t drawLineCalls = 0;
        size_t drawTextCalls = 0;
        size_t totalCalls = 0;
        
        void reset() {
            fillRectCalls = 0;
            drawRectCalls = 0;
            drawLineCalls = 0;
            drawTextCalls = 0;
            totalCalls = 0;
        }
        
        std::string toString() const {
            char buf[256];
            snprintf(buf, sizeof(buf),
                "总调用: %zu | fillRect: %zu | drawRect: %zu | drawLine: %zu | drawText: %zu",
                totalCalls, fillRectCalls, drawRectCalls, drawLineCalls, drawTextCalls);
            return buf;
        }
    };
    
    void fillRect(const Rect& rect, const Vec4& color, const CornerRadius& cornerRadius = CornerRadius()) {
        stats_.fillRectCalls++;
        stats_.totalCalls++;
        RenderList::fillRect(rect, color, cornerRadius);
    }
    
    void drawRect(const Rect& rect, const Vec4& color, float borderWidth,
                  const CornerRadius& cornerRadius = CornerRadius()) {
        stats_.drawRectCalls++;
        stats_.totalCalls++;
        RenderList::drawRect(rect, color, borderWidth, cornerRadius);
    }
    
    void drawLine(const Vec2& start, const Vec2& end, const Vec4& color, float width = 1.0f) {
        stats_.drawLineCalls++;
        stats_.totalCalls++;
        RenderList::drawLine(start, end, color, width);
    }
    
    void drawText(const char* text, const Vec2& position, const FontFallbackChain& fallbackChain,
                  float fontSize, const Vec4& color, float letterSpacing = 0.0f) {
        stats_.drawTextCalls++;
        stats_.totalCalls++;
        RenderList::drawText(text, position, fallbackChain, fontSize, color, letterSpacing);
    }
    
    const CallStats& getStats() const { return stats_; }
    void resetStats() { stats_.reset(); }
    
private:
    CallStats stats_;
};

//==========================================================================================
// Test Fixtures
//==========================================================================================

class LevelMeterTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockFontProvider_ = std::make_unique<NiceMock<MockFontProvider>>();
        mockThemeProvider_ = std::make_unique<MockThemeProvider>();
        
        // 设置基本的font provider行为
        ON_CALL(*mockFontProvider_, getDefaultNarrowBoldFont())
            .WillByDefault(Return(FontHandle(1)));
        ON_CALL(*mockFontProvider_, measureText(_, _))
            .WillByDefault(Return(Vec2(50.0f, 10.0f)));
        
        FontMetrics defaultMetrics;
        defaultMetrics.ascender = 8.0f;
        defaultMetrics.descender = -2.0f;
        defaultMetrics.lineHeight = 12.0f;
        defaultMetrics.maxAdvance = 10.0f;
        
        ON_CALL(*mockFontProvider_, getFontMetrics(_, _))
            .WillByDefault(Return(defaultMetrics));
        
        // 使用指针转换
        IFontProvider* fontProviderPtr = mockFontProvider_.get();
        IThemeProvider* themeProviderPtr = mockThemeProvider_.get();
        
        uiContext_ = std::make_unique<UIContext>(fontProviderPtr, themeProviderPtr);
    }
    
    void TearDown() override {
        uiContext_.reset();
        mockThemeProvider_.reset();
        mockFontProvider_.reset();
    }
    
    std::unique_ptr<NiceMock<MockFontProvider>> mockFontProvider_;
    std::unique_ptr<MockThemeProvider> mockThemeProvider_;
    std::unique_ptr<UIContext> uiContext_;
};


//==========================================================================================
// 1. 功能测试 - MeterScale
//==========================================================================================

TEST_F(LevelMeterTest, MeterScale_SamplePeak_DbMapping) {
    MeterScale scale(ScaleType::SAMPLE_PEAK);
    
    // 测试关键点的映射精度
    EXPECT_NEAR(scale.mapDbToPosition(0.0f), 1.0f, 0.001f);      // 0dB -> 顶部
    EXPECT_NEAR(scale.mapDbToPosition(-6.0f), 0.925f, 0.01f);    // -6dB
    EXPECT_NEAR(scale.mapDbToPosition(-20.0f), 0.686f, 0.01f);   // -20dB
    EXPECT_NEAR(scale.mapDbToPosition(-40.0f), 0.1875f, 0.001f); // -40dB -> 18.75%
    EXPECT_NEAR(scale.mapDbToPosition(-60.0f), 0.01118f, 0.001f);// -60dB
    EXPECT_NEAR(scale.mapDbToPosition(-144.0f), 0.0f, 0.001f);   // -144dB -> 底部
    
    // 测试双向映射
    for (float db : {0.0f, -6.0f, -12.0f, -20.0f, -40.0f, -60.0f}) {
        float pos = scale.mapDbToPosition(db);
        float recoveredDb = scale.mapPositionToDb(pos);
        EXPECT_NEAR(db, recoveredDb, 0.1f) << "双向映射失败: " << db << "dB";
    }
}

TEST_F(LevelMeterTest, MeterScale_K12_LinearMapping) {
    MeterScale scale(ScaleType::K12);
    
    // K12应该是线性的
    EXPECT_NEAR(scale.mapDbToPosition(12.0f), 1.0f, 0.001f);
    EXPECT_NEAR(scale.mapDbToPosition(0.0f), 0.8333f, 0.01f);   // (0 - (-60)) / 72
    EXPECT_NEAR(scale.mapDbToPosition(-24.0f), 0.5f, 0.01f);    // 中点
    EXPECT_NEAR(scale.mapDbToPosition(-60.0f), 0.0f, 0.001f);
}

TEST_F(LevelMeterTest, MeterScale_AllTypes_ValidRange) {
    std::vector<ScaleType> types = {
        ScaleType::SAMPLE_PEAK,
        ScaleType::K12,
        ScaleType::K14,
        ScaleType::VU,
        ScaleType::LINEAR_DB
    };
    
    for (auto type : types) {
        MeterScale scale(type);
        
        // 测试范围边界
        EXPECT_GE(scale.mapDbToPosition(scale.getMinDb()), 0.0f);
        EXPECT_LE(scale.mapDbToPosition(scale.getMaxDb()), 1.0f);
        
        // 测试刻度标记
        const auto& ticks = scale.getTickMarks();
        EXPECT_GT(ticks.size(), 0) << "Scale type " << static_cast<int>(type) << " 没有刻度";
        
        for (const auto& tick : ticks) {
            EXPECT_GE(tick.position, 0.0f);
            EXPECT_LE(tick.position, 1.0f);
        }
    }
}

//==========================================================================================
// 2. 功能测试 - ChannelLevelData
//==========================================================================================

TEST_F(LevelMeterTest, ChannelLevelData_BasicDecay) {
    ChannelLevelData channel;
    channel.setDecayRate(40.0f);  // 40dB/s
    
    // 初始状态
    EXPECT_FLOAT_EQ(channel.getCurrentLevel(), -144.0f);
    EXPECT_FLOAT_EQ(channel.getDisplayLevel(), -144.0f);
    
    // 上升到-12dB
    channel.updateLevel(-12.0f, 16.0f);  // 16ms一帧
    EXPECT_FLOAT_EQ(channel.getCurrentLevel(), -12.0f);
    EXPECT_FLOAT_EQ(channel.getDisplayLevel(), -12.0f);
    
    // 信号停止，应该衰减
    channel.updateLevel(-144.0f, 16.0f);
    float expectedDecay = -12.0f - (40.0f * 0.016f);  // -12.64dB
    EXPECT_NEAR(channel.getDisplayLevel(), expectedDecay, 0.1f);
}

TEST_F(LevelMeterTest, ChannelLevelData_PeakHold) {
    ChannelLevelData channel;
    channel.setPeakHoldTime(1000.0f);  // 1秒峰值保持
    
    // 峰值触发
    channel.updateLevel(-6.0f, 16.0f);
    EXPECT_FLOAT_EQ(channel.getPeakLevel(), -6.0f);
    
    // 信号下降，但峰值应该保持
    channel.updateLevel(-20.0f, 16.0f);
    EXPECT_FLOAT_EQ(channel.getPeakLevel(), -6.0f);
    
    // 等待峰值保持时间结束
    for (int i = 0; i < 70; ++i) {  // 70帧 * 16ms ≈ 1120ms
        channel.updateLevel(-20.0f, 16.0f);
    }
    
    // 峰值应该开始衰减
    EXPECT_LT(channel.getPeakLevel(), -6.0f);
}

TEST_F(LevelMeterTest, ChannelLevelData_PeakIndicator) {
    ChannelLevelData channel;
    
    // 未触发峰值指示器
    channel.updateLevel(-12.0f, 16.0f);
    EXPECT_FALSE(channel.isPeakIndicatorActive());
    
    // 触发峰值指示器（>= -0.1dB）
    channel.updateLevel(-0.05f, 16.0f);
    EXPECT_TRUE(channel.isPeakIndicatorActive());
    
    // 信号下降，但指示器应该保持3秒
    channel.updateLevel(-20.0f, 16.0f);
    EXPECT_TRUE(channel.isPeakIndicatorActive());
}

//==========================================================================================
// 3. 性能测试 - 这是重点！
//==========================================================================================

class LevelMeterPerformanceTest : public LevelMeterTest {
protected:
    struct PerformanceResult {
        size_t channelCount;
        double renderTimeMs;
        size_t totalDrawCalls;
        size_t fillRectCalls;
        size_t drawLineCalls;
        
        std::string toString() const {
            char buf[512];
            snprintf(buf, sizeof(buf),
                "\n通道数: %2zu | 渲染时间: %6.2fms | "
                "总调用: %5zu | fillRect: %5zu | drawLine: %4zu | "
                "每通道调用数: %.0f",
                channelCount, renderTimeMs, totalDrawCalls, fillRectCalls, drawLineCalls,
                static_cast<double>(totalDrawCalls) / channelCount);
            return buf;
        }
    };
    
    PerformanceResult measureRenderPerformance(size_t channelCount, int iterations = 100) {
        Rect bounds(0, 0, 100, 240);
        LevelMeter meter(uiContext_.get(), bounds, channelCount, ScaleType::SAMPLE_PEAK);
        
        // 设置测试数据
        std::vector<float> levels(channelCount, -12.0f);
        meter.updateLevels(levels);
        
        InstrumentedRenderList cmdList;
        
        // 预热
        for (int i = 0; i < 10; ++i) {
            cmdList.reset();
            meter.addDrawCommands(cmdList);
        }
        
        // 正式测量
        cmdList.resetStats();
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            cmdList.reset();
            meter.addDrawCommands(cmdList);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        double totalMs = std::chrono::duration<double, std::milli>(end - start).count();
        
        PerformanceResult result;
        result.channelCount = channelCount;
        result.renderTimeMs = totalMs / iterations;
        result.totalDrawCalls = cmdList.getStats().totalCalls;
        result.fillRectCalls = cmdList.getStats().fillRectCalls;
        result.drawLineCalls = cmdList.getStats().drawLineCalls;
        
        return result;
    }
};

TEST_F(LevelMeterPerformanceTest, VerifyRenderingActuallyHappens) {
    Rect bounds(0, 0, 100, 240);
    LevelMeter meter(uiContext_.get(), bounds, 2, ScaleType::SAMPLE_PEAK);
    
    std::vector<float> levels = {-12.0f, -18.0f};
    meter.updateLevels(levels);
    
    RenderList cmdList;  // 使用普通的 RenderList
    meter.addDrawCommands(cmdList);
    
    // 检查是否有渲染命令
    const auto& commands = cmdList.getCommands();
    std::cout << "实际渲染命令数量: " << commands.size() << std::endl;
    
    EXPECT_GT(commands.size(), 0) << "LevelMeter 应该生成渲染命令";
}

TEST_F(LevelMeterPerformanceTest, RenderCallCount_SingleChannel) {
    auto result = measureRenderPerformance(1, 1);
    
    std::cout << "\n=== 单通道渲染调用统计 ===" << result.toString() << std::endl;
    
    // 预期：一个通道应该不超过100次调用
    // 实际：可能会有上千次！
    EXPECT_LT(result.totalDrawCalls, 100)
        << "单通道调用次数过多！这证实了逐像素渲染的性能问题";
}

TEST_F(LevelMeterPerformanceTest, CRITICAL_RenderCallCount_StereoChannels) {
    auto result = measureRenderPerformance(2, 1);
    
    std::cout << "\n=== 立体声（2通道）渲染调用统计 ===" << result.toString() << std::endl;
    
    // 一个7像素宽的通道，224像素高，分成3个区域
    // 预期合理调用：每通道约10-20次（背景3块 + 填充3块 + 峰值线 + 边框 + 刻度）
    // 实际可能：每通道上千次fillRect调用！
    
    size_t expectedMaxCalls = 50;  // 非常保守的估计
    EXPECT_LT(result.totalDrawCalls, expectedMaxCalls)
        << "\n❌ 严重性能问题！预期最多" << expectedMaxCalls << "次调用，实际"
        << result.totalDrawCalls << "次\n"
        << "这是逐像素渲染导致的，每个像素都调用一次fillRect！";
        
    // fillRect调用应该远小于通道宽度*高度
    size_t pixelCount = 7 * 224 * 2;  // 7px宽 * 224px高 * 2通道
    EXPECT_LT(result.fillRectCalls, pixelCount / 10)
        << "fillRect调用次数接近像素总数，证实逐像素渲染问题";
}

TEST_F(LevelMeterPerformanceTest, CRITICAL_PerformanceScaling_MultiChannel) {
    std::cout << "\n=== 多通道性能扩展测试 ===" << std::endl;
    std::cout << "这将揭露性能如何随通道数恶化...\n" << std::endl;
    
    std::vector<size_t> channelCounts = {1, 2, 4, 8, 16, 32};
    std::vector<PerformanceResult> results;
    
    for (size_t count : channelCounts) {
        auto result = measureRenderPerformance(count, 50);
        results.push_back(result);
        std::cout << result.toString() << std::endl;
    }
    
    // 分析性能扩展性
    std::cout << "\n=== 性能扩展性分析 ===" << std::endl;
    for (size_t i = 1; i < results.size(); ++i) {
        double timeRatio = results[i].renderTimeMs / results[0].renderTimeMs;
        double callRatio = static_cast<double>(results[i].totalDrawCalls) / results[0].totalDrawCalls;
        size_t channelRatio = results[i].channelCount / results[0].channelCount;
        
        std::cout << "通道数x" << channelRatio
                  << " -> 时间x" << std::fixed << std::setprecision(1) << timeRatio
                  << " | 调用数x" << std::fixed << std::setprecision(1) << callRatio << std::endl;
    }
    
    // 理想情况：调用次数应该线性增长（O(n)）
    // 实际情况：由于逐像素渲染，会精确线性增长，但基数太大！
    double callGrowthRate = static_cast<double>(results.back().totalDrawCalls) /
                           results.front().totalDrawCalls /
                           (results.back().channelCount / results.front().channelCount);
    
    EXPECT_NEAR(callGrowthRate, 1.0, 0.2)
        << "调用次数增长不是线性的，说明有额外问题";
        
    // 但是，即使是线性的，如果基数太大也不行
    EXPECT_LT(results.back().totalDrawCalls, 1000)
        << "32通道的总调用次数: " << results.back().totalDrawCalls
        << " - 这太多了！";
}

TEST_F(LevelMeterPerformanceTest, CRITICAL_60FPS_Feasibility) {
    std::cout << "\n=== 60FPS可行性测试 ===" << std::endl;
    std::cout << "目标：单帧 < 16.67ms（60fps）\n" << std::endl;
    
    constexpr double TARGET_FRAME_TIME_MS = 16.67;
    
    struct FPSTest {
        size_t channels;
        double renderTimeMs;
        bool canMaintain60FPS;
        double fpsIfAlone;  // 如果只有渲染，能达到的FPS
    };
    
    std::vector<FPSTest> tests;
    
    for (size_t channels : {2, 8, 16, 32}) {
        auto result = measureRenderPerformance(channels, 100);
        
        FPSTest test;
        test.channels = channels;
        test.renderTimeMs = result.renderTimeMs;
        test.canMaintain60FPS = result.renderTimeMs < TARGET_FRAME_TIME_MS;
        test.fpsIfAlone = 1000.0 / result.renderTimeMs;
        
        tests.push_back(test);
        
        std::cout << std::fixed << std::setprecision(2)
                  << "通道数: " << std::setw(2) << channels
                  << " | 渲染时间: " << std::setw(6) << result.renderTimeMs << "ms"
                  << " | 理论FPS: " << std::setw(6) << test.fpsIfAlone
                  << " | 60FPS: " << (test.canMaintain60FPS ? "✓" : "✗")
                  << std::endl;
    }
    
    // 对于音频应用，8通道是最常见的配置
    auto it = std::find_if(tests.begin(), tests.end(),
        [](const FPSTest& t) { return t.channels == 8; });
    
    ASSERT_NE(it, tests.end());
    EXPECT_TRUE(it->canMaintain60FPS)
        << "8通道无法维持60FPS！渲染时间: " << it->renderTimeMs << "ms";
        
    // 专业音频应用可能需要32通道
    it = std::find_if(tests.begin(), tests.end(),
        [](const FPSTest& t) { return t.channels == 32; });
    
    ASSERT_NE(it, tests.end());
    if (!it->canMaintain60FPS) {
        std::cout << "\n⚠️  警告：32通道无法维持60FPS（"
                  << it->renderTimeMs << "ms）" << std::endl;
        std::cout << "这在高端音频工作站中不可接受！" << std::endl;
    }
}

TEST_F(LevelMeterPerformanceTest, BlendCache_EffectivenessAnalysis) {
    std::cout << "\n=== BlendedColorCache 效率分析 ===" << std::endl;
    
    // 创建电平表
    Rect bounds(0, 0, 100, 240);
    LevelMeter meter(uiContext_.get(), bounds, 2, ScaleType::SAMPLE_PEAK);
    
    std::vector<float> levels = {-12.0f, -18.0f};
    meter.updateLevels(levels);
    
    InstrumentedRenderList cmdList;
    
    // 第一次渲染（冷缓存）
    auto start = std::chrono::high_resolution_clock::now();
    meter.addDrawCommands(cmdList);
    auto end = std::chrono::high_resolution_clock::now();
    double coldTime = std::chrono::duration<double, std::milli>(end - start).count();
    
    // 后续渲染（热缓存）
    cmdList.reset();
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; ++i) {
        cmdList.reset();
        meter.addDrawCommands(cmdList);
    }
    end = std::chrono::high_resolution_clock::now();
    double warmTime = std::chrono::duration<double, std::milli>(end - start).count() / 100.0;
    
    std::cout << "冷缓存（首次）: " << coldTime << "ms" << std::endl;
    std::cout << "热缓存（平均）: " << warmTime << "ms" << std::endl;
    std::cout << "加速比: " << (coldTime / warmTime) << "x" << std::endl;
    
    // 缓存应该有显著效果
    EXPECT_LT(warmTime, coldTime * 0.8)
        << "BlendedColorCache没有显著效果，可能设计有问题";
}

//==========================================================================================
// 4. 正确性测试 - LevelMeter集成
//==========================================================================================

TEST_F(LevelMeterTest, LevelMeter_Construction) {
    Rect bounds(10, 10, 100, 240);
    LevelMeter meter(uiContext_.get(), bounds, 2);
    EXPECT_EQ(meter.getChannelCount(), 2);
}

TEST_F(LevelMeterTest, LevelMeter_UpdateLevels) {
    LevelMeter meter(uiContext_.get(), Rect(0, 0, 100, 240), 2);
    
    std::vector<float> levels = {-6.0f, -12.0f};
    meter.updateLevels(levels);
    
    // 动态改变通道数
    std::vector<float> levels2 = {-6.0f, -12.0f, -18.0f};
    meter.updateLevels(levels2);
    EXPECT_EQ(meter.getChannelCount(), 3);
}

TEST_F(LevelMeterTest, LevelMeter_Reset) {
    LevelMeter meter(uiContext_.get(), Rect(0, 0, 100, 240), 2);
    
    std::vector<float> levels = {-6.0f, -12.0f};
    meter.updateLevels(levels);
    
    meter.reset();
    // 无法直接验证内部状态，但至少不应该崩溃
}

TEST_F(LevelMeterTest, LevelMeter_ScaleTypeSwitch) {
    LevelMeter meter(uiContext_.get(), Rect(0, 0, 100, 240), 2);
    
    EXPECT_EQ(meter.getScaleType(), ScaleType::SAMPLE_PEAK);
    
    meter.setScaleType(ScaleType::K12);
    EXPECT_EQ(meter.getScaleType(), ScaleType::K12);
    EXPECT_EQ(meter.getScaleTypeName(), "K-12");
    
    meter.setScaleType(ScaleType::VU);
    EXPECT_EQ(meter.getScaleType(), ScaleType::VU);
    EXPECT_EQ(meter.getScaleTypeName(), "VU");
}

TEST_F(LevelMeterTest, LevelMeter_ConfigValidation) {
    MeterConfig config;
    EXPECT_TRUE(config.isValid());
    
    config.setDecayRate(40.0f);
    config.setPeakHoldTime(2000.0f);
    EXPECT_TRUE(config.isValid());
}

TEST_F(LevelMeterTest, MeterDimensions_Calculations) {
    // 单声道
    EXPECT_FLOAT_EQ(MeterDimensions::getChannelWidth(1), 8.0f);
    EXPECT_FLOAT_EQ(MeterDimensions::getTotalWidth(1), 8.0f + 13.0f);
    
    // 立体声
    EXPECT_FLOAT_EQ(MeterDimensions::getChannelWidth(2), 7.0f);
    float stereoGroupWidth = 7.0f + (7.0f - 1.0f);  // 两个通道，重叠1px
    EXPECT_FLOAT_EQ(MeterDimensions::getChannelGroupWidth(2), stereoGroupWidth);
    
    // 多通道
    EXPECT_FLOAT_EQ(MeterDimensions::getChannelWidth(8), 6.0f);
    
    // 总高度
    float totalHeight = 6.0f + 3.0f + 224.0f + 3.0f;
    EXPECT_FLOAT_EQ(MeterDimensions::getTotalHeight(), totalHeight);
}

//==========================================================================================
// 5. 边界情况测试
//==========================================================================================

TEST_F(LevelMeterTest, EdgeCase_ZeroChannels) {
    LevelMeter meter(uiContext_.get(), Rect(0, 0, 100, 240), 0);
    InstrumentedRenderList cmdList;
    meter.addDrawCommands(cmdList);
    // 应该不崩溃，不绘制任何内容
    EXPECT_EQ(cmdList.getStats().totalCalls, 0);
}

TEST_F(LevelMeterTest, EdgeCase_ExtremeDbValues) {
    ChannelLevelData channel;
    
    // 极端高值
    channel.updateLevel(100.0f, 16.0f);
    EXPECT_LE(channel.getCurrentLevel(), 0.0f);  // 应该被钳位到0dB
    
    // 极端低值
    channel.updateLevel(-1000.0f, 16.0f);
    EXPECT_GE(channel.getCurrentLevel(), -144.0f);  // 应该被钳位到-144dB
}

TEST_F(LevelMeterTest, EdgeCase_VeryLargeChannelCount) {
    // 测试极端情况：100个通道
    LevelMeter meter(uiContext_.get(), Rect(0, 0, 1000, 240), 100);
    std::vector<float> levels(100, -12.0f);
    meter.updateLevels(levels);
    
    InstrumentedRenderList cmdList;
    auto start = std::chrono::high_resolution_clock::now();
    meter.addDrawCommands(cmdList);
    auto end = std::chrono::high_resolution_clock::now();
    
    double renderTime = std::chrono::duration<double, std::milli>(end - start).count();
    std::cout << "\n100通道渲染时间: " << renderTime << "ms" << std::endl;
    std::cout << "总调用次数: " << cmdList.getStats().totalCalls << std::endl;
    
    // 这应该非常慢！
    if (renderTime > 50.0) {
        std::cout << "⚠️  警告：100通道渲染时间超过50ms，这不可接受！" << std::endl;
    }
}
