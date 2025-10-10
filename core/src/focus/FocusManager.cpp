#include "YuchenUI/focus/FocusManager.h"
#include "YuchenUI/widgets/UIComponent.h"
#include "YuchenUI/core/Assert.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace YuchenUI {

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

void FocusManager::setFocus(UIComponent* component, FocusReason reason)
{
    if (m_focused == component) return;
    if (component) component = component->effectiveFocusWidget();
    if (component && !component->canAcceptFocus()) return;
    if (m_focused) m_focused->notifyFocusOut(reason);
    m_focused = component;
    if (m_focused) m_focused->notifyFocusIn(reason);
}

void FocusManager::clearFocus()
{
    if (m_focused) setFocus(nullptr, FocusReason::OtherFocusReason);
}

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

void FocusManager::registerComponent(UIComponent* component)
{
    YUCHEN_ASSERT(component);
    auto it = std::find(m_all.begin(), m_all.end(), component);
    if (it == m_all.end())
    {
        m_all.push_back(component);
        m_dirty = true;
    }
}

void FocusManager::unregisterComponent(UIComponent* component)
{
    YUCHEN_ASSERT(component);
    
    auto it = std::find(m_all.begin(), m_all.end(), component);
    if (it != m_all.end())
    {
        m_all.erase(it);
        m_dirty = true;
    }
    
    if (m_focused == component) m_focused = nullptr;
}

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

void FocusManager::rebuildFocusChain()
{
    m_chain = m_all;
    std::stable_sort(m_chain.begin(), m_chain.end(),
        [](UIComponent* a, UIComponent* b){
            int orderA = a->getTabOrder();
            int orderB = b->getTabOrder();
            if (orderA < 0 && orderB < 0) return false;
            if (orderA < 0) return false;
            if (orderB < 0) return true;
            return orderA < orderB;
        });
    m_dirty = false;
}

bool FocusManager::focusNext(FocusReason reason)
{
    if (m_dirty) rebuildFocusChain();
    if (m_chain.empty()) return false;
    size_t start = 0;
    if (m_focused)
    {
        auto it = std::find(m_chain.begin(), m_chain.end(), m_focused);
        if (it != m_chain.end())
            start = (it - m_chain.begin() + 1) % m_chain.size();
    }
    for (size_t i = 0; i < m_chain.size(); ++i)
    {
        size_t index = (start + i) % m_chain.size();
        UIComponent* candidate = m_chain[index];
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
    for (size_t i = 0; i < m_chain.size(); ++i)
    {
        size_t index = (start + m_chain.size() - i) % m_chain.size();
        UIComponent* candidate = m_chain[index];
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
    for (UIComponent* component : m_chain)
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
        UIComponent* component = *it;
        if (component->canAcceptFocus() && component->acceptsTabFocus())
        {
            setFocus(component, FocusReason::OtherFocusReason);
            return true;
        }
    }
    return false;
}

bool FocusManager::focusByDirection(FocusDirection direction)
{
    if (!m_focused) return focusFirst();
    if (m_dirty) rebuildFocusChain();
    Rect currentBounds = m_focused->getBounds();
    UIComponent* best = findBestCandidate(currentBounds, direction);
    
    if (best)
    {
        setFocus(best, FocusReason::OtherFocusReason);
        return true;
    }
    
    return false;
}

UIComponent* FocusManager::findBestCandidate(const Rect& fromBounds, FocusDirection direction)
{
    UIComponent* best = nullptr;
    float bestScore = std::numeric_limits<float>::max();
    
    for (UIComponent* candidate : m_chain)
    {
        if (!candidate->canAcceptFocus() || candidate == m_focused) continue;
        if (!candidate->acceptsTabFocus()) continue;
        Rect toBounds = candidate->getBounds();
        float score = calculateScore(fromBounds, toBounds, direction);
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
    float fromCenterX = from.x + from.width / 2.0f;
    float fromCenterY = from.y + from.height / 2.0f;
    float toCenterX = to.x + to.width / 2.0f;
    float toCenterY = to.y + to.height / 2.0f;
    
    float dx = toCenterX - fromCenterX;
    float dy = toCenterY - fromCenterY;
    
    switch (direction)
    {
        case FocusDirection::Up:    if (dy >= 0) return -1.0f;  break;
        case FocusDirection::Down:  if (dy <= 0) return -1.0f;  break;
        case FocusDirection::Left:  if (dx >= 0) return -1.0f;  break;
        case FocusDirection::Right: if (dx <= 0) return -1.0f;  break;
        default:                    return -1.0f;
    }
    return std::sqrt(dx * dx + dy * dy);
}

} // namespace YuchenUI
