#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/core/Validation.h"
#include "YuchenUI/core/Config.h"
#include <cstring>

namespace YuchenUI {

RenderList::RenderList()
{
    m_commands.reserve(1000);
}

RenderList::~RenderList()
{
    reset();
}

void RenderList::clear(const Vec4& color)
{
    Validation::AssertColor(color);
    
    RenderCommand cmd = RenderCommand::CreateClear(color);
    addCommand(cmd);
}

void RenderList::fillRect(const Rect& rect, const Vec4& color, const CornerRadius& cornerRadius)
{
    Validation::AssertRect(rect);
    Validation::AssertColor(color);
    Validation::AssertCornerRadiusForRect(cornerRadius, rect);
    
    RenderCommand cmd = RenderCommand::CreateFillRect(rect, color, cornerRadius);
    addCommand(cmd);
}

void RenderList::drawRect(const Rect& rect, const Vec4& color, float borderWidth, const CornerRadius& cornerRadius)
{
    Validation::AssertRect(rect);
    Validation::AssertColor(color);
    Validation::AssertBorderWidth(borderWidth, rect);
    Validation::AssertCornerRadiusForRect(cornerRadius, rect);
    
    RenderCommand cmd = RenderCommand::CreateDrawRect(rect, color, borderWidth, cornerRadius);
    addCommand(cmd);
}

void RenderList::drawText(const char* text, const Vec2& position, FontHandle westernFont, FontHandle chineseFont, float fontSize, const Vec4& color)
{
    YUCHEN_ASSERT(text);
    YUCHEN_ASSERT(position.isValid());
    YUCHEN_ASSERT(fontSize > 0.0f);
    YUCHEN_ASSERT(color.isValid());
    
    size_t textLength[[maybe_unused]] = strlen(text);
    YUCHEN_ASSERT(textLength > 0 && textLength <= Config::Text::MAX_LENGTH);
    
    RenderCommand cmd = RenderCommand::CreateDrawText(text, position, westernFont, chineseFont, fontSize, color);
    addCommand(cmd);
}

void RenderList::drawImage(const char* resourceIdentifier, const Rect& destRect, ScaleMode scaleMode, const NineSliceMargins& nineSlice)
{
    YUCHEN_ASSERT(resourceIdentifier);
    YUCHEN_ASSERT(destRect.isValid());
    
    RenderCommand cmd;
    cmd.type = RenderCommandType::DrawImage;
    cmd.rect = destRect;
    cmd.text = resourceIdentifier;
    cmd.scaleMode = scaleMode;
    cmd.sourceRect = Rect();
    cmd.textureHandle = nullptr;
    cmd.nineSliceMargins = nineSlice;
    
    validateCommand(cmd);
    addCommand(cmd);
}

void RenderList::drawLine(const Vec2& start, const Vec2& end, const Vec4& color, float width)
{
    YUCHEN_ASSERT(start.isValid());
    YUCHEN_ASSERT(end.isValid());
    Validation::AssertColor(color);
    YUCHEN_ASSERT(width > 0.0f);
    
    RenderCommand cmd = RenderCommand::CreateDrawLine(start, end, color, width);
    validateCommand(cmd);
    addCommand(cmd);
}

void RenderList::fillTriangle(const Vec2& p1, const Vec2& p2, const Vec2& p3, const Vec4& color)
{
    YUCHEN_ASSERT(p1.isValid());
    YUCHEN_ASSERT(p2.isValid());
    YUCHEN_ASSERT(p3.isValid());
    Validation::AssertColor(color);
    
    RenderCommand cmd = RenderCommand::CreateFillTriangle(p1, p2, p3, color);
    validateCommand(cmd);
    addCommand(cmd);
}

void RenderList::drawTriangle(const Vec2& p1, const Vec2& p2, const Vec2& p3, const Vec4& color, float borderWidth)
{
    YUCHEN_ASSERT(p1.isValid());
    YUCHEN_ASSERT(p2.isValid());
    YUCHEN_ASSERT(p3.isValid());
    Validation::AssertColor(color);
    YUCHEN_ASSERT(borderWidth > 0.0f);
    
    RenderCommand cmd = RenderCommand::CreateDrawTriangle(p1, p2, p3, color, borderWidth);
    validateCommand(cmd);
    addCommand(cmd);
}

void RenderList::fillCircle(const Vec2& center, float radius, const Vec4& color)
{
    YUCHEN_ASSERT(center.isValid());
    YUCHEN_ASSERT(radius > 0.0f);
    Validation::AssertColor(color);
    
    RenderCommand cmd = RenderCommand::CreateFillCircle(center, radius, color);
    validateCommand(cmd);
    addCommand(cmd);
}

void RenderList::drawCircle(const Vec2& center, float radius, const Vec4& color, float borderWidth)
{
    YUCHEN_ASSERT(center.isValid());
    YUCHEN_ASSERT(radius > 0.0f);
    Validation::AssertColor(color);
    YUCHEN_ASSERT(borderWidth > 0.0f);
    
    RenderCommand cmd = RenderCommand::CreateDrawCircle(center, radius, color, borderWidth);
    validateCommand(cmd);
    addCommand(cmd);
}

void RenderList::reset() {
    m_commands.clear();
    m_clipStack.clear();
}

bool RenderList::isEmpty() const
{
    return m_commands.empty();
}

size_t RenderList::getCommandCount() const
{
    return m_commands.size();
}

const std::vector<RenderCommand>& RenderList::getCommands() const
{
    return m_commands;
}

bool RenderList::validate() const
{
    YUCHEN_ASSERT(m_commands.size() <= Config::Rendering::MAX_COMMANDS_PER_LIST);

    for (const auto& cmd : m_commands)
    {
        switch (cmd.type) {
            case RenderCommandType::Clear:
                YUCHEN_ASSERT(Validation::ValidateColor(cmd.color));
                break;
                
            case RenderCommandType::FillRect:
            case RenderCommandType::DrawRect:
                YUCHEN_ASSERT(Validation::ValidateRect(cmd.rect));
                YUCHEN_ASSERT(Validation::ValidateColor(cmd.color));
                YUCHEN_ASSERT(Validation::ValidateCornerRadius(cmd.cornerRadius));
                YUCHEN_ASSERT(Validation::ValidateBorderWidth(cmd.borderWidth, cmd.rect));
                break;
                
            case RenderCommandType::DrawText:
                YUCHEN_ASSERT(!cmd.text.empty());
                YUCHEN_ASSERT(cmd.textPosition.isValid());
                YUCHEN_ASSERT(cmd.fontSize > 0.0f);
                YUCHEN_ASSERT(Validation::ValidateColor(cmd.textColor));
                YUCHEN_ASSERT(cmd.westernFont != INVALID_FONT_HANDLE);
                YUCHEN_ASSERT(cmd.chineseFont != INVALID_FONT_HANDLE);
                YUCHEN_ASSERT(cmd.text.length() <= Config::Text::MAX_LENGTH);
                break;
                
            case RenderCommandType::DrawImage:
                YUCHEN_ASSERT(!cmd.text.empty());
                YUCHEN_ASSERT(Validation::ValidateRect(cmd.rect));
                if (cmd.scaleMode == ScaleMode::NineSlice) {
                    YUCHEN_ASSERT(cmd.nineSliceMargins.isValid());
                }
                break;
                
            case RenderCommandType::DrawLine:
                YUCHEN_ASSERT(cmd.lineStart.isValid());
                YUCHEN_ASSERT(cmd.lineEnd.isValid());
                YUCHEN_ASSERT(Validation::ValidateColor(cmd.color));
                YUCHEN_ASSERT(cmd.lineWidth > 0.0f);
                break;
                
            case RenderCommandType::FillTriangle:
            case RenderCommandType::DrawTriangle:
                YUCHEN_ASSERT(cmd.triangleP1.isValid());
                YUCHEN_ASSERT(cmd.triangleP2.isValid());
                YUCHEN_ASSERT(cmd.triangleP3.isValid());
                YUCHEN_ASSERT(Validation::ValidateColor(cmd.color));
                if (cmd.type == RenderCommandType::DrawTriangle) {
                    YUCHEN_ASSERT(cmd.borderWidth > 0.0f);
                }
                break;
                
            case RenderCommandType::FillCircle:
            case RenderCommandType::DrawCircle:
                YUCHEN_ASSERT(cmd.circleCenter.isValid());
                YUCHEN_ASSERT(cmd.circleRadius > 0.0f);
                YUCHEN_ASSERT(Validation::ValidateColor(cmd.color));
                if (cmd.type == RenderCommandType::DrawCircle) {
                    YUCHEN_ASSERT(cmd.borderWidth > 0.0f);
                }
                break;
                
            default:
                YUCHEN_UNREACHABLE();
        }
    }
    
    return true;
}

void RenderList::addCommand(const RenderCommand& cmd)
{
    YUCHEN_ASSERT(m_commands.size() < Config::Rendering::MAX_COMMANDS_PER_LIST);
    m_commands.push_back(cmd);
}

void RenderList::validateCommand(const RenderCommand& cmd) const
{
    switch (cmd.type)
    {
        case RenderCommandType::Clear:
            Validation::AssertColor(cmd.color);
            break;
            
        case RenderCommandType::FillRect:
        case RenderCommandType::DrawRect:
            Validation::AssertRect(cmd.rect);
            Validation::AssertColor(cmd.color);
            Validation::AssertCornerRadius(cmd.cornerRadius);
            Validation::AssertBorderWidth(cmd.borderWidth, cmd.rect);
            break;
            
        case RenderCommandType::DrawText:
            YUCHEN_ASSERT(!cmd.text.empty());
            YUCHEN_ASSERT(cmd.textPosition.isValid());
            YUCHEN_ASSERT(cmd.fontSize > 0.0f);
            YUCHEN_ASSERT(Validation::ValidateColor(cmd.textColor));
            YUCHEN_ASSERT(cmd.westernFont != INVALID_FONT_HANDLE);
            YUCHEN_ASSERT(cmd.chineseFont != INVALID_FONT_HANDLE);
            YUCHEN_ASSERT(cmd.text.length() <= Config::Text::MAX_LENGTH);
            break;
            
        case RenderCommandType::DrawImage:
            YUCHEN_ASSERT(!cmd.text.empty());
            YUCHEN_ASSERT(Validation::ValidateRect(cmd.rect));
            if (cmd.scaleMode == ScaleMode::NineSlice) {
                YUCHEN_ASSERT(cmd.nineSliceMargins.isValid());
            }
            break;
            
        case RenderCommandType::DrawLine:
            YUCHEN_ASSERT(cmd.lineStart.isValid());
            YUCHEN_ASSERT(cmd.lineEnd.isValid());
            YUCHEN_ASSERT(Validation::ValidateColor(cmd.color));
            YUCHEN_ASSERT(cmd.lineWidth > 0.0f);
            break;
            
        case RenderCommandType::FillTriangle:
        case RenderCommandType::DrawTriangle:
            YUCHEN_ASSERT(cmd.triangleP1.isValid());
            YUCHEN_ASSERT(cmd.triangleP2.isValid());
            YUCHEN_ASSERT(cmd.triangleP3.isValid());
            YUCHEN_ASSERT(Validation::ValidateColor(cmd.color));
            if (cmd.type == RenderCommandType::DrawTriangle) {
                YUCHEN_ASSERT(cmd.borderWidth > 0.0f);
            }
            break;
            
        case RenderCommandType::FillCircle:
        case RenderCommandType::DrawCircle:
            YUCHEN_ASSERT(cmd.circleCenter.isValid());
            YUCHEN_ASSERT(cmd.circleRadius > 0.0f);
            YUCHEN_ASSERT(Validation::ValidateColor(cmd.color));
            if (cmd.type == RenderCommandType::DrawCircle) {
                YUCHEN_ASSERT(cmd.borderWidth > 0.0f);
            }
            break;
            
        default:
            YUCHEN_UNREACHABLE();
    }
}

void RenderList::pushClipRect(const Rect& rect) {
    YUCHEN_ASSERT(rect.isValid());
    m_clipStack.push_back(rect);
    
    RenderCommand cmd;
    cmd.type = RenderCommandType::PushClip;
    cmd.rect = rect;
    addCommand(cmd);
}

void RenderList::popClipRect() {
    YUCHEN_ASSERT(!m_clipStack.empty());
    m_clipStack.pop_back();
    
    RenderCommand cmd;
    cmd.type = RenderCommandType::PopClip;
    addCommand(cmd);
}

}
