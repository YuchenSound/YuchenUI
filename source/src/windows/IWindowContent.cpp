#include "YuchenUI/windows/IWindowContent.h"
#include "YuchenUI/windows/BaseWindow.h"
#include "YuchenUI/widgets/UIComponent.h"
#include "YuchenUI/widgets/TextInput.h"

#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/core/Assert.h"
#include <algorithm>
#include <chrono>

namespace YuchenUI {

IWindowContent::IWindowContent()
    : m_window(nullptr)
    , m_contentArea()
    , m_result(WindowContentResult::None)
    , m_userData(nullptr)
    , m_components()
    , m_focusedComponent(nullptr)
    , m_focusableComponents()
    , m_focusOrderDirty(false)
{
}

IWindowContent::~IWindowContent() {
    clearComponents();
}

void IWindowContent::onDestroy() {
    clearComponents();
}

void IWindowContent::onShow() {
}

void IWindowContent::onHide() {
}

void IWindowContent::onResize(const Rect& newArea) {
    m_contentArea = newArea;
}

void IWindowContent::onUpdate() {
    static auto lastFrameTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
    lastFrameTime = currentTime;
    
    for (UIComponent* component : m_components) {
        if (component && component->isVisible()) {
            component->update(deltaTime);
        }
    }
}

void IWindowContent::render(RenderList& commandList) {
    for (UIComponent* component : m_components) {
        if (component && component->isVisible()) {
            component->addDrawCommands(commandList);
        }
    }
}

bool IWindowContent::handleMouseMove(const Vec2& position) {
    for (auto it = m_components.rbegin(); it != m_components.rend(); ++it) {
        UIComponent* component = *it;
        if (component && component->isVisible() && component->isEnabled()) {
            if (component->handleMouseMove(position)) {
                return true;
            }
        }
    }
    return false;
}

bool IWindowContent::handleMouseClick(const Vec2& position, bool pressed) {
    if (pressed) {
        for (auto it = m_components.rbegin(); it != m_components.rend(); ++it) {
            UIComponent* component = *it;
            if (component && component->isVisible() && component->isEnabled()) {
                if (component->handleMouseClick(position, pressed)) {
                    return true;
                }
            }
        }
        setFocusedComponent(nullptr);
        return false;
    } else {
        for (auto it = m_components.rbegin(); it != m_components.rend(); ++it) {
            UIComponent* component = *it;
            if (component && component->isVisible() && component->isEnabled()) {
                if (component->handleMouseClick(position, pressed)) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool IWindowContent::handleKeyEvent(const Event& event) {
    YUCHEN_ASSERT(event.type == EventType::KeyPressed || event.type == EventType::KeyReleased);
    
    if (event.type == EventType::KeyPressed && event.key.key == KeyCode::Tab) {
        cycleFocus(!event.key.modifiers.hasShift());
        return true;
    }
    
    if (m_focusedComponent && m_focusedComponent->isVisible() && m_focusedComponent->isEnabled()) {
        return m_focusedComponent->handleKeyPress(event);
    }
    
    return false;
}

bool IWindowContent::handleTextInput(const Event& event) {
    YUCHEN_ASSERT(event.type == EventType::TextInput || event.type == EventType::TextComposition);
    
    if (!m_focusedComponent || !m_focusedComponent->isVisible() || !m_focusedComponent->isEnabled()) {
        return false;
    }
    
    if (event.type == EventType::TextComposition) {
        return m_focusedComponent->handleComposition(
            event.textComposition.text,
            event.textComposition.cursorPosition,
            event.textComposition.selectionLength
        );
    }
    
    return m_focusedComponent->handleTextInput(event.textInput.codepoint);
}

bool IWindowContent::handleScroll(const Event& event) {
    YUCHEN_ASSERT(event.type == EventType::MouseScrolled);
    
    Vec2 delta = event.mouseScroll.delta;
    Vec2 position = event.mouseScroll.position;
    
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

void IWindowContent::addComponent(UIComponent* component) {
    YUCHEN_ASSERT_MSG(component != nullptr, "Component cannot be null");
    
    if (m_window) {
        component->setOwnerWindow(dynamic_cast<BaseWindow*>(m_window));
    }
    component->setOwnerContent(this);
    
    auto it = std::find(m_components.begin(), m_components.end(), component);
    if (it == m_components.end()) {
        m_components.push_back(component);
    }
    
    if (component->canReceiveFocus()) {
        registerFocusableComponent(component);
    }
}

void IWindowContent::removeComponent(UIComponent* component) {
    YUCHEN_ASSERT_MSG(component != nullptr, "Component cannot be null");
    
    if (m_focusedComponent == component) {
        setFocusedComponent(nullptr);
    }
    
    unregisterFocusableComponent(component);
    
    auto it = std::find(m_components.begin(), m_components.end(), component);
    if (it != m_components.end()) {
        m_components.erase(it);
    }
}

void IWindowContent::clearComponents() {
    setFocusedComponent(nullptr);
    m_focusableComponents.clear();
    m_components.clear();
}

void IWindowContent::setFocusedComponent(UIComponent* component) {
    if (m_focusedComponent == component) return;
    
    if (m_focusedComponent) {
        m_focusedComponent->onFocusLost();
    }
    
    m_focusedComponent = component;
    
    if (m_focusedComponent) {
        m_focusedComponent->onFocusGained();
    }
}

void IWindowContent::cycleFocus(bool forward) {
    if (m_focusOrderDirty) {
        std::stable_sort(m_focusableComponents.begin(), m_focusableComponents.end(),
            [](UIComponent* a, UIComponent* b) {
                int orderA = a->getTabOrder();
                int orderB = b->getTabOrder();
                if (orderA < 0 && orderB < 0) return false;
                if (orderA < 0) return false;
                if (orderB < 0) return true;
                return orderA < orderB;
            });
        m_focusOrderDirty = false;
    }
    
    if (m_focusableComponents.empty()) return;
    
    size_t currentIndex = 0;
    bool found = false;
    
    if (m_focusedComponent) {
        for (size_t i = 0; i < m_focusableComponents.size(); ++i) {
            if (m_focusableComponents[i] == m_focusedComponent) {
                currentIndex = i;
                found = true;
                break;
            }
        }
    }
    
    size_t newIndex;
    if (!found) {
        newIndex = forward ? 0 : m_focusableComponents.size() - 1;
    } else {
        if (forward) {
            newIndex = (currentIndex + 1) % m_focusableComponents.size();
        } else {
            newIndex = (currentIndex == 0) ? m_focusableComponents.size() - 1 : currentIndex - 1;
        }
    }
    
    UIComponent* newFocusComponent = m_focusableComponents[newIndex];
    if (newFocusComponent && newFocusComponent->isVisible() && newFocusComponent->isEnabled()) {
        setFocusedComponent(newFocusComponent);
    }
}

void IWindowContent::registerFocusableComponent(UIComponent* component) {
    YUCHEN_ASSERT_MSG(component != nullptr, "Component cannot be null");
    
    auto it = std::find(m_focusableComponents.begin(), m_focusableComponents.end(), component);
    if (it == m_focusableComponents.end()) {
        m_focusableComponents.push_back(component);
        m_focusOrderDirty = true;
    }
}

void IWindowContent::unregisterFocusableComponent(UIComponent* component) {
    YUCHEN_ASSERT_MSG(component != nullptr, "Component cannot be null");
    
    auto it = std::find(m_focusableComponents.begin(), m_focusableComponents.end(), component);
    if (it != m_focusableComponents.end()) {
        m_focusableComponents.erase(it);
    }
}

void IWindowContent::markFocusOrderDirty() {
    m_focusOrderDirty = true;
}

void IWindowContent::requestTextInput(bool enable) {
    if (m_window) {
        if (enable) {
            m_window->enableTextInput();
        }
        else {
            m_window->disableTextInput();
        }
    }
}

void IWindowContent::setIMEEnabled(bool enable) {
    if (m_window) {
        m_window->setIMEEnabled(enable);
    }
}

Rect IWindowContent::getInputMethodCursorRect() const {
    if (!m_focusedComponent) {
        return Rect();
    }
    
    IInputMethodSupport* inputMethodSupport = dynamic_cast<IInputMethodSupport*>(m_focusedComponent);
    if (!inputMethodSupport) {
        return Rect();
    }
    
    Rect localCursorRect = inputMethodSupport->getInputMethodCursorRect();
    if (localCursorRect.width == 0.0f || localCursorRect.height == 0.0f) {
        return Rect();
    }
    
    return m_focusedComponent->mapToWindow(localCursorRect);
}

}
