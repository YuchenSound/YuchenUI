#pragma once

#include "YuchenUI/core/Types.h"
#include <vector>

namespace YuchenUI {

class RenderList
{
public:
    RenderList();
    ~RenderList();
    
    void clear(const Vec4& color);
    void fillRect(const Rect& rect, const Vec4& color, const CornerRadius& cornerRadius = CornerRadius());
    void drawRect(const Rect& rect, const Vec4& color, float borderWidth, const CornerRadius& cornerRadius = CornerRadius());
    void drawText(const char* text, const Vec2& position, FontHandle westernFont, FontHandle chineseFont, float fontSize, const Vec4& color);
    void drawImage(const char* resourceIdentifier, const Rect& destRect, ScaleMode scaleMode = ScaleMode::Stretch, const NineSliceMargins& nineSlice = NineSliceMargins());
    
    void drawLine(const Vec2& start, const Vec2& end, const Vec4& color, float width = 1.0f);
    void fillTriangle(const Vec2& p1, const Vec2& p2, const Vec2& p3, const Vec4& color);
    void drawTriangle(const Vec2& p1, const Vec2& p2, const Vec2& p3, const Vec4& color, float borderWidth = 1.0f);
    void fillCircle(const Vec2& center, float radius, const Vec4& color);
    void drawCircle(const Vec2& center, float radius, const Vec4& color, float borderWidth = 1.0f);

    void pushClipRect(const Rect& rect);
    void popClipRect();
    
    void reset();
    bool isEmpty() const;
    size_t getCommandCount() const;
    const std::vector<RenderCommand>& getCommands() const;
    bool validate() const;
    
private:
    void addCommand(const RenderCommand& cmd);
    void validateCommand(const RenderCommand& cmd) const;
    
    std::vector<RenderCommand> m_commands;
    std::vector<Rect> m_clipStack;
};

}
