/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Windows module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

//==========================================================================================
/** @file IWindowContent.cpp
    
    Implementation notes:
    - Components tested in reverse order (top-most receives events first)
    - Focus cleared when clicking empty space
    - Direction keys (arrows/tab/home/end) handled by focus manager
    - IME cursor position queried from focused component
*/

#include "YuchenUI/windows/IWindowContent.h"
#include "YuchenUI/windows/BaseWindow.h"
#include "YuchenUI/widgets/UIComponent.h"
#include "YuchenUI/widgets/TextInput.h"
#include "YuchenUI/focus/FocusManager.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/core/Assert.h"
#include <algorithm>
#include <chrono>

namespace YuchenUI {

//==========================================================================================
// Construction and Destruction

IWindowContent::IWindowContent()
    : m_window(nullptr)
    , m_contentArea()
    , m_result(WindowContentResult::None)
    , m_userData(nullptr)
    , m_components()
    , m_focusManager(nullptr)
{
    m_focusManager = std::make_unique<FocusManager> (this);
}

IWindowContent::~IWindowContent()
{
    clearComponents();
}

//==========================================================================================
// Lifecycle Callbacks

void IWindowContent::onDestroy()
{
    clearComponents();
}

void IWindowContent::onShow()
{
}

void IWindowContent::onHide()
{
}

void IWindowContent::onResize (const Rect& newArea)
{
    m_contentArea = newArea;
}

//==========================================================================================
// Update and Rendering

void IWindowContent::onUpdate()
{
    static auto lastFrameTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
    lastFrameTime = currentTime;
    
    for (UIComponent* component : m_components)
    {
        if (component && component->isVisible())
        {
            component->update (deltaTime);
        }
    }
}

void IWindowContent::render (RenderList& commandList)
{
    for (UIComponent* component : m_components)
    {
        if (component && component->isVisible())
        {
            component->addDrawCommands (commandList);
        }
    }
}

//==========================================================================================
// Event Handling

bool IWindowContent::handleMouseEvent(const Vec2& position, bool pressed, bool isMoveEvent)
{
    bool handled = false;
    
    for (auto it = m_components.rbegin(); it != m_components.rend(); ++it)
    {
        UIComponent* component = *it;
        if (component && component->isVisible() && component->isEnabled())
        {
            if (isMoveEvent) {
                if (component->handleMouseMove(position)) {
                    handled = true;
                    break;
                }
            } else {
                if (component->handleMouseClick(position, pressed)) {
                    handled = true;
                    if (pressed) {
                        break;  // Click is handled, no need to continue
                    }
                }
            }
        }
    }

    if (!handled && !pressed) {
        // Clear focus only if mouse click is released and no component handled it
        m_focusManager->clearFocus();
    }

    return handled;
}

bool IWindowContent::handleMouseMove(const Vec2& position)
{
    return handleMouseEvent(position, true, true);
}

bool IWindowContent::handleMouseClick(const Vec2& position, bool pressed)
{
    return handleMouseEvent(position, pressed, false);
}

bool IWindowContent::handleKeyEvent (const Event& event)
{
    YUCHEN_ASSERT (event.type == EventType::KeyPressed || event.type == EventType::KeyReleased);
    
    if (event.type == EventType::KeyReleased)
        return false;
    
    bool shift = event.key.modifiers.hasShift();
    auto focused = m_focusManager->getFocusedComponent();
    
    // Handle navigation keys
    switch (event.key.key)
    {
        case KeyCode::Tab:
        {
            FocusDirection direction = shift ? FocusDirection::Previous : FocusDirection::Next;
            if (focused && focused->shouldHandleDirectionKey (direction))
            {
                return focused->handleKeyPress (event);
            }
            return m_focusManager->handleTabKey (shift);
        }
        
        case KeyCode::Home:
            return m_focusManager->moveFocus (FocusDirection::First);
            
        case KeyCode::End:
            return m_focusManager->moveFocus (FocusDirection::Last);
            
        case KeyCode::UpArrow:
            if (focused && focused->shouldHandleDirectionKey (FocusDirection::Up))
            {
                return focused->handleKeyPress (event);
            }
            return m_focusManager->handleDirectionKey (FocusDirection::Up);
            
        case KeyCode::DownArrow:
            if (focused && focused->shouldHandleDirectionKey (FocusDirection::Down))
            {
                return focused->handleKeyPress (event);
            }
            return m_focusManager->handleDirectionKey (FocusDirection::Down);
            
        case KeyCode::LeftArrow:
            if (focused && focused->shouldHandleDirectionKey (FocusDirection::Left))
            {
                return focused->handleKeyPress (event);
            }
            return m_focusManager->handleDirectionKey (FocusDirection::Left);
            
        case KeyCode::RightArrow:
            if (focused && focused->shouldHandleDirectionKey (FocusDirection::Right))
            {
                return focused->handleKeyPress (event);
            }
            return m_focusManager->handleDirectionKey (FocusDirection::Right);
            
        default:
            break;
    }
    
    // Forward to focused component
    if (focused && focused->isVisible() && focused->isEnabled())
    {
        return focused->handleKeyPress (event);
    }
    
    return false;
}

bool IWindowContent::handleTextInput (const Event& event)
{
    YUCHEN_ASSERT (event.type == EventType::TextInput || event.type == EventType::TextComposition);
    
    auto focused = m_focusManager->getFocusedComponent();
    if (!focused || !focused->isVisible() || !focused->isEnabled())
    {
        return false;
    }
    
    if (event.type == EventType::TextComposition)
    {
        return focused->handleComposition (
            event.textComposition.text,
            event.textComposition.cursorPosition,
            event.textComposition.selectionLength
        );
    }
    
    return focused->handleTextInput (event.textInput.codepoint);
}

bool IWindowContent::handleScroll (const Event& event)
{
    YUCHEN_ASSERT (event.type == EventType::MouseScrolled);
    
    Vec2 delta = event.mouseScroll.delta;
    Vec2 position = event.mouseScroll.position;
    
    for (auto it = m_components.rbegin(); it != m_components.rend(); ++it)
    {
        UIComponent* component = *it;
        if (component && component->isVisible() && component->isEnabled())
        {
            if (component->handleMouseWheel (delta, position))
            {
                return true;
            }
        }
    }
    
    return false;
}

//==========================================================================================
// Component Management

void IWindowContent::addComponent (UIComponent* component)
{
    YUCHEN_ASSERT_MSG (component != nullptr, "Component cannot be null");
    
    if (m_window)
    {
        component->setOwnerWindow (dynamic_cast<BaseWindow*>(m_window));
    }
    component->setOwnerContent (this);
    component->m_focusManagerAccessor = m_focusManager.get();
    
    auto it = std::find (m_components.begin(), m_components.end(), component);
    if (it == m_components.end())
    {
        m_components.push_back (component);
    }
    
    if (component->getFocusPolicy() != FocusPolicy::NoFocus)
    {
        m_focusManager->registerComponent (component);
    }
}

void IWindowContent::removeComponent (UIComponent* component)
{
    YUCHEN_ASSERT_MSG (component != nullptr, "Component cannot be null");
    
    m_focusManager->unregisterComponent (component);
    
    auto it = std::find (m_components.begin(), m_components.end(), component);
    if (it != m_components.end())
    {
        m_components.erase (it);
    }
}

void IWindowContent::clearComponents()
{
    m_focusManager->clearFocus();
    m_components.clear();
}

//==========================================================================================
// Focus Management

UIComponent* IWindowContent::getFocusedComponent() const
{
    return m_focusManager->getFocusedComponent();
}

void IWindowContent::registerFocusableComponent (UIComponent* component)
{
    YUCHEN_ASSERT (component);
    if (m_focusManager)
    {
        m_focusManager->registerComponent (component);
    }
}

void IWindowContent::unregisterFocusableComponent (UIComponent* component)
{
    YUCHEN_ASSERT (component);
    if (m_focusManager)
    {
        m_focusManager->unregisterComponent (component);
    }
}

void IWindowContent::setFocusManagerAccessor (UIComponent* component)
{
    YUCHEN_ASSERT (component);
    if (m_focusManager)
    {
        component->m_focusManagerAccessor = m_focusManager.get();
    }
}

//==========================================================================================
// Text Input and IME

void IWindowContent::requestTextInput (bool enable)
{
    if (m_window)
    {
        if (enable)
        {
            m_window->enableTextInput();
        }
        else
        {
            m_window->disableTextInput();
        }
    }
}

void IWindowContent::setIMEEnabled (bool enable)
{
    if (m_window)
    {
        m_window->setIMEEnabled (enable);
    }
}

Rect IWindowContent::getInputMethodCursorRect() const
{
    auto focused = m_focusManager->getFocusedComponent();
    if (!focused)
    {
        return Rect();
    }
    
    IInputMethodSupport* inputMethodSupport = dynamic_cast<IInputMethodSupport*>(focused);
    if (!inputMethodSupport)
    {
        return Rect();
    }
    
    Rect localCursorRect = inputMethodSupport->getInputMethodCursorRect();
    if (localCursorRect.width == 0.0f || localCursorRect.height == 0.0f)
    {
        return Rect();
    }
    
    return focused->mapToWindow (localCursorRect);
}

} // namespace YuchenUI
