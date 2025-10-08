#pragma once

#include "YuchenUI/widgets/UIComponent.h"
#include "YuchenUI/windows/IWindowContent.h"
#include "YuchenUI/core/Types.h"
#include <vector>
#include <memory>

namespace YuchenUI {

class RenderList;

class Widget : public UIComponent {
public:
    explicit Widget(const Rect& bounds);
    virtual ~Widget();
    
    template<typename T, typename... Args>
    T* addChild(Args&&... args) {
        auto child = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = child.get();
        ptr->setOwnerWindow(m_ownerWindow);
        ptr->setOwnerContent(m_ownerContent);
        ptr->setParent(this);
        
        if (m_ownerContent && ptr->canReceiveFocus()) {
            m_ownerContent->registerFocusableComponent(ptr);
        }
        
        m_children.push_back(std::move(child));
        return ptr;
    }
    
    void removeChild(UIComponent* child);
    void clearChildren();
    size_t getChildCount() const;
    
    void setBounds(const Rect& bounds);
    const Rect& getBounds() const override { return m_bounds; }
    
    void setPadding(float padding);
    void setPadding(float left, float top, float right, float bottom);
    float getPaddingLeft() const { return m_paddingLeft; }
    float getPaddingTop() const { return m_paddingTop; }
    float getPaddingRight() const { return m_paddingRight; }
    float getPaddingBottom() const { return m_paddingBottom; }
    
    virtual void addDrawCommands(RenderList& commandList, const Vec2& offset = Vec2()) const override = 0;
    bool handleMouseMove(const Vec2& position, const Vec2& offset = Vec2()) override;
    bool handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset = Vec2()) override;
    bool handleMouseWheel(const Vec2& delta, const Vec2& position, const Vec2& offset = Vec2()) override;
    void update(float deltaTime) override;
    
    void setOwnerContent(IWindowContent* content) override;
    void setOwnerWindow(BaseWindow* window) override;

protected:
    Rect getContentRect() const;
    void renderChildren(RenderList& commandList, const Vec2& offset) const;
    bool dispatchMouseEvent(const Vec2& position, bool pressed, const Vec2& offset, bool isMove);
    
    std::vector<std::unique_ptr<UIComponent>> m_children;
    Rect m_bounds;
    float m_paddingLeft;
    float m_paddingTop;
    float m_paddingRight;
    float m_paddingBottom;
};

}
