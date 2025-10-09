#pragma once

#include "YuchenUI/widgets/Widget.h"
#include "YuchenUI/core/Types.h"
#include <string>

namespace YuchenUI {

class RenderList;

class GroupBox : public Widget {
public:
    explicit GroupBox(const Rect& bounds);
    virtual ~GroupBox();
    
    void addDrawCommands(RenderList& commandList, const Vec2& offset = Vec2()) const override;
    
    // 重写鼠标事件处理，应用标题栏偏移
    bool handleMouseMove(const Vec2& position, const Vec2& offset) override;
    bool handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset) override;
    bool handleMouseWheel(const Vec2& delta, const Vec2& position, const Vec2& offset) override;
    
    void setTitle(const std::string& title);
    void setTitle(const char* title);
    const std::string& getTitle() const { return m_title; }
    
    void setTitleFont(FontHandle fontHandle);
    FontHandle getTitleFont() const;
    void resetTitleFont();
    
    void setTitleFontSize(float fontSize);
    float getTitleFontSize() const { return m_titleFontSize; }
    
    void setTitleColor(const Vec4& color);
    Vec4 getTitleColor() const;
    void resetTitleColor();
    
    void setBackgroundColor(const Vec4& color);
    Vec4 getBackgroundColor() const;
    void resetBackgroundColor();
    
    void setBorderColor(const Vec4& color);
    Vec4 getBorderColor() const;
    void resetBorderColor();
    
    void setBorderWidth(float width);
    float getBorderWidth() const { return m_borderWidth; }
    
    void setCornerRadius(const CornerRadius& radius);
    void setCornerRadius(float radius);
    const CornerRadius& getCornerRadius() const { return m_cornerRadius; }
    
    bool isValid() const;

private:
    std::string m_title;
    FontHandle m_titleFont;
    float m_titleFontSize;
    Vec4 m_titleColor;
    Vec4 m_backgroundColor;
    Vec4 m_borderColor;
    float m_borderWidth;
    CornerRadius m_cornerRadius;
    
    // 标记是否为用户自定义
    bool m_hasCustomTitleFont;
    bool m_hasCustomTitleColor;
    bool m_hasCustomBackground;
    bool m_hasCustomBorderColor;
};

}
