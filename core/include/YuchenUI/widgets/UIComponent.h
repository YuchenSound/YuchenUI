#pragma once

#include "YuchenUI/core/Types.h"
#include "YuchenUI/events/Event.h"
#include "YuchenUI/focus/FocusPolicy.h"

namespace YuchenUI {

class RenderList;
class UIContext;
class IUIContent;
class Menu;
class FocusManager;

class UIComponent {
public:
    UIComponent();
    virtual ~UIComponent() = default;
    
    virtual void addDrawCommands(RenderList& commandList, const Vec2& offset = Vec2()) const = 0;
    virtual bool handleMouseMove(const Vec2& position, const Vec2& offset = Vec2()) = 0;
    virtual bool handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset = Vec2()) = 0;
    virtual bool handleMouseWheel(const Vec2& delta, const Vec2& position, const Vec2& offset = Vec2()) { return false; }
    virtual bool handleKeyPress(const Event& event) { return false; }
    virtual bool handleTextInput(uint32_t codepoint) { return false; }
    virtual bool handleComposition(const char* text, int cursorPos, int selectionLength) { return false; }
    virtual void update(float deltaTime) {}
    
    virtual const Rect& getBounds() const = 0;
    
    Rect mapToWindow(const Rect& localRect) const;
    
    virtual bool isVisible() const { return m_isVisible; }
    virtual void setVisible(bool visible);
    
    virtual bool isEnabled() const { return m_isEnabled; }
    virtual void setEnabled(bool enabled);
    
    virtual void setOwnerContext(UIContext* context);
    UIContext* getOwnerContext() const { return m_ownerContext; }
    
    virtual void setOwnerContent(IUIContent* content) { m_ownerContent = content; }
    IUIContent* getOwnerContent() const { return m_ownerContent; }
    
    void setParent(UIComponent* parent) { m_parent = parent; }
    UIComponent* getParent() const { return m_parent; }
    
    void setContextMenu(Menu* menu) { m_contextMenu = menu; }
    Menu* getContextMenu() const { return m_contextMenu; }
    bool hasContextMenu() const { return m_contextMenu != nullptr; }
    
    void setFocusPolicy(FocusPolicy policy);
    FocusPolicy getFocusPolicy() const { return m_focusPolicy; }
    
    bool canAcceptFocus() const;
    bool acceptsTabFocus() const;
    bool acceptsClickFocus() const;
    
    bool hasFocus() const { return m_hasFocus; }
    
    void setFocus(FocusReason reason = FocusReason::OtherFocusReason);
    void clearFocus();
    
    void requestFocus(FocusReason reason = FocusReason::OtherFocusReason) {
        setFocus(reason);
    }
    
    void setFocusProxy(UIComponent* proxy);
    UIComponent* getFocusProxy() const { return m_focusProxy; }
    UIComponent* effectiveFocusWidget();
    
    void setTabOrder(int order);
    int getTabOrder() const { return m_tabOrder; }
    
    void setShowFocusIndicator(bool show) { m_showFocusIndicator = show; }
    bool showsFocusIndicator() const { return m_showFocusIndicator; }
    
    virtual bool shouldHandleDirectionKey(FocusDirection direction) const {
        return false;
    }
    
protected:
    void captureMouse();
    void releaseMouse();
    
    virtual void drawFocusIndicator(RenderList& commandList, const Vec2& offset) const;
    virtual CornerRadius getFocusIndicatorCornerRadius() const { return CornerRadius(); }
    
    virtual void focusInEvent(FocusReason reason) {}
    virtual void focusOutEvent(FocusReason reason) {}
    
    bool m_isVisible;
    bool m_isEnabled;
    UIContext* m_ownerContext;
    IUIContent* m_ownerContent;
    UIComponent* m_parent;
    Menu* m_contextMenu;

private:
    void onFocusIn(FocusReason reason);
    void onFocusOut(FocusReason reason);
    
    void scrollIntoViewIfNeeded();
    
    void notifyFocusIn(FocusReason reason);
    void notifyFocusOut(FocusReason reason);
    void setFocusState(bool focused) { m_hasFocus = focused; }
    
    FocusPolicy m_focusPolicy;
    bool m_hasFocus;
    int m_tabOrder;
    UIComponent* m_focusProxy;
    bool m_showFocusIndicator;
    FocusManager* m_focusManagerAccessor;
    
    friend class FocusManager;
    friend class IUIContent;
};

}
