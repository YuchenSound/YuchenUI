/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Widgets module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

#pragma once

namespace YuchenUI {

/**
    Knob center behavior type.
    
    Determines whether the knob has a center point (zero position) in the middle
    or at the minimum value.
    
    - NoCentered: Minimum value at left rotation (e.g., Volume: 0-100%)
    - Centered: Zero at center rotation (e.g., Pan: Left-Center-Right)
*/
enum class KnobType {
    NoCentered,  ///< No center point, min value at left
    Centered     ///< Center point at middle, zero at center
};

enum class ScrollbarOrientation {
    Vertical,
    Horizontal
};

enum class ScrollbarButtonType {
    UpLeft,
    DownRight
};

enum class ScrollbarButtonState {
    Normal,
    Hovered,
    Pressed
};

} // namespace YuchenUI
