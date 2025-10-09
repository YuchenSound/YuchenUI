/****************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Widgets module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
****************************************************************************/

#pragma once

#include "YuchenUI/widgets/UIComponent.h"
#include "YuchenUI/windows/IWindowContent.h"
#include "YuchenUI/core/Types.h"
#include <vector>
#include <memory>

namespace YuchenUI {

class RenderList;

//==========================================================================================
/**
    A container widget that can hold and manage child UI components.
    
    The Widget class provides layout capabilities through padding and handles
    event propagation to its children in reverse order (top-most first). This
    ensures that overlapping UI elements receive input events correctly.
    
    Example usage:
    @code
    auto* panel = new Panel(Rect(0, 0, 800, 600));
    auto* button = panel->addChild<Button>(Rect(10, 10, 100, 30));
    panel->setPadding(10.0f);
    @endcode
    
    @see UIComponent, Panel, Container
*/
class Widget : public UIComponent
{
public:
    //======================================================================================
    /** Creates a Widget with the specified bounds.
        
        @param bounds  The initial rectangle defining position and size
    */
    explicit Widget (const Rect& bounds);
    
    /** Destructor. Releases all child components and unregisters them from focus system. */
    virtual ~Widget();
    
    //======================================================================================
    /** Creates and adds a child component to this widget.
        
        This template method constructs a component of type T and adds it to the widget's
        child collection. The order of internal operations is critical for proper focus
        management:
        
        1. Create child and set parent relationships
        2. Add to internal children container
        3. Set owner content (triggers focus manager registration)
        4. Register with focus manager if component is focusable
        
        This sequence ensures the child can successfully call requestFocus() immediately
        after being added.
        
        @tparam T      Component type to create (must inherit from UIComponent)
        @param args    Constructor arguments forwarded to the component
        
        @returns A pointer to the created component. The Widget retains ownership.
        
        @see removeChild, clearChildren
    */
    template<typename T, typename... Args>
    T* addChild (Args&&... args)
    {
        auto child = std::make_unique<T> (std::forward<Args>(args)...);
        T* ptr = child.get();
        ptr->setOwnerWindow (m_ownerWindow);
        ptr->setParent (this);
        
        m_children.push_back (std::move (child));
        
        ptr->setOwnerContent (m_ownerContent);
        
        if (m_ownerContent)
        {
            m_ownerContent->setFocusManagerAccessor (ptr);
            if (ptr->getFocusPolicy() != FocusPolicy::NoFocus)
                m_ownerContent->registerFocusableComponent (ptr);
        }
        
        return ptr;
    }
    
    /** Removes the specified child from this widget.
        
        The child is unregistered from the focus system before removal.
        
        @param child  Pointer to the child component to remove
    */
    void removeChild (UIComponent* child);
    
    /** Removes all children from this widget.
        
        All children are unregistered from the focus system before clearing.
    */
    void clearChildren();
    
    /** Returns the number of child components currently managed by this widget. */
    size_t getChildCount() const;
    
    //======================================================================================
    /** Sets the bounds of this widget.
        
        @param bounds  The new rectangle defining position and size
    */
    void setBounds (const Rect& bounds);
    
    /** Returns the current bounds of this widget. */
    const Rect& getBounds() const override { return m_bounds; }
    
    //======================================================================================
    /** Sets uniform padding on all sides.
        
        @param padding  The padding value to apply to left, top, right, and bottom
    */
    void setPadding (float padding);
    
    /** Sets individual padding for each side.
        
        @param left    Left padding
        @param top     Top padding
        @param right   Right padding
        @param bottom  Bottom padding
    */
    void setPadding (float left, float top, float right, float bottom);
    
    /** Returns the left padding value. */
    float getPaddingLeft() const { return m_paddingLeft; }
    
    /** Returns the top padding value. */
    float getPaddingTop() const { return m_paddingTop; }
    
    /** Returns the right padding value. */
    float getPaddingRight() const { return m_paddingRight; }
    
    /** Returns the bottom padding value. */
    float getPaddingBottom() const { return m_paddingBottom; }
    
    //======================================================================================
    virtual void addDrawCommands (RenderList& commandList, const Vec2& offset = Vec2()) const override = 0;
    bool handleMouseMove (const Vec2& position, const Vec2& offset = Vec2()) override;
    bool handleMouseClick (const Vec2& position, bool pressed, const Vec2& offset = Vec2()) override;
    bool handleMouseWheel (const Vec2& delta, const Vec2& position, const Vec2& offset = Vec2()) override;
    void update (float deltaTime) override;
    
    void setOwnerContent (IWindowContent* content) override;
    void setOwnerWindow (BaseWindow* window) override;

protected:
    //======================================================================================
    /** Returns the content area after subtracting padding from bounds.
        
        This area represents the space available for child component placement.
        Subclasses use this to determine layout constraints.
        
        @returns Content rectangle with padding applied
    */
    Rect getContentRect() const;
    
    /** Renders all visible children in order.
        
        This method should be called by subclass implementations of addDrawCommands()
        to ensure child components are rendered properly.
        
        @param commandList  The render command list to populate
        @param offset       Parent widget offset for coordinate transformation
    */
    void renderChildren (RenderList& commandList, const Vec2& offset) const;
    
    /** Dispatches mouse events to children in reverse order.
        
        Children are tested from top-most (last added) to bottom-most. Event
        propagation stops when a child handles the event.
        
        @param position  Mouse position in window coordinates
        @param pressed   Mouse button state (for click events)
        @param offset    Parent widget offset for coordinate transformation
        @param isMove    True for mouse move events, false for click events
        
        @returns True if any child handled the event, false otherwise
    */
    bool dispatchMouseEvent (const Vec2& position, bool pressed,
                            const Vec2& offset, bool isMove);
    
    //======================================================================================
    std::vector<std::unique_ptr<UIComponent>> m_children;
    Rect m_bounds;
    float m_paddingLeft;
    float m_paddingTop;
    float m_paddingRight;
    float m_paddingBottom;
};

} // namespace YuchenUI
