#include "YuchenUI/widgets/UIComponent.h"
#include "YuchenUI/widgets/IScrollable.h"
#include "YuchenUI/core/UIContext.h"
#include "YuchenUI/core/IUIContent.h"
#include "YuchenUI/core/Validation.h"
#include "YuchenUI/core/Assert.h"
#include "YuchenUI/focus/FocusManager.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/theme/Theme.h"
#include <algorithm>

namespace YuchenUI {

UIComponent::UIComponent()
    : m_isVisible(true)
    , m_isEnabled(true)
    , m_ownerContext(nullptr)
    , m_ownerContent(nullptr)
    , m_parent(nullptr)
    , m_contextMenu(nullptr)
    , m_bounds()
    , m_ownedChildren()
    , m_paddingLeft(0.0f)
    , m_paddingTop(0.0f)
    , m_paddingRight(0.0f)
    , m_paddingBottom(0.0f)
    , m_focusPolicy(FocusPolicy::NoFocus)
    , m_hasFocus(false)
    , m_tabOrder(-1)
    , m_focusProxy(nullptr)
    , m_showFocusIndicator(true)
    , m_focusManagerAccessor(nullptr)
{
}

UIComponent::~UIComponent()
{
    clearChildren();
}

//======================================================================================
// Geometry Management

void UIComponent::setBounds(const Rect& bounds)
{
    Validation::AssertRect(bounds);
    m_bounds = bounds;
}

void UIComponent::setPadding(float padding)
{
    YUCHEN_ASSERT(padding >= 0.0f);
    m_paddingLeft = m_paddingTop = m_paddingRight = m_paddingBottom = padding;
}

void UIComponent::setPadding(float left, float top, float right, float bottom)
{
    YUCHEN_ASSERT(left >= 0.0f && top >= 0.0f && right >= 0.0f && bottom >= 0.0f);
    m_paddingLeft = left;
    m_paddingTop = top;
    m_paddingRight = right;
    m_paddingBottom = bottom;
}

Rect UIComponent::getContentRect() const
{
    return Rect(
        m_paddingLeft,
        m_paddingTop,
        std::max(0.0f, m_bounds.width - m_paddingLeft - m_paddingRight),
        std::max(0.0f, m_bounds.height - m_paddingTop - m_paddingBottom)
    );
}

Rect UIComponent::mapToWindow(const Rect& localRect) const
{
    Rect result = localRect;
    const UIComponent* current = this->getParent();
    
    while (current != nullptr)
    {
        const Rect& bounds = current->getBounds();
        result.x += bounds.x;
        result.y += bounds.y;
        current = current->getParent();
    }
    
    return result;
}

//======================================================================================
// Child Component Management

void UIComponent::removeChild(UIComponent* child)
{
    YUCHEN_ASSERT(child);
    
    if (m_ownerContent && child->getFocusPolicy() != FocusPolicy::NoFocus)
    {
        m_ownerContent->unregisterFocusableComponent(child);
    }
    
    auto it = std::find(m_ownedChildren.begin(), m_ownedChildren.end(), child);
    
    if (it != m_ownedChildren.end())
    {
        delete child;
        m_ownedChildren.erase(it);
    }
}

void UIComponent::clearChildren()
{
    if (m_ownerContent)
    {
        for (auto* child : m_ownedChildren)
        {
            if (child && child->getFocusPolicy() != FocusPolicy::NoFocus)
            {
                m_ownerContent->unregisterFocusableComponent(child);
            }
        }
    }
    
    for (auto* child : m_ownedChildren)
    {
        delete child;
    }
    m_ownedChildren.clear();
}

//======================================================================================
// Visibility and Enabled State

void UIComponent::setVisible(bool visible)
{
    if (m_isVisible == visible) return;
    
    m_isVisible = visible;
    
    if (!visible && m_hasFocus)
    {
        clearFocus();
    }
}

void UIComponent::setEnabled(bool enabled)
{
    if (m_isEnabled == enabled) return;
    
    m_isEnabled = enabled;
    
    if (!enabled && m_hasFocus)
    {
        clearFocus();
    }
}

//======================================================================================
// Context and Ownership

void UIComponent::setOwnerContext(UIContext* context)
{
    m_ownerContext = context;
    
    if (context)
    {
        m_focusManagerAccessor = &context->getFocusManager();
    }
    else
    {
        m_focusManagerAccessor = nullptr;
    }
    
    for (auto* child : m_ownedChildren)
    {
        if (child)
            child->setOwnerContext(context);
    }
}

void UIComponent::setOwnerContent(IUIContent* content)
{
    m_ownerContent = content;
    
    if (!content)
        return;
    
    for (auto* child : m_ownedChildren)
    {
        if (child)
        {
            child->setOwnerContent(content);
            
            if (child->getFocusPolicy() != FocusPolicy::NoFocus)
            {
                content->registerFocusableComponent(child);
            }
        }
    }
}

//======================================================================================
// Mouse Capture

void UIComponent::captureMouse()
{
    if (m_ownerContext)
    {
        m_ownerContext->captureMouse(this);
    }
}

void UIComponent::releaseMouse()
{
    if (m_ownerContext)
    {
        m_ownerContext->releaseMouse();
    }
}

//======================================================================================
// Focus Management

void UIComponent::setFocusPolicy(FocusPolicy policy)
{
    if (m_focusPolicy == policy) return;
    
    m_focusPolicy = policy;
    
    if (policy == FocusPolicy::NoFocus && m_hasFocus)
    {
        clearFocus();
    }
}

bool UIComponent::canAcceptFocus() const
{
    return m_focusPolicy != FocusPolicy::NoFocus &&
           m_isEnabled &&
           m_isVisible;
}

bool UIComponent::acceptsTabFocus() const
{
    return canAcceptFocus() && canGetFocusByTab(m_focusPolicy);
}

bool UIComponent::acceptsClickFocus() const
{
    return canAcceptFocus() && canGetFocusByClick(m_focusPolicy);
}

void UIComponent::setFocus(FocusReason reason)
{
    if (m_focusManagerAccessor)
    {
        m_focusManagerAccessor->setFocus(this, reason);
    }
}

void UIComponent::clearFocus()
{
    if (m_focusManagerAccessor && m_hasFocus)
    {
        m_focusManagerAccessor->clearFocus();
    }
}

void UIComponent::setFocusProxy(UIComponent* proxy)
{
    m_focusProxy = proxy;
}

UIComponent* UIComponent::effectiveFocusWidget()
{
    return m_focusProxy ? m_focusProxy : this;
}

void UIComponent::setTabOrder(int order)
{
    if (m_tabOrder == order) return;
    
    m_tabOrder = order;
    
    if (m_focusManagerAccessor)
    {
        m_focusManagerAccessor->markDirty();
    }
}

//======================================================================================
// Focus Indicator

void UIComponent::drawFocusIndicator(RenderList& commandList, const Vec2& offset) const
{
    if (!m_hasFocus || !m_showFocusIndicator) return;
    
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    if (!style) return;
    
    const float borderWidth = UIStyle::FOCUS_INDICATOR_BORDER_WIDTH;
    
    FocusIndicatorDrawInfo info;
    info.bounds = Rect(
        m_bounds.x + offset.x - borderWidth,
        m_bounds.y + offset.y - borderWidth,
        m_bounds.width + borderWidth * 2.0f,
        m_bounds.height + borderWidth * 2.0f
    );
    info.cornerRadius = getFocusIndicatorCornerRadius();
    
    style->drawFocusIndicator(info, commandList);
}

//======================================================================================
// Focus Events (Private)

void UIComponent::notifyFocusIn(FocusReason reason)
{
    m_hasFocus = true;
    onFocusIn(reason);
    focusInEvent(reason);
}

void UIComponent::notifyFocusOut(FocusReason reason)
{
    m_hasFocus = false;
    onFocusOut(reason);
    focusOutEvent(reason);
}

void UIComponent::onFocusIn(FocusReason reason)
{
    scrollIntoViewIfNeeded();
}

void UIComponent::onFocusOut(FocusReason reason)
{
}

void UIComponent::scrollIntoViewIfNeeded()
{
    UIComponent* parent = m_parent;
    while (parent)
    {
        IScrollable* scrollable = dynamic_cast<IScrollable*>(parent);
        if (scrollable)
        {
            Rect bounds = getBounds();
            Vec2 posInScrollable(bounds.x, bounds.y);
            
            UIComponent* current = m_parent;
            while (current && current != parent)
            {
                const Rect& currentBounds = current->getBounds();
                posInScrollable.x += currentBounds.x;
                posInScrollable.y += currentBounds.y;
                current = current->getParent();
            }
            
            Rect rectInScrollable(posInScrollable.x, posInScrollable.y,
                                 bounds.width, bounds.height);
            scrollable->scrollRectIntoView(rectInScrollable);
            
            break;
        }
        parent = parent->getParent();
    }
}

//======================================================================================
// Child Rendering and Event Dispatch

void UIComponent::renderChildren(RenderList& commandList, const Vec2& offset) const
{
    for (const auto* child : m_ownedChildren)
    {
        if (child && child->isVisible())
            child->addDrawCommands(commandList, offset);
    }
}

bool UIComponent::dispatchMouseEvent(const Vec2& position, bool pressed, const Vec2& offset, bool isMove)
{
    if (!m_isEnabled || !m_isVisible)
        return false;
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect absRect(absPos.x, absPos.y, m_bounds.width, m_bounds.height);
    
    if (!absRect.contains(position))
        return false;
    
    for (auto it = m_ownedChildren.rbegin(); it != m_ownedChildren.rend(); ++it)
    {
        if (!(*it) || !(*it)->isVisible())
            continue;
        
        bool handled = isMove
            ? (*it)->handleMouseMove(position, absPos)
            : (*it)->handleMouseClick(position, pressed, absPos);
        
        if (handled)
            return true;
    }
    
    return false;
}

void UIComponent::update(float deltaTime)
{
    for (auto* child : m_ownedChildren)
    {
        if (child && child->isVisible())
            child->update(deltaTime);
    }
}

} // namespace YuchenUI
