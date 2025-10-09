#pragma once

#include "YuchenUI/windows/Window.h"
#include "YuchenUI/core/Types.h"
#include "YuchenUI/events/EventManager.h"
#include "YuchenUI/windows/IWindowContent.h"
#include "YuchenUI/platform/WindowImpl.h"
#include <memory>
#include <functional>

namespace YuchenUI {

class GraphicsContext;
class UIComponent;

using DialogResultCallback = std::function<void(WindowContentResult result, void* userData)>;

enum class WindowState {
    Uninitialized,
    Created,
    RendererReady,
    Shown
};

class BaseWindow : public Window {
public:
    explicit BaseWindow(WindowType type = WindowType::Main);
    virtual ~BaseWindow();

    bool create(int width, int height, const char* title, Window* parent = nullptr) override;
    void destroy() override;
    bool shouldClose() override;
    Vec2 getSize() const override;
    
    Vec2 getMousePosition() const override;
    bool isMousePressed() const override;
    
    void* getNativeWindowHandle() const override;
    Vec2 getWindowPosition() const override;
    
    void enableTextInput() override;
    void disableTextInput() override;
    void setIMEEnabled(bool enabled) override;

    void handleNativeEvent(void* event);
    void renderContent();
    
    void show();
    void hide();
    bool isVisible() const;
    
    void showModal();
    void closeModal();
    void closeWithResult(WindowContentResult result);
    void setResultCallback(DialogResultCallback callback);
    
    void onResize(int width, int height);
    
    template<typename ContentType, typename... Args>
    bool createWithContent(int width, int height, const char* title, Window* parent, Args&&... args) {
        if (!create(width, height, title, parent)) {
            return false;
        }
        
        auto content = std::make_unique<ContentType>(std::forward<Args>(args)...);
        setContent(std::move(content));
        return true;
    }
    
    void setContent(std::unique_ptr<IWindowContent> content);
    IWindowContent* getContent() const;
    
    void captureMouse(UIComponent* component);
    void releaseMouse();
    void handleMarkedText(const char* text, int cursorPos, int selectionLength);
    void handleUnmarkText();
    void handleEvent(const Event& event);
    Rect getInputMethodCursorRect() const;
    
    Vec2 mapToScreen(const Vec2& windowPos) const override;

protected:
    GraphicsContext* getGraphicsContext() { return m_renderer; }
    
    virtual void onWindowReady() {}
    virtual void setupUserInterface();
    
    bool isInState(WindowState state) const { return m_state == state; }
    bool hasReachedState(WindowState state) const { return m_state >= state; }
    
    Rect calculateContentArea() const;
    Vec4 getBackgroundColor() const;

    std::unique_ptr<WindowImpl> m_impl;
    GraphicsContext* m_renderer;
    Window* m_parentWindow;
    WindowType m_windowType;
    
    WindowState m_state;
    
    bool m_shouldClose;
    int m_width;
    int m_height;
    float m_dpiScale;

    std::unique_ptr<EventManager> m_eventManager;
    std::unique_ptr<IWindowContent> m_content;
    bool m_contentCreated;
    
    DialogResultCallback m_resultCallback;
    bool m_isModal;
    
    UIComponent* m_capturedComponent;

private:
    bool initializeRenderer();
    void detectDPIScale();
    void releaseResources();
    
    void transitionToState(WindowState newState);
    bool canTransitionTo(WindowState newState) const;
    
    friend class MetalRenderer;
};

}
