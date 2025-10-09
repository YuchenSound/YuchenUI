#pragma once

#include "YuchenUI/focus/FocusPolicy.h"
#include "YuchenUI/core/Types.h"
#include <vector>

namespace YuchenUI {

class UIComponent;
class IWindowContent;

class FocusManager {
public:
    explicit FocusManager(IWindowContent* owner);
    ~FocusManager();
    
    void setFocus(UIComponent* component, FocusReason reason);
    void clearFocus();
    UIComponent* getFocusedComponent() const { return m_focused; }
    
    bool moveFocus(FocusDirection direction);
    
    void registerComponent(UIComponent* component);
    void unregisterComponent(UIComponent* component);
    void markDirty() { m_dirty = true; }
    
    bool handleTabKey(bool shift);
    bool handleDirectionKey(FocusDirection direction);
    
private:
    void rebuildFocusChain();
    bool focusNext(FocusReason reason);
    bool focusPrevious(FocusReason reason);
    bool focusByDirection(FocusDirection direction);
    bool focusFirst();
    bool focusLast();
    
    UIComponent* findBestCandidate(const Rect& fromBounds, FocusDirection direction);
    float calculateScore(const Rect& from, const Rect& to, FocusDirection direction);
    
   // IWindowContent* m_owner;
    UIComponent* m_focused;
    std::vector<UIComponent*> m_all;
    std::vector<UIComponent*> m_chain;
    bool m_dirty;
    
    FocusManager(const FocusManager&) = delete;
    FocusManager& operator=(const FocusManager&) = delete;
};

}
