#pragma once

#include "YuchenUI/core/Types.h"

namespace YuchenUI {

class GraphicsContext;

class Window {
public:
    virtual ~Window() = default;
    
    virtual bool create(int width, int height, const char* title, Window* parent = nullptr) = 0;
    virtual void destroy() = 0;
    virtual bool shouldClose() = 0;
    virtual Vec2 getSize() const = 0;
    
    virtual Vec2 getMousePosition() const = 0;
    virtual bool isMousePressed() const = 0;
    
    virtual void* getNativeWindowHandle() const = 0;
    virtual Vec2 getWindowPosition() const = 0;
    
    virtual void enableTextInput() = 0;
    virtual void disableTextInput() = 0;
    virtual void setIMEEnabled(bool enabled) = 0;
    
    virtual Vec2 mapToScreen(const Vec2& windowPos) const = 0;
    
    static Window* create();
};

}
