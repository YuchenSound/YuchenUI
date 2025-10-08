#pragma once

namespace YuchenUI {
namespace Config {

namespace Window {
    static constexpr int MIN_SIZE = 1;
    static constexpr int MAX_SIZE = 8192;
    static constexpr size_t MAX_TITLE_LENGTH = 256;
}

namespace Font {
    static constexpr float MIN_SIZE = 1.0f;
    static constexpr float MAX_SIZE = 500.0f;
    static constexpr float DEFAULT_SIZE = 11.0f;
    static constexpr size_t MAX_FONTS = 64;
    static constexpr size_t MAX_CACHED_SIZES = 8;

    static constexpr int FREETYPE_DPI = 72;


}

namespace Text {
    static constexpr size_t MAX_LENGTH = 8192;
    static constexpr size_t MAX_GLYPHS_PER_TEXT = 1024;
    static constexpr float DEFAULT_PADDING = 0.0f;
}

namespace Rendering {
    static constexpr size_t MAX_COMMANDS_PER_LIST = 10000;
    static constexpr size_t MAX_TEXT_VERTICES = 32768;
}

namespace GlyphCache {
    static constexpr uint32_t BASE_ATLAS_WIDTH = 1024;
    static constexpr uint32_t BASE_ATLAS_HEIGHT = 1024;
    static constexpr uint32_t GLYPH_PADDING = 2;
    static constexpr uint32_t MAX_ATLASES = 8;
    static constexpr uint32_t GLYPH_EXPIRE_FRAMES = 300;
    static constexpr uint32_t CLEANUP_INTERVAL_FRAMES = 60;
}

namespace Events {
    static constexpr size_t EVENT_QUEUE_SIZE = 512;
    static constexpr size_t MAX_KEYS = 256;
    static constexpr size_t MAX_BUTTONS = 8;
}

}
}
