#include "YuchenUI/core/UIContext.h"
#include "YuchenUI/core/IUIContent.h"
#include "YuchenUI/focus/FocusManager.h"
#include "YuchenUI/platform/ICoordinateMapper.h"
#include "YuchenUI/widgets/UIComponent.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/platform/ITextInputHandler.h"
#include <vector>
#include <algorithm>
#include <chrono>

namespace YuchenUI {

struct UIContext::Impl {
    std::unique_ptr<IUIContent> content;
    std::unique_ptr<FocusManager> focusManager;
    std::vector<UIComponent*> components;
    
    Vec2 viewportSize;
    float dpiScale;
    
    UIComponent* capturedComponent;
    ITextInputHandler* textInputHandler;
    ICoordinateMapper* coordinateMapper;
    
    std::chrono::time_point<std::chrono::high_resolution_clock> lastFrameTime;
    
    Impl()
        : content(nullptr)
        , focusManager(nullptr)
        , viewportSize(800, 600)
        , dpiScale(1.0f)
        , capturedComponent(nullptr)
        , textInputHandler(nullptr)
        , coordinateMapper(nullptr)
        , lastFrameTime(std::chrono::high_resolution_clock::now())
    {}
};

UIContext::UIContext()
    : m_impl(std::make_unique<Impl>())
{
    m_impl->focusManager = std::make_unique<FocusManager>();
}

UIContext::~UIContext() = default;

void UIContext::setContent(std::unique_ptr<IUIContent> content) {
    if (m_impl->content) {
        m_impl->content->onDestroy();
    }
    
    m_impl->content = std::move(content);
    
    if (m_impl->content) {
        Rect contentArea(0, 0, m_impl->viewportSize.x, m_impl->viewportSize.y);
        m_impl->content->onCreate(this, contentArea);
    }
}

IUIContent* UIContext::getContent() const {
    return m_impl->content.get();
}

void UIContext::render(RenderList& outCommandList) {
    if (m_impl->content) {
        m_impl->content->render(outCommandList);
    }
}

void UIContext::beginFrame() {
    auto now = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float>(now - m_impl->lastFrameTime).count();
    m_impl->lastFrameTime = now;
    
    if (m_impl->content) {
        m_impl->content->onUpdate(deltaTime);
    }
}

void UIContext::endFrame() {
}

bool UIContext::handleMouseMove(const Vec2& position) {
    if (m_impl->capturedComponent) {
        return m_impl->capturedComponent->handleMouseMove(position);
    }
    
    if (m_impl->content) {
        return m_impl->content->handleMouseMove(position);
    }
    
    return false;
}

bool UIContext::handleMouseClick(const Vec2& position, bool pressed) {
    if (m_impl->capturedComponent) {
        return m_impl->capturedComponent->handleMouseClick(position, pressed);
    }
    
    if (m_impl->content) {
        return m_impl->content->handleMouseClick(position, pressed);
    }
    
    return false;
}

bool UIContext::handleMouseWheel(const Vec2& delta, const Vec2& position) {
    if (m_impl->content) {
        return m_impl->content->handleMouseWheel(delta, position);
    }
    return false;
}

bool UIContext::handleKeyEvent(KeyCode key, bool pressed, const KeyModifiers& mods, bool isRepeat) {
    if (m_impl->content) {
        Event event;
        event.type = pressed ? EventType::KeyPressed : EventType::KeyReleased;
        event.key.key = key;
        event.key.modifiers = mods;
        event.key.isRepeat = isRepeat;
        
        return m_impl->content->handleKeyEvent(event);
    }
    return false;
}

bool UIContext::handleTextInput(uint32_t codepoint) {
    if (m_impl->content) {
        Event event;
        event.type = EventType::TextInput;
        event.textInput.codepoint = codepoint;
        
        return m_impl->content->handleTextInput(event);
    }
    return false;
}

bool UIContext::handleTextComposition(const char* text, int cursorPos, int selectionLength) {
    if (m_impl->content) {
        Event event;
        event.type = EventType::TextComposition;
        strncpy(event.textComposition.text, text, 255);
        event.textComposition.text[255] = '\0';
        event.textComposition.cursorPosition = cursorPos;
        event.textComposition.selectionLength = selectionLength;
        
        return m_impl->content->handleTextInput(event);
    }
    return false;
}

void UIContext::setViewportSize(const Vec2& size) {
    m_impl->viewportSize = size;
    
    if (m_impl->content) {
        Rect newArea(0, 0, size.x, size.y);
        m_impl->content->onResize(newArea);
    }
}

Vec2 UIContext::getViewportSize() const {
    return m_impl->viewportSize;
}

void UIContext::setDPIScale(float scale) {
    m_impl->dpiScale = scale;
}

float UIContext::getDPIScale() const {
    return m_impl->dpiScale;
}

FocusManager& UIContext::getFocusManager() {
    return *m_impl->focusManager;
}

const FocusManager& UIContext::getFocusManager() const {
    return *m_impl->focusManager;
}

void UIContext::captureMouse(UIComponent* component) {
    m_impl->capturedComponent = component;
}

void UIContext::releaseMouse() {
    m_impl->capturedComponent = nullptr;
}

UIComponent* UIContext::getCapturedComponent() const {
    return m_impl->capturedComponent;
}

Rect UIContext::getInputMethodCursorRect() const {
    if (m_impl->content) {
        return m_impl->content->getInputMethodCursorRect();
    }
    return Rect();
}

void UIContext::addComponent(UIComponent* component) {
    auto it = std::find(m_impl->components.begin(), m_impl->components.end(), component);
    if (it == m_impl->components.end()) {
        m_impl->components.push_back(component);
    }
}

void UIContext::removeComponent(UIComponent* component) {
    auto it = std::find(m_impl->components.begin(), m_impl->components.end(), component);
    if (it != m_impl->components.end()) {
        m_impl->components.erase(it);
    }
}

void UIContext::setTextInputHandler(ITextInputHandler* handler) {
    m_impl->textInputHandler = handler;
}

void UIContext::requestTextInput(bool enable) {
    if (!m_impl->textInputHandler) return;
    
    if (enable) {
        m_impl->textInputHandler->enableTextInput();
    } else {
        m_impl->textInputHandler->disableTextInput();
    }
}

void UIContext::setIMEEnabled(bool enabled) {
    if (m_impl->textInputHandler) {
        m_impl->textInputHandler->setIMEEnabled(enabled);
    }
}

void UIContext::setCoordinateMapper(ICoordinateMapper* mapper) {
    m_impl->coordinateMapper = mapper;
}

Vec2 UIContext::mapToScreen(const Vec2& windowPos) const {
    if (!m_impl->coordinateMapper) {
        return windowPos;
    }
    return m_impl->coordinateMapper->mapToScreen(windowPos);
}
}
