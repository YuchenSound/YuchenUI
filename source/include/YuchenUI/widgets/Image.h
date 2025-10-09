#pragma once

#include "YuchenUI/widgets/UIComponent.h"
#include "YuchenUI/core/Types.h"
#include <string>

namespace YuchenUI {

class RenderList;

class Image : public UIComponent {
public:
    explicit Image(const Rect& bounds);
    virtual ~Image();
    
    void addDrawCommands(RenderList& commandList, const Vec2& offset = Vec2()) const override;
    bool handleMouseMove(const Vec2& position, const Vec2& offset = Vec2()) override { return false; }
    bool handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset = Vec2()) override { return false; }
    
    void setResource(const char* resourceIdentifier);
    const std::string& getResource() const { return m_resourceIdentifier; }
    
    void setScaleMode(ScaleMode mode);
    ScaleMode getScaleMode() const { return m_scaleMode; }
    
    void setNineSliceMargins(float left, float top, float right, float bottom);
    void setNineSliceMargins(const NineSliceMargins& margins);
    const NineSliceMargins& getNineSliceMargins() const { return m_nineSliceMargins; }
    
    void setBounds(const Rect& bounds);
    const Rect& getBounds() const override { return m_bounds; }
    
    bool isValid() const;

private:
    std::string m_resourceIdentifier;
    Rect m_bounds;
    ScaleMode m_scaleMode;
    NineSliceMargins m_nineSliceMargins;
};

}
