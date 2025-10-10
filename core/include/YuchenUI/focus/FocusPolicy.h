#pragma once

namespace YuchenUI {

// 焦点策略 - 定义控件如何获得焦点
enum class FocusPolicy {
    NoFocus = 0,           // 不接受焦点
    TabFocus = 1,          // 只能通过Tab键获得焦点
    ClickFocus = 2,        // 只能通过鼠标点击获得焦点
    StrongFocus = 3,       // Tab和点击都可以 (TabFocus | ClickFocus)
    WheelFocus = 7         // Tab、点击、滚轮都可以 (StrongFocus | 4)
};

// 焦点原因 - 记录焦点如何获得
enum class FocusReason {
    MouseFocusReason,      // 鼠标点击
    TabFocusReason,        // Tab键
    BacktabFocusReason,    // Shift+Tab
    ShortcutFocusReason,   // 快捷键
    PopupFocusReason,      // 弹出窗口
    MenuBarFocusReason,    // 菜单栏
    ActiveWindowFocusReason, // 窗口激活
    OtherFocusReason       // 程序设置 (requestFocus)
};

// 焦点方向 - 用于方向键导航
enum class FocusDirection {
    Next,      // 下一个 (Tab)
    Previous,  // 上一个 (Shift+Tab)
    Up,
    Down,
    Left,
    Right,
    First,     // 第一个
    Last       // 最后一个
};

// 辅助函数
inline bool canGetFocusByTab(FocusPolicy policy) {
    int p = static_cast<int>(policy);
    int tab = static_cast<int>(FocusPolicy::TabFocus);
    return (p & tab) != 0;
}

inline bool canGetFocusByClick(FocusPolicy policy) {
    int p = static_cast<int>(policy);
    int click = static_cast<int>(FocusPolicy::ClickFocus);
    return (p & click) != 0;
}

inline bool canGetFocusByWheel(FocusPolicy policy) {
    return policy == FocusPolicy::WheelFocus;
}

}
