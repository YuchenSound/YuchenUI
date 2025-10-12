/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Core module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

#include "YuchenUI/core/UIContext.h"
#include "YuchenUI/core/IUIContent.h"
#include "YuchenUI/focus/FocusManager.h"
#include "YuchenUI/platform/ICoordinateMapper.h"
#include "YuchenUI/widgets/UIComponent.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/platform/ITextInputHandler.h"
#include "YuchenUI/text/IFontProvider.h"
#include "YuchenUI/text/FontManager.h"
#include "YuchenUI/theme/IThemeProvider.h"
#include "YuchenUI/theme/ThemeManager.h"
#include "YuchenUI/theme/Theme.h"
#include <vector>
#include <algorithm>
#include <chrono>

namespace YuchenUI {

//==========================================================================================
// Implementation details

struct UIContext::Impl
{
    std::unique_ptr<IUIContent> content;
    std::unique_ptr<FocusManager> focusManager;
    std::vector<UIComponent*> components;
    
    Vec2 viewportSize;
    float dpiScale;
    
    UIComponent* capturedComponent;
    ITextInputHandler* textInputHandler;
    ICoordinateMapper* coordinateMapper;
    IFontProvider* fontProvider;
    IThemeProvider* themeProvider;
    
    std::chrono::time_point<std::chrono::high_resolution_clock> lastFrameTime;
    
    Impl(IFontProvider* font, IThemeProvider* theme)
    : content(nullptr)
    , focusManager(nullptr)
    , viewportSize(800, 600)
    , dpiScale(1.0f)
    , capturedComponent(nullptr)
    , textInputHandler(nullptr)
    , coordinateMapper(nullptr)
    , fontProvider(font)
    , themeProvider(theme)
    , lastFrameTime(std::chrono::high_resolution_clock::now())
    {}
};

//==========================================================================================
// Construction

UIContext::UIContext(IFontProvider* fontProvider, IThemeProvider* themeProvider)
: m_impl(std::make_unique<Impl>(fontProvider, themeProvider))
{
    m_impl->focusManager = std::make_unique<FocusManager>();
}

UIContext::~UIContext() = default;

//==========================================================================================
// Font Provider Access

IFontProvider* UIContext::getFontProvider() const
{
    YUCHEN_ASSERT_MSG(m_impl->fontProvider != nullptr,
        "Font provider not set. Call setFontProvider() before using UIContext.");
    return m_impl->fontProvider;
}

void UIContext::setFontProvider(IFontProvider* provider)
{
    YUCHEN_ASSERT_MSG(provider != nullptr, "Font provider cannot be null");
    m_impl->fontProvider = provider;
}

//==========================================================================================
// Theme Provider Access

IThemeProvider* UIContext::getThemeProvider() const
{
    YUCHEN_ASSERT_MSG(m_impl->themeProvider != nullptr,
        "Theme provider not set. Call setThemeProvider() before using UIContext.");
    return m_impl->themeProvider;
}

void UIContext::setThemeProvider(IThemeProvider* provider)
{
    YUCHEN_ASSERT_MSG(provider != nullptr, "Theme provider cannot be null");
    m_impl->themeProvider = provider;
}

UIStyle* UIContext::getCurrentStyle() const
{
    return getThemeProvider()->getCurrentStyle();
}

//==========================================================================================
// Content management

void UIContext::setContent(std::unique_ptr<IUIContent> content)
{
    if (m_impl->content) m_impl->content->onDestroy();
    
    m_impl->content = std::move(content);
    
    if (m_impl->content)
    {
        Rect contentArea(0, 0, m_impl->viewportSize.x, m_impl->viewportSize.y);
        m_impl->content->onCreate(this, contentArea);
    }
}

IUIContent* UIContext::getContent() const
{
    return m_impl->content.get();
}

//==========================================================================================
// Frame lifecycle

void UIContext::render(RenderList& outCommandList)
{
    if (m_impl->content) m_impl->content->render(outCommandList);
}

void UIContext::beginFrame()
{
    auto now = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float>(now - m_impl->lastFrameTime).count();
    m_impl->lastFrameTime = now;
    
    if (m_impl->content) m_impl->content->onUpdate(deltaTime);
}

void UIContext::endFrame() {}

//==========================================================================================
// Mouse event handling

bool UIContext::handleMouseMove(const Vec2& position)
{
    if (m_impl->capturedComponent)
        return m_impl->capturedComponent->handleMouseMove(position);
    
    if (m_impl->content)
        return m_impl->content->handleMouseMove(position);
    
    return false;
}

bool UIContext::handleMouseClick(const Vec2& position, bool pressed)
{
    if (m_impl->capturedComponent)
        return m_impl->capturedComponent->handleMouseClick(position, pressed);
    
    if (m_impl->content)
        return m_impl->content->handleMouseClick(position, pressed);
    
    return false;
}

bool UIContext::handleMouseWheel(const Vec2& delta, const Vec2& position)
{
    if (m_impl->content)
        return m_impl->content->handleMouseWheel(delta, position);
    return false;
}

//==========================================================================================
// Keyboard event handling

bool UIContext::handleKeyEvent(KeyCode key, bool pressed, const KeyModifiers& mods, bool isRepeat)
{
    if (m_impl->content)
    {
        Event event;
        event.type = pressed ? EventType::KeyPressed : EventType::KeyReleased;
        event.key.key = key;
        event.key.modifiers = mods;
        event.key.isRepeat = isRepeat;
        
        return m_impl->content->handleKeyEvent(event);
    }
    return false;
}

bool UIContext::handleTextInput(uint32_t codepoint)
{
    if (m_impl->content) {
        Event event;
        event.type = EventType::TextInput;
        event.textInput.codepoint = codepoint;
        
        return m_impl->content->handleTextInput(event);
    }
    return false;
}

bool UIContext::handleTextComposition(const char* text, int cursorPos, int selectionLength)
{
    if (m_impl->content)
    {
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

//==========================================================================================
// Viewport and scaling

void UIContext::setViewportSize(const Vec2& size)
{
    m_impl->viewportSize = size;
    
    if (m_impl->content)
    {
        Rect newArea(0, 0, size.x, size.y);
        m_impl->content->onResize(newArea);
    }
}

Vec2 UIContext::getViewportSize() const
{
    return m_impl->viewportSize;
}

void UIContext::setDPIScale(float scale)
{
    m_impl->dpiScale = scale;
}

float UIContext::getDPIScale() const
{
    return m_impl->dpiScale;
}

//==========================================================================================
// Focus management

FocusManager& UIContext::getFocusManager()
{
    return *m_impl->focusManager;
}

const FocusManager& UIContext::getFocusManager() const
{
    return *m_impl->focusManager;
}

//==========================================================================================
// Mouse capture

void UIContext::captureMouse(UIComponent* component)
{
    m_impl->capturedComponent = component;
}

void UIContext::releaseMouse()
{
    m_impl->capturedComponent = nullptr;
}

UIComponent* UIContext::getCapturedComponent() const
{
    return m_impl->capturedComponent;
}

//==========================================================================================
// IME support

Rect UIContext::getInputMethodCursorRect() const
{
    if (m_impl->content)
        return m_impl->content->getInputMethodCursorRect();
    return Rect();
}

//==========================================================================================
// Component management

void UIContext::addComponent(UIComponent* component)
{
    auto it = std::find(m_impl->components.begin(), m_impl->components.end(), component);
    if (it == m_impl->components.end())
        m_impl->components.push_back(component);
}

void UIContext::removeComponent(UIComponent* component)
{
    auto it = std::find(m_impl->components.begin(), m_impl->components.end(), component);
    if (it != m_impl->components.end())
        m_impl->components.erase(it);
}

//==========================================================================================
// Text input control

void UIContext::setTextInputHandler(ITextInputHandler* handler)
{
    m_impl->textInputHandler = handler;
}

void UIContext::requestTextInput(bool enable)
{
    if (!m_impl->textInputHandler) return;
    
    if (enable)
    {
        m_impl->textInputHandler->enableTextInput();
    }
    else
    {
        m_impl->textInputHandler->disableTextInput();
    }
}

void UIContext::setIMEEnabled(bool enabled)
{
    if (m_impl->textInputHandler)
        m_impl->textInputHandler->setIMEEnabled(enabled);
}

//==========================================================================================
// Coordinate mapping

void UIContext::setCoordinateMapper(ICoordinateMapper* mapper)
{
    m_impl->coordinateMapper = mapper;
}

Vec2 UIContext::mapToScreen(const Vec2& windowPos) const
{
    if (!m_impl->coordinateMapper) return windowPos;
    return m_impl->coordinateMapper->mapToScreen(windowPos);
}

} // namespace YuchenUI
