#pragma once

#include "YuchenUI/core/Types.h"
#include "YuchenUI/events/Event.h"
#include <memory>

namespace YuchenUI {

class IUIContent;
class IGraphicsBackend;
class FocusManager;
class UIComponent;
class ITextInputHandler;
class ICoordinateMapper;
class RenderList;

class UIContext {
public:
    UIContext();
    ~UIContext();
    
    void setContent(std::unique_ptr<IUIContent> content);
    IUIContent* getContent() const;
    
    void beginFrame();
    void render(RenderList& outCommandList);
    void endFrame();
    
    bool handleMouseMove(const Vec2& position);
    bool handleMouseClick(const Vec2& position, bool pressed);
    bool handleMouseWheel(const Vec2& delta, const Vec2& position);
    bool handleKeyEvent(KeyCode key, bool pressed, const KeyModifiers& mods, bool isRepeat);
    bool handleTextInput(uint32_t codepoint);
    bool handleTextComposition(const char* text, int cursorPos, int selectionLength);
    
    void setViewportSize(const Vec2& size);
    Vec2 getViewportSize() const;
    void setDPIScale(float scale);
    float getDPIScale() const;
    
    FocusManager& getFocusManager();
    const FocusManager& getFocusManager() const;
    
    void captureMouse(UIComponent* component);
    void releaseMouse();
    UIComponent* getCapturedComponent() const;
    
    Rect getInputMethodCursorRect() const;
    
    void addComponent(UIComponent* component);
    void removeComponent(UIComponent* component);
    
    void setTextInputHandler(ITextInputHandler* handler);
    void requestTextInput(bool enable);
    void setIMEEnabled(bool enabled);
    
    void setCoordinateMapper(ICoordinateMapper* mapper);
    Vec2 mapToScreen(const Vec2& windowPos) const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    
    UIContext(const UIContext&) = delete;
    UIContext& operator=(const UIContext&) = delete;
};

}
