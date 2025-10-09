#include "YuchenUI/windows/BaseWindow.h"
#include "YuchenUI/windows/WindowManager.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/rendering/GraphicsContext.h"
#include "YuchenUI/platform/PlatformBackend.h"
#include "YuchenUI/platform/WindowImpl.h"
#include "YuchenUI/events/EventManager.h"
#include "YuchenUI/core/Colors.h"
#include "YuchenUI/theme/ThemeManager.h"
#include "YuchenUI/widgets/UIComponent.h"
#include "YuchenUI/core/Config.h"
#include "YuchenUI/core/Assert.h"

namespace YuchenUI
{

BaseWindow::BaseWindow(WindowType type)
    : m_impl(nullptr)
    , m_renderer(nullptr)
    , m_parentWindow(nullptr)
    , m_windowType(type)
    , m_state(WindowState::Uninitialized)
    , m_shouldClose(false)
    , m_width(0)
    , m_height(0)
    , m_dpiScale(1.0f)
    , m_eventManager(nullptr)
    , m_content(nullptr)
    , m_contentCreated(false)
    , m_resultCallback(nullptr)
    , m_isModal(false)
    , m_capturedComponent(nullptr)
{
    m_impl.reset(WindowImplFactory::create());
    YUCHEN_ASSERT(m_impl);
}

BaseWindow::~BaseWindow()
{
    destroy();
}

bool BaseWindow::create(int width, int height, const char* title, Window* parent)
{
    YUCHEN_ASSERT(width >= Config::Window::MIN_SIZE && width <= Config::Window::MAX_SIZE);
    YUCHEN_ASSERT(height >= Config::Window::MIN_SIZE && height <= Config::Window::MAX_SIZE);
    YUCHEN_ASSERT(title);
    YUCHEN_ASSERT(isInState(WindowState::Uninitialized));

    m_parentWindow = parent;
    m_width = width;
    m_height = height;

    WindowConfig config(width, height, title, parent, m_windowType);

    YUCHEN_ASSERT_MSG(m_impl->create(config), "Failed to create window implementation");
    m_impl->setBaseWindow(this);

    detectDPIScale();

    YUCHEN_ASSERT_MSG(initializeRenderer(), "Failed to initialize renderer");

    m_eventManager.reset(PlatformBackend::createEventManager(m_impl->getNativeHandle()));
    YUCHEN_ASSERT_MSG(m_eventManager, "Failed to create EventManager");

    YUCHEN_ASSERT_MSG(m_eventManager->initialize(), "Failed to initialize EventManager");

    m_eventManager->setEventCallback([this](const Event& event) {
        this->handleEvent(event);
    });

    setupUserInterface();

    transitionToState(WindowState::Created);
    transitionToState(WindowState::RendererReady);

    if (m_windowType == WindowType::Main) {
        show();
    }

    return true;
}

void BaseWindow::destroy()
{
    if (isInState(WindowState::Uninitialized)) return;
    
    if (m_content)
    {
        m_content->onDestroy();
        m_content.reset();
    }
    
    if (m_eventManager)
    {
        m_eventManager->clearEventCallback();
        m_eventManager->destroy();
        m_eventManager.reset();
    }

    releaseResources();
    
    if (m_impl)
    {
        m_impl->destroy();
        m_impl.reset();
    }
    
    m_state = WindowState::Uninitialized;
}

bool BaseWindow::shouldClose()
{
    return m_shouldClose;
}

Vec2 BaseWindow::getSize() const
{
    return m_impl ? m_impl->getSize() : Vec2(m_width, m_height);
}

Vec2 BaseWindow::getMousePosition() const
{
    if (m_eventManager) return m_eventManager->getMousePosition();
    return Vec2();
}

bool BaseWindow::isMousePressed() const
{
    if (m_eventManager) return m_eventManager->isMouseButtonPressed(MouseButton::Left);
    return false;
}

void* BaseWindow::getNativeWindowHandle() const
{
    return m_impl ? m_impl->getNativeHandle() : nullptr;
}

Vec2 BaseWindow::getWindowPosition() const
{
    return m_impl ? m_impl->getPosition() : Vec2();
}

void BaseWindow::show()
{
    if (!m_impl) return;
    
    m_impl->show();
    
    if (!isInState(WindowState::Shown)) {
        transitionToState(WindowState::Shown);
        onWindowReady();
    }
}

void BaseWindow::hide()
{
    if (m_impl) {
        m_impl->hide();
        if (isInState(WindowState::Shown)) {
            m_state = WindowState::RendererReady;
        }
    }
}

bool BaseWindow::isVisible() const
{
    return m_impl ? m_impl->isVisible() : false;
}

void BaseWindow::showModal()
{
    if (!m_impl || m_windowType != WindowType::Dialog) return;
    
    m_isModal = true;
    if (m_content) m_content->onShow();
    
    m_impl->showModal();
    
    if (m_content) m_content->onHide();
    
    if (m_resultCallback)
    {
        void* userData = m_content ? m_content->getUserData() : nullptr;
        m_resultCallback(WindowContentResult::Close, userData);
        m_resultCallback = nullptr;
    }
    
    WindowManager::getInstance().scheduleDialogDestruction(this);
}

void BaseWindow::closeModal()
{
    if (m_isModal && m_impl) {
        m_impl->closeModal();
    }
    m_isModal = false;
}

void BaseWindow::closeWithResult(WindowContentResult result)
{
    if (m_windowType == WindowType::Dialog)
    {
        closeModal();
    }
    else
    {
        m_shouldClose = true;

        WindowManager& wm = WindowManager::getInstance();
        if (wm.isMainWindow(this))
        {
            wm.closeMainWindow(this);
        }
    }
}

void BaseWindow::setResultCallback(DialogResultCallback callback)
{
    m_resultCallback = callback;
}

void BaseWindow::onResize(int width, int height)
{
    if (width != m_width || height != m_height)
    {
        m_width = width;
        m_height = height;
        if (m_renderer) m_renderer->resize(width, height);
    }
}

void BaseWindow::setContent(std::unique_ptr<IWindowContent> content)
{
    if (m_content) m_content->onDestroy();
    
    m_content = std::move(content);
    
    if (m_content && hasReachedState(WindowState::Created) && !m_contentCreated)
    {
        Rect contentArea = calculateContentArea();
        m_content->onCreate(this, contentArea);
        m_contentCreated = true;
    }
}

IWindowContent* BaseWindow::getContent() const
{
    return m_content.get();
}

void BaseWindow::captureMouse(UIComponent* component)
{
    m_capturedComponent = component;
}

void BaseWindow::releaseMouse()
{
    m_capturedComponent = nullptr;
}

void BaseWindow::setupUserInterface()
{
    if (m_content && !m_contentCreated)
    {
        Rect contentArea = calculateContentArea();
        m_content->onCreate(this, contentArea);
        m_contentCreated = true;
    }
}

void BaseWindow::handleNativeEvent(void* event)
{
    YUCHEN_ASSERT(event);
    YUCHEN_ASSERT(m_eventManager);
    YUCHEN_ASSERT(m_eventManager->isInitialized());

    m_eventManager->handleNativeEvent(event);
}

void BaseWindow::renderContent()
{
    if (!m_renderer || !hasReachedState(WindowState::Created)) return;
    
    m_renderer->beginFrame();
    
    RenderList commandList;
    commandList.clear(getBackgroundColor());
    
    if (m_content)
    {
        m_content->onUpdate();
        m_content->render(commandList);
    }
    
    m_renderer->executeRenderCommands(commandList);
    m_renderer->endFrame();
}

bool BaseWindow::initializeRenderer()
{
    WindowManager& windowManager = WindowManager::getInstance();

    m_renderer = PlatformBackend::createRenderer();
    YUCHEN_ASSERT_MSG(m_renderer, "createRenderer returned null");

    void* sharedDevice = windowManager.getSharedRenderDevice();
    YUCHEN_ASSERT_MSG(sharedDevice, "Shared device is null");

    m_renderer->setSharedDevice(sharedDevice);

    bool success = m_renderer->initialize(m_width, m_height, m_dpiScale);
    if (!success) {
        delete m_renderer;
        m_renderer = nullptr;
        YUCHEN_ASSERT_MSG(false, "Renderer initialize failed");
        return false;
    }

    void* surface = m_impl->getRenderSurface();
    YUCHEN_ASSERT_MSG(surface, "Surface is null");

    m_renderer->setSurface(surface);

    return true;
}

void BaseWindow::detectDPIScale()
{
    YUCHEN_ASSERT(m_impl);
    m_dpiScale = m_impl->getDpiScale();
    YUCHEN_ASSERT(m_dpiScale > 0.0f);
}

void BaseWindow::releaseResources()
{
    delete m_renderer;
    m_renderer = nullptr;
}

void BaseWindow::handleEvent(const Event& event)
{
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
        
        if (handled) return;
    }
    
    if (m_content)
    {
        bool handled = false;
        
        switch (event.type)
        {
            case EventType::MouseButtonPressed:
            case EventType::MouseButtonReleased:
                handled = m_content->handleMouseClick(event.mouseButton.position,
                                                     event.type == EventType::MouseButtonPressed);
                break;
                
            case EventType::MouseMoved:
                handled = m_content->handleMouseMove(event.mouseMove.position);
                break;
                
            case EventType::MouseScrolled:
                handled = m_content->handleScroll(event);
                break;
                
            case EventType::KeyPressed:
            case EventType::KeyReleased:
                handled = m_content->handleKeyEvent(event);
                break;
                
            case EventType::TextInput:
                handled = m_content->handleTextInput(event);
                break;
            case EventType::TextComposition:
                handled = m_content->handleTextInput(event);
                break;
                
            default:
                break;
        }
        
        if (handled) return;
    }
    
    switch (event.type)
    {
        case EventType::WindowClosed:
        {
            m_shouldClose = true;
            WindowManager& wm = WindowManager::getInstance();
            if (wm.isMainWindow(this))
            {
                wm.closeMainWindow(this);
            }
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

Rect BaseWindow::calculateContentArea() const
{
    return Rect(0, 0, static_cast<float>(m_width), static_cast<float>(m_height));
}

Vec4 BaseWindow::getBackgroundColor() const
{
    return ThemeManager::getInstance().getCurrentStyle()->getWindowBackground(m_windowType);
}

void BaseWindow::transitionToState(WindowState newState)
{
    YUCHEN_ASSERT_MSG(canTransitionTo(newState), "Invalid state transition");
    m_state = newState;
}

bool BaseWindow::canTransitionTo(WindowState newState) const
{
    return static_cast<int>(newState) == static_cast<int>(m_state) + 1;
}

Window* Window::create()
{
    return new BaseWindow(WindowType::Main);
}

void BaseWindow::enableTextInput() {
    if (m_eventManager) {
        m_eventManager->enableTextInput();
    }
}

void BaseWindow::disableTextInput() {
    if (m_eventManager) {
        m_eventManager->disableTextInput();
    }
}

void BaseWindow::setIMEEnabled(bool enabled) {
    if (m_impl) {
        m_impl->setIMEEnabled(enabled);
    }
}

void BaseWindow::handleMarkedText(const char* text, int cursorPos, int selectionLength) {
    if (m_eventManager) {
        m_eventManager->handleMarkedText(text, cursorPos, selectionLength);
    }
}

void BaseWindow::handleUnmarkText() {
    if (m_eventManager) {
        m_eventManager->handleUnmarkText();
    }
}

Rect BaseWindow::getInputMethodCursorRect() const {
    if (!m_content) {
        return Rect();
    }
    
    return m_content->getInputMethodCursorRect();
}

Vec2 BaseWindow::mapToScreen(const Vec2& windowPos) const {
    return m_impl ? m_impl->mapToScreen(windowPos) : Vec2();
}

}
