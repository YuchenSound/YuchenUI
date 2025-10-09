#pragma once

#include "YuchenUI/core/Types.h"
#include "YuchenUI/events/Event.h"
#include "YuchenUI/windows/Window.h"
#include "YuchenUI/widgets/IInputMethodSupport.h"

#include <vector>

namespace YuchenUI {

class RenderList;
class Window;
class UIComponent;

enum class WindowContentResult {
    None,
    Close,
    Minimize,
    Custom
};

class IWindowContent {
public:
    IWindowContent();
    virtual ~IWindowContent();
    
    virtual void onCreate(Window* window, const Rect& contentArea) = 0;
    virtual void onDestroy();
    virtual void onShow();
    virtual void onHide();
    virtual void onResize(const Rect& newArea);
    virtual void onUpdate();
    virtual void render(RenderList& commandList);
    
    virtual bool handleMouseMove(const Vec2& position);
    virtual bool handleMouseClick(const Vec2& position, bool pressed);
    virtual bool handleKeyEvent(const Event& event);
    virtual bool handleTextInput(const Event& event);
    virtual bool handleScroll(const Event& event);
    
    virtual WindowContentResult getResult() const { return m_result; }
    virtual void setResult(WindowContentResult result) { m_result = result; }

    virtual void* getUserData() const { return m_userData; }
    virtual void setUserData(void* data) { m_userData = data; }
    
    void addComponent(UIComponent* component);
    void removeComponent(UIComponent* component);
    void clearComponents();
    
    void setFocusedComponent(UIComponent* component);
    UIComponent* getFocusedComponent() const { return m_focusedComponent; }
    void cycleFocus(bool forward);
    
    void registerFocusableComponent(UIComponent* component);
    void unregisterFocusableComponent(UIComponent* component);
    
    void markFocusOrderDirty();
    
    virtual void requestTextInput(bool enable);
    void setIMEEnabled(bool enable);
    virtual Rect getInputMethodCursorRect() const;

protected:
    Window* m_window;
    Rect m_contentArea;
    WindowContentResult m_result;
    void* m_userData;
    std::vector<UIComponent*> m_components;
    
    UIComponent* m_focusedComponent;
    std::vector<UIComponent*> m_focusableComponents;
    bool m_focusOrderDirty;
};

}
