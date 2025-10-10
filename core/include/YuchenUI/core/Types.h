#pragma once

#include "YuchenUI/core/Assert.h"
#include <cmath>
#include <vector>
#include <string>

// Platform-specific packing macros
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

    struct Vec2
    {
        float x, y;

        Vec2() : x(0.0f), y(0.0f) {}
        Vec2(float x, float y) : x(x), y(y) {}

        bool isValid() const
        {
            return std::isfinite(x) && std::isfinite(y);
        }
    };

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
    };

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
    };

    struct CornerRadius
    {
        float topLeft, topRight, bottomLeft, bottomRight;

        CornerRadius() : topLeft(0.0f), topRight(0.0f), bottomLeft(0.0f), bottomRight(0.0f) {}

        CornerRadius(float radius) : topLeft(radius), topRight(radius), bottomLeft(radius), bottomRight(radius)
        {
            if (radius < 0.0f) {
                topLeft = topRight = bottomLeft = bottomRight = 0.0f;
            }
        }

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
        Original,
        Stretch,
        Fill,
        NineSlice
    };

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


        typedef size_t FontHandle;
    const FontHandle INVALID_FONT_HANDLE = SIZE_MAX;

    struct FontMetrics {
        float ascender;
        float descender;
        float lineHeight;
        float maxAdvance;

        FontMetrics() : ascender(0.0f), descender(0.0f), lineHeight(0.0f), maxAdvance(0.0f) {}

        bool isValid() const {
            return std::isfinite(ascender) && std::isfinite(descender) &&
                std::isfinite(lineHeight) && std::isfinite(maxAdvance) &&
                lineHeight > 0.0f;
        }
    };

    struct GlyphMetrics {
        uint32_t glyphIndex;
        Vec2 bearing;
        Vec2 size;
        float advance;

        GlyphMetrics() : glyphIndex(0), bearing(), size(), advance(0.0f) {}

        bool isValid() const {
            return bearing.isValid() && size.isValid() &&
                std::isfinite(advance) && advance >= 0.0f;
        }
    };

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

        struct ShapedGlyph {
        uint32_t glyphIndex;
        Vec2 position;
        float advance;
        uint32_t cluster;
        FontHandle fontHandle;

        ShapedGlyph() : glyphIndex(0), position(), advance(0.0f), cluster(0), fontHandle(INVALID_FONT_HANDLE) {}

        bool isValid() const {
            return position.isValid() && std::isfinite(advance) && advance >= 0.0f && fontHandle != INVALID_FONT_HANDLE;
        }
    };

    struct ShapedText {
        std::vector<ShapedGlyph> glyphs;
        float totalAdvance;
        Vec2 totalSize;

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

    struct TextSegment {
        std::string text;
        FontHandle fontHandle;
        size_t originalStartIndex;
        size_t originalLength;
    };

    struct GlyphKey
    {
        FontHandle fontHandle;
        uint32_t glyphIndex;
        uint32_t fontSize;

        GlyphKey(FontHandle font, uint32_t glyph, float size)
            : fontHandle(font), glyphIndex(glyph), fontSize(static_cast<uint32_t>(size * 64.0f)) {
        }

        bool operator==(const GlyphKey& other) const
        {
            return fontHandle == other.fontHandle &&
                glyphIndex == other.glyphIndex &&
                fontSize == other.fontSize;
        }
    };

    struct GlyphKeyHash
    {
        size_t operator()(const GlyphKey& key) const
        {
            return ((size_t)key.fontHandle << 32) | ((size_t)key.glyphIndex << 16) | key.fontSize;
        }
    };

    struct GlyphCacheEntry
    {
        Rect textureRect;
        Vec2 bearing;
        float advance;
        uint32_t lastUsedFrame;
        bool isValid;

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

    struct GlyphAtlas
    {
        uint32_t width;
        uint32_t height;
        uint32_t currentX;
        uint32_t currentY;
        uint32_t rowHeight;
        bool isFull;
        void* textureHandle;

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

    struct RenderCommand
    {
        RenderCommandType type;

        Rect rect;
        Vec4 color;
        CornerRadius cornerRadius;
        float borderWidth;

        Vec2 textPosition;
        std::string text;
        FontHandle westernFont;
        FontHandle chineseFont;
        float fontSize;
        Vec4 textColor;

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
            , westernFont(INVALID_FONT_HANDLE)
            , chineseFont(INVALID_FONT_HANDLE)
            , fontSize(11.0f)
            , textColor()
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
            FontHandle westernFont, FontHandle chineseFont,
            float fontSize, const Vec4& textColor)
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
            cmd.westernFont = westernFont;
            cmd.chineseFont = chineseFont;
            cmd.fontSize = fontSize;
            cmd.textColor = textColor;
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

    enum class TextureFormat {
        R8_Unorm,
        RGBA8_Unorm
    };

    enum class WindowType {
        Main,
        Dialog,
        ToolWindow
    };
}
