/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Core module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

#pragma once

namespace YuchenUI {

//==========================================================================================
/** Framework-wide configuration constants.
    
    Defines size limits, defaults, and capacity constraints for various subsystems.
    Organized by module for clarity.
*/
namespace Config {

//==========================================================================================
/** Window configuration limits */
namespace Window {
    static constexpr int MIN_SIZE = 1;                      ///< Minimum window dimension (pixels)
    static constexpr int MAX_SIZE = 8192;                   ///< Maximum window dimension (pixels)
    static constexpr size_t MAX_TITLE_LENGTH = 256;         ///< Maximum window title length
}

//==========================================================================================
/** Font system configuration */
namespace Font {
    static constexpr float MIN_SIZE = 1.0f;                 ///< Minimum font size (points)
    static constexpr float MAX_SIZE = 500.0f;               ///< Maximum font size (points)
    static constexpr float DEFAULT_SIZE = 11.0f;            ///< Default font size (points)
    static constexpr size_t MAX_FONTS = 64;                 ///< Maximum loaded fonts
    static constexpr size_t MAX_CACHED_SIZES = 8;           ///< Maximum cached font sizes per font
    static constexpr int FREETYPE_DPI = 72;                 ///< DPI for FreeType rendering
    static constexpr int LOAD_FLAGS_METRICS = 0x0;          ///< Load glyph for metrics only (FT_LOAD_DEFAULT)
    static constexpr int LOAD_FLAGS_RENDER = 0x4;           ///< Load and render glyph bitmap (FT_LOAD_RENDER)
    constexpr int32_t EMBOLDEN_STRENGTH = 16;               ///< 0.5 pixel boldness
}

//==========================================================================================
/** Text rendering configuration */
namespace Text {
    static constexpr size_t MAX_LENGTH = 8192;              ///< Maximum text length (characters)
    static constexpr size_t MAX_GLYPHS_PER_TEXT = 8192;     ///< Maximum glyphs per text object
    static constexpr float DEFAULT_PADDING = 0.0f;          ///< Default text padding
}

//==========================================================================================
/** Rendering system configuration */
namespace Rendering {
    static constexpr size_t MAX_COMMANDS_PER_LIST = 10000;  ///< Maximum render commands per frame
    static constexpr size_t MAX_TEXT_VERTICES = 32768;      ///< Maximum text vertices per frame
    static const Vec4 DEFAULT_CLEAR_COLOR = Vec4::FromRGBA(0,0,0,0);
    static constexpr int DEFAULT_FPS = 60;                  /// render Default FPS
}

//==========================================================================================
/** Glyph cache configuration */
namespace GlyphCache {
    static constexpr uint32_t BASE_ATLAS_WIDTH = 1024;      ///< Initial atlas texture width
    static constexpr uint32_t BASE_ATLAS_HEIGHT = 1024;     ///< Initial atlas texture height
    static constexpr uint32_t GLYPH_PADDING = 2;            ///< Padding between glyphs (pixels)
    static constexpr uint32_t MAX_ATLASES = 8;              ///< Maximum atlas textures
    static constexpr uint32_t GLYPH_EXPIRE_FRAMES = 300;    ///< Frames before unused glyph expires
    static constexpr uint32_t CLEANUP_INTERVAL_FRAMES = 60; ///< Frames between cache cleanups
}

//==========================================================================================
/** Event system configuration */
namespace Events {
    static constexpr size_t EVENT_QUEUE_SIZE = 512;         ///< Maximum queued events
    static constexpr size_t MAX_KEYS = 256;                 ///< Maximum tracked keys
    static constexpr size_t MAX_BUTTONS = 8;                ///< Maximum mouse buttons
}

} // namespace Config
} // namespace YuchenUI
