#include "YuchenUI/widgets/UIComponent.h"
#include "YuchenUI/windows/BaseWindow.h"
#include "YuchenUI/windows/IWindowContent.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/theme/ThemeManager.h"
#include "YuchenUI/theme/Theme.h"

namespace YuchenUI {

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

void UIComponent::requestFocus() {
    if (m_ownerContent && canReceiveFocus()) {
        m_ownerContent->setFocusedComponent(this);
    }
}

bool UIComponent::isFocused() const {
    return m_ownerContent && m_ownerContent->getFocusedComponent() == this;
}

void UIComponent::setTabOrder(int order) {
    m_tabOrder = order;
    if (m_ownerContent) {
        m_ownerContent->markFocusOrderDirty();
    }
}

void UIComponent::drawFocusIndicator(RenderList& commandList, const Vec2& offset) const {
    if (!isFocused() || !m_drawsFocusIndicator) return;
    
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

}
