#include "YuchenUI/core/IUIContent.h"
#include "YuchenUI/core/UIContext.h"
#include "YuchenUI/widgets/UIComponent.h"
#include "YuchenUI/focus/FocusManager.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/platform/IInputMethodSupport.h"
#include <algorithm>

namespace YuchenUI {

IUIContent::IUIContent()
    : m_context(nullptr)
    , m_contentArea()
    , m_result(WindowContentResult::None)
    , m_userData(nullptr)
    , m_components()
    , m_closeCallback(nullptr)
{}

IUIContent::~IUIContent() {
    clearComponents();
}

bool IUIContent::handleMouseEvent(const Vec2& position, bool pressed, bool isMoveEvent) {
    for (auto it = m_components.rbegin(); it != m_components.rend(); ++it) {
        UIComponent* component = *it;
        if (component && component->isVisible() && component->isEnabled()) {
            bool handled = isMoveEvent
                ? component->handleMouseMove(position)
                : component->handleMouseClick(position, pressed);
            
            if (handled) {
                if (!isMoveEvent && pressed) {
                    if (component->canAcceptFocus()) {
                        component->setFocus(FocusReason::MouseFocusReason);
                    }
                }
                return true;
            }
        }
    }
    
    if (!isMoveEvent && !pressed && m_context) {
        m_context->getFocusManager().clearFocus();
    }
    
    return false;
}

void IUIContent::requestClose(WindowContentResult result) {
    setResult(result);
    if (m_closeCallback) {
        m_closeCallback(result);
    }
}

bool IUIContent::handleMouseMove(const Vec2& position) {
    return handleMouseEvent(position, false, true);
}

bool IUIContent::handleMouseClick(const Vec2& position, bool pressed) {
    return handleMouseEvent(position, pressed, false);
}

bool IUIContent::handleMouseWheel(const Vec2& delta, const Vec2& position) {
    for (auto it = m_components.rbegin(); it != m_components.rend(); ++it) {
        UIComponent* component = *it;
        if (component && component->isVisible() && component->isEnabled()) {
            if (component->handleMouseWheel(delta, position)) {
                return true;
            }
        }
    }
    return false;
}

bool IUIContent::handleKeyEvent(const Event& event) {
    if (!m_context) return false;
    
    auto* focused = m_context->getFocusManager().getFocusedComponent();
    if (focused && focused->isVisible() && focused->isEnabled()) {
        return focused->handleKeyPress(event);
    }
    
    return false;
}

bool IUIContent::handleTextInput(const Event& event) {
    if (!m_context) return false;
    
    auto* focused = m_context->getFocusManager().getFocusedComponent();
    if (!focused || !focused->isVisible() || !focused->isEnabled()) {
        return false;
    }
    
    if (event.type == EventType::TextComposition) {
        return focused->handleComposition(
            event.textComposition.text,
            event.textComposition.cursorPosition,
            event.textComposition.selectionLength
        );
    }
    
    return focused->handleTextInput(event.textInput.codepoint);
}

Rect IUIContent::getInputMethodCursorRect() const {
    if (!m_context) return Rect();
    
    auto* focused = m_context->getFocusManager().getFocusedComponent();
    if (!focused) return Rect();
    
    IInputMethodSupport* inputSupport = dynamic_cast<IInputMethodSupport*>(focused);
    if (!inputSupport) return Rect();
    
    Rect localRect = inputSupport->getInputMethodCursorRect();
    return focused->mapToWindow(localRect);
}

void IUIContent::addComponent(UIComponent* component) {
    if (!component) return;
    
    if (m_context) {
        component->setOwnerContext(m_context);
        m_context->addComponent(component);
    }
    
    auto it = std::find(m_components.begin(), m_components.end(), component);
    if (it == m_components.end()) {
        m_components.push_back(component);
    }
}

void IUIContent::removeComponent(UIComponent* component) {
    if (!component) return;
    
    if (m_context) {
        unregisterFocusableComponent(component);
        m_context->removeComponent(component);
    }
    
    auto it = std::find(m_components.begin(), m_components.end(), component);
    if (it != m_components.end()) {
        m_components.erase(it);
    }
}

void IUIContent::clearComponents() {
    if (m_context) {
        m_context->getFocusManager().clearFocus();
    }
    m_components.clear();
}

UIComponent* IUIContent::getFocusedComponent() const {
    if (!m_context) return nullptr;
    return m_context->getFocusManager().getFocusedComponent();
}

void IUIContent::registerFocusableComponent(UIComponent* component) {
    if (m_context && component->getFocusPolicy() != FocusPolicy::NoFocus) {
        m_context->getFocusManager().registerComponent(component);
    }
}

void IUIContent::unregisterFocusableComponent(UIComponent* component) {
    if (m_context) {
        m_context->getFocusManager().unregisterComponent(component);
    }
}

}
