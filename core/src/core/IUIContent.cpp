/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Core module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

//==========================================================================================
/** @file IUIContent.cpp
    
    Implementation notes:
    - Mouse events are dispatched to components in reverse order (top to bottom)
    - Mouse click on focusable component automatically sets focus
    - Click on empty area clears focus
    - Key events are routed to currently focused component only
    - Text composition events handled by focused component with IME support
*/

#include "YuchenUI/core/IUIContent.h"
#include "YuchenUI/core/UIContext.h"
#include "YuchenUI/widgets/UIComponent.h"
#include "YuchenUI/focus/FocusManager.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/platform/IInputMethodSupport.h"
#include <algorithm>

namespace YuchenUI {

//==========================================================================================

IUIContent::IUIContent()
    : m_context(nullptr)
    , m_contentArea()
    , m_result(WindowContentResult::None)
    , m_userData(nullptr)
    , m_components()
    , m_closeCallback(nullptr)
{}

IUIContent::~IUIContent()
{
    clearComponents();
}

//==========================================================================================
// Mouse event handling

bool IUIContent::handleMouseEvent(const Vec2& position, bool pressed, bool isMoveEvent)
{
    // Dispatch to components in reverse order (topmost first)
    for (auto it = m_components.rbegin(); it != m_components.rend(); ++it)
    {
        UIComponent* component = *it;
        if (component && component->isVisible() && component->isEnabled())
        {
            bool handled = isMoveEvent
                ? component->handleMouseMove(position)
                : component->handleMouseClick(position, pressed);
            
            if (handled)
            {
                // On mouse press, set focus if component accepts click focus
                if (!isMoveEvent && pressed)
                {
                    if (component->canAcceptFocus())
                    {
                        component->setFocus(FocusReason::MouseFocusReason);
                    }
                }
                return true;
            }
        }
    }
    
    // Click on empty area clears focus
    if (!isMoveEvent && !pressed && m_context)
    {
        m_context->getFocusManager().clearFocus();
    }
    
    return false;
}

void IUIContent::requestClose(WindowContentResult result)
{
    setResult(result);
    if (m_closeCallback)
    {
        m_closeCallback(result);
    }
}

bool IUIContent::handleMouseMove(const Vec2& position)
{
    return handleMouseEvent(position, false, true);
}

bool IUIContent::handleMouseClick(const Vec2& position, bool pressed)
{
    return handleMouseEvent(position, pressed, false);
}

bool IUIContent::handleMouseWheel(const Vec2& delta, const Vec2& position)
{
    for (auto it = m_components.rbegin(); it != m_components.rend(); ++it)
    {
        UIComponent* component = *it;
        if (component && component->isVisible() && component->isEnabled())
        {
            if (component->handleMouseWheel(delta, position)) return true;
        }
    }
    return false;
}

//==========================================================================================
// Keyboard event handling

bool IUIContent::handleKeyEvent(const Event& event)
{
    if (!m_context) return false;
    
    // Route to focused component
    auto* focused = m_context->getFocusManager().getFocusedComponent();
    if (focused && focused->isVisible() && focused->isEnabled())
    {
        return focused->handleKeyPress(event);
    }
    
    return false;
}

//==========================================================================================
// Text input event handling

bool IUIContent::handleTextInput(const Event& event) {
    if (!m_context) return false;
    
    auto* focused = m_context->getFocusManager().getFocusedComponent();
    if (!focused || !focused->isVisible() || !focused->isEnabled()) return false;
    
    // Handle IME composition or regular text input
    if (event.type == EventType::TextComposition)
    {
        return focused->handleComposition(
            event.textComposition.text,
            event.textComposition.cursorPosition,
            event.textComposition.selectionLength
        );
    }
    
    return focused->handleTextInput(event.textInput.codepoint);
}

//==========================================================================================
// IME support

Rect IUIContent::getInputMethodCursorRect() const
{
    if (!m_context) return Rect();
    
    auto* focused = m_context->getFocusManager().getFocusedComponent();
    if (!focused) return Rect();
    
    // Query component for IME cursor position if it supports IME
    IInputMethodSupport* inputSupport = dynamic_cast<IInputMethodSupport*>(focused);
    if (!inputSupport) return Rect();
    
    // Convert from component-local to window coordinates
    Rect localRect = inputSupport->getInputMethodCursorRect();
    return focused->mapToWindow(localRect);
}

//==========================================================================================
// Component management

void IUIContent::addComponent(UIComponent* component)
{
    if (!component) return;
    
    // Set ownership and register with context
    if (m_context)
    {
        component->setOwnerContext(m_context);
        m_context->addComponent(component);
    }
    
    // Add to component list if not already present
    auto it = std::find(m_components.begin(), m_components.end(), component);
    if (it == m_components.end())
    {
        m_components.push_back(component);
    }
}

void IUIContent::removeComponent(UIComponent* component)
{
    if (!component) return;
    
    // Unregister from focus system and context
    if (m_context)
    {
        unregisterFocusableComponent(component);
        m_context->removeComponent(component);
    }
    
    // Remove from component list
    auto it = std::find(m_components.begin(), m_components.end(), component);
    if (it != m_components.end())
    {
        m_components.erase(it);
    }
}

void IUIContent::clearComponents()
{
    if (m_context) m_context->getFocusManager().clearFocus();
    m_components.clear();
}

//==========================================================================================
// Focus management

UIComponent* IUIContent::getFocusedComponent() const
{
    if (!m_context) return nullptr;
    return m_context->getFocusManager().getFocusedComponent();
}

void IUIContent::registerFocusableComponent(UIComponent* component)
{
    if (m_context && component->getFocusPolicy() != FocusPolicy::NoFocus)
    {
        m_context->getFocusManager().registerComponent(component);
    }
}

void IUIContent::unregisterFocusableComponent(UIComponent* component)
{
    if (m_context) m_context->getFocusManager().unregisterComponent(component);
}

} // namespace YuchenUI
