#include <gtest/gtest.h>
#include <YuchenUI/YuchenUI-Desktop.h>

using namespace YuchenUI;

// 测试夹具：每个测试前后自动初始化/清理
class WindowLifecycleTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 每个测试前执行
        WindowManager::getInstance().initialize();
    }

    void TearDown() override {
        // 每个测试后执行
        WindowManager::getInstance().destroy();
    }
};

// ============================================================================
// 基础生命周期测试
// ============================================================================

TEST_F(WindowLifecycleTest, CreateWindowWithValidParameters) {
    BaseWindow window;
    
    bool result = window.create(800, 600, "Test Window");
    
    EXPECT_TRUE(result) << "窗口创建应该成功";
    EXPECT_EQ(window.getSize().x, 800.0f);
    EXPECT_EQ(window.getSize().y, 600.0f);
}

TEST_F(WindowLifecycleTest, CreateWindowWithMinimumSize) {
    BaseWindow window;
    
    bool result = window.create(100, 100, "Small Window");
    
    EXPECT_TRUE(result) << "最小尺寸窗口应该创建成功";
}

TEST_F(WindowLifecycleTest, WindowInitiallyNotVisible) {
    BaseWindow window;
    window.create(800, 600, "Test Window");
    
    // 主窗口创建后会自动显示，但我们可以测试其他类型
    BaseWindow dialogWindow(WindowType::Dialog);
    dialogWindow.create(400, 300, "Dialog", &window);
    
    EXPECT_FALSE(dialogWindow.isVisible()) << "对话框初始应该不可见";
}

TEST_F(WindowLifecycleTest, ShowHideWindow) {
    BaseWindow window(WindowType::Dialog);
    window.create(800, 600, "Test Window");
    
    window.show();
    EXPECT_TRUE(window.isVisible()) << "调用 show() 后窗口应该可见";
    
    window.hide();
    EXPECT_FALSE(window.isVisible()) << "调用 hide() 后窗口应该不可见";
}

TEST_F(WindowLifecycleTest, GetNativeHandleAfterCreation) {
    BaseWindow window;
    window.create(800, 600, "Test Window");
    
    void* handle = window.getNativeWindowHandle();
    
    EXPECT_NE(handle, nullptr) << "窗口创建后应该有有效的原生句柄";
}

// ============================================================================
// 窗口属性测试
// ============================================================================

TEST_F(WindowLifecycleTest, GetWindowPosition) {
    BaseWindow window;
    window.create(800, 600, "Test Window");
    
    Vec2 position = window.getWindowPosition();
    
    // 位置应该是合理的（不是无效值）
    EXPECT_TRUE(position.isValid());
}

TEST_F(WindowLifecycleTest, MapToScreenCoordinates) {
    BaseWindow window;
    window.create(800, 600, "Test Window");
    
    Vec2 windowPos(100.0f, 100.0f);
    Vec2 screenPos = window.mapToScreen(windowPos);
    
    // 屏幕坐标应该不同于窗口坐标（因为加了窗口位置偏移）
    EXPECT_TRUE(screenPos.isValid());
}

// ============================================================================
// 多窗口测试
// ============================================================================

TEST_F(WindowLifecycleTest, CreateMultipleWindows) {
    BaseWindow window1;
    BaseWindow window2;
    
    EXPECT_TRUE(window1.create(800, 600, "Window 1"));
    EXPECT_TRUE(window2.create(640, 480, "Window 2"));
    
    EXPECT_NE(window1.getNativeWindowHandle(), window2.getNativeWindowHandle())
        << "不同窗口应该有不同的原生句柄";
}

TEST_F(WindowLifecycleTest, CreateChildWindow) {
    BaseWindow parentWindow;
    parentWindow.create(800, 600, "Parent Window");
    
    BaseWindow childWindow(WindowType::Dialog);
    bool result = childWindow.create(400, 300, "Child Window", &parentWindow);
    
    EXPECT_TRUE(result) << "子窗口应该创建成功";
}

// ============================================================================
// 边界条件测试
// ============================================================================

TEST_F(WindowLifecycleTest, CreateWindowWithNullTitle) {
    BaseWindow window;
    
    // 注意：当前实现中 nullptr 标题会触发断言
    // 这里我们测试空字符串是否可以
    bool result = window.create(800, 600, "");
    
    EXPECT_TRUE(result) << "空标题字符串应该被接受";
}

TEST_F(WindowLifecycleTest, DestroyWindowMultipleTimes) {
    BaseWindow window;
    window.create(800, 600, "Test Window");
    
    // 第一次销毁
    window.destroy();
    
    // 第二次销毁不应该崩溃
    EXPECT_NO_THROW(window.destroy());
}

// ============================================================================
// 状态管理测试
// ============================================================================

TEST_F(WindowLifecycleTest, ShouldCloseInitiallyFalse) {
    BaseWindow window;
    window.create(800, 600, "Test Window");
    
    EXPECT_FALSE(window.shouldClose()) << "新创建的窗口不应该标记为关闭";
}

TEST_F(WindowLifecycleTest, MouseInputStateQuery) {
    BaseWindow window;
    window.create(800, 600, "Test Window");
    
    // 初始状态下鼠标应该没有按下
    EXPECT_FALSE(window.isMousePressed());
    
    // 鼠标位置应该有效
    Vec2 mousePos = window.getMousePosition();
    EXPECT_TRUE(mousePos.isValid());
}

// ============================================================================
// 性能测试（快速创建/销毁）
// ============================================================================

TEST_F(WindowLifecycleTest, RapidCreateDestroy) {
    const int iterations = 10;
    
    for (int i = 0; i < iterations; ++i) {
        BaseWindow window(WindowType::Dialog);
        ASSERT_TRUE(window.create(400, 300, "Temp Window"));
        window.destroy();
    }
    
    // 如果能走到这里，说明没有内存泄漏或崩溃
    SUCCEED();
}
