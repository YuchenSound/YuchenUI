#pragma once

#include "YuchenUI/focus/FocusPolicy.h"
#include "YuchenUI/core/Types.h"
#include <vector>
#include <algorithm>

namespace YuchenUI {

class UIComponent;

class FocusManager {
public:
    FocusManager();
    ~FocusManager();
    
    void setFocus(UIComponent* component, FocusReason reason);
    void clearFocus();
    UIComponent* getFocusedComponent() const { return m_focused; }
    
    bool moveFocus(FocusDirection direction);
    
    void registerComponent(UIComponent* component);
    void unregisterComponent(UIComponent* component);
    
    bool handleTabKey(bool shift);
    bool handleDirectionKey(FocusDirection direction);
    
    void markDirty() { m_dirty = true; }

private:
    void rebuildFocusChain();
    bool focusNext(FocusReason reason);
    bool focusPrevious(FocusReason reason);
    bool focusFirst();
    bool focusLast();
    bool focusByDirection(FocusDirection direction);
    
    UIComponent* findBestCandidate(const Rect& fromBounds, FocusDirection direction);
    float calculateScore(const Rect& from, const Rect& to, FocusDirection direction);
    
    UIComponent* m_focused;
    std::vector<UIComponent*> m_all;
    std::vector<UIComponent*> m_chain;
    bool m_dirty;
};

}
