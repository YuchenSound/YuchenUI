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

#include "YuchenUI/core/Assert.h"
#include <cmath>
#include <vector>
#include <string>

//==========================================================================================
// Platform-specific struct packing macros

#ifdef _MSC_VER
    #define YUCHEN_PACK_BEGIN __pragma(pack(push, 1))
    #define YUCHEN_PACK_END __pragma(pack(pop))
    #define YUCHEN_PACKED
#else
    #define YUCHEN_PACK_BEGIN
    #define YUCHEN_PACK_END
    #define YUCHEN_PACKED __attribute__((packed))
#endif

namespace YuchenUI {

//==========================================================================================
/** 2D vector for positions and sizes */
struct Vec2
{
    float x, y;

    Vec2() : x(0.0f), y(0.0f) {}
    Vec2(float x, float y) : x(x), y(y) {}

    /** Returns true if both components are finite */
    bool isValid() const
    {
        return std::isfinite(x) && std::isfinite(y);
    }
    
    bool operator==(const Vec2& other) const
    {
        return x == other.x && y == other.y;
    }
    
    bool operator!=(const Vec2& other) const
    {
        return !(*this == other);
    }
};


//==========================================================================================
/** 4D vector for colors (RGBA) and general purposes */
struct Vec4
{
    float x, y, z, w;

    Vec4() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
    Vec4(float x, float y, float z, float w = 1.0f) : x(x), y(y), z(z), w(w) {}

    static Vec4 FromRGBA(int r, int g, int b, int a = 255)
    {
        r = (r < 0) ? 0 : (r > 255) ? 255 : r;
        g = (g < 0) ? 0 : (g > 255) ? 255 : g;
        b = (b < 0) ? 0 : (b > 255) ? 255 : b;
        a = (a < 0) ? 0 : (a > 255) ? 255 : a;
        return Vec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
    }

    bool isValid() const
    {
        return std::isfinite(x) && std::isfinite(y) && std::isfinite(z) && std::isfinite(w);
    }
    
    bool operator==(const Vec4& other) const
    {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }
    
    bool operator!=(const Vec4& other) const
    {
        return !(*this == other);
    }
};

//==========================================================================================
/** Rectangle defined by position and size */
struct Rect
{
    float x, y, width, height;

    Rect() : x(0.0f), y(0.0f), width(0.0f), height(0.0f) {}
    Rect(float x, float y, float w, float h) : x(x), y(y), width(w), height(h)
    {
        if (w < 0.0f) this->width = 0.0f;
        if (h < 0.0f) this->height = 0.0f;
    }

    bool contains(float px, float py) const
    {
        return px >= x && px <= (x + width) && py >= y && py <= (y + height);
    }

    bool contains(const Vec2& point) const
    {
        return contains(point.x, point.y);
    }

    bool isValid() const
    {
        return std::isfinite(x) && std::isfinite(y) && std::isfinite(width) && std::isfinite(height) && width >= 0.0f && height >= 0.0f;
    }
    
    bool operator==(const Rect& other) const
    {
        return x == other.x && y == other.y && width == other.width && height == other.height;
    }
    
    bool operator!=(const Rect& other) const
    {
        return !(*this == other);
    }
};

//==========================================================================================
/** Rounded corner radii for rectangles */
struct CornerRadius
{
    float topLeft, topRight, bottomLeft, bottomRight;

    CornerRadius() : topLeft(0.0f), topRight(0.0f), bottomLeft(0.0f), bottomRight(0.0f) {}

    /** Uniform radius for all corners */
    CornerRadius(float radius) : topLeft(radius), topRight(radius), bottomLeft(radius), bottomRight(radius)
    {
        if (radius < 0.0f) {
            topLeft = topRight = bottomLeft = bottomRight = 0.0f;
        }
    }

    /** Individual corner radii */
    CornerRadius(float tl, float tr, float bl, float br) : topLeft(tl), topRight(tr), bottomLeft(bl), bottomRight(br)
    {
        if (tl < 0.0f) topLeft = 0.0f;
        if (tr < 0.0f) topRight = 0.0f;
        if (bl < 0.0f) bottomLeft = 0.0f;
        if (br < 0.0f) bottomRight = 0.0f;
    }

    bool isValid() const
    {
        return std::isfinite(topLeft) && std::isfinite(topRight) && std::isfinite(bottomLeft) && std::isfinite(bottomRight) &&
            topLeft >= 0.0f && topRight >= 0.0f && bottomLeft >= 0.0f && bottomRight >= 0.0f;
    }

    /** Validates that corner radii are appropriate for the given rectangle.
        
        Ensures no radius exceeds half the rect's dimensions and opposing corners don't overlap.
    */
    void validateForRect(const Rect& rect) const
    {
        YUCHEN_ASSERT(isValid());
        YUCHEN_ASSERT_MSG(topLeft <= rect.width * 0.5f && topLeft <= rect.height * 0.5f, "Top-left corner radius too large");
        YUCHEN_ASSERT_MSG(topRight <= rect.width * 0.5f && topRight <= rect.height * 0.5f, "Top-right corner radius too large");
        YUCHEN_ASSERT_MSG(bottomLeft <= rect.width * 0.5f && bottomLeft <= rect.height * 0.5f, "Bottom-left corner radius too large");
        YUCHEN_ASSERT_MSG(bottomRight <= rect.width * 0.5f && bottomRight <= rect.height * 0.5f, "Bottom-right corner radius too large");
        YUCHEN_ASSERT_MSG(topLeft + topRight <= rect.width, "Top corner radii sum too large");
        YUCHEN_ASSERT_MSG(bottomLeft + bottomRight <= rect.width, "Bottom corner radii sum too large");
        YUCHEN_ASSERT_MSG(topLeft + bottomLeft <= rect.height, "Left corner radii sum too large");
        YUCHEN_ASSERT_MSG(topRight + bottomRight <= rect.height, "Right corner radii sum too large");
    }
};

//==========================================================================================
/** Nine-slice scaling margins for images */
struct NineSliceMargins
{
    float left, top, right, bottom;

    NineSliceMargins() : left(0.0f), top(0.0f), right(0.0f), bottom(0.0f) {}

    NineSliceMargins(float l, float t, float r, float b) : left(l), top(t), right(r), bottom(b)
    {
        YUCHEN_ASSERT(std::isfinite(l) && std::isfinite(t) && std::isfinite(r) && std::isfinite(b));
        YUCHEN_ASSERT(l >= 0.0f && t >= 0.0f && r >= 0.0f && b >= 0.0f);
    }

    bool isValid() const
    {
        return std::isfinite(left) && std::isfinite(top) && std::isfinite(right) && std::isfinite(bottom) &&
            left >= 0.0f && top >= 0.0f && right >= 0.0f && bottom >= 0.0f;
    }

    bool isZero() const
    {
        return left == 0.0f && top == 0.0f && right == 0.0f && bottom == 0.0f;
    }
};

//==========================================================================================
// Alignment and scaling enums

enum class TextAlignment {
    Left = 0,
    Center,
    Right,
    Justify
};

enum class VerticalAlignment {
    Top = 0,
    Middle,
    Bottom,
    Baseline
};

enum class ScaleMode {
    Original,   ///< No scaling, use original size
    Stretch,    ///< Stretch to fill destination
    Fill,       ///< Scale to fill while maintaining aspect ratio
    NineSlice,  ///< Nine-slice scaling
    Tile        ///< Tile/repeat texture to fill destination
};

//==========================================================================================
/** Vertex for rounded rectangle rendering.
    
    Packed structure sent to GPU. Contains position, rect bounds, corner radii,
    color, and border width for shader-based rounded rect rendering.
*/
YUCHEN_PACK_BEGIN
struct RectVertex
{
    Vec2 position;
    Vec2 rectOrigin;
    Vec2 rectSize;
    Vec4 cornerRadius;
    Vec4 color;
    float borderWidth;

    RectVertex() : position(), rectOrigin(), rectSize(), cornerRadius(), color(), borderWidth(0.0f) {}

    RectVertex(const Vec2& pos, const Rect& rect, const CornerRadius& radius, const Vec4& col, float border = 0.0f)
        : position(pos), rectOrigin(rect.x, rect.y), rectSize(rect.width, rect.height),
        cornerRadius(radius.topLeft, radius.topRight, radius.bottomLeft, radius.bottomRight),
        color(col), borderWidth(border < 0.0f ? 0.0f : border) {
    }

    bool isValid() const
    {
        return position.isValid() && rectOrigin.isValid() && rectSize.isValid() &&
            cornerRadius.isValid() && color.isValid() && std::isfinite(borderWidth) && borderWidth >= 0.0f;
    }
} YUCHEN_PACKED;
YUCHEN_PACK_END

//==========================================================================================
// Font system types

// OLD: const FontHandle INVALID_FONT_HANDLE = 0;
// NEW: Use SIZE_MAX as invalid handle, allowing handle to start from 0
typedef size_t FontHandle;
const FontHandle INVALID_FONT_HANDLE = SIZE_MAX;

/** Font-level metrics */
struct FontMetrics {
    float ascender;     ///< Height above baseline
    float descender;    ///< Depth below baseline (negative)
    float lineHeight;   ///< Recommended line spacing
    float maxAdvance;   ///< Maximum horizontal advance

    FontMetrics() : ascender(0.0f), descender(0.0f), lineHeight(0.0f), maxAdvance(0.0f) {}
    
    FontMetrics(float asc, float desc, float lineH, float maxAdv)
        : ascender(asc), descender(desc), lineHeight(lineH), maxAdvance(maxAdv) {}

    bool isValid() const {
        return std::isfinite(ascender) && std::isfinite(descender) &&
            std::isfinite(lineHeight) && std::isfinite(maxAdvance) &&
            lineHeight > 0.0f;
    }
};

/** Per-glyph metrics */
struct GlyphMetrics {
    uint32_t glyphIndex;    ///< Glyph index in font
    Vec2 bearing;           ///< Offset from cursor to glyph top-left
    Vec2 size;              ///< Glyph bitmap size
    float advance;          ///< Horizontal advance to next glyph

    GlyphMetrics() : glyphIndex(0), bearing(), size(), advance(0.0f) {}

    bool isValid() const {
        return bearing.isValid() && size.isValid() &&
            std::isfinite(advance) && advance >= 0.0f;
    }
};

//==========================================================================================
/**
    Font fallback chain for multi-font text rendering.
    
    Defines an ordered list of fonts to try when rendering text. When a character
    cannot be rendered with the primary font, the system tries each font in the
    chain until it finds one that supports the character.
    
    This is similar to:
    - Qt: QFont::setFamilies()
    - CSS: font-family: Arial, "PingFang SC", "Apple Color Emoji"
    - Flutter: TextStyle(fontFamilyFallback: [...])
    
    Usage example:
    @code
    FontFallbackChain chain;
    chain.addFont(arialFont);           // Primary font
    chain.addFont(pingFangFont);        // CJK fallback
    chain.addFont(emojiFont);           // Emoji fallback
    
    // Or use builder pattern:
    FontFallbackChain chain = FontFallbackChain()
        .withFont(arialFont)
        .withFont(pingFangFont)
        .withFont(emojiFont);
    @endcode
*/
struct FontFallbackChain {
    std::vector<FontHandle> fonts;  ///< Ordered list of font handles (priority order)
    
    /** Creates empty fallback chain. */
    FontFallbackChain() = default;
    
    /** Creates fallback chain with single font.
        
        @param primaryFont  The primary font handle
    */
    explicit FontFallbackChain(FontHandle primaryFont) {
        if (primaryFont != INVALID_FONT_HANDLE) {
            fonts.push_back(primaryFont);
        }
    }
    
    /** Creates fallback chain with two fonts.
        
        @param primaryFont    Primary font (e.g., Arial)
        @param fallbackFont   Fallback font (e.g., PingFang SC)
    */
    FontFallbackChain(FontHandle primaryFont, FontHandle fallbackFont) {
        if (primaryFont != INVALID_FONT_HANDLE) fonts.push_back(primaryFont);
        if (fallbackFont != INVALID_FONT_HANDLE) fonts.push_back(fallbackFont);
    }
    
    /** Creates fallback chain with three fonts.
        
        @param primaryFont    Primary font (e.g., Arial)
        @param fallbackFont1  First fallback (e.g., PingFang SC)
        @param fallbackFont2  Second fallback (e.g., Apple Color Emoji)
    */
    FontFallbackChain(FontHandle primaryFont, FontHandle fallbackFont1, FontHandle fallbackFont2) {
        if (primaryFont != INVALID_FONT_HANDLE) fonts.push_back(primaryFont);
        if (fallbackFont1 != INVALID_FONT_HANDLE) fonts.push_back(fallbackFont1);
        if (fallbackFont2 != INVALID_FONT_HANDLE) fonts.push_back(fallbackFont2);
    }
    
    /** Creates fallback chain with four fonts.
        
        @param primaryFont    Primary font (e.g., Arial)
        @param fallbackFont1  First fallback (e.g., PingFang SC)
        @param fallbackFont2  Second fallback (e.g., Apple Color Emoji)
        @param fallbackFont3  Third fallback (e.g., Segoe UI Symbol)
    */
    FontFallbackChain(FontHandle primaryFont, FontHandle fallbackFont1,
                      FontHandle fallbackFont2, FontHandle fallbackFont3) {
        if (primaryFont != INVALID_FONT_HANDLE) fonts.push_back(primaryFont);
        if (fallbackFont1 != INVALID_FONT_HANDLE) fonts.push_back(fallbackFont1);
        if (fallbackFont2 != INVALID_FONT_HANDLE) fonts.push_back(fallbackFont2);
        if (fallbackFont3 != INVALID_FONT_HANDLE) fonts.push_back(fallbackFont3);
    }
    
    /** Adds a font to the fallback chain.
        
        @param fontHandle  Font to add
        @returns Reference to this chain for method chaining
    */
    FontFallbackChain& addFont(FontHandle fontHandle) {
        if (fontHandle != INVALID_FONT_HANDLE) {
            fonts.push_back(fontHandle);
        }
        return *this;
    }
    
    /** Adds a font to the fallback chain (builder pattern).
        
        @param fontHandle  Font to add
        @returns Reference to this chain for method chaining
    */
    FontFallbackChain& withFont(FontHandle fontHandle) {
        return addFont(fontHandle);
    }
    
    /** Returns true if the chain is empty. */
    bool isEmpty() const {
        return fonts.empty();
    }
    
    /** Returns the number of fonts in the chain. */
    size_t size() const {
        return fonts.size();
    }
    
    /** Returns the primary (first) font in the chain.
        
        @returns Primary font handle, or INVALID_FONT_HANDLE if chain is empty
    */
    FontHandle getPrimary() const {
        return !fonts.empty() ? fonts[0] : INVALID_FONT_HANDLE;
    }
    
    /** Returns font at specified index.
        
        @param index  Index in the chain
        @returns Font handle at index, or INVALID_FONT_HANDLE if out of bounds
    */
    FontHandle getFont(size_t index) const {
        return (index < fonts.size()) ? fonts[index] : INVALID_FONT_HANDLE;
    }
    
    /** Clears all fonts from the chain. */
    void clear() {
        fonts.clear();
    }
    
    /** Returns true if the chain is valid (has at least one font). */
    bool isValid() const {
        return !fonts.empty() && fonts[0] != INVALID_FONT_HANDLE;
    }
};

//==========================================================================================
/**
    Character to font mapping result.
    
    Records which font was selected for rendering a specific character.
    Used internally by the font fallback system.
*/
struct CharFontMapping {
    uint32_t codepoint;         ///< Unicode code point
    FontHandle selectedFont;    ///< Font selected from fallback chain
    size_t byteOffset;          ///< Byte offset in UTF-8 string
    size_t byteLength;          ///< Byte length of this character in UTF-8
    
    CharFontMapping()
        : codepoint(0)
        , selectedFont(INVALID_FONT_HANDLE)
        , byteOffset(0)
        , byteLength(0) {
    }
    
    CharFontMapping(uint32_t cp, FontHandle font, size_t offset, size_t length)
        : codepoint(cp)
        , selectedFont(font)
        , byteOffset(offset)
        , byteLength(length) {
    }
};

//==========================================================================================
/** Vertex for text rendering */
YUCHEN_PACK_BEGIN
struct TextVertex {
    Vec2 position;
    Vec2 texCoord;
    Vec4 color;

    TextVertex() : position(), texCoord(), color() {}
    TextVertex(const Vec2& pos, const Vec2& tex, const Vec4& col)
        : position(pos), texCoord(tex), color(col) {
    }

    bool isValid() const {
        return position.isValid() && texCoord.isValid() && color.isValid();
    }
} YUCHEN_PACKED;
YUCHEN_PACK_END

//==========================================================================================
/** Shaped glyph with position and metadata */
struct ShapedGlyph {
    uint32_t glyphIndex;    ///< Glyph index in font
    Vec2 position;          ///< Position relative to text origin
    float advance;          ///< Horizontal advance
    uint32_t cluster;       ///< Character cluster index
    FontHandle fontHandle;  ///< Font this glyph belongs to

    ShapedGlyph() : glyphIndex(0), position(), advance(0.0f), cluster(0), fontHandle(INVALID_FONT_HANDLE) {}

    bool isValid() const {
        return position.isValid() && std::isfinite(advance) && advance >= 0.0f && fontHandle != INVALID_FONT_HANDLE;
    }
};

/** Result of text shaping */
struct ShapedText {
    std::vector<ShapedGlyph> glyphs;
    float totalAdvance;     ///< Total horizontal extent
    Vec2 totalSize;         ///< Bounding box size

    ShapedText() : glyphs(), totalAdvance(0.0f), totalSize() {}

    void clear() {
        glyphs.clear();
        totalAdvance = 0.0f;
        totalSize = Vec2();
    }

    bool isEmpty() const {
        return glyphs.empty();
    }

    bool isValid() const {
        return totalSize.isValid() && std::isfinite(totalAdvance) && totalAdvance >= 0.0f;
    }
};

/** Text segment with font assignment */
struct TextSegment {
    std::string text;
    FontHandle fontHandle;
    size_t originalStartIndex;
    size_t originalLength;
};

//==========================================================================================
/**
    Cache key for glyph lookup.
    
    Identifies a unique glyph by font, glyph index, size, and boldness strength.
    Each combination of these parameters produces a different cached glyph bitmap.
*/
struct GlyphKey
{
    FontHandle fontHandle;    ///< Font handle
    uint32_t glyphIndex;      ///< Glyph index in font
    uint32_t quantizedSize;   ///< Font size * 64 (26.6 fixed-point)
    uint32_t boldness;        ///< Embolden strength (FT_Pos value)
    
    /**
        Constructs cache key with optional boldness.
        
        @param fh        Font handle
        @param gi        Glyph index
        @param fontSize  Font size in points (will be quantized)
        @param bold      Embolden strength (default: 0 = no boldness)
    */
    GlyphKey(FontHandle fh, uint32_t gi, float fontSize, uint32_t bold = 0)
        : fontHandle(fh)
        , glyphIndex(gi)
        , quantizedSize(static_cast<uint32_t>(fontSize * 64.0f))
        , boldness(bold)
    {
    }
    
    bool operator==(const GlyphKey& other) const
    {
        return fontHandle == other.fontHandle &&
               glyphIndex == other.glyphIndex &&
               quantizedSize == other.quantizedSize &&
               boldness == other.boldness;
    }
};

/** Hash functor for GlyphKey. */
struct GlyphKeyHash
{
    size_t operator()(const GlyphKey& key) const
    {
        size_t hash = std::hash<FontHandle>()(key.fontHandle);
        hash ^= std::hash<uint32_t>()(key.glyphIndex) << 1;
        hash ^= std::hash<uint32_t>()(key.quantizedSize) << 2;
        hash ^= std::hash<uint32_t>()(key.boldness) << 3;
        return hash;
    }
};

/** Cached glyph data */
struct GlyphCacheEntry
{
    Rect textureRect;       ///< Location in atlas texture
    Vec2 bearing;           ///< Glyph bearing
    float advance;          ///< Horizontal advance
    uint32_t lastUsedFrame; ///< Last frame this glyph was used
    bool isValid;           ///< Entry validity flag

    GlyphCacheEntry() : textureRect(), bearing(), advance(0.0f), lastUsedFrame(0), isValid(false) {}

    void markUsed(uint32_t frame)
    {
        lastUsedFrame = frame;
    }

    bool isExpired(uint32_t currentFrame, uint32_t expireFrames) const
    {
        return (currentFrame - lastUsedFrame) > expireFrames;
    }
};

/** Glyph atlas texture */
struct GlyphAtlas
{
    uint32_t width;
    uint32_t height;
    uint32_t currentX;      ///< Current packing position X
    uint32_t currentY;      ///< Current packing position Y
    uint32_t rowHeight;     ///< Current row height for packing
    bool isFull;            ///< Atlas is full, cannot add more glyphs
    void* textureHandle;    ///< Graphics backend texture handle

    GlyphAtlas(uint32_t w, uint32_t h)
        : width(w), height(h), currentX(0), currentY(0), rowHeight(0), isFull(false), textureHandle(nullptr) {
    }

    void reset()
    {
        currentX = 0;
        currentY = 0;
        rowHeight = 0;
        isFull = false;
    }
};

//==========================================================================================
// Rendering types

enum class RenderCommandType {
    Clear = 0,
    FillRect,
    DrawRect,
    DrawText,
    DrawImage,
    DrawLine,
    FillTriangle,
    DrawTriangle,
    FillCircle,
    DrawCircle,
    PushClip,
    PopClip
};

/** Rendering command - describes a single draw operation */
struct RenderCommand
{
    RenderCommandType type;

    Rect rect;
    Vec4 color;
    CornerRadius cornerRadius;
    float borderWidth;

    Vec2 textPosition;
    std::string text;
    std::string resourceNamespace;

    float fontSize;
    Vec4 textColor;
    
    FontFallbackChain fontFallbackChain;
    float letterSpacing;

    void* textureHandle;
    Rect sourceRect;
    ScaleMode scaleMode;
    NineSliceMargins nineSliceMargins;

    Vec2 lineStart;
    Vec2 lineEnd;
    float lineWidth;

    Vec2 triangleP1;
    Vec2 triangleP2;
    Vec2 triangleP3;

    Vec2 circleCenter;
    float circleRadius;

    RenderCommand()
        : type(RenderCommandType::Clear)
        , rect()
        , color()
        , cornerRadius()
        , borderWidth(0.0f)
        , textPosition()
        , text()
        , resourceNamespace()
        , fontSize(11.0f)
        , textColor()
        , fontFallbackChain()
        , letterSpacing(0.0f)
        , textureHandle(nullptr)
        , sourceRect()
        , scaleMode(ScaleMode::Stretch)
        , nineSliceMargins()
        , lineStart()
        , lineEnd()
        , lineWidth(1.0f)
        , triangleP1()
        , triangleP2()
        , triangleP3()
        , circleCenter()
        , circleRadius(0.0f)
    {
    }

    static RenderCommand CreateClear(const Vec4& color)
    {
        RenderCommand cmd;
        cmd.type = RenderCommandType::Clear;
        cmd.color = color;
        return cmd;
    }

    static RenderCommand CreateFillRect(const Rect& rect, const Vec4& color, const CornerRadius& cornerRadius)
    {
        RenderCommand cmd;
        cmd.type = RenderCommandType::FillRect;
        cmd.rect = rect;
        cmd.color = color;
        cmd.cornerRadius = cornerRadius;
        cmd.borderWidth = 0.0f;
        return cmd;
    }

    static RenderCommand CreateDrawRect(const Rect& rect, const Vec4& color, float borderWidth, const CornerRadius& cornerRadius)
    {
        RenderCommand cmd;
        cmd.type = RenderCommandType::DrawRect;
        cmd.rect = rect;
        cmd.color = color;
        cmd.cornerRadius = cornerRadius;
        cmd.borderWidth = borderWidth;
        return cmd;
    }

    static RenderCommand CreateDrawText(const char* text, const Vec2& position,
        const FontFallbackChain& fallbackChain,
        float fontSize, const Vec4& textColor,
        float letterSpacing = 0.0f)
    {
        if (!text || !position.isValid() || fontSize <= 0.0f || !textColor.isValid()) {
            RenderCommand cmd;
            cmd.type = RenderCommandType::Clear;
            return cmd;
        }

        RenderCommand cmd;
        cmd.type = RenderCommandType::DrawText;
        cmd.text = text;
        cmd.textPosition = position;
        cmd.fontFallbackChain = fallbackChain;
        cmd.fontSize = fontSize;
        cmd.textColor = textColor;
        cmd.letterSpacing = letterSpacing;
        
        return cmd;
    }

    static RenderCommand CreateDrawImage(void* textureHandle, const Rect& destRect,
        const Rect& sourceRect, ScaleMode scaleMode,
        const NineSliceMargins& nineSlice = NineSliceMargins())
    {
        if (!textureHandle || !destRect.isValid() || !sourceRect.isValid()) {
            RenderCommand cmd;
            cmd.type = RenderCommandType::Clear;
            return cmd;
        }

        RenderCommand cmd;
        cmd.type = RenderCommandType::DrawImage;
        cmd.textureHandle = textureHandle;
        cmd.rect = destRect;
        cmd.sourceRect = sourceRect;
        cmd.scaleMode = scaleMode;
        cmd.nineSliceMargins = nineSlice;
        return cmd;
    }

    static RenderCommand CreateDrawLine(const Vec2& start, const Vec2& end, const Vec4& color, float width)
    {
        if (!start.isValid() || !end.isValid() || !color.isValid() || width <= 0.0f) {
            RenderCommand cmd;
            cmd.type = RenderCommandType::Clear;
            return cmd;
        }

        RenderCommand cmd;
        cmd.type = RenderCommandType::DrawLine;
        cmd.lineStart = start;
        cmd.lineEnd = end;
        cmd.color = color;
        cmd.lineWidth = width;
        return cmd;
    }

    static RenderCommand CreateFillTriangle(const Vec2& p1, const Vec2& p2, const Vec2& p3, const Vec4& color)
    {
        if (!p1.isValid() || !p2.isValid() || !p3.isValid() || !color.isValid()) {
            RenderCommand cmd;
            cmd.type = RenderCommandType::Clear;
            return cmd;
        }

        RenderCommand cmd;
        cmd.type = RenderCommandType::FillTriangle;
        cmd.triangleP1 = p1;
        cmd.triangleP2 = p2;
        cmd.triangleP3 = p3;
        cmd.color = color;
        cmd.borderWidth = 0.0f;
        return cmd;
    }

    static RenderCommand CreateDrawTriangle(const Vec2& p1, const Vec2& p2, const Vec2& p3, const Vec4& color, float borderWidth)
    {
        if (!p1.isValid() || !p2.isValid() || !p3.isValid() || !color.isValid() || borderWidth <= 0.0f) {
            RenderCommand cmd;
            cmd.type = RenderCommandType::Clear;
            return cmd;
        }

        RenderCommand cmd;
        cmd.type = RenderCommandType::DrawTriangle;
        cmd.triangleP1 = p1;
        cmd.triangleP2 = p2;
        cmd.triangleP3 = p3;
        cmd.color = color;
        cmd.borderWidth = borderWidth;
        return cmd;
    }

    static RenderCommand CreateFillCircle(const Vec2& center, float radius, const Vec4& color)
    {
        if (!center.isValid() || radius <= 0.0f || !color.isValid()) {
            RenderCommand cmd;
            cmd.type = RenderCommandType::Clear;
            return cmd;
        }

        RenderCommand cmd;
        cmd.type = RenderCommandType::FillCircle;
        cmd.circleCenter = center;
        cmd.circleRadius = radius;
        cmd.color = color;
        cmd.borderWidth = 0.0f;
        return cmd;
    }

    static RenderCommand CreateDrawCircle(const Vec2& center, float radius, const Vec4& color, float borderWidth)
    {
        if (!center.isValid() || radius <= 0.0f || !color.isValid() || borderWidth <= 0.0f) {
            RenderCommand cmd;
            cmd.type = RenderCommandType::Clear;
            return cmd;
        }

        RenderCommand cmd;
        cmd.type = RenderCommandType::DrawCircle;
        cmd.circleCenter = center;
        cmd.circleRadius = radius;
        cmd.color = color;
        cmd.borderWidth = borderWidth;
        return cmd;
    }
};

//==========================================================================================
// Miscellaneous types

enum class TextureFormat {
    R8_Unorm,       ///< Single-channel 8-bit normalized (grayscale)
    RGBA8_Unorm     ///< Four-channel 8-bit normalized (color with alpha)
};

enum class WindowType {
    Main,           ///< Main application window
    Dialog,         ///< Modal dialog window
    ToolWindow      ///< Tool palette window
};

} // namespace YuchenUI
