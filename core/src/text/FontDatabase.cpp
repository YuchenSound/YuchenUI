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

#include "YuchenUI/text/FontDatabase.h"
#include "YuchenUI/text/Font.h"
#include "YuchenUI/text/FontManager.h"
#include "YuchenUI/core/Assert.h"
#include "../generated/embedded_resources.h"
#include <algorithm>
#include <cctype>
#include <iostream>

namespace YuchenUI {

//==========================================================================================
// Lifecycle

FontDatabase::FontDatabase()
    : m_library(nullptr)
    , m_isInitialized(false)
    , m_descriptors()
    , m_familyMap()
    , m_roleAssignments()
{
}

FontDatabase::~FontDatabase()
{
    shutdown();
}

bool FontDatabase::initialize(FT_Library library)
{
    YUCHEN_ASSERT_MSG(!m_isInitialized, "FontDatabase already initialized");
    YUCHEN_ASSERT_MSG(library != nullptr, "FreeType library is null");
    
    m_library = library;
    m_isInitialized = true;
    return true;
}

void FontDatabase::shutdown()
{
    if (!m_isInitialized) return;
    
    m_descriptors.clear();
    m_familyMap.clear();
    m_roleAssignments.clear();
    m_library = nullptr;
    m_isInitialized = false;
}

//==========================================================================================
// Font Registration

FontHandle FontDatabase::registerFont(const char* path, const char* name, FontEntry* entry)
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "FontDatabase not initialized");
    YUCHEN_ASSERT_MSG(path != nullptr, "Path cannot be null");
    YUCHEN_ASSERT_MSG(entry != nullptr, "FontEntry cannot be null");
    
    entry->file = std::make_unique<FontFile>();
    if (!entry->file->loadFromFile(path, name ? name : ""))
    {
        return INVALID_FONT_HANDLE;
    }
    
    entry->face = std::make_unique<FontFace>(m_library);
    if (!entry->face->createFromFontFile(*entry->file))
    {
        return INVALID_FONT_HANDLE;
    }
    
    FontDescriptor descriptor;
    descriptor.handle = static_cast<FontHandle>(m_descriptors.size());
    descriptor.sourcePath = path;
    
    FT_Face ftFace = entry->face->getFTFace();
    if (!extractFontMetadata(ftFace, descriptor))
    {
        return INVALID_FONT_HANDLE;
    }
    
    entry->cache = std::make_unique<FontCache>();
    entry->name = descriptor.fullName;
    entry->isValid = true;
    
    FontHandle handle = descriptor.handle;
    m_descriptors[handle] = descriptor;
    m_familyMap[descriptor.familyName].push_back(handle);
    
    return handle;
}

FontHandle FontDatabase::registerFontFromMemory(const void* data, size_t size,
                                                const char* name, FontEntry* entry)
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "FontDatabase not initialized");
    YUCHEN_ASSERT_MSG(data != nullptr, "Data cannot be null");
    YUCHEN_ASSERT_MSG(size > 0, "Size must be positive");
    YUCHEN_ASSERT_MSG(entry != nullptr, "FontEntry cannot be null");
    
    entry->file = std::make_unique<FontFile>();
    if (!entry->file->loadFromMemory(data, size, name ? name : ""))
    {
        return INVALID_FONT_HANDLE;
    }
    
    entry->face = std::make_unique<FontFace>(m_library);
    if (!entry->face->createFromFontFile(*entry->file))
    {
        return INVALID_FONT_HANDLE;
    }
    
    FontDescriptor descriptor;
    descriptor.handle = static_cast<FontHandle>(m_descriptors.size());
    
    descriptor.sourcePath = name ? name : "<embedded>";
    
    FT_Face ftFace = entry->face->getFTFace();
    if (!extractFontMetadata(ftFace, descriptor))
    {
        return INVALID_FONT_HANDLE;
    }
    
    entry->cache = std::make_unique<FontCache>();
    entry->name = descriptor.fullName;
    entry->isValid = true;
    
    FontHandle handle = descriptor.handle;
    m_descriptors[handle] = descriptor;
    m_familyMap[descriptor.familyName].push_back(handle);
    
    return handle;
}

//==========================================================================================
// Auto-Discovery

int FontDatabase::discoverAndRegisterFonts(std::vector<FontEntry>& fontEntries)
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "FontDatabase not initialized");
    
    int count = 0;
    const Resources::ResourceData* allResources = Resources::getAllResources();
    size_t resourceCount = Resources::getResourceCount();
    
    for (size_t i = 0; i < resourceCount; ++i)
    {
        const auto& res = allResources[i];
        std::string path(res.path);
        
        // Skip non-font resources
        if (path.find("fonts/") != 0) continue;
        
        // Check file extension
        if (path.size() < 4) continue;
        std::string ext = path.substr(path.size() - 4);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        if (ext != ".ttf" && ext != ".otf" && ext != ".ttc") continue;
        
        // Extract filename without extension
        size_t lastSlash = path.find_last_of('/');
        size_t lastDot = path.find_last_of('.');
        std::string fileName = path.substr(lastSlash + 1, lastDot - lastSlash - 1);
        
        // Register font
        size_t fontIndex = fontEntries.size();
        fontEntries.emplace_back();
        
        FontHandle handle = registerFontFromMemory(
            res.data,
            res.size,
            fileName.c_str(),
            &fontEntries[fontIndex]
        );
        
        if (handle != INVALID_FONT_HANDLE)
        { count++; }
        else
        { fontEntries.pop_back(); }
    }
    
    if   (count > 0) { assignFontRoles(); }
    else { std::cerr << "[FontDatabase] WARNING: No embedded fonts found" << std::endl; }
    
    return count;
}

//==========================================================================================
// Role Assignment

void FontDatabase::assignFontRoles()
{
    m_roleAssignments.clear();
    
    FontHandle defaultRegular   = INVALID_FONT_HANDLE;
    FontHandle defaultBold      = INVALID_FONT_HANDLE;
    FontHandle defaultItalic    = INVALID_FONT_HANDLE;
    FontHandle defaultNarrow    = INVALID_FONT_HANDLE;
    FontHandle cjkFont          = INVALID_FONT_HANDLE;
    FontHandle emojiFont        = INVALID_FONT_HANDLE;
    FontHandle symbolFont       = INVALID_FONT_HANDLE;
    FontHandle monospaceFont    = INVALID_FONT_HANDLE;
    
    for (const auto& pair : m_descriptors)
    {
        const FontDescriptor& desc = pair.second;
        if (desc.hasColorGlyphs && emojiFont == INVALID_FONT_HANDLE)
        { emojiFont = desc.handle; continue; }
        if (hasCJKCoverage(desc) && cjkFont == INVALID_FONT_HANDLE)
        { cjkFont = desc.handle; }
        if (desc.isFixedPitch && monospaceFont == INVALID_FONT_HANDLE)
        { monospaceFont = desc.handle; }
        
        if (hasLatinCoverage(desc))
        {
            if (desc.stretch == FontStretch::Condensed && desc.weight == FontWeight::Normal && desc.style == FontStyle::Normal && defaultNarrow == INVALID_FONT_HANDLE)
            { defaultNarrow = desc.handle; }
            if (desc.stretch == FontStretch::Normal)
            {
                if (desc.weight == FontWeight::Normal && desc.style == FontStyle::Normal)
                { if (defaultRegular == INVALID_FONT_HANDLE) defaultRegular = desc.handle; }
                else if (desc.weight == FontWeight::Bold && desc.style == FontStyle::Normal)
                { if (defaultBold == INVALID_FONT_HANDLE) defaultBold = desc.handle; }
                else if (desc.style == FontStyle::Italic && desc.weight == FontWeight::Normal)
                { if (defaultItalic == INVALID_FONT_HANDLE) defaultItalic = desc.handle; }
            }
        }
    }
    if (defaultRegular != INVALID_FONT_HANDLE)  m_roleAssignments[FontRole::DefaultRegular] = defaultRegular;
    if (defaultBold != INVALID_FONT_HANDLE)     m_roleAssignments[FontRole::DefaultBold]    = defaultBold;
    if (defaultItalic != INVALID_FONT_HANDLE)   m_roleAssignments[FontRole::DefaultItalic]  = defaultItalic;
    if (defaultNarrow != INVALID_FONT_HANDLE)   m_roleAssignments[FontRole::DefaultNarrow]  = defaultNarrow;
    if (cjkFont != INVALID_FONT_HANDLE)         m_roleAssignments[FontRole::CJK]            = cjkFont;
    if (emojiFont != INVALID_FONT_HANDLE)       m_roleAssignments[FontRole::Emoji]          = emojiFont;
    if (symbolFont != INVALID_FONT_HANDLE)      m_roleAssignments[FontRole::Symbol]         = symbolFont;
    if (monospaceFont != INVALID_FONT_HANDLE)   m_roleAssignments[FontRole::Monospace]      = monospaceFont;
}

FontHandle FontDatabase::getFontForRole(FontRole role) const
{
    auto it = m_roleAssignments.find(role);
    if (it != m_roleAssignments.end()) return it->second;
    return INVALID_FONT_HANDLE;
}

//==========================================================================================
// Font Queries

FontHandle FontDatabase::findFont(const char* familyName, FontWeight weight,
                                  FontStyle style) const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "FontDatabase not initialized");
    YUCHEN_ASSERT_MSG(familyName != nullptr, "Family name cannot be null");
    
    auto familyIt = m_familyMap.end();
    for (auto it = m_familyMap.begin(); it != m_familyMap.end(); ++it)
    {
        if (stringEqualsCaseInsensitive(it->first, familyName))
        {
            familyIt = it;
            break;
        }
    }
    
    if (familyIt == m_familyMap.end())
    {
        return INVALID_FONT_HANDLE;
    }
    
    for (FontHandle handle : familyIt->second)
    {
        const auto& desc = m_descriptors.at(handle);
        if (desc.weight == weight && desc.style == style)
        {
            return handle;
        }
    }
    
    return INVALID_FONT_HANDLE;
}

FontHandle FontDatabase::findFont(const FontQuery& query) const
{
    return findFont(query.familyName.c_str(), query.weight, query.style);
}

std::vector<std::string> FontDatabase::families() const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "FontDatabase not initialized");
    
    std::vector<std::string> result;
    result.reserve(m_familyMap.size());
    
    for (const auto& pair : m_familyMap)
    {
        result.push_back(pair.first);
    }
    
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<FontDescriptor> FontDatabase::fontsForFamily(const char* familyName) const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "FontDatabase not initialized");
    YUCHEN_ASSERT_MSG(familyName != nullptr, "Family name cannot be null");
    
    std::vector<FontDescriptor> result;
    
    auto familyIt = m_familyMap.end();
    for (auto it = m_familyMap.begin(); it != m_familyMap.end(); ++it)
    {
        if (stringEqualsCaseInsensitive(it->first, familyName))
        {
            familyIt = it;
            break;
        }
    }
    
    if (familyIt == m_familyMap.end())
    {
        return result;
    }
    
    result.reserve(familyIt->second.size());
    for (FontHandle handle : familyIt->second)
    {
        result.push_back(m_descriptors.at(handle));
    }
    
    return result;
}

const FontDescriptor* FontDatabase::getFontDescriptor(FontHandle handle) const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "FontDatabase not initialized");
    
    auto it = m_descriptors.find(handle);
    if (it == m_descriptors.end())
    {
        return nullptr;
    }
    
    return &it->second;
}

//==========================================================================================
// Smart Matching

FontHandle FontDatabase::selectBestMatch(const FontQuery& query) const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "FontDatabase not initialized");
    
    if (query.familyName.empty())
    {
        if (m_descriptors.empty())
        {
            return INVALID_FONT_HANDLE;
        }
        return m_descriptors.begin()->first;
    }
    
    auto familyIt = m_familyMap.end();
    for (auto it = m_familyMap.begin(); it != m_familyMap.end(); ++it)
    {
        if (stringEqualsCaseInsensitive(it->first, query.familyName))
        {
            familyIt = it;
            break;
        }
    }
    
    if (familyIt == m_familyMap.end())
    {
        if (m_descriptors.empty())
        {
            return INVALID_FONT_HANDLE;
        }
        return m_descriptors.begin()->first;
    }
    
    const auto& handles = familyIt->second;
    if (handles.empty())
    {
        return INVALID_FONT_HANDLE;
    }
    
    for (FontHandle handle : handles)
    {
        const auto& desc = m_descriptors.at(handle);
        if (desc.weight == query.weight && desc.style == query.style)
        {
            return handle;
        }
    }
    
    FontHandle closestWeightHandle = INVALID_FONT_HANDLE;
    int minWeightDistance = 1000;
    
    for (FontHandle handle : handles)
    {
        const auto& desc = m_descriptors.at(handle);
        if (desc.style == query.style)
        {
            int distance = calculateWeightDistance(desc.weight, query.weight);
            if (distance < minWeightDistance)
            {
                minWeightDistance = distance;
                closestWeightHandle = handle;
            }
        }
    }
    
    if (closestWeightHandle != INVALID_FONT_HANDLE)
    {
        return closestWeightHandle;
    }
    
    for (FontHandle handle : handles)
    {
        const auto& desc = m_descriptors.at(handle);
        if (desc.weight == query.weight)
        {
            return handle;
        }
    }
    
    closestWeightHandle = INVALID_FONT_HANDLE;
    minWeightDistance = 1000;
    
    for (FontHandle handle : handles)
    {
        const auto& desc = m_descriptors.at(handle);
        int distance = calculateWeightDistance(desc.weight, query.weight);
        if (distance < minWeightDistance)
        {
            minWeightDistance = distance;
            closestWeightHandle = handle;
        }
    }
    
    if (closestWeightHandle != INVALID_FONT_HANDLE)
    {
        return closestWeightHandle;
    }
    
    return handles[0];
}

//==========================================================================================
// Metadata Access

bool FontDatabase::supportsCharacter(FontHandle handle, uint32_t codepoint) const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "FontDatabase not initialized");
    
    const FontDescriptor* desc = getFontDescriptor(handle);
    if (!desc)
    {
        return false;
    }
    
    for (const auto& range : desc->unicodeRanges)
    {
        if (codepoint >= range.first && codepoint <= range.second)
        {
            return true;
        }
    }
    
    return false;
}

std::vector<std::pair<uint32_t, uint32_t>> FontDatabase::getUnicodeCoverage(
    FontHandle handle) const
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "FontDatabase not initialized");
    
    const FontDescriptor* desc = getFontDescriptor(handle);
    if (!desc)
    {
        return std::vector<std::pair<uint32_t, uint32_t>>();
    }
    
    return desc->unicodeRanges;
}

//==========================================================================================
// Metadata Extraction

bool FontDatabase::extractFontMetadata(FT_Face face, FontDescriptor& descriptor)
{
    YUCHEN_ASSERT_MSG(face != nullptr, "FT_Face is null");
    
    descriptor.familyName = face->family_name ? face->family_name : "";
    descriptor.styleName = face->style_name ? face->style_name : "";
    descriptor.fullName = descriptor.familyName + " " + descriptor.styleName;
    
    const char* psName = FT_Get_Postscript_Name(face);
    descriptor.postScriptName = psName ? psName : "";
    
    descriptor.weight = detectFontWeight(face->style_name, face->style_flags);
    descriptor.style = detectFontStyle(face->style_name, face->style_flags);
    descriptor.stretch = detectFontStretch(face->style_name);
    
    descriptor.isFixedPitch = FT_IS_FIXED_WIDTH(face) != 0;
    descriptor.isScalable = FT_IS_SCALABLE(face) != 0;
    descriptor.numGlyphs = face->num_glyphs;
    descriptor.unitsPerEM = face->units_per_EM;
    
    descriptor.hasColorGlyphs = FT_HAS_COLOR(face) != 0;
    
    analyzeUnicodeCoverage(face, descriptor);
    
    return true;
}

FontWeight FontDatabase::detectFontWeight(const char* styleName, long styleFlags) const
{
    if (!styleName)
    {
        return FontWeight::Normal;
    }
    
    std::string style = styleName;
    std::transform(style.begin(), style.end(), style.begin(), ::tolower);
    
    if (style.find("thin") != std::string::npos || style.find("hairline") != std::string::npos)
        return FontWeight::Thin;
    if (style.find("extralight") != std::string::npos || style.find("ultralight") != std::string::npos)
        return FontWeight::ExtraLight;
    if (style.find("light") != std::string::npos)
        return FontWeight::Light;
    if (style.find("medium") != std::string::npos)
        return FontWeight::Medium;
    if (style.find("semibold") != std::string::npos || style.find("demibold") != std::string::npos)
        return FontWeight::SemiBold;
    if (style.find("extrabold") != std::string::npos || style.find("ultrabold") != std::string::npos)
        return FontWeight::ExtraBold;
    if (style.find("black") != std::string::npos || style.find("heavy") != std::string::npos)
        return FontWeight::Black;
    if (style.find("bold") != std::string::npos || (styleFlags & FT_STYLE_FLAG_BOLD))
        return FontWeight::Bold;
    
    return FontWeight::Normal;
}

FontStyle FontDatabase::detectFontStyle(const char* styleName, long styleFlags) const
{
    if (!styleName)
    {
        return FontStyle::Normal;
    }
    
    std::string style = styleName;
    std::transform(style.begin(), style.end(), style.begin(), ::tolower);
    
    if (styleFlags & FT_STYLE_FLAG_ITALIC)
        return FontStyle::Italic;
    
    if (style.find("italic") != std::string::npos)
        return FontStyle::Italic;
    if (style.find("oblique") != std::string::npos || style.find("slanted") != std::string::npos)
        return FontStyle::Oblique;
    
    return FontStyle::Normal;
}

FontStretch FontDatabase::detectFontStretch(const char* styleName) const
{
    if (!styleName)
    {
        return FontStretch::Normal;
    }
    
    std::string style = styleName;
    std::transform(style.begin(), style.end(), style.begin(), ::tolower);
    
    if (style.find("ultracondensed") != std::string::npos ||
        style.find("ultracompressed") != std::string::npos)
        return FontStretch::UltraCondensed;
    if (style.find("extracondensed") != std::string::npos ||
        style.find("extracompressed") != std::string::npos)
        return FontStretch::ExtraCondensed;
    if (style.find("semicondensed") != std::string::npos)
        return FontStretch::SemiCondensed;
    if (style.find("condensed") != std::string::npos || style.find("narrow") != std::string::npos)
        return FontStretch::Condensed;
    if (style.find("semiexpanded") != std::string::npos)
        return FontStretch::SemiExpanded;
    if (style.find("extraexpanded") != std::string::npos)
        return FontStretch::ExtraExpanded;
    if (style.find("ultraexpanded") != std::string::npos)
        return FontStretch::UltraExpanded;
    if (style.find("expanded") != std::string::npos || style.find("extended") != std::string::npos)
        return FontStretch::Expanded;
    
    return FontStretch::Normal;
}

void FontDatabase::analyzeUnicodeCoverage(FT_Face face, FontDescriptor& descriptor)
{
    YUCHEN_ASSERT_MSG(face != nullptr, "FT_Face is null");
    
    descriptor.unicodeRanges.clear();
    
    FT_UInt glyphIndex;
    FT_ULong charcode = FT_Get_First_Char(face, &glyphIndex);
    
    uint32_t rangeStart = static_cast<uint32_t>(charcode);
    uint32_t rangeEnd = rangeStart;
    
    while (glyphIndex != 0)
    {
        FT_ULong nextCharcode = FT_Get_Next_Char(face, charcode, &glyphIndex);
        
        if (glyphIndex == 0)
        {
            break;
        }
        
        if (nextCharcode == charcode + 1)
        {
            rangeEnd = static_cast<uint32_t>(nextCharcode);
        }
        else
        {
            descriptor.unicodeRanges.push_back(std::make_pair(rangeStart, rangeEnd));
            rangeStart = static_cast<uint32_t>(nextCharcode);
            rangeEnd = rangeStart;
        }
        
        charcode = nextCharcode;
    }
    
    if (rangeStart <= rangeEnd)
    {
        descriptor.unicodeRanges.push_back(std::make_pair(rangeStart, rangeEnd));
    }
}

//==========================================================================================
// Role Assignment Helpers

bool FontDatabase::hasLatinCoverage(const FontDescriptor& desc) const
{
    for (const auto& range : desc.unicodeRanges)
    {
        if (range.first <= 0x007A && range.second >= 0x0041)
        {
            return true;
        }
    }
    return false;
}

bool FontDatabase::hasCJKCoverage(const FontDescriptor& desc) const
{
    for (const auto& range : desc.unicodeRanges)
    {
        if (range.first <= 0x9FFF && range.second >= 0x4E00)
        {
            return true;
        }
    }
    return false;
}

bool FontDatabase::hasArabicCoverage(const FontDescriptor& desc) const
{
    for (const auto& range : desc.unicodeRanges)
    {
        if (range.first <= 0x06FF && range.second >= 0x0600)
        {
            return true;
        }
    }
    return false;
}

bool FontDatabase::hasHebrewCoverage(const FontDescriptor& desc) const
{
    for (const auto& range : desc.unicodeRanges)
    {
        if (range.first <= 0x05FF && range.second >= 0x0590)
        {
            return true;
        }
    }
    return false;
}

//==========================================================================================
// Utilities

int FontDatabase::calculateWeightDistance(FontWeight w1, FontWeight w2) const
{
    return std::abs(static_cast<int>(w1) - static_cast<int>(w2));
}

bool FontDatabase::stringEqualsCaseInsensitive(const std::string& s1,
                                                const std::string& s2) const
{
    if (s1.size() != s2.size())
    {
        return false;
    }
    
    for (size_t i = 0; i < s1.size(); ++i)
    {
        if (std::tolower(static_cast<unsigned char>(s1[i])) !=
            std::tolower(static_cast<unsigned char>(s2[i])))
        {
            return false;
        }
    }
    
    return true;
}

} // namespace YuchenUI
