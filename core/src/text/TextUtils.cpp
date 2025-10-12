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
/** @file TextUtils.cpp
    
    Implementation notes:
    - UTF-8 decoding handles 1-4 byte sequences
    - Returns replacement character (U+FFFD) for invalid UTF-8 sequences
    - Western character ranges include Latin, Greek, Cyrillic, and common punctuation
    - CJK ranges include unified ideographs, extensions, and fullwidth forms
    - Emoji ranges include emoticons, pictographs, and emoji modifiers
    - Symbol ranges include miscellaneous symbols, dingbats, and geometric shapes
    - Script detection uses Unicode block ranges
    - Text segmentation creates contiguous segments by font requirement
    - Segment indices track original character positions for cursor mapping
    
    Version 2.0 Changes:
    - Added isEmojiCharacter() for emoji detection
    - Added isSymbolCharacter() for symbol detection
    - Added mapCharactersToFonts() for per-character font selection
    - Added segmentTextWithFallback() for fallback-aware segmentation
    - Enhanced Unicode range coverage
    - Improved performance with better character classification
*/

#include "YuchenUI/text/TextUtils.h"
#include "YuchenUI/text/IFontProvider.h"
#include "YuchenUI/core/Assert.h"
#include <cstring>

namespace YuchenUI {

//==========================================================================================
// UTF-8 Decoding

uint32_t TextUtils::decodeUTF8(const char*& text)
{
    const unsigned char* p = reinterpret_cast<const unsigned char*>(text);
    uint32_t codepoint = 0;
    
    if (*p == 0) return 0;  // End of string
    
    if ((*p & 0x80) == 0)
    {
        // 1-byte sequence (ASCII): 0xxxxxxx
        codepoint = *p;
        text += 1;
    }
    else if ((*p & 0xE0) == 0xC0)
    {
        // 2-byte sequence: 110xxxxx 10xxxxxx
        if ((*(p+1) & 0xC0) != 0x80)
        {
            codepoint = 0xFFFD;  // Invalid sequence
            text += 1;
            return codepoint;
        }
        codepoint = ((*p & 0x1F) << 6) | (*(p+1) & 0x3F);
        text += 2;
    }
    else if ((*p & 0xF0) == 0xE0)
    {
        // 3-byte sequence: 1110xxxx 10xxxxxx 10xxxxxx
        if ((*(p+1) & 0xC0) != 0x80 || (*(p+2) & 0xC0) != 0x80)
        {
            codepoint = 0xFFFD;  // Invalid sequence
            text += 1;
            return codepoint;
        }
        codepoint = ((*p & 0x0F) << 12) | ((*(p+1) & 0x3F) << 6) | (*(p+2) & 0x3F);
        text += 3;
    }
    else if ((*p & 0xF8) == 0xF0)
    {
        // 4-byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        // This handles emoji in supplementary planes
        if ((*(p+1) & 0xC0) != 0x80 || (*(p+2) & 0xC0) != 0x80 || (*(p+3) & 0xC0) != 0x80)
        {
            codepoint = 0xFFFD;  // Invalid sequence
            text += 1;
            return codepoint;
        }
        codepoint = ((*p & 0x07) << 18) | ((*(p+1) & 0x3F) << 12) |
                    ((*(p+2) & 0x3F) << 6) | (*(p+3) & 0x3F);
        text += 4;
    }
    else
    {
        // Invalid UTF-8 sequence - return replacement character
        codepoint = 0xFFFD;
        text += 1;
    }
    
    return codepoint;
}

//==========================================================================================
// Character Classification

bool TextUtils::isWesternCharacter(uint32_t codepoint)
{
    return (codepoint <= 0x024F) ||                          // Basic Latin + Latin Extended
           (codepoint >= 0x0370 && codepoint <= 0x03FF) ||   // Greek and Coptic
           (codepoint >= 0x0400 && codepoint <= 0x04FF) ||   // Cyrillic
           (codepoint >= 0x2000 && codepoint <= 0x206F) ||   // General Punctuation
           (codepoint >= 0x2190 && codepoint <= 0x22FF) ||   // Arrows + Mathematical Operators
           (codepoint >= 0x2100 && codepoint <= 0x214F);     // Letterlike Symbols
}

bool TextUtils::isChineseCharacter(uint32_t codepoint)
{
    return (codepoint >= 0x4E00 && codepoint <= 0x9FFF) ||   // CJK Unified Ideographs
           (codepoint >= 0x3400 && codepoint <= 0x4DBF) ||   // CJK Extension A
           (codepoint >= 0x3000 && codepoint <= 0x303F) ||   // CJK Symbols and Punctuation
           (codepoint >= 0xFF00 && codepoint <= 0xFFEF);     // Halfwidth and Fullwidth Forms
}

bool TextUtils::isEmojiCharacter(uint32_t codepoint)
{
    // Emoji ranges (Unicode 15.0)
    return (codepoint >= 0x1F600 && codepoint <= 0x1F64F) ||  // Emoticons
           (codepoint >= 0x1F300 && codepoint <= 0x1F5FF) ||  // Miscellaneous Symbols and Pictographs
           (codepoint >= 0x1F680 && codepoint <= 0x1F6FF) ||  // Transport and Map Symbols
           (codepoint >= 0x1F700 && codepoint <= 0x1F77F) ||  // Alchemical Symbols
           (codepoint >= 0x1F780 && codepoint <= 0x1F7FF) ||  // Geometric Shapes Extended
           (codepoint >= 0x1F800 && codepoint <= 0x1F8FF) ||  // Supplemental Arrows-C
           (codepoint >= 0x1F900 && codepoint <= 0x1F9FF) ||  // Supplemental Symbols and Pictographs
           (codepoint >= 0x1FA00 && codepoint <= 0x1FA6F) ||  // Chess Symbols
           (codepoint >= 0x1FA70 && codepoint <= 0x1FAFF) ||  // Symbols and Pictographs Extended-A
           (codepoint >= 0x2600 && codepoint <= 0x26FF) ||    // Miscellaneous Symbols (partial emoji)
           (codepoint >= 0x2700 && codepoint <= 0x27BF) ||    // Dingbats (partial emoji)
           (codepoint >= 0xFE00 && codepoint <= 0xFE0F) ||    // Variation Selectors (emoji variants)
           (codepoint >= 0x1F3FB && codepoint <= 0x1F3FF);    // Emoji Modifiers (skin tones)
}

bool TextUtils::isSymbolCharacter(uint32_t codepoint)
{
    // Symbol ranges that are not emoji
    return (codepoint >= 0x2300 && codepoint <= 0x23FF) ||    // Miscellaneous Technical
           (codepoint >= 0x2400 && codepoint <= 0x243F) ||    // Control Pictures
           (codepoint >= 0x2440 && codepoint <= 0x245F) ||    // Optical Character Recognition
           (codepoint >= 0x2460 && codepoint <= 0x24FF) ||    // Enclosed Alphanumerics
           (codepoint >= 0x2500 && codepoint <= 0x257F) ||    // Box Drawing
           (codepoint >= 0x2580 && codepoint <= 0x259F) ||    // Block Elements
           (codepoint >= 0x25A0 && codepoint <= 0x25FF) ||    // Geometric Shapes
           (codepoint >= 0x2980 && codepoint <= 0x29FF) ||    // Miscellaneous Mathematical Symbols-B
           (codepoint >= 0x2A00 && codepoint <= 0x2AFF) ||    // Supplemental Mathematical Operators
           (codepoint >= 0x2B00 && codepoint <= 0x2BFF) ||    // Miscellaneous Symbols and Arrows
           (codepoint >= 0x2E00 && codepoint <= 0x2E7F);      // Supplemental Punctuation
}

//==========================================================================================
// Script Detection

hb_script_t TextUtils::detectScript(uint32_t codepoint)
{
    // CJK Unified Ideographs (Chinese, Japanese, Korean)
    if ((codepoint >= 0x4E00 && codepoint <= 0x9FFF) ||      // Main block
        (codepoint >= 0x3400 && codepoint <= 0x4DBF) ||      // Extension A
        (codepoint >= 0x20000 && codepoint <= 0x2A6DF) ||    // Extension B
        (codepoint >= 0x2A700 && codepoint <= 0x2B73F) ||    // Extension C
        (codepoint >= 0x2B740 && codepoint <= 0x2B81F) ||    // Extension D
        (codepoint >= 0xF900 && codepoint <= 0xFAFF) ||      // Compatibility Ideographs
        (codepoint >= 0x2F800 && codepoint <= 0x2FA1F))      // Compatibility Supplement
        return HB_SCRIPT_HAN;
    
    // Latin script
    if ((codepoint >= 0x0020 && codepoint <= 0x007F) ||      // Basic Latin
        (codepoint >= 0x00A0 && codepoint <= 0x00FF) ||      // Latin-1 Supplement
        (codepoint >= 0x0100 && codepoint <= 0x017F) ||      // Latin Extended-A
        (codepoint >= 0x0180 && codepoint <= 0x024F))        // Latin Extended-B
        return HB_SCRIPT_LATIN;
    
    // Japanese scripts
    if (codepoint >= 0x3040 && codepoint <= 0x309F) return HB_SCRIPT_HIRAGANA;
    if (codepoint >= 0x30A0 && codepoint <= 0x30FF) return HB_SCRIPT_KATAKANA;
    
    // Korean Hangul
    if (codepoint >= 0xAC00 && codepoint <= 0xD7AF) return HB_SCRIPT_HANGUL;
    
    // Arabic
    if (codepoint >= 0x0600 && codepoint <= 0x06FF) return HB_SCRIPT_ARABIC;
    
    // Hebrew
    if (codepoint >= 0x0590 && codepoint <= 0x05FF) return HB_SCRIPT_HEBREW;
    
    // Thai
    if (codepoint >= 0x0E00 && codepoint <= 0x0E7F) return HB_SCRIPT_THAI;
    
    // Default to common for punctuation, symbols, emoji, etc.
    return HB_SCRIPT_COMMON;
}

hb_script_t TextUtils::detectTextScript(const char* text)
{
    const char* p = text;
    hb_script_t dominantScript = HB_SCRIPT_COMMON;
    int hanCount = 0, latinCount = 0, otherCount = 0;
    
    // Count characters by script
    while (*p)
    {
        uint32_t codepoint = decodeUTF8(p);
        if (codepoint == 0) break;
        if (codepoint == 0xFFFD) continue;  // Skip replacement character
        
        hb_script_t script = detectScript(codepoint);
        if (script == HB_SCRIPT_HAN)
            hanCount++;
        else if (script == HB_SCRIPT_LATIN)
            latinCount++;
        else if (script != HB_SCRIPT_COMMON)
        {
            otherCount++;
            dominantScript = script;  // Remember first non-common script
        }
    }
    
    // Return dominant script with priority: Han > Other > Latin > Common
    if (hanCount > 0) return HB_SCRIPT_HAN;
    if (otherCount > 0) return dominantScript;
    return latinCount > 0 ? HB_SCRIPT_LATIN : HB_SCRIPT_COMMON;
}

//==========================================================================================
// Language Mapping

const char* TextUtils::getLanguageForScript(hb_script_t script)
{
    switch (script) {
        case HB_SCRIPT_HAN:     return "zh-cn";  // Chinese Simplified
        case HB_SCRIPT_LATIN:   return "en";     // English
        case HB_SCRIPT_HIRAGANA:
        case HB_SCRIPT_KATAKANA:return "ja";     // Japanese
        case HB_SCRIPT_HANGUL:  return "ko";     // Korean
        case HB_SCRIPT_ARABIC:  return "ar";     // Arabic
        case HB_SCRIPT_HEBREW:  return "he";     // Hebrew
        case HB_SCRIPT_THAI:    return "th";     // Thai
        default:                return "en";     // Default to English
    }
}

//==========================================================================================
// Font Fallback Support

std::vector<CharFontMapping> TextUtils::mapCharactersToFonts(
    const char* text,
    const FontFallbackChain& fallbackChain,
    IFontProvider* fontProvider)
{
    YUCHEN_ASSERT(text != nullptr);
    YUCHEN_ASSERT(!fallbackChain.isEmpty());
    YUCHEN_ASSERT(fontProvider != nullptr);
    
    std::vector<CharFontMapping> mappings;
    const char* p = text;
    size_t byteOffset = 0;
    
    while (*p)
    {
        const char* charStart = p;
        uint32_t codepoint = decodeUTF8(p);
        if (codepoint == 0) break;
        
        size_t charLength = p - charStart;
        
        // Skip replacement characters
        if (codepoint == 0xFFFD)
        {
            byteOffset += charLength;
            continue;
        }
        
        // Select best font from fallback chain for this character
        FontHandle selectedFont = fontProvider->selectFontForCodepoint(codepoint, fallbackChain);
        
        // Record mapping
        CharFontMapping mapping(codepoint, selectedFont, byteOffset, charLength);
        mappings.push_back(mapping);
        
        byteOffset += charLength;
    }
    
    return mappings;
}

std::vector<TextSegment> TextUtils::segmentTextWithFallback(
    const char* text,
    const FontFallbackChain& fallbackChain,
    IFontProvider* fontProvider)
{
    YUCHEN_ASSERT(text != nullptr);
    YUCHEN_ASSERT(!fallbackChain.isEmpty());
    YUCHEN_ASSERT(fontProvider != nullptr);
    
    // Step 1: Map each character to optimal font
    std::vector<CharFontMapping> charMappings = mapCharactersToFonts(text, fallbackChain, fontProvider);
    
    if (charMappings.empty())
    {
        return std::vector<TextSegment>();
    }
    
    // Step 2: Merge consecutive characters with same font into segments
    std::vector<TextSegment> segments;
    const char* textStart = text;
    
    for (size_t i = 0; i < charMappings.size(); )
    {
        FontHandle currentFont = charMappings[i].selectedFont;
        size_t segmentStart = charMappings[i].byteOffset;
        size_t segmentEnd = segmentStart;
        
        // Find all consecutive characters using same font
        while (i < charMappings.size() && charMappings[i].selectedFont == currentFont)
        {
            segmentEnd = charMappings[i].byteOffset + charMappings[i].byteLength;
            ++i;
        }
        
        // Create segment
        TextSegment segment;
        segment.fontHandle = currentFont;
        segment.originalStartIndex = segmentStart;
        segment.originalLength = segmentEnd - segmentStart;
        segment.text = std::string(textStart + segmentStart, segmentEnd - segmentStart);
        
        segments.push_back(segment);
    }
    
    return segments;
}

//==========================================================================================
// Legacy Text Segmentation (Deprecated)

std::vector<TextSegment> TextUtils::segmentText(const char* text, FontHandle westernFont, FontHandle chineseFont)
{
    YUCHEN_ASSERT_MSG(westernFont != INVALID_FONT_HANDLE, "Invalid western font handle");
    YUCHEN_ASSERT_MSG(chineseFont != INVALID_FONT_HANDLE, "Invalid chinese font handle");
    
    std::vector<TextSegment> segments;
    const char* p = text;
    
    // Process each character and group by font requirement
    while (*p)
    {
        const char* charStart = p;
        uint32_t codepoint = decodeUTF8(p);
        if (codepoint == 0) break;
        
        size_t charLength = p - charStart;
        FontHandle selectedFont;
        
        // Select font based on character type (legacy: only Western vs CJK)
        if (isWesternCharacter(codepoint))
        {
            selectedFont = westernFont;
        }
        else if (isChineseCharacter(codepoint))
        {
            selectedFont = chineseFont;
        }
        else
        {
            // For emoji and symbols, default to Western font (incorrect but legacy behavior)
            selectedFont = westernFont;
        }
        
        // Create new segment if font changes or no segments exist
        if (segments.empty() || segments.back().fontHandle != selectedFont)
        {
            TextSegment segment;
            segment.fontHandle = selectedFont;
            segment.originalStartIndex = charStart - text;
            segment.originalLength = 0;
            segments.push_back(segment);
        }
        
        // Append character to current segment
        segments.back().text.append(charStart, charLength);
        segments.back().originalLength += charLength;
    }
    
    return segments;
}

} // namespace YuchenUI
