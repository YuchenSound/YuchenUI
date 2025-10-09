#include "YuchenUI/widgets/UIComponent.h"
#include "YuchenUI/widgets/Widget.h"
#include "YuchenUI/widgets/IScrollable.h"
#include "YuchenUI/windows/BaseWindow.h"
#include "YuchenUI/windows/IWindowContent.h"
#include "YuchenUI/focus/FocusManager.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/theme/ThemeManager.h"
#include "YuchenUI/theme/Theme.h"

namespace YuchenUI {

UIComponent::UIComponent()
    : m_isVisible(true)
    , m_isEnabled(true)
    , m_ownerWindow(nullptr)
    , m_ownerContent(nullptr)
    , m_parent(nullptr)
    , m_contextMenu(nullptr)
    , m_focusPolicy(FocusPolicy::NoFocus)
    , m_hasFocus(false)
    , m_tabOrder(-1)
    , m_focusProxy(nullptr)
    , m_showFocusIndicator(true)
    , m_focusManagerAccessor(nullptr)
{
}

Rect UIComponent::mapToWindow(const Rect& localRect) const {
    Rect result = localRect;
    const UIComponent* current = this;
    
    while (current != nullptr) {
        const Rect& bounds = current->getBounds();
        result.x += bounds.x;
        result.y += bounds.y;
        current = current->getParent();
    }
    
    return result;
}

void UIComponent::setVisible(bool visible) {
    if (m_isVisible == visible) return;
    
    m_isVisible = visible;
    
    if (!visible && m_hasFocus) {
        clearFocus();
    }
}

void UIComponent::setEnabled(bool enabled) {
    if (m_isEnabled == enabled) return;
    
    m_isEnabled = enabled;
    
    if (!enabled && m_hasFocus) {
        clearFocus();
    }
}

void UIComponent::captureMouse() {
    if (m_ownerWindow) {
        m_ownerWindow->captureMouse(this);
    }
}

void UIComponent::releaseMouse() {
    if (m_ownerWindow) {
        m_ownerWindow->releaseMouse();
    }
}

void UIComponent::setFocusPolicy(FocusPolicy policy) {
    if (m_focusPolicy == policy) return;
    
    m_focusPolicy = policy;
    
    if (policy == FocusPolicy::NoFocus && m_hasFocus) {
        clearFocus();
    }
}

bool UIComponent::canAcceptFocus() const {
    return m_focusPolicy != FocusPolicy::NoFocus &&
           m_isEnabled &&
           m_isVisible;
}

bool UIComponent::acceptsTabFocus() const {
    return canAcceptFocus() && canGetFocusByTab(m_focusPolicy);
}

bool UIComponent::acceptsClickFocus() const {
    return canAcceptFocus() && canGetFocusByClick(m_focusPolicy);
}

void UIComponent::setFocus(FocusReason reason) {
    if (m_focusManagerAccessor) {
        m_focusManagerAccessor->setFocus(this, reason);
    }
}

void UIComponent::clearFocus() {
    if (m_focusManagerAccessor && m_hasFocus) {
        m_focusManagerAccessor->clearFocus();
    }
}

void UIComponent::setFocusProxy(UIComponent* proxy) {
    m_focusProxy = proxy;
}

UIComponent* UIComponent::effectiveFocusWidget() {
    return m_focusProxy ? m_focusProxy : this;
}

void UIComponent::setTabOrder(int order) {
    if (m_tabOrder == order) return;
    
    m_tabOrder = order;
    
    if (m_focusManagerAccessor) {
        m_focusManagerAccessor->markDirty();
    }
}

void UIComponent::drawFocusIndicator(RenderList& commandList, const Vec2& offset) const {
    if (!m_hasFocus || !m_showFocusIndicator) return;
    
    UIStyle* style = ThemeManager::getInstance().getCurrentStyle();
    
    const float borderWidth = UIStyle::FOCUS_INDICATOR_BORDER_WIDTH;
    
    FocusIndicatorDrawInfo info;
    info.bounds = Rect(
        getBounds().x + offset.x - borderWidth,
        getBounds().y + offset.y - borderWidth,
        getBounds().width + borderWidth * 2.0f,
        getBounds().height + borderWidth * 2.0f
    );
    info.cornerRadius = getFocusIndicatorCornerRadius();
    
    style->drawFocusIndicator(info, commandList);
}

void UIComponent::notifyFocusIn(FocusReason reason) {
    m_hasFocus = true;
    onFocusIn(reason);
    focusInEvent(reason);
}

void UIComponent::notifyFocusOut(FocusReason reason) {
    m_hasFocus = false;
    onFocusOut(reason);
    focusOutEvent(reason);
}

void UIComponent::onFocusIn(FocusReason reason) {
    scrollIntoViewIfNeeded();
}

void UIComponent::onFocusOut(FocusReason reason) {
}

void UIComponent::scrollIntoViewIfNeeded() {
    UIComponent* parent = m_parent;
    while (parent) {
        IScrollable* scrollable = dynamic_cast<IScrollable*>(parent);
        if (scrollable) {
            Rect bounds = getBounds();
            Vec2 posInScrollable(bounds.x, bounds.y);
            
            UIComponent* current = m_parent;
            while (current && current != parent) {
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

}
