#include "YuchenUI/text/TextUtils.h"
#include "YuchenUI/core/Assert.h"

namespace YuchenUI {

uint32_t TextUtils::decodeUTF8(const char*& text)
{
    const unsigned char* p = reinterpret_cast<const unsigned char*>(text);
    uint32_t codepoint = 0;
    
    if (*p == 0) return 0;
    if ((*p & 0x80) == 0)
    {
        codepoint = *p;
        text += 1;
    }
    else if ((*p & 0xE0) == 0xC0)
    {
        YUCHEN_ASSERT_MSG((*(p+1) & 0xC0) == 0x80, "Invalid UTF-8 sequence");
        codepoint = ((*p & 0x1F) << 6) | (*(p+1) & 0x3F);
        text += 2;
    }
    else if ((*p & 0xF0) == 0xE0)
    {
        YUCHEN_ASSERT_MSG((*(p+1) & 0xC0) == 0x80, "Invalid UTF-8 sequence");
        YUCHEN_ASSERT_MSG((*(p+2) & 0xC0) == 0x80, "Invalid UTF-8 sequence");
        codepoint = ((*p & 0x0F) << 12) | ((*(p+1) & 0x3F) << 6) | (*(p+2) & 0x3F);
        text += 3;
    }
    else if ((*p & 0xF8) == 0xF0)
    {
        YUCHEN_ASSERT_MSG((*(p+1) & 0xC0) == 0x80, "Invalid UTF-8 sequence");
        YUCHEN_ASSERT_MSG((*(p+2) & 0xC0) == 0x80, "Invalid UTF-8 sequence");
        YUCHEN_ASSERT_MSG((*(p+3) & 0xC0) == 0x80, "Invalid UTF-8 sequence");
        codepoint = ((*p & 0x07) << 18) | ((*(p+1) & 0x3F) << 12) |
        ((*(p+2) & 0x3F) << 6) | (*(p+3) & 0x3F);
        text += 4;
    }
    else
    {
        codepoint = 0xFFFD;
        text += 1;
    }
    return codepoint;
}

bool TextUtils::isWesternCharacter(uint32_t codepoint)
{
    return (codepoint <= 0x024F) ||
           (codepoint >= 0x0370 && codepoint <= 0x03FF) || 
           (codepoint >= 0x0400 && codepoint <= 0x04FF) ||
           (codepoint >= 0x2000 && codepoint <= 0x206F) || 
           (codepoint >= 0x2190 && codepoint <= 0x22FF) ||
           (codepoint >= 0x2100 && codepoint <= 0x214F);
}

bool TextUtils::isChineseCharacter(uint32_t codepoint)
{
    return (codepoint >= 0x4E00 && codepoint <= 0x9FFF) ||
           (codepoint >= 0x3400 && codepoint <= 0x4DBF) ||
           (codepoint >= 0x3000 && codepoint <= 0x303F) || 
           (codepoint >= 0xFF00 && codepoint <= 0xFFEF);
}

hb_script_t TextUtils::detectScript(uint32_t codepoint)
{
    if ((codepoint >= 0x4E00 && codepoint <= 0x9FFF) ||
        (codepoint >= 0x3400 && codepoint <= 0x4DBF) ||
        (codepoint >= 0x20000 && codepoint <= 0x2A6DF) || 
        (codepoint >= 0x2A700 && codepoint <= 0x2B73F) ||
        (codepoint >= 0x2B740 && codepoint <= 0x2B81F) || 
        (codepoint >= 0xF900 && codepoint <= 0xFAFF) ||
        (codepoint >= 0x2F800 && codepoint <= 0x2FA1F)) return HB_SCRIPT_HAN;
    if ((codepoint >= 0x0020 && codepoint <= 0x007F) || 
        (codepoint >= 0x00A0 && codepoint <= 0x00FF) ||
        (codepoint >= 0x0100 && codepoint <= 0x017F) ||
        (codepoint >= 0x0180 && codepoint <= 0x024F)) return HB_SCRIPT_LATIN;
    if (codepoint >= 0x3040 && codepoint <= 0x309F) return HB_SCRIPT_HIRAGANA;
    if (codepoint >= 0x30A0 && codepoint <= 0x30FF) return HB_SCRIPT_KATAKANA;
    if (codepoint >= 0xAC00 && codepoint <= 0xD7AF) return HB_SCRIPT_HANGUL;
    return HB_SCRIPT_COMMON;
}

hb_script_t TextUtils::detectTextScript(const char* text)
{
    const char* p = text;
    hb_script_t dominantScript = HB_SCRIPT_COMMON;
    int hanCount = 0, latinCount = 0, otherCount = 0;
    
    while (*p)
    {
        uint32_t codepoint = decodeUTF8(p);
        if (codepoint == 0) break;
        if (codepoint == 0xFFFD) continue;
        
        hb_script_t script = detectScript(codepoint);
        if (script == HB_SCRIPT_HAN) hanCount++;
        else if (script == HB_SCRIPT_LATIN) latinCount++;
        else if (script != HB_SCRIPT_COMMON)
        {
            otherCount++;
            dominantScript = script;
        }
    }
    
    if (hanCount > 0) return HB_SCRIPT_HAN;
    if (otherCount > 0) return dominantScript;
    return latinCount > 0 ? HB_SCRIPT_LATIN : HB_SCRIPT_COMMON;
}

const char* TextUtils::getLanguageForScript(hb_script_t script)
{
    switch (script) {
        case HB_SCRIPT_HAN:     return "zh-cn";
        case HB_SCRIPT_LATIN:   return "en";
        case HB_SCRIPT_HIRAGANA:
        case HB_SCRIPT_KATAKANA:return "ja";
        case HB_SCRIPT_HANGUL:  return "ko";
        case HB_SCRIPT_ARABIC:  return "ar";
        case HB_SCRIPT_HEBREW:  return "he";
        case HB_SCRIPT_THAI:    return "th";
        default:                return "en";
    }
}

std::vector<TextSegment> TextUtils::segmentText(const char* text, FontHandle westernFont, FontHandle chineseFont)
{
    YUCHEN_ASSERT_MSG(westernFont != INVALID_FONT_HANDLE, "Invalid western font handle");
    YUCHEN_ASSERT_MSG(chineseFont != INVALID_FONT_HANDLE, "Invalid chinese font handle");
    
    std::vector<TextSegment> segments;
    const char* p = text;
    
    while (*p)
    {
        const char* charStart = p;
        uint32_t codepoint = decodeUTF8(p);
        if (codepoint == 0) break;
        
        size_t charLength = p - charStart;
        FontHandle selectedFont;
        
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
            selectedFont = westernFont;
        }
        
        if (segments.empty() || segments.back().fontHandle != selectedFont)
        {
            TextSegment segment;
            segment.fontHandle = selectedFont;
            segment.originalStartIndex = charStart - text;
            segment.originalLength = 0;
            segments.push_back(segment);
        }
        
        segments.back().text.append(charStart, charLength);
        segments.back().originalLength += charLength;
    }
    
    return segments;
}

} // namespace YuchenUI
