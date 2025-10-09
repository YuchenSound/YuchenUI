#pragma once

#include "YuchenUI/core/Types.h"

namespace YuchenUI {

class Window;
class BaseWindow;

enum class WindowType {
    Main,
    Dialog,
    ToolWindow
};

struct WindowConfig {
    int width;
    int height;
    const char* title;
    Window* parent;
    WindowType type;
    bool resizable;
    bool floating;
    
    WindowConfig(int w, int h, const char* t, Window* p = nullptr, WindowType wt = WindowType::Main)
        : width(w), height(h), title(t), parent(p), type(wt), resizable(true), floating(false) {}
};

class WindowImpl {
public:
    virtual ~WindowImpl() = default;
    
    virtual bool create(const WindowConfig& config) = 0;
    virtual void destroy() = 0;
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void showModal() = 0;
    virtual void closeModal() = 0;
    virtual Vec2 getSize() const = 0;
    virtual Vec2 getPosition() const = 0;
    virtual bool isVisible() const = 0;
    virtual void* getNativeHandle() const = 0;
    
    virtual void setBaseWindow(BaseWindow* baseWindow) = 0;
    virtual void* getRenderSurface() const = 0;
    virtual float getDpiScale() const = 0;
    virtual Vec2 mapToScreen(const Vec2& windowPos) const = 0;
    virtual Rect getInputMethodCursorWindowRect() const = 0;
    virtual void setIMEEnabled(bool enabled) = 0;
};

class WindowImplFactory {
public:
    static WindowImpl* create();
};

}
