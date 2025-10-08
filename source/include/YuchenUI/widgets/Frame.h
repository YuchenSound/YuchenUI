#pragma once

#include "YuchenUI/widgets/Widget.h"
#include "YuchenUI/core/Types.h"

namespace YuchenUI {

class RenderList;

class Frame : public Widget {
public:
    explicit Frame(const Rect& bounds);
    virtual ~Frame();
    
    void addDrawCommands(RenderList& commandList, const Vec2& offset = Vec2()) const override;
    
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
    Vec4 m_backgroundColor;
    Vec4 m_borderColor;
    float m_borderWidth;
    CornerRadius m_cornerRadius;
    
    // 标记是否为用户自定义
    bool m_hasCustomBackground;
    bool m_hasCustomBorderColor;
};

}
