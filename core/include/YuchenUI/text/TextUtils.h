/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Text module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

//==========================================================================================
/** @file TextUtils.h
    
    Text processing utilities for Unicode, script detection, and segmentation.
    
    Provides utilities for:
    - UTF-8 decoding
    - Character classification (Western/CJK)
    - Unicode script detection for HarfBuzz
    - Text segmentation by font requirements
    
    Script detection:
    - Supports major scripts: Latin, Han (CJK), Hiragana, Katakana, Hangul, Arabic, Hebrew, Thai
    - Uses Unicode ranges for classification
    - Provides ISO language codes for HarfBuzz
    
    Text segmentation:
    - Splits text into Western and CJK segments
    - Each segment assigned appropriate font
    - Preserves character indices for cursor mapping
*/

#pragma once

#include "YuchenUI/core/Types.h"
#include <hb.h>
#include <vector>
#include <cstdint>

namespace YuchenUI {

//==========================================================================================
/**
    Text processing utilities.
    
    TextUtils provides static utility functions for Unicode text processing.
    All methods are static - class cannot be instantiated.
    
    Key features:
    - UTF-8 decoding with error handling
    - Character classification by Unicode ranges
    - Script detection for HarfBuzz shaping
    - Text segmentation by font requirements
    
    @see TextRenderer, FontManager
*/
class TextUtils {
public:
    //======================================================================================
    /** Decodes next UTF-8 code point from string.
        
        Advances pointer past decoded character. Returns replacement character
        (U+FFFD) for invalid sequences.
        
        Supports:
        - 1-byte: ASCII (U+0000 to U+007F)
        - 2-byte: U+0080 to U+07FF
        - 3-byte: U+0800 to U+FFFF
        - 4-byte: U+10000 to U+10FFFF
        
        @param text  Pointer to UTF-8 string (advanced past character)
        @returns Decoded Unicode code point, or 0 if end of string
    */
    static uint32_t decodeUTF8(const char*& text);
    
    //======================================================================================
    /** Tests if code point is Western character.
        
        Western characters include:
        - Basic Latin and Latin-1 Supplement (U+0000-U+024F)
        - Greek and Coptic (U+0370-U+03FF)
        - Cyrillic (U+0400-U+04FF)
        - General Punctuation (U+2000-U+206F)
        - Mathematical Operators (U+2190-U+22FF)
        - Letterlike Symbols (U+2100-U+214F)
        
        @param codepoint  Unicode code point
        @returns True if Western character
    */
    static bool isWesternCharacter(uint32_t codepoint);
    
    /** Tests if code point is Chinese/CJK character.
        
        CJK characters include:
        - CJK Unified Ideographs (U+4E00-U+9FFF)
        - CJK Extension A (U+3400-U+4DBF)
        - CJK Symbols and Punctuation (U+3000-U+303F)
        - Halfwidth and Fullwidth Forms (U+FF00-U+FFEF)
        
        @param codepoint  Unicode code point
        @returns True if CJK character
    */
    static bool isChineseCharacter(uint32_t codepoint);
    
    //======================================================================================
    /** Detects Unicode script for single code point.
        
        Returns HarfBuzz script constant for character. Used for script-specific
        shaping features.
        
        Supported scripts:
        - HB_SCRIPT_LATIN: Western scripts
        - HB_SCRIPT_HAN: Chinese/CJK ideographs
        - HB_SCRIPT_HIRAGANA: Japanese hiragana
        - HB_SCRIPT_KATAKANA: Japanese katakana
        - HB_SCRIPT_HANGUL: Korean hangul
        - HB_SCRIPT_COMMON: Punctuation, symbols, etc.
        
        @param codepoint  Unicode code point
        @returns HarfBuzz script constant
    */
    static hb_script_t detectScript(uint32_t codepoint);
    
    /** Detects dominant script for text string.
        
        Analyzes all characters and returns most common script. Used to set
        HarfBuzz buffer script for shaping.
        
        Priority:
        1. HB_SCRIPT_HAN if any CJK characters present
        2. First non-common script found
        3. HB_SCRIPT_LATIN if only Latin characters
        4. HB_SCRIPT_COMMON as fallback
        
        @param text  UTF-8 text string
        @returns Dominant HarfBuzz script
    */
    static hb_script_t detectTextScript(const char* text);
    
    /** Returns ISO language code for script.
        
        Maps HarfBuzz script to ISO 639 language code for shaping.
        
        Mappings:
        - HB_SCRIPT_HAN -> "zh-cn" (Chinese Simplified)
        - HB_SCRIPT_LATIN -> "en" (English)
        - HB_SCRIPT_HIRAGANA/KATAKANA -> "ja" (Japanese)
        - HB_SCRIPT_HANGUL -> "ko" (Korean)
        - HB_SCRIPT_ARABIC -> "ar" (Arabic)
        - HB_SCRIPT_HEBREW -> "he" (Hebrew)
        - HB_SCRIPT_THAI -> "th" (Thai)
        - Default -> "en"
        
        @param script  HarfBuzz script constant
        @returns ISO 639 language code
    */
    static const char* getLanguageForScript(hb_script_t script);
    
    //======================================================================================
    /** Segments text by font requirements.
        
        Splits text into contiguous segments requiring same font (Western or CJK).
        Each segment tracks original character indices for cursor mapping.
        
        Process:
        1. Decode each character
        2. Classify as Western or CJK
        3. Start new segment on font change
        4. Track original indices in source string
        
        @param text          UTF-8 text string
        @param westernFont   Font handle for Western characters
        @param chineseFont   Font handle for CJK characters
        @returns Vector of text segments with font assignments
    */
    static std::vector<TextSegment> segmentText(const char* text, FontHandle westernFont, FontHandle chineseFont);

private:
    /** Private constructor. Class contains only static methods. */
    TextUtils() = delete;
};

} // namespace YuchenUI
