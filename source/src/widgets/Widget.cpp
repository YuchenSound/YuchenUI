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

//==========================================================================================
/** @file Widget.cpp
    
    Implementation notes:
    - Children stored in vector for fast iteration
    - Events propagate in reverse order (top-most child receives events first)
    - Focus manager accessor must be set before component registration
    - All operations are main-thread only
*/
//==========================================================================================

#include "YuchenUI/widgets/Widget.h"
#include "YuchenUI/windows/IWindowContent.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/focus/FocusManager.h"
#include "YuchenUI/core/Validation.h"
#include "YuchenUI/core/Assert.h"
#include <algorithm>

namespace YuchenUI {

//==========================================================================================
// Construction and Destruction

Widget::Widget (const Rect& bounds)
    : m_children()
    , m_bounds(bounds)
    , m_paddingLeft(0.0f)
    , m_paddingTop(0.0f)
    , m_paddingRight(0.0f)
    , m_paddingBottom(0.0f)
{
    Validation::AssertRect (bounds);
}

Widget::~Widget()
{
    clearChildren();
}

//==========================================================================================
// Child Management

void Widget::removeChild (UIComponent* child)
{
    YUCHEN_ASSERT (child);
    
    // Unregister from focus manager first to prevent dangling pointers
    if (m_ownerContent && child->getFocusPolicy() != FocusPolicy::NoFocus)
    {
        m_ownerContent->unregisterFocusableComponent (child);
    }
    
    auto it = std::find_if (m_children.begin(), m_children.end(),
        [child](const std::unique_ptr<UIComponent>& ptr) {
            return ptr.get() == child;
        });
    
    if (it != m_children.end())
        m_children.erase (it);
}

void Widget::clearChildren()
{
    if (m_ownerContent)
    {
        for (auto& child : m_children)
        {
            if (child && child->getFocusPolicy() != FocusPolicy::NoFocus)
            {
                m_ownerContent->unregisterFocusableComponent (child.get());
            }
        }
    }
    m_children.clear();
}

size_t Widget::getChildCount() const
{
    return m_children.size();
}

//==========================================================================================
// Layout and Geometry

void Widget::setBounds (const Rect& bounds)
{
    Validation::AssertRect (bounds);
    m_bounds = bounds;
}

void Widget::setPadding (float padding)
{
    YUCHEN_ASSERT (padding >= 0.0f);
    m_paddingLeft = m_paddingTop = m_paddingRight = m_paddingBottom = padding;
}

void Widget::setPadding (float left, float top, float right, float bottom)
{
    YUCHEN_ASSERT (left >= 0.0f && top >= 0.0f && right >= 0.0f && bottom >= 0.0f);
    m_paddingLeft = left;
    m_paddingTop = top;
    m_paddingRight = right;
    m_paddingBottom = bottom;
}

Rect Widget::getContentRect() const
{
    return Rect (
        m_paddingLeft,
        m_paddingTop,
        std::max (0.0f, m_bounds.width - m_paddingLeft - m_paddingRight),
        std::max (0.0f, m_bounds.height - m_paddingTop - m_paddingBottom)
    );
}

//==========================================================================================
// Rendering

void Widget::renderChildren (RenderList& commandList, const Vec2& offset) const
{
    for (const auto& child : m_children)
    {
        if (child && child->isVisible())
            child->addDrawCommands (commandList, offset);
    }
}

//==========================================================================================
// Event Handling

bool Widget::handleMouseMove (const Vec2& position, const Vec2& offset)
{
    return dispatchMouseEvent (position, false, offset, true);
}

bool Widget::handleMouseClick (const Vec2& position, bool pressed, const Vec2& offset)
{
    return dispatchMouseEvent (position, pressed, offset, false);
}

bool Widget::handleMouseWheel (const Vec2& delta, const Vec2& position, const Vec2& offset)
{
    if (!m_isEnabled || !m_isVisible)
        return false;
    
    Vec2 absPos (m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect absRect (absPos.x, absPos.y, m_bounds.width, m_bounds.height);
    
    if (!absRect.contains (position))
        return false;
    
    // Propagate to children in reverse order (top-most first)
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it)
    {
        if ((*it) && (*it)->isVisible() && (*it)->isEnabled())
        {
            if ((*it)->handleMouseWheel (delta, position, absPos))
                return true;
        }
    }
    return false;
}

bool Widget::dispatchMouseEvent (const Vec2& position, bool pressed, const Vec2& offset, bool isMove)
{
    if (!m_isEnabled || !m_isVisible)
        return false;
    
    Vec2 absPos (m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect absRect (absPos.x, absPos.y, m_bounds.width, m_bounds.height);
    
    if (!absRect.contains (position))
        return false;
    
    // Dispatch to children in reverse order - last added (top-most) gets first chance
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it)
    {
        if (!(*it) || !(*it)->isVisible())
            continue;
        
        bool handled = isMove
            ? (*it)->handleMouseMove (position, absPos)
            : (*it)->handleMouseClick (position, pressed, absPos);
        
        if (handled)
            return true;
    }
    
    return false;
}

void Widget::update (float deltaTime)
{
    for (auto& child : m_children)
    {
        if (child && child->isVisible())
            child->update (deltaTime);
    }
}

//==========================================================================================
// Ownership Management

void Widget::setOwnerContent (IWindowContent* content)
{
    UIComponent::setOwnerContent (content);

    if (!content)
        return;
    
    // Propagate to all children and register with focus system
    for (auto& child : m_children)
    {
        if (child)
        {
            child->setOwnerContent (content);

            // CRITICAL: Set accessor before registration so requestFocus() works immediately
            content->setFocusManagerAccessor (child.get());
            
            if (child->getFocusPolicy() != FocusPolicy::NoFocus)
            {
                content->registerFocusableComponent (child.get());
            }
        }
    }
}

void Widget::setOwnerWindow (BaseWindow* window)
{
    UIComponent::setOwnerWindow (window);
    
    for (auto& child : m_children)
    {
        if (child)
            child->setOwnerWindow (window);
    }
}

} // namespace YuchenUI
