#pragma once

#include "YuchenUI/core/Types.h"

namespace YuchenUI {
namespace Colors {

// Basic Colors
static const Vec4 BLACK = Vec4::FromRGBA(0, 0, 0, 255);                 ///< Pure black
static const Vec4 GRAY = Vec4::FromRGBA(128, 128, 128, 255);            ///< Medium gray
static const Vec4 DARK_GRAY = Vec4::FromRGBA(30, 30, 30, 255);          ///< Dark gray

// Button States
static const Vec4 BUTTON_DEFAULT = Vec4::FromRGBA(100, 100, 100, 255); ///< Button normal state
static const Vec4 BUTTON_HOVER = Vec4::FromRGBA(120, 120, 120, 255);   ///< Button hover state
static const Vec4 BUTTON_PRESSED = Vec4::FromRGBA(80, 80, 80, 255);    ///< Button pressed state
static const Vec4 BUTTON_DISABLED = Vec4::FromRGBA(60, 60, 60, 255);   ///< Button disabled state

// Border States
static const Vec4 BORDER_DEFAULT = Vec4::FromRGBA(128, 128, 128, 255); ///< Border normal state
static const Vec4 BORDER_HOVER = Vec4::FromRGBA(150, 150, 150, 255);   ///< Border hover state
static const Vec4 BORDER_PRESSED = Vec4::FromRGBA(100, 100, 100, 255); ///< Border pressed state
static const Vec4 BORDER_DISABLED = Vec4::FromRGBA(80, 80, 80, 255);   ///< Border disabled state

// Text Colors
static const Vec4 TEXT_DEFAULT = Vec4::FromRGBA(255, 255, 255, 255);   ///< Primary text color
static const Vec4 TEXT_DISABLED = Vec4::FromRGBA(120, 120, 120, 255);  ///< Disabled text color
static const Vec4 TEXT_SECONDARY = Vec4::FromRGBA(200, 200, 200, 255); ///< Secondary text color

// Window Backgrounds
static const Vec4 DIALOG_BACKGROUND = Vec4::FromRGBA(64, 64, 64, 255);      ///< Dialog window background
static const Vec4 TOOL_WINDOW_BACKGROUND = Vec4::FromRGBA(48, 48, 48, 255); ///< Tool window background
static const Vec4 DEFAULT_CLEAR_COLOR = Vec4::FromRGBA(48, 48, 48, 255);    ///< Default renderer clear color

// Action Buttons
static const Vec4 CONFIRM_BUTTON = Vec4::FromRGBA(0, 122, 255, 255);        ///< Confirmation button color
static const Vec4 CONFIRM_BUTTON_HOVER = Vec4::FromRGBA(0, 100, 220, 255);  ///< Confirmation button hover

static const Vec4 FRAME_BACKGROUND = Vec4::FromRGBA(255, 255, 255, 51);
}
}
