/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Windows module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

//==========================================================================================
/** @file BaseWindow.cpp
    
    Cross-platform base window implementation.
    
    This file implements the core window functionality that is shared across all platforms.
    It manages window lifecycle, rendering, event handling, and UI content integration.
    
    Architecture:
    - BaseWindow is a concrete implementation of the abstract Window interface
    - Platform-specific behavior delegated to WindowImpl (created by factory)
    - Rendering abstracted through IGraphicsBackend interface
    - Event handling abstracted through EventManager
    - UI content managed through UIContext which owns IUIContent
    
    Key responsibilities:
    - Window creation, destruction, and state management
    - Renderer initialization and frame rendering
    - Event routing to UI components and content
    - Mouse capture for drag operations
    - Text input and IME coordination
    - Modal dialog lifecycle management
    - DPI scaling detection and propagation
    
    Threading model:
    - All methods must be called from the main thread
    - No internal synchronization (single-threaded by design)
    
    State machine:
    - Uninitialized -> Created -> RendererReady -> Shown
    - Transitions enforced through assertions in debug builds
*/

#include "YuchenUI/windows/BaseWindow.h"
#include "YuchenUI/windows/WindowManager.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/rendering/IGraphicsBackend.h"
#include "YuchenUI/platform/PlatformBackend.h"
#include "YuchenUI/platform/WindowImpl.h"
#include "YuchenUI/events/EventManager.h"
#include "YuchenUI/theme/ThemeManager.h"
#include "YuchenUI/widgets/UIComponent.h"
#include "YuchenUI/core/Config.h"
#include "YuchenUI/core/Assert.h"

namespace YuchenUI
{

//==========================================================================================
// Construction and Destruction

BaseWindow::BaseWindow(WindowType type)
    : m_impl(nullptr)
    , m_backend(nullptr)
    , m_uiContext()
    , m_parentWindow(nullptr)
    , m_windowType(type)
    , m_state(WindowState::Uninitialized)
    , m_shouldClose(false)
    , m_width(0)
    , m_height(0)
    , m_dpiScale(1.0f)
    , m_eventManager(nullptr)
    , m_resultCallback(nullptr)
    , m_isModal(false)
    , m_capturedComponent(nullptr)
    , m_affectsAppLifetime(type == WindowType::Main)
{
    m_impl.reset(WindowImplFactory::create());
    YUCHEN_ASSERT(m_impl);
}

BaseWindow::~BaseWindow()
{
    destroy();
}

//==========================================================================================
// Window Lifecycle

bool BaseWindow::create(int width, int height, const char* title, Window* parent)
{
    YUCHEN_ASSERT(width >= Config::Window::MIN_SIZE && width <= Config::Window::MAX_SIZE);
    YUCHEN_ASSERT(height >= Config::Window::MIN_SIZE && height <= Config::Window::MAX_SIZE);
    YUCHEN_ASSERT(title);
    YUCHEN_ASSERT(isInState(WindowState::Uninitialized));

    m_parentWindow = parent;
    m_width = width;
    m_height = height;

    // Create native platform window
    WindowConfig config(width, height, title, parent, m_windowType);
    YUCHEN_ASSERT_MSG(m_impl->create(config), "Failed to create window implementation");
    m_impl->setBaseWindow(this);

    // Detect DPI scale for this window
    detectDPIScale();

    // Create and initialize event manager for this window
    m_eventManager.reset(PlatformBackend::createEventManager(m_impl->getNativeHandle()));
    YUCHEN_ASSERT_MSG(m_eventManager, "Failed to create EventManager");
    YUCHEN_ASSERT_MSG(m_eventManager->initialize(), "Failed to initialize EventManager");

    // Set up event callback to route events to this window
    m_eventManager->setEventCallback([this](const Event& event) {
        this->handleEvent(event);
    });

    // Set up UI layer (content may be set later by user)
    setupUserInterface();

    // Transition to Created state
    // Note: RendererReady transition happens in setFontProvider()
    transitionToState(WindowState::Created);

    return true;
}

void BaseWindow::destroy()
{
    if (isInState(WindowState::Uninitialized))
        return;
    
    // Destroy UI content first
    IUIContent* content = m_uiContext.getContent();
    if (content)
    {
        content->onDestroy();
    }
    m_uiContext.setContent(nullptr);
    
    // Clean up event manager
    if (m_eventManager)
    {
        m_eventManager->clearEventCallback();
        m_eventManager->destroy();
        m_eventManager.reset();
    }

    // Release rendering resources
    releaseResources();
    
    // Destroy platform window
    if (m_impl)
    {
        m_impl->destroy();
        m_impl.reset();
    }
    
    m_state = WindowState::Uninitialized;
}

//==========================================================================================
// Window Visibility

void BaseWindow::show()
{
    if (!m_impl)
        return;
    
    m_impl->show();
    
    if (!isInState(WindowState::Shown))
    {
        transitionToState(WindowState::Shown);
        onWindowReady();
    }
}

void BaseWindow::hide()
{
    if (m_impl)
    {
        m_impl->hide();
        if (isInState(WindowState::Shown))
        {
            m_state = WindowState::RendererReady;
        }
    }
}

bool BaseWindow::isVisible() const
{
    return m_impl ? m_impl->isVisible() : false;
}

bool BaseWindow::shouldClose()
{
    return m_shouldClose;
}

//==========================================================================================
// Modal Dialog Management

void BaseWindow::showModal()
{
    if (!m_impl || m_windowType != WindowType::Dialog)
        return;
    
    m_isModal = true;
    
    // Notify content it's being shown
    IUIContent* content = m_uiContext.getContent();
    if (content)
        content->onShow();
    
    // Enter platform modal loop (blocks until closeModal called)
    m_impl->showModal();
    
    // Notify content it's being hidden
    if (content)
        content->onHide();
    
    // Invoke result callback if registered
    if (m_resultCallback)
    {
        void* userData = content ? content->getUserData() : nullptr;
        m_resultCallback(content ? content->getResult() : WindowContentResult::Close, userData);
        m_resultCallback = nullptr;
    }
    
    // Schedule destruction after modal loop exits
    WindowManager::getInstance().scheduleDialogDestruction(this);
}

void BaseWindow::closeModal()
{
    if (m_isModal && m_impl)
    {
        // Exit platform modal loop
        m_impl->closeModal();
    }
    m_isModal = false;
}

//==========================================================================================
// Window Closing

void BaseWindow::closeWithResult(WindowContentResult result)
{
    WindowManager& wm = WindowManager::getInstance();
    
    if (m_windowType == WindowType::Dialog)
    {
        closeModal();
    }
    else
    {
        m_shouldClose = true;
        wm.closeWindow(this);
    }
}

void BaseWindow::setResultCallback(DialogResultCallback callback)
{
    m_resultCallback = callback;
}

//==========================================================================================
// Content Management

void BaseWindow::setContent(std::unique_ptr<IUIContent> content)
{
    // Set up close callback so content can request window closure
    if (content) {
        content->setCloseCallback([this](WindowContentResult result) {
            if (m_windowType == WindowType::Dialog) {
                closeModal();
            } else {
                closeWithResult(result);
            }
        });
    }
    
    // Transfer ownership to UIContext
    m_uiContext.setContent(std::move(content));
}

IUIContent* BaseWindow::getContent() const
{
    return m_uiContext.getContent();
}

//==========================================================================================
// Window Properties

Vec2 BaseWindow::getSize() const
{
    return m_impl ? m_impl->getSize() : Vec2(m_width, m_height);
}

void BaseWindow::onResize(int width, int height)
{
    if (width != m_width || height != m_height)
    {
        m_width = width;
        m_height = height;
        
        // Update renderer viewport
        if (m_backend) {
            m_backend->resize(width, height);
        }
        
        // Update UI layout
        m_uiContext.setViewportSize(Vec2(width, height));
    }
}

Vec2 BaseWindow::getWindowPosition() const
{
    return m_impl ? m_impl->getPosition() : Vec2();
}

void* BaseWindow::getNativeWindowHandle() const
{
    return m_impl ? m_impl->getNativeHandle() : nullptr;
}

Vec2 BaseWindow::mapToScreen(const Vec2& windowPos) const {
    return m_impl ? m_impl->mapToScreen(windowPos) : windowPos;
}

//==========================================================================================
// Rendering

Rect BaseWindow::calculateContentArea() const
{
    return Rect(0, 0, static_cast<float>(m_width), static_cast<float>(m_height));
}

Vec4 BaseWindow::getBackgroundColor() const
{
    return m_uiContext.getCurrentStyle()->getWindowBackground(m_windowType);
}

void BaseWindow::renderContent()
{
    if (!m_backend || !hasReachedState(WindowState::Created))
        return;
    
    // Begin rendering frame
    m_backend->beginFrame();
    
    // Build command list
    RenderList commandList;
    commandList.clear(getBackgroundColor());
    
    // Let UI context render its content
    m_uiContext.beginFrame();
    m_uiContext.render(commandList);
    m_uiContext.endFrame();
    
    // Execute rendering commands and present
    m_backend->executeRenderCommands(commandList);
    m_backend->endFrame();
}

//==========================================================================================
// Renderer Initialization

bool BaseWindow::initializeRenderer(IFontProvider* fontProvider)
{
    YUCHEN_ASSERT_MSG(fontProvider, "Font provider cannot be null");
    YUCHEN_ASSERT_MSG(!m_backend, "Renderer already initialized");
    
    // Create platform-specific graphics backend
    m_backend.reset(PlatformBackend::createGraphicsBackend());
    YUCHEN_ASSERT_MSG(m_backend, "createGraphicsBackend returned null");

    // Get render surface from platform implementation
    void* surface = m_impl->getRenderSurface();
    YUCHEN_ASSERT_MSG(surface, "Surface is null");

    // Initialize backend with window surface, DPI, and font provider
    bool success = m_backend->initialize(surface, m_width, m_height, m_dpiScale, fontProvider);
    YUCHEN_ASSERT_MSG(success, "Backend initialize failed");
    
    // Configure UI context
    m_uiContext.setViewportSize(Vec2(m_width, m_height));
    m_uiContext.setDPIScale(m_dpiScale);
    
    // Register this window as text input handler
    m_uiContext.setTextInputHandler(this);
    
    // Register this window as coordinate mapper
    m_uiContext.setCoordinateMapper(this);

    return success;
}

void BaseWindow::detectDPIScale()
{
    YUCHEN_ASSERT(m_impl);
    m_dpiScale = m_impl->getDpiScale();
    YUCHEN_ASSERT(m_dpiScale > 0.0f);
}

void BaseWindow::releaseResources()
{
    m_backend.reset();
}

//==========================================================================================
// Event Handling

void BaseWindow::handleNativeEvent(void* event)
{
    YUCHEN_ASSERT(event);
    YUCHEN_ASSERT(m_eventManager);
    YUCHEN_ASSERT(m_eventManager->isInitialized());

    // Forward platform event to event manager for translation
    m_eventManager->handleNativeEvent(event);
}

void BaseWindow::handleEvent(const Event& event)
{
    // If a component has mouse capture, route mouse events to it first
    if (m_capturedComponent)
    {
        bool handled = false;
        
        switch (event.type)
        {
            case EventType::MouseButtonPressed:
            case EventType::MouseButtonReleased:
                handled = m_capturedComponent->handleMouseClick(
                    event.mouseButton.position,
                    event.type == EventType::MouseButtonPressed
                );
                break;
                
            case EventType::MouseMoved:
                handled = m_capturedComponent->handleMouseMove(event.mouseMove.position);
                break;
                
            case EventType::MouseScrolled:
                handled = m_capturedComponent->handleMouseWheel(
                    event.mouseScroll.delta,
                    event.mouseScroll.position
                );
                break;
                
            default:
                break;
        }
        
        if (handled)
            return;
    }
    
    // Try to handle event through UI context
    bool handled = false;
    
    switch (event.type)
    {
        case EventType::MouseButtonPressed:
        case EventType::MouseButtonReleased:
            handled = m_uiContext.handleMouseClick(
                event.mouseButton.position,
                event.type == EventType::MouseButtonPressed
            );
            break;
            
        case EventType::MouseMoved:
            handled = m_uiContext.handleMouseMove(event.mouseMove.position);
            break;
            
        case EventType::MouseScrolled:
            handled = m_uiContext.handleMouseWheel(
                event.mouseScroll.delta,
                event.mouseScroll.position
            );
            break;
            
        case EventType::KeyPressed:
        case EventType::KeyReleased:
            handled = m_uiContext.handleKeyEvent(
                event.key.key,
                event.type == EventType::KeyPressed,
                event.key.modifiers,
                event.key.isRepeat
            );
            break;
            
        case EventType::TextInput:
            handled = m_uiContext.handleTextInput(event.textInput.codepoint);
            break;
            
        case EventType::TextComposition:
            handled = m_uiContext.handleTextComposition(
                event.textComposition.text,
                event.textComposition.cursorPosition,
                event.textComposition.selectionLength
            );
            break;
            
        default:
            break;
    }
    
    // If UI didn't handle event, process window-level events
    if (handled)
        return;
    
    switch (event.type)
    {
        case EventType::WindowClosed:
        {
            m_shouldClose = true;
            WindowManager::getInstance().closeWindow(this);
            break;
        }
        case EventType::WindowResized:
            YUCHEN_ASSERT(event.window.isValid());
            onResize(static_cast<int>(event.window.size.x),
                     static_cast<int>(event.window.size.y));
            break;
        default:
            break;
    }
}

//==========================================================================================
// Input State Queries

Vec2 BaseWindow::getMousePosition() const
{
    if (m_eventManager)
        return m_eventManager->getMousePosition();
    return Vec2();
}

bool BaseWindow::isMousePressed() const
{
    if (m_eventManager)
        return m_eventManager->isMouseButtonPressed(MouseButton::Left);
    return false;
}

//==========================================================================================
// Mouse Capture

void BaseWindow::captureMouse(UIComponent* component)
{
    m_capturedComponent = component;
}

void BaseWindow::releaseMouse()
{
    m_capturedComponent = nullptr;
}

//==========================================================================================
// Text Input Management

void BaseWindow::enableTextInput()
{
    if (m_eventManager)
    {
        m_eventManager->enableTextInput();
    }
}

void BaseWindow::disableTextInput()
{
    if (m_eventManager)
    {
        m_eventManager->disableTextInput();
    }
}

void BaseWindow::setIMEEnabled(bool enabled)
{
    if (m_impl)
    {
        m_impl->setIMEEnabled(enabled);
    }
}

void BaseWindow::handleMarkedText(const char* text, int cursorPos, int selectionLength)
{
    if (m_eventManager)
    {
        m_eventManager->handleMarkedText(text, cursorPos, selectionLength);
    }
}

void BaseWindow::handleUnmarkText()
{
    if (m_eventManager)
    {
        m_eventManager->handleUnmarkText();
    }
}

Rect BaseWindow::getInputMethodCursorRect() const
{
    return m_uiContext.getInputMethodCursorRect();
}

//==========================================================================================
// Font Provider Injection

void BaseWindow::setFontProvider(IFontProvider* provider)
{
    YUCHEN_ASSERT_MSG(provider, "Font provider cannot be null");
    YUCHEN_ASSERT_MSG(isInState(WindowState::Created), "Window must be in Created state");
    YUCHEN_ASSERT_MSG(!m_backend, "Renderer already initialized");
    
    // Initialize renderer with font provider
    if (!initializeRenderer(provider))
    {
        YUCHEN_ASSERT_MSG(false, "Failed to initialize renderer with font provider");
        return;
    }
    
    // Inject font provider into UI context
    m_uiContext.setFontProvider(provider);
    
    // Transition to RendererReady state
    transitionToState(WindowState::RendererReady);
    
    // Show main windows immediately after renderer is ready
    if (m_windowType == WindowType::Main)
    {
        show();
    }
}

//==========================================================================================
// State Management

void BaseWindow::transitionToState(WindowState newState)
{
    YUCHEN_ASSERT_MSG(canTransitionTo(newState), "Invalid state transition");
    m_state = newState;
}

bool BaseWindow::canTransitionTo(WindowState newState) const
{
    return static_cast<int>(newState) == static_cast<int>(m_state) + 1;
}

//==========================================================================================
// Window Factory

Window* Window::create()
{
    return new BaseWindow(WindowType::Main);
}

//==========================================================================================
// UI Setup

void BaseWindow::setupUserInterface()
{
    // UIContext internally manages content, no additional setup needed at window level
}

//==========================================================================================
// Application Lifetime Control

void BaseWindow::setAffectsAppLifetime(bool affects)
{
    m_affectsAppLifetime = affects;
}


} // namespace YuchenUI
