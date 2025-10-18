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

#include "YuchenUI/windows/WindowManager.h"
#include "YuchenUI/windows/BaseWindow.h"
#include "YuchenUI/windows/Window.h"
#include "YuchenUI/platform/PlatformBackend.h"
#include "YuchenUI/menu/IMenuBackend.h"
#include "YuchenUI/core/Assert.h"
#include "YuchenUI/core/Config.h"
#include "WindowManagerPlatform.h"
#include <iostream>
#include <algorithm>

namespace YuchenUI
{

// Forward declaration of platform-specific menu backend registration
namespace Platform {
    void registerMenuBackend();
}

WindowManager* WindowManager::s_instance = nullptr;

//==========================================================================================
// Singleton Access

WindowManager& WindowManager::getInstance()
{
    if (!s_instance)
        s_instance = new WindowManager();
    return *s_instance;
}

WindowManager::WindowManager()
    : m_isInitialized(false)
    , m_isRunning(false)
    , m_fontProvider(nullptr)
    , m_themeProvider(nullptr)
    , m_resourceResolver(nullptr)
    , m_mainWindows()
    , m_sharedRenderDevice(nullptr)
{
    m_allWindows.reserve(16);
    m_dialogs.reserve(8);
    m_toolWindows.reserve(8);
    m_mainWindows.reserve(4);
}

WindowManager::~WindowManager()
{
    destroy();
    if (s_instance == this)
        s_instance = nullptr;
}

//==========================================================================================
// Initialization and Lifecycle

bool WindowManager::initialize()
{
    YUCHEN_ASSERT(!m_isInitialized);
    
    // Initialize menu backend first to avoid static initialization order issues
    initializeMenuBackend();
    
    YUCHEN_ASSERT_MSG(createSharedRenderDevice(), "Failed to create shared render device");
    
    m_isInitialized = true;
    return true;
}

void WindowManager::destroy()
{
    if (!m_isInitialized)
        return;
    
    closeAllWindows();
    cleanupResources();
    m_isInitialized = false;
}

void WindowManager::setFontProvider(IFontProvider* provider)
{
    YUCHEN_ASSERT_MSG(provider, "Font provider cannot be null");
    m_fontProvider = provider;
}

void WindowManager::setThemeProvider(IThemeProvider* provider)
{
    YUCHEN_ASSERT_MSG(provider, "Theme provider cannot be null");
    m_themeProvider = provider;
}

void WindowManager::setResourceResolver(IResourceResolver* resolver)
{
    YUCHEN_ASSERT_MSG(resolver, "Resource resolver cannot be null");
    m_resourceResolver = resolver;
}

bool WindowManager::createSharedRenderDevice()
{
    m_sharedRenderDevice = PlatformBackend::createSharedDevice();
    return m_sharedRenderDevice != nullptr;
}

void WindowManager::initializeMenuBackend()
{
    // Delegate to platform-specific registration function
    Platform::registerMenuBackend();
}

void WindowManager::cleanupResources()
{
    m_mainWindows.clear();
    m_dialogs.clear();
    m_toolWindows.clear();
    m_allWindows.clear();

    if (m_sharedRenderDevice)
    {
        PlatformBackend::destroySharedDevice(m_sharedRenderDevice);
        m_sharedRenderDevice = nullptr;
    }
}

//==========================================================================================
// Event Loop

void WindowManager::run()
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "WindowManager not initialized");
    YUCHEN_ASSERT_MSG(!m_isRunning, "WindowManager already running");

    m_isRunning = true;
    
    // Delegate to platform-specific event loop implementation
    Platform::runEventLoop(m_isRunning, this);
    
    m_isRunning = false;
}

void WindowManager::quit()
{
    if (!m_isRunning)
        return;
    
    m_isRunning = false;
    
    // Delegate to platform-specific quit implementation
    Platform::quitEventLoop();
}

void WindowManager::scheduleDialogDestruction(BaseWindow* dialog)
{
    YUCHEN_ASSERT(dialog);
    m_scheduledDialogDestructions.push_back(dialog);
}

void WindowManager::processScheduledDestructions()
{
    if (m_scheduledDialogDestructions.empty())
        return;

    std::vector<BaseWindow*> toDestroy = std::move(m_scheduledDialogDestructions);
    m_scheduledDialogDestructions.clear();

    for (BaseWindow* dialog : toDestroy)
    {
        closeWindow(dialog);
    }
}

//==========================================================================================
// Window Registry

void WindowManager::registerWindow(Window* window)
{
    YUCHEN_ASSERT(window);
    
    auto it = std::find(m_allWindows.begin(), m_allWindows.end(), window);
    if (it == m_allWindows.end())
    {
        m_allWindows.push_back(window);
    }
}

void WindowManager::unregisterWindow(Window* window)
{
    YUCHEN_ASSERT(window);
    
    auto it = std::find(m_allWindows.begin(), m_allWindows.end(), window);
    if (it != m_allWindows.end())
    {
        m_allWindows.erase(it);
    }
}

//==========================================================================================
// Unified Window Closing

void WindowManager::closeWindow(BaseWindow* window)
{
    YUCHEN_ASSERT(window);
    
    // Determine which collection this window belongs to
    auto findAndRemove = [window, this](auto& collection) -> bool {
        auto it = std::find_if(collection.begin(), collection.end(),
            [window](const std::unique_ptr<BaseWindow>& ptr) {
                return ptr.get() == window;
            });
        
        if (it != collection.end())
        {
            unregisterWindow(static_cast<Window*>(window));
            (*it)->destroy();
            collection.erase(it);
            return true;
        }
        return false;
    };
    
    // Try to remove from appropriate collection
    bool removed = findAndRemove(m_mainWindows) ||
                   findAndRemove(m_dialogs) ||
                   findAndRemove(m_toolWindows);
    
    YUCHEN_ASSERT_MSG(removed, "Window not found in any collection");
    
    // Check if we should quit after this window closes
    if (window->affectsAppLifetime())
    {
        // Count remaining windows that affect app lifetime
        if (getLifetimeAffectingWindowCount() == 0)
        {
            quit();
        }
    }
}

void WindowManager::closeAllWindows()
{
    // Close main windows
    for (auto& mainWindow : m_mainWindows)
    {
        if (mainWindow)
        {
            unregisterWindow(static_cast<Window*>(mainWindow.get()));
            mainWindow->destroy();
        }
    }
    m_mainWindows.clear();

    // Close dialogs
    for (auto& dialog : m_dialogs)
    {
        if (dialog)
        {
            unregisterWindow(static_cast<Window*>(dialog.get()));
            dialog->destroy();
        }
    }
    m_dialogs.clear();

    // Close tool windows
    for (auto& toolWindow : m_toolWindows)
    {
        if (toolWindow)
        {
            unregisterWindow(static_cast<Window*>(toolWindow.get()));
            toolWindow->destroy();
        }
    }
    m_toolWindows.clear();

    m_allWindows.clear();
}

size_t WindowManager::getLifetimeAffectingWindowCount() const
{
    size_t count = 0;
    
    // Count in main windows
    for (const auto& window : m_mainWindows)
    {
        if (window && window->affectsAppLifetime())
            ++count;
    }
    
    // Count in dialogs
    for (const auto& window : m_dialogs)
    {
        if (window && window->affectsAppLifetime())
            ++count;
    }
    
    // Count in tool windows
    for (const auto& window : m_toolWindows)
    {
        if (window && window->affectsAppLifetime())
            ++count;
    }
    
    return count;
}

//==========================================================================================
// Window Access

const std::vector<Window*>& WindowManager::getAllWindows() const
{
    return m_allWindows;
}

} // namespace YuchenUI
