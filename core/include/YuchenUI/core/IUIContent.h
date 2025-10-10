#pragma once

#include "YuchenUI/core/Types.h"
#include "YuchenUI/events/Event.h"
#include <vector>
#include <memory>
#include <functional>

namespace YuchenUI {

class UIContext;
class RenderList;
class UIComponent;

enum class WindowContentResult {
    None,
    Close,
    Minimize,
    Custom
};

using ContentCloseCallback = std::function<void(WindowContentResult result)>;

class IUIContent {
public:
    IUIContent();
    virtual ~IUIContent();
    
    virtual void onCreate(UIContext* context, const Rect& contentArea) = 0;
    virtual void onDestroy() {}
    virtual void onResize(const Rect& newArea) {}
    virtual void onUpdate(float deltaTime) {}
    virtual void render(RenderList& commandList) = 0;
    
    virtual void onShow() {}
    virtual void onHide() {}
    
    virtual bool handleMouseMove(const Vec2& position);
    virtual bool handleMouseClick(const Vec2& position, bool pressed);
    virtual bool handleMouseWheel(const Vec2& delta, const Vec2& position);
    virtual bool handleKeyEvent(const Event& event);
    virtual bool handleTextInput(const Event& event);
    
    virtual Rect getInputMethodCursorRect() const;
    
    virtual WindowContentResult getResult() const { return m_result; }
    virtual void setResult(WindowContentResult result) { m_result = result; }
    
    virtual void* getUserData() const { return m_userData; }
    virtual void setUserData(void* data) { m_userData = data; }
    
    void setCloseCallback(ContentCloseCallback callback) { m_closeCallback = callback; }
    
    void addComponent(UIComponent* component);
    void removeComponent(UIComponent* component);
    void clearComponents();
    
    UIComponent* getFocusedComponent() const;
    void registerFocusableComponent(UIComponent* component);
    void unregisterFocusableComponent(UIComponent* component);

protected:
    void requestClose(WindowContentResult result);
    
    UIContext* m_context;
    Rect m_contentArea;
    WindowContentResult m_result;
    void* m_userData;
    std::vector<UIComponent*> m_components;
    ContentCloseCallback m_closeCallback;
    
private:
    bool handleMouseEvent(const Vec2& position, bool pressed, bool isMoveEvent);
};

}
