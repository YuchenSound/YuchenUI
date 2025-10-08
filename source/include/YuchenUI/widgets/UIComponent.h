#pragma once

#include "YuchenUI/core/Types.h"
#include "YuchenUI/events/Event.h"

namespace YuchenUI {

class RenderList;
class BaseWindow;
class IWindowContent;
class Menu;

class UIComponent {
public:
    UIComponent() : m_isVisible(true), m_isEnabled(true), m_tabOrder(-1), m_ownerWindow(nullptr), m_ownerContent(nullptr), m_parent(nullptr), m_contextMenu(nullptr), m_drawsFocusIndicator(true) {}
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
    virtual void setVisible(bool visible) { m_isVisible = visible; }
    
    virtual bool isEnabled() const { return m_isEnabled; }
    virtual void setEnabled(bool enabled) { m_isEnabled = enabled; }
    
    virtual void setOwnerWindow(BaseWindow* window) { m_ownerWindow = window; }
    BaseWindow* getOwnerWindow() const { return m_ownerWindow; }
    
    virtual void setOwnerContent(IWindowContent* content) { m_ownerContent = content; }
    IWindowContent* getOwnerContent() const { return m_ownerContent; }
    
    void setParent(UIComponent* parent) { m_parent = parent; }
    UIComponent* getParent() const { return m_parent; }
    
    virtual bool canReceiveFocus() const { return false; }
    virtual void onFocusGained() {}
    virtual void onFocusLost() {}
    
    void requestFocus();
    bool isFocused() const;
    
    void setTabOrder(int order);
    int getTabOrder() const { return m_tabOrder; }
    
    void setContextMenu(Menu* menu) { m_contextMenu = menu; }
    Menu* getContextMenu() const { return m_contextMenu; }
    bool hasContextMenu() const { return m_contextMenu != nullptr; }
    
    void setDrawsFocusIndicator(bool draws) { m_drawsFocusIndicator = draws; }
    bool drawsFocusIndicator() const { return m_drawsFocusIndicator; }

protected:
    void captureMouse();
    void releaseMouse();
    
    virtual void drawFocusIndicator(RenderList& commandList, const Vec2& offset) const;
    virtual CornerRadius getFocusIndicatorCornerRadius() const { return CornerRadius(); }
    
    bool m_isVisible;
    bool m_isEnabled;
    int m_tabOrder;
    BaseWindow* m_ownerWindow;
    IWindowContent* m_ownerContent;
    UIComponent* m_parent;
    Menu* m_contextMenu;
    bool m_drawsFocusIndicator;
};

}
