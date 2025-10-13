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
/** @file FontDatabase.h
    
    Qt-style font database with automatic role-based font assignment.
    
    Provides automatic font discovery from embedded resources, intelligent font
    matching by family/weight/style, and semantic role assignment based on
    Unicode coverage and font properties.
    
    Key Features:
    - Zero-configuration font loading from resources
    - Automatic role assignment (Default, CJK, Emoji, Symbol, etc.)
    - FreeType-based metadata extraction
    - Smart font matching algorithm
    - Unicode coverage analysis
    
    Design Pattern:
    Similar to Qt's QFontDatabase but with automatic role assignment to eliminate
    hardcoded font names. The system analyzes each font's Unicode coverage and
    properties to determine its optimal role.
    
    Usage Example:
    @code
    FontDatabase db;
    db.initialize(ftLibrary);
    db.discoverAndRegisterFonts();
    
    // Get font by role (no hardcoded names!)
    FontHandle defaultFont = db.getFontForRole(FontRole::DefaultRegular);
    FontHandle cjkFont = db.getFontForRole(FontRole::CJK);
    FontHandle emojiFont = db.getFontForRole(FontRole::Emoji);
    
    // Or query by family if needed
    FontHandle arial = db.findFont("Arial", FontWeight::Bold);
    @endcode
    
    @see FontManager, IFontProvider
*/

#pragma once

#include "YuchenUI/core/Types.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace YuchenUI {

//==========================================================================================
/** Font weight classification.
    
    Standard weight values matching CSS font-weight and OpenType usWeightClass.
*/
enum class FontWeight {
    Thin = 100,
    ExtraLight = 200,
    Light = 300,
    Normal = 400,
    Medium = 500,
    SemiBold = 600,
    Bold = 700,
    ExtraBold = 800,
    Black = 900
};

//==========================================================================================
/** Font style classification. */
enum class FontStyle {
    Normal,
    Italic,
    Oblique
};

//==========================================================================================
/** Font stretch/width classification. */
enum class FontStretch {
    UltraCondensed,
    ExtraCondensed,
    Condensed,
    SemiCondensed,
    Normal,
    SemiExpanded,
    Expanded,
    ExtraExpanded,
    UltraExpanded
};

//==========================================================================================
/** Semantic font roles for automatic assignment.
    
    The system automatically analyzes registered fonts and assigns them to
    appropriate roles based on Unicode coverage and font properties.
*/
enum class FontRole {
    Unknown = 0,
    DefaultRegular,     ///< Primary UI font (Latin, normal weight)
    DefaultBold,        ///< Bold variant of primary font
    DefaultItalic,      ///< Italic variant of primary font
    DefaultNarrow,      ///< Condensed variant of primary font
    CJK,                ///< Chinese/Japanese/Korean support
    Arabic,             ///< Arabic script support
    Hebrew,             ///< Hebrew script support
    Emoji,              ///< Color emoji support
    Symbol,             ///< Symbol/icon font
    Monospace           ///< Fixed-width font
};

//==========================================================================================
/** Font descriptor with complete metadata. */
struct FontDescriptor {
    std::string familyName;
    std::string styleName;
    std::string fullName;
    std::string postScriptName;
    
    FontWeight weight;
    FontStyle style;
    FontStretch stretch;
    
    bool isFixedPitch;
    bool isScalable;
    bool hasColorGlyphs;
    
    int numGlyphs;
    int unitsPerEM;
    
    std::vector<std::pair<uint32_t, uint32_t>> unicodeRanges;
    
    FontHandle handle;
    std::string sourcePath;
    
    FontDescriptor()
        : weight(FontWeight::Normal)
        , style(FontStyle::Normal)
        , stretch(FontStretch::Normal)
        , isFixedPitch(false)
        , isScalable(true)
        , hasColorGlyphs(false)
        , numGlyphs(0)
        , unitsPerEM(0)
        , handle(INVALID_FONT_HANDLE)
    {}
};

//==========================================================================================
/** Font query specification. */
struct FontQuery {
    std::string familyName;
    FontWeight weight;
    FontStyle style;
    FontStretch stretch;
    
    FontQuery()
        : weight(FontWeight::Normal)
        , style(FontStyle::Normal)
        , stretch(FontStretch::Normal)
    {}
    
    FontQuery(const char* family, FontWeight w = FontWeight::Normal,
              FontStyle s = FontStyle::Normal)
        : familyName(family ? family : "")
        , weight(w)
        , style(s)
        , stretch(FontStretch::Normal)
    {}
};

//==========================================================================================
class FontFile;
class FontFace;
class FontCache;
struct FontEntry;

//==========================================================================================
/**
    Qt-style font database with automatic role assignment.
    
    Provides centralized font registration, discovery, metadata extraction,
    and automatic semantic role assignment. Eliminates hardcoded font names
    by analyzing Unicode coverage and font properties.
    
    Thread Safety: Not thread-safe.
    
    @see FontManager, FontDescriptor, FontQuery
*/
class FontDatabase {
public:
    FontDatabase();
    ~FontDatabase();
    
    //======================================================================================
    // Initialization
    
    bool initialize(FT_Library library);
    void shutdown();
    bool isInitialized() const { return m_isInitialized; }
    
    //======================================================================================
    // Font Registration
    
    FontHandle registerFont(const char* path, const char* name, FontEntry* entry);
    FontHandle registerFontFromMemory(const void* data, size_t size,
                                      const char* name, FontEntry* entry);
    
    //======================================================================================
    // Auto-Discovery and Role Assignment
    
    /** Discovers all embedded fonts from resources and assigns roles.
        
        This is the main entry point for automatic font loading.
        Scans resources/fonts/ directory, registers all fonts, analyzes their
        properties, and assigns them to semantic roles automatically.
        
        @param fontEntries  Vector to store font entries
        @returns Number of fonts discovered and registered
    */
    int discoverAndRegisterFonts(std::vector<FontEntry>& fontEntries);
    
    /** Assigns semantic roles to all registered fonts.
        
        Analyzes Unicode coverage and font properties to determine the best
        font for each role. Called automatically by discoverAndRegisterFonts().
    */
    void assignFontRoles();
    
    /** Returns font handle for specified role.
        
        @param role  Semantic font role
        @returns Font handle, or INVALID_FONT_HANDLE if role unassigned
    */
    FontHandle getFontForRole(FontRole role) const;
    
    //======================================================================================
    // Font Queries
    
    FontHandle findFont(const char* familyName,
                        FontWeight weight = FontWeight::Normal,
                        FontStyle style = FontStyle::Normal) const;
    
    FontHandle findFont(const FontQuery& query) const;
    
    std::vector<std::string> families() const;
    std::vector<FontDescriptor> fontsForFamily(const char* familyName) const;
    const FontDescriptor* getFontDescriptor(FontHandle handle) const;
    
    //======================================================================================
    // Smart Matching
    
    FontHandle selectBestMatch(const FontQuery& query) const;
    
    //======================================================================================
    // Metadata Access
    
    bool supportsCharacter(FontHandle handle, uint32_t codepoint) const;
    std::vector<std::pair<uint32_t, uint32_t>> getUnicodeCoverage(FontHandle handle) const;

private:
    //======================================================================================
    // Metadata Extraction
    
    bool extractFontMetadata(FT_Face face, FontDescriptor& descriptor);
    FontWeight detectFontWeight(const char* styleName, long styleFlags) const;
    FontStyle detectFontStyle(const char* styleName, long styleFlags) const;
    FontStretch detectFontStretch(const char* styleName) const;
    void analyzeUnicodeCoverage(FT_Face face, FontDescriptor& descriptor);
    
    //======================================================================================
    // Role Assignment Helpers
    
    /** Checks if font has good Latin script coverage. */
    bool hasLatinCoverage(const FontDescriptor& desc) const;
    
    /** Checks if font has CJK (Chinese/Japanese/Korean) coverage. */
    bool hasCJKCoverage(const FontDescriptor& desc) const;
    
    /** Checks if font has Arabic script coverage. */
    bool hasArabicCoverage(const FontDescriptor& desc) const;
    
    /** Checks if font has Hebrew script coverage. */
    bool hasHebrewCoverage(const FontDescriptor& desc) const;
    
    /** Finds best font for role based on criteria. */
    FontHandle findBestFontForRole(FontRole role) const;
    
    //======================================================================================
    // Utilities
    
    int calculateWeightDistance(FontWeight w1, FontWeight w2) const;
    bool stringEqualsCaseInsensitive(const std::string& s1, const std::string& s2) const;
    
    //======================================================================================
    FT_Library m_library;
    bool m_isInitialized;
    std::unordered_map<FontHandle, FontDescriptor> m_descriptors;
    std::unordered_map<std::string, std::vector<FontHandle>> m_familyMap;
    std::unordered_map<FontRole, FontHandle> m_roleAssignments;
    
    FontDatabase(const FontDatabase&) = delete;
    FontDatabase& operator=(const FontDatabase&) = delete;
};

} // namespace YuchenUI
