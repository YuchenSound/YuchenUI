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
/** @file WindowManager.cpp
    
    Implementation notes:
    - Singleton pattern with lazy initialization
    - All windows share a single rendering device
    - Main windows keep the application running
    - Dialogs scheduled for destruction after modal loop exits
    - Platform-specific event loop delegated to Platform namespace
    - Menu backend explicitly initialized to avoid static initialization order issues
    
    Architecture:
    - This file contains only cross-platform window management logic
    - Platform-specific event loop code is in WindowManager_macOS.mm / WindowManager_Windows.cpp
    - Platform abstraction defined in WindowManagerPlatform.h
    - Menu backend registration delegated to platform-specific factory functions
*/

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
//
// The event loop is platform-specific and delegated to the Platform namespace.
// See WindowManager_macOS.mm or WindowManager_Windows.cpp for implementations.

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

//==========================================================================================
// Main Window Management

void WindowManager::closeMainWindow(BaseWindow* mainWindow)
{
    YUCHEN_ASSERT(mainWindow);

    auto it = std::find_if(m_mainWindows.begin(), m_mainWindows.end(),
        [mainWindow](const std::unique_ptr<BaseWindow>& ptr) {
            return ptr.get() == mainWindow;
        });

    YUCHEN_ASSERT_MSG(it != m_mainWindows.end(), "Main window not found in list");

    unregisterWindow(static_cast<Window*>(mainWindow));
    (*it)->destroy();
    m_mainWindows.erase(it);

    // Quit when last main window closes
    if (m_mainWindows.empty())
    {
        quit();
    }
}

bool WindowManager::isMainWindow(const BaseWindow* window) const
{
    if (!window)
        return false;

    return std::any_of(m_mainWindows.begin(), m_mainWindows.end(),
        [window](const std::unique_ptr<BaseWindow>& ptr) {
            return ptr.get() == window;
        });
}

//==========================================================================================
// Dialog Management

void WindowManager::closeDialog(BaseWindow* dialog)
{
    YUCHEN_ASSERT(dialog);

    auto it = std::find_if(m_dialogs.begin(), m_dialogs.end(),
        [dialog](const std::unique_ptr<BaseWindow>& ptr) {
            return ptr.get() == dialog;
        });

    if (it != m_dialogs.end())
    {
        unregisterWindow(static_cast<Window*>(dialog));
        (*it)->destroy();
        m_dialogs.erase(it);
    }
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
        closeDialog(dialog);
    }
}

//==========================================================================================
// Tool Window Management

void WindowManager::closeToolWindow(BaseWindow* toolWindow)
{
    YUCHEN_ASSERT(toolWindow);

    auto it = std::find_if(m_toolWindows.begin(), m_toolWindows.end(),
        [toolWindow](const std::unique_ptr<BaseWindow>& ptr) {
            return ptr.get() == toolWindow;
        });

    if (it != m_toolWindows.end())
    {
        unregisterWindow(static_cast<Window*>(toolWindow));
        (*it)->destroy();
        m_toolWindows.erase(it);
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

//==========================================================================================
// Window Access
//
// These methods provide access to window collections for platform event loop implementations.

const std::vector<Window*>& WindowManager::getAllWindows() const
{
    return m_allWindows;
}

} // namespace YuchenUI
