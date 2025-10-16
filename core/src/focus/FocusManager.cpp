/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Focus module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

//==========================================================================================
/** @file FocusManager.cpp
    
    Implementation notes:
    - Focus chain rebuilt only when dirty flag set (lazy evaluation)
    - Stable sort preserves registration order for equal TabOrder values
    - Directional navigation uses center-to-center Euclidean distance
    - Components with negative TabOrder excluded from Tab navigation
    - Focus proxy resolution handled by UIComponent::effectiveFocusWidget()
    - Distance calculation rejects candidates in opposite direction
*/

#include "YuchenUI/focus/FocusManager.h"
#include "YuchenUI/widgets/Widget.h"
#include "YuchenUI/core/Assert.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace YuchenUI {

//==========================================================================================
// Lifecycle

FocusManager::FocusManager()
    : m_focused(nullptr)
    , m_all()
    , m_chain()
    , m_dirty(true)
{
}

FocusManager::~FocusManager()
{
    clearFocus();
}

//==========================================================================================
// Focus Control

void FocusManager::setFocus(Widget* component, FocusReason reason)
{
    // Early exit if already focused
    if (m_focused == component) return;
    
    // Resolve focus proxy if present
    if (component) component = component->effectiveFocusWidget();
    
    // Validate component can accept focus
    if (component && !component->canAcceptFocus()) return;
    
    // Notify previous focused component
    if (m_focused) m_focused->notifyFocusOut(reason);
    
    // Update focused component
    m_focused = component;
    
    // Notify new focused component
    if (m_focused) m_focused->notifyFocusIn(reason);
}

void FocusManager::clearFocus()
{
    if (m_focused) setFocus(nullptr, FocusReason::OtherFocusReason);
}

//==========================================================================================
// Focus Navigation

bool FocusManager::moveFocus(FocusDirection direction)
{
    switch (direction)
    {
        case FocusDirection::Next:      return focusNext(FocusReason::TabFocusReason);
        case FocusDirection::Previous:  return focusPrevious(FocusReason::BacktabFocusReason);
        case FocusDirection::First:     return focusFirst();
        case FocusDirection::Last:      return focusLast();
        case FocusDirection::Up:
        case FocusDirection::Down:
        case FocusDirection::Left:
        case FocusDirection::Right:     return focusByDirection(direction);
        default:                        return false;
    }
}

//==========================================================================================
// Component Registration

void FocusManager::registerComponent(Widget* component)
{
    YUCHEN_ASSERT(component);
    
    // Check if already registered
    auto it = std::find(m_all.begin(), m_all.end(), component);
    if (it == m_all.end())
    {
        m_all.push_back(component);
        m_dirty = true;
    }
}

void FocusManager::unregisterComponent(Widget* component)
{
    YUCHEN_ASSERT(component);
    
    // Remove from registry
    auto it = std::find(m_all.begin(), m_all.end(), component);
    if (it != m_all.end())
    {
        m_all.erase(it);
        m_dirty = true;
    }
    
    // Clear focus if component was focused
    if (m_focused == component) m_focused = nullptr;
}

//==========================================================================================
// Keyboard Input Handling

bool FocusManager::handleTabKey(bool shift)
{
    if (shift)
    {
        return focusPrevious(FocusReason::BacktabFocusReason);
    }
    else
    {
        return focusNext(FocusReason::TabFocusReason);
    }
}

bool FocusManager::handleDirectionKey(FocusDirection direction)
{
    return focusByDirection(direction);
}

//==========================================================================================
// Focus Chain Management

void FocusManager::rebuildFocusChain()
{
    // Copy all registered components
    m_chain = m_all;
    
    // Stable sort by TabOrder (preserves registration order for equal values)
    std::stable_sort(m_chain.begin(), m_chain.end(),
        [](Widget* a, Widget* b){
            int orderA = a->getTabOrder();
            int orderB = b->getTabOrder();
            
            // Negative TabOrder components sort to end
            if (orderA < 0 && orderB < 0) return false;
            if (orderA < 0) return false;
            if (orderB < 0) return true;
            
            return orderA < orderB;
        });
    
    m_dirty = false;
}

//==========================================================================================
// Linear Navigation

bool FocusManager::focusNext(FocusReason reason)
{
    if (m_dirty) rebuildFocusChain();
    if (m_chain.empty()) return false;
    
    // Determine starting index
    size_t start = 0;
    if (m_focused)
    {
        auto it = std::find(m_chain.begin(), m_chain.end(), m_focused);
        if (it != m_chain.end())
            start = (it - m_chain.begin() + 1) % m_chain.size();
    }
    
    // Search for next focusable component
    for (size_t i = 0; i < m_chain.size(); ++i)
    {
        size_t index = (start + i) % m_chain.size();
        Widget* candidate = m_chain[index];
        
        if (candidate->canAcceptFocus() && candidate->acceptsTabFocus())
        {
            setFocus(candidate, reason);
            return true;
        }
    }
    
    return false;
}

bool FocusManager::focusPrevious(FocusReason reason)
{
    if (m_dirty) rebuildFocusChain();
    if (m_chain.empty()) return false;
    
    // Determine starting index (wrap backwards)
    size_t start = m_chain.size() - 1;
    if (m_focused)
    {
        auto it = std::find(m_chain.begin(), m_chain.end(), m_focused);
        if (it != m_chain.end())
        {
            size_t currentIndex = it - m_chain.begin();
            start = (currentIndex == 0) ? m_chain.size() - 1 : currentIndex - 1;
        }
    }
    
    // Search backwards for previous focusable component
    for (size_t i = 0; i < m_chain.size(); ++i)
    {
        size_t index = (start + m_chain.size() - i) % m_chain.size();
        Widget* candidate = m_chain[index];
        
        if (candidate->canAcceptFocus() && candidate->acceptsTabFocus())
        {
            setFocus(candidate, reason);
            return true;
        }
    }
    
    return false;
}

bool FocusManager::focusFirst()
{
    if (m_dirty) rebuildFocusChain();
    
    for (Widget* component : m_chain)
    {
        if (component->canAcceptFocus() && component->acceptsTabFocus())
        {
            setFocus(component, FocusReason::OtherFocusReason);
            return true;
        }
    }
    
    return false;
}

bool FocusManager::focusLast()
{
    if (m_dirty) rebuildFocusChain();
    
    for (auto it = m_chain.rbegin(); it != m_chain.rend(); ++it)
    {
        Widget* component = *it;
        if (component->canAcceptFocus() && component->acceptsTabFocus())
        {
            setFocus(component, FocusReason::OtherFocusReason);
            return true;
        }
    }
    
    return false;
}

//==========================================================================================
// Directional Navigation

bool FocusManager::focusByDirection(FocusDirection direction)
{
    // Must have a focused component to navigate directionally
    if (!m_focused) return focusFirst();
    
    if (m_dirty) rebuildFocusChain();
    
    Rect currentBounds = m_focused->getBounds();
    Widget* best = findBestCandidate(currentBounds, direction);
    
    if (best)
    {
        setFocus(best, FocusReason::OtherFocusReason);
        return true;
    }
    
    return false;
}

Widget* FocusManager::findBestCandidate(const Rect& fromBounds, FocusDirection direction)
{
    Widget* best = nullptr;
    float bestScore = std::numeric_limits<float>::max();
    
    for (Widget* candidate : m_chain)
    {
        // Skip current focused component and non-focusable components
        if (!candidate->canAcceptFocus() || candidate == m_focused) continue;
        if (!candidate->acceptsTabFocus()) continue;
        
        Rect toBounds = candidate->getBounds();
        float score = calculateScore(fromBounds, toBounds, direction);
        
        // Valid score and better than current best
        if (score >= 0 && score < bestScore)
        {
            bestScore = score;
            best = candidate;
        }
    }
    
    return best;
}

float FocusManager::calculateScore(const Rect& from, const Rect& to, FocusDirection direction)
{
    // Calculate center points
    float fromCenterX = from.x + from.width / 2.0f;
    float fromCenterY = from.y + from.height / 2.0f;
    float toCenterX = to.x + to.width / 2.0f;
    float toCenterY = to.y + to.height / 2.0f;
    
    float dx = toCenterX - fromCenterX;
    float dy = toCenterY - fromCenterY;
    
    // Reject candidates in wrong direction
    switch (direction)
    {
        case FocusDirection::Up:    if (dy >= 0) return -1.0f;  break;
        case FocusDirection::Down:  if (dy <= 0) return -1.0f;  break;
        case FocusDirection::Left:  if (dx >= 0) return -1.0f;  break;
        case FocusDirection::Right: if (dx <= 0) return -1.0f;  break;
        default:                    return -1.0f;
    }
    
    // Return Euclidean distance
    return std::sqrt(dx * dx + dy * dy);
}

} // namespace YuchenUI
