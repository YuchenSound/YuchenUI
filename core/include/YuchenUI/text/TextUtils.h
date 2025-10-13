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
    
    Text processing utilities for Unicode, script detection, segmentation, and font fallback.
    
    Version 2.0 Changes:
    - Added mapCharactersToFonts() for per-character font selection
    - Added segmentTextWithFallback() for fallback-aware text segmentation
    - Enhanced script detection for emoji and symbols
    - Improved Unicode range coverage
    
    Provides utilities for:
    - UTF-8 encoding/decoding
    - Character classification (Western/CJK/Emoji/Symbol)
    - Unicode script detection for HarfBuzz
    - Text segmentation by font requirements with fallback support
    - Font fallback chain resolution
    
    Script detection:
    - Supports major scripts: Latin, Han (CJK), Hiragana, Katakana, Hangul, Arabic, Hebrew, Thai
    - Added emoji and symbol detection
    - Uses Unicode ranges for classification
    - Provides ISO language codes for HarfBuzz
    
    Text segmentation:
    - Splits text into Western, CJK, Emoji, and Symbol segments
    - Each segment assigned appropriate font from fallback chain
    - Preserves character indices for cursor mapping
    - Optimizes by merging consecutive characters using same font
*/

#pragma once

#include "YuchenUI/core/Types.h"
#include <hb.h>
#include <vector>
#include <cstdint>

namespace YuchenUI {

class IFontProvider;

//==========================================================================================
/**
    Text processing utilities.
    
    TextUtils provides static utility functions for Unicode text processing,
    script detection, and font fallback resolution. All methods are static -
    class cannot be instantiated.
    
    Key features:
    - UTF-8 decoding with error handling
    - Character classification by Unicode ranges
    - Script detection for HarfBuzz shaping
    - Text segmentation by font requirements
    - Font fallback chain resolution per character
    
    @see TextRenderer, FontManager, IFontProvider
*/
class TextUtils {
public:
    //======================================================================================
    // UTF-8 Processing
    
    /**
        Decodes next UTF-8 code point from string.
        
        Advances pointer past decoded character. Returns replacement character
        (U+FFFD) for invalid sequences.
        
        Supports:
        - 1-byte: ASCII (U+0000 to U+007F)
        - 2-byte: U+0080 to U+07FF
        - 3-byte: U+0800 to U+FFFF
        - 4-byte: U+10000 to U+10FFFF (includes emoji)
        
        @param text  Pointer to UTF-8 string (advanced past character)
        @returns Decoded Unicode code point, or 0 if end of string
    */
    static uint32_t decodeUTF8(const char*& text);
    
    //======================================================================================
    // Character Classification
    
    /**
        Tests if code point is Western character.
        
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
    
    /**
        Tests if code point is Chinese/CJK character.
        
        CJK characters include:
        - CJK Unified Ideographs (U+4E00-U+9FFF)
        - CJK Extension A (U+3400-U+4DBF)
        - CJK Symbols and Punctuation (U+3000-U+303F)
        - Halfwidth and Fullwidth Forms (U+FF00-U+FFEF)
        
        @param codepoint  Unicode code point
        @returns True if CJK character
    */
    static bool isChineseCharacter(uint32_t codepoint);
    
    /**
        Tests if code point is emoji.
        
        Emoji ranges include:
        - Emoticons (U+1F600-U+1F64F)
        - Miscellaneous Symbols and Pictographs (U+1F300-U+1F5FF)
        - Transport and Map Symbols (U+1F680-U+1F6FF)
        - Supplemental Symbols and Pictographs (U+1F900-U+1F9FF)
        - Symbols and Pictographs Extended-A (U+1FA70-U+1FAFF)
        - Emoji Components (U+FE00-U+FE0F, U+1F3FB-U+1F3FF)
        
        @param codepoint  Unicode code point
        @returns True if emoji character
    */
    static bool isEmojiCharacter(uint32_t codepoint);
    
    /**
        Tests if code point is symbol.
        
        Symbol ranges include:
        - Miscellaneous Symbols (U+2600-U+26FF)
        - Dingbats (U+2700-U+27BF)
        - Miscellaneous Mathematical Symbols (U+2980-U+29FF)
        - Supplemental Punctuation (U+2E00-U+2E7F)
        - Geometric Shapes (U+25A0-U+25FF)
        
        @param codepoint  Unicode code point
        @returns True if symbol character
    */
    static bool isSymbolCharacter(uint32_t codepoint);
    
    //======================================================================================
    // Script Detection
    
    /**
        Detects Unicode script for single code point.
        
        Returns HarfBuzz script constant for character. Used for script-specific
        shaping features.
        
        Supported scripts:
        - HB_SCRIPT_LATIN: Western scripts
        - HB_SCRIPT_HAN: Chinese/CJK ideographs
        - HB_SCRIPT_HIRAGANA: Japanese hiragana
        - HB_SCRIPT_KATAKANA: Japanese katakana
        - HB_SCRIPT_HANGUL: Korean hangul
        - HB_SCRIPT_COMMON: Punctuation, symbols, emoji, etc.
        
        @param codepoint  Unicode code point
        @returns HarfBuzz script constant
    */
    static hb_script_t detectScript(uint32_t codepoint);
    
    /**
        Detects dominant script for text string.
        
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
    
    /**
        Returns ISO language code for script.
        
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
    // Font Fallback Support (New in v2.0)
    
    /**
        Maps each character in text to best font from fallback chain.
        
        This is the core of the font fallback system. For each character:
        1. Decode UTF-8 to get Unicode code point
        2. Use IFontProvider::selectFontForCodepoint() to find best font
        3. Record mapping with byte offsets for later segmentation
        
        The result contains per-character font selections with original byte positions,
        which can be used for:
        - Text segmentation (merging consecutive same-font characters)
        - Debugging font selection
        - Cursor position mapping
        
        Example:
        @code
        const char* text = "Hello世界😊";
        auto mappings = TextUtils::mapCharactersToFonts(text, fallbackChain, fontProvider);
        
        // Results:
        // mappings[0]: 'H' -> Arial, offset=0, length=1
        // mappings[1]: 'e' -> Arial, offset=1, length=1
        // ...
        // mappings[5]: '世' -> PingFang, offset=5, length=3
        // mappings[6]: '界' -> PingFang, offset=8, length=3
        // mappings[7]: '😊' -> Emoji, offset=11, length=4
        @endcode
        
        @param text             UTF-8 text string
        @param fallbackChain    Ordered font fallback chain
        @param fontProvider     Font provider for glyph queries
        @returns Vector of character-to-font mappings with byte positions
    */
    static std::vector<CharFontMapping> mapCharactersToFonts(
        const char* text,
        const FontFallbackChain& fallbackChain,
        IFontProvider* fontProvider
    );
    
    /**
        Segments text by font requirements using fallback chain.
        
        Uses mapCharactersToFonts() to select fonts per character, then merges
        consecutive characters using the same font into segments. This is more
        efficient than the legacy segmentText() which only handles Western/CJK.
        
        Algorithm:
        1. Map each character to optimal font via fallback chain
        2. Merge consecutive characters with same font into segments
        3. Track original byte positions for each segment
        
        Example:
        @code
        // Input: "Hello世界😊"
        // Chain: [Arial, PingFang, Emoji]
        
        auto segments = TextUtils::segmentTextWithFallback(text, chain, fontProvider);
        
        // Result:
        // segments[0]: "Hello" -> Arial (offset=0, length=5)
        // segments[1]: "世界" -> PingFang (offset=5, length=6)
        // segments[2]: "😊" -> Emoji (offset=11, length=4)
        @endcode
        
        This is the recommended method for text segmentation in the new system.
        
        @param text             UTF-8 text string
        @param fallbackChain    Ordered font fallback chain
        @param fontProvider     Font provider for glyph queries
        @returns Vector of text segments with font assignments
    */
    static std::vector<TextSegment> segmentTextWithFallback(
        const char* text,
        const FontFallbackChain& fallbackChain,
        IFontProvider* fontProvider
    );
    

private:
    /**
        Private constructor. Class contains only static methods.
    */
    TextUtils() = delete;
};

} // namespace YuchenUI
