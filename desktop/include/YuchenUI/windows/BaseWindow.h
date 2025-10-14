#pragma once

#include "YuchenUI/core/Types.h"
#include "YuchenUI/core/UIContext.h"
#include "YuchenUI/core/IUIContent.h"
#include "YuchenUI/platform/ITextInputHandler.h"
#include "YuchenUI/platform/ICoordinateMapper.h"
#include "YuchenUI/platform/WindowImpl.h"
#include "YuchenUI/windows/Window.h"
#include "YuchenUI/events/EventManager.h"

#include <memory>
#include <functional>

namespace YuchenUI {

class IGraphicsBackend;
class UIComponent;
class IFontProvider;

using DialogResultCallback = std::function<void(WindowContentResult result, void* userData)>;

enum class WindowState {
    Uninitialized,
    Created,
    RendererReady,
    Shown
};

class BaseWindow : public Window, public ITextInputHandler, public ICoordinateMapper
{
public:
    explicit BaseWindow (WindowType type = WindowType::Main);
    virtual ~BaseWindow();

    bool create (int width, int height, const char* title, Window* parent = nullptr) override;
    void destroy() override;
    bool shouldClose() override;
    Vec2 getSize() const override;
    Vec2 getMousePosition() const override;
    bool isMousePressed() const override;
    void* getNativeWindowHandle() const override;
    Vec2 getWindowPosition() const override;
    
    void enableTextInput() override;
    void disableTextInput() override;
    void setIMEEnabled (bool enabled) override;
    
    Vec2 mapToScreen(const Vec2& windowPos) const override;

    void handleNativeEvent (void* event);
    void renderContent();
    void show();
    void hide();
    bool isVisible() const;
    
    void showModal();
    void closeModal();
    void closeWithResult (WindowContentResult result);
    void setResultCallback (DialogResultCallback callback);
    
    void onResize (int width, int height);
    
    void setContent (std::unique_ptr<IUIContent> content);
    IUIContent* getContent() const;
    
    void captureMouse (UIComponent* component);
    void releaseMouse();
    
    void handleMarkedText (const char* text, int cursorPos, int selectionLength);
    void handleUnmarkText();
    void handleEvent (const Event& event);
    Rect getInputMethodCursorRect() const;
    
    UIContext& getUIContext() { return m_uiContext; }
    const UIContext& getUIContext() const { return m_uiContext; }
    void setFontProvider(IFontProvider* provider);

    void setAffectsAppLifetime(bool affects);
    bool affectsAppLifetime() const { return m_affectsAppLifetime; }
protected:
    IGraphicsBackend* getGraphicsBackend() { return m_backend.get(); }
    virtual void onWindowReady() {}
    virtual void setupUserInterface();
    bool isInState (WindowState state) const { return m_state == state; }
    bool hasReachedState (WindowState state) const { return m_state >= state; }
    Rect calculateContentArea() const;
    Vec4 getBackgroundColor() const;

    std::unique_ptr<WindowImpl> m_impl;
    std::unique_ptr<IGraphicsBackend> m_backend;
    UIContext m_uiContext;
    Window* m_parentWindow;
    WindowType m_windowType;
    
    WindowState m_state;
    
    bool m_shouldClose;
    int m_width;
    int m_height;
    float m_dpiScale;

    bool m_affectsAppLifetime;

    std::unique_ptr<EventManager> m_eventManager;
    
    DialogResultCallback m_resultCallback;
    bool m_isModal;
    
    UIComponent* m_capturedComponent;

private:
    bool initializeRenderer(IFontProvider* fontProvider);
    void detectDPIScale();
    void releaseResources();
    void transitionToState (WindowState newState);
    bool canTransitionTo (WindowState newState) const;
    
    friend class MetalRenderer;
};

}
