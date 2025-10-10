#include "YuchenUI/widgets/Widget.h"
#include "YuchenUI/core/IUIContent.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/focus/FocusManager.h"
#include "YuchenUI/core/Validation.h"
#include "YuchenUI/core/Assert.h"
#include <algorithm>

namespace YuchenUI {

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

void Widget::removeChild (UIComponent* child)
{
    YUCHEN_ASSERT (child);
    
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

void Widget::renderChildren (RenderList& commandList, const Vec2& offset) const
{
    for (const auto& child : m_children)
    {
        if (child && child->isVisible())
            child->addDrawCommands (commandList, offset);
    }
}

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

void Widget::setOwnerContent (IUIContent* content)
{
    UIComponent::setOwnerContent (content);

    if (!content)
        return;
    
    for (auto& child : m_children)
    {
        if (child)
        {
            child->setOwnerContent (content);
            
            if (child->getFocusPolicy() != FocusPolicy::NoFocus)
            {
                content->registerFocusableComponent (child.get());
            }
        }
    }
}

void Widget::setOwnerContext (UIContext* context)
{
    UIComponent::setOwnerContext (context);
    
    for (auto& child : m_children)
    {
        if (child)
            child->setOwnerContext (context);
    }
}

}
