/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Focus module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

//==========================================================================================
/** @file FocusPolicy.h
    
    Focus policy enumerations and helper functions for focus behavior configuration.
    
    Defines how UI components accept keyboard focus and the reason why focus changed.
    Focus policies use bitwise flags to support combinations (e.g., StrongFocus = Tab | Click).
    
    Usage:
    - FocusPolicy: Set on components to control how they accept focus
    - FocusReason: Passed to focus event handlers to indicate cause
    - FocusDirection: Used for directional navigation (arrow keys, Tab)
*/

#pragma once

namespace YuchenUI {

//==========================================================================================
/** Defines how a UI component can receive keyboard focus.
    
    Uses bitwise flags to allow combinations. For example, StrongFocus (value 3)
    combines TabFocus (1) and ClickFocus (2).
*/
enum class FocusPolicy {
    NoFocus = 0,           ///< Component never accepts focus
    TabFocus = 1,          ///< Focus via Tab key navigation only
    ClickFocus = 2,        ///< Focus via mouse click only
    StrongFocus = 3,       ///< Tab and click (TabFocus | ClickFocus)
    WheelFocus = 7         ///< Tab, click, and wheel (StrongFocus | 4)
};

//==========================================================================================
/** Indicates why a component gained or lost focus.
    
    Passed to focusInEvent() and focusOutEvent() handlers to allow components
    to customize behavior based on how focus changed.
*/
enum class FocusReason {
    MouseFocusReason,        ///< Focus changed by mouse click
    TabFocusReason,          ///< Focus changed by Tab key
    BacktabFocusReason,      ///< Focus changed by Shift+Tab
    ShortcutFocusReason,     ///< Focus changed by keyboard shortcut
    PopupFocusReason,        ///< Focus changed by popup menu/dialog
    MenuBarFocusReason,      ///< Focus changed by menu bar activation
    ActiveWindowFocusReason, ///< Focus changed by window activation
    OtherFocusReason         ///< Focus changed programmatically (setFocus)
};

//==========================================================================================
/** Direction for focus navigation.
    
    Used by FocusManager to handle Tab and arrow key navigation.
    Next/Previous follow focus chain order, directional keys use spatial layout.
*/
enum class FocusDirection {
    Next,      ///< Next in focus chain (Tab)
    Previous,  ///< Previous in focus chain (Shift+Tab)
    Up,        ///< Spatially upward
    Down,      ///< Spatially downward
    Left,      ///< Spatially leftward
    Right,     ///< Spatially rightward
    First,     ///< First in focus chain
    Last       ///< Last in focus chain
};

//==========================================================================================
// Helper Functions

/** Tests if policy accepts focus via Tab key.
    
    @param policy  Focus policy to test
    @returns True if TabFocus bit is set
*/
inline bool canGetFocusByTab(FocusPolicy policy) {
    int p = static_cast<int>(policy);
    int tab = static_cast<int>(FocusPolicy::TabFocus);
    return (p & tab) != 0;
}

/** Tests if policy accepts focus via mouse click.
    
    @param policy  Focus policy to test
    @returns True if ClickFocus bit is set
*/
inline bool canGetFocusByClick(FocusPolicy policy) {
    int p = static_cast<int>(policy);
    int click = static_cast<int>(FocusPolicy::ClickFocus);
    return (p & click) != 0;
}

/** Tests if policy accepts focus via mouse wheel.
    
    @param policy  Focus policy to test
    @returns True if policy is WheelFocus (includes all focus methods)
*/
inline bool canGetFocusByWheel(FocusPolicy policy) {
    return policy == FocusPolicy::WheelFocus;
}

} // namespace YuchenUI
