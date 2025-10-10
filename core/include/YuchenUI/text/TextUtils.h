#pragma once

#include "YuchenUI/core/Types.h"
#include <hb.h>
#include <vector>
#include <cstdint>

namespace YuchenUI {

class TextUtils {
public:
    static uint32_t decodeUTF8(const char*& text);
    static bool isWesternCharacter(uint32_t codepoint);
    static bool isChineseCharacter(uint32_t codepoint);
    static hb_script_t detectScript(uint32_t codepoint);
    static hb_script_t detectTextScript(const char* text);
    static const char* getLanguageForScript(hb_script_t script);
    static std::vector<TextSegment> segmentText(const char* text, FontHandle westernFont, FontHandle chineseFont);

private:
    TextUtils() = delete;
};

}