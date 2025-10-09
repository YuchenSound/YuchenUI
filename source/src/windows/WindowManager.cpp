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
    - Platform-specific event loop implementations
*/

#include "YuchenUI/windows/WindowManager.h"
#include "YuchenUI/windows/BaseWindow.h"
#include "YuchenUI/windows/Window.h"
#include "YuchenUI/platform/PlatformBackend.h"
#include "YuchenUI/core/Assert.h"
#include "YuchenUI/core/Config.h"
#include "yuchen_version.h"
#include <iostream>
#include <algorithm>

#ifdef __APPLE__
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

namespace YuchenUI
{

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
    m_allWindows.reserve (16);
    m_dialogs.reserve (8);
    m_toolWindows.reserve (8);
    m_mainWindows.reserve (4);
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
    std::cout << "[YuchenUI v" << YuchenUI::Version::String << " build " << YuchenUI::Version::Build << "]" << std::endl;
    
    YUCHEN_ASSERT (!m_isInitialized);
    YUCHEN_ASSERT_MSG (createSharedRenderDevice(), "Failed to create shared render device");
    
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

void WindowManager::cleanupResources()
{
    m_mainWindows.clear();
    m_dialogs.clear();
    m_toolWindows.clear();
    m_allWindows.clear();

    if (m_sharedRenderDevice)
    {
        PlatformBackend::destroySharedDevice (m_sharedRenderDevice);
        m_sharedRenderDevice = nullptr;
    }
}

//==========================================================================================
// Event Loop

void WindowManager::run()
{
    YUCHEN_ASSERT_MSG (m_isInitialized, "WindowManager not initialized");
    YUCHEN_ASSERT_MSG (!m_isRunning, "WindowManager already running");

    m_isRunning = true;

#ifdef __APPLE__
    @autoreleasepool
    {
        while (m_isRunning)
        {
            @autoreleasepool
            {
                NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                                     untilDate:[NSDate distantFuture]
                                                        inMode:NSDefaultRunLoopMode
                                                       dequeue:YES];
                if (event)
                {
                    [NSApp sendEvent:event];
                }

                processScheduledDestructions();
            }
        }
    }
#elif defined(_WIN32)
    MSG msg;
    while (m_isRunning)
    {
        if (PeekMessageW (&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                m_isRunning = false;
                break;
            }

            TranslateMessage (&msg);
            DispatchMessageW (&msg);
        }
        else
        {
            // Render all visible windows
            for (Window* window : m_allWindows)
            {
                if (window)
                {
                    BaseWindow* baseWindow = static_cast<BaseWindow*>(window);
                    if (baseWindow->isVisible())
                    {
                        baseWindow->renderContent();
                    }
                }
            }
        }

        processScheduledDestructions();
    }
#endif

    m_isRunning = false;
}

void WindowManager::quit()
{
    if (!m_isRunning)
        return;
    
    m_isRunning = false;

#ifdef __APPLE__
    @autoreleasepool
    {
        [NSApp stop:nil];
        
        // Post dummy event to wake up event loop
        NSEvent* event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                            location:NSMakePoint(0, 0)
                                       modifierFlags:0
                                           timestamp:0
                                        windowNumber:0
                                             context:nil
                                             subtype:0
                                               data1:0
                                               data2:0];
        [NSApp postEvent:event atStart:YES];
    }
#elif defined(_WIN32)
    PostQuitMessage (0);
#endif
}

//==========================================================================================
// Main Window Management

void WindowManager::closeMainWindow (BaseWindow* mainWindow)
{
    YUCHEN_ASSERT (mainWindow);

    auto it = std::find_if (m_mainWindows.begin(), m_mainWindows.end(),
        [mainWindow](const std::unique_ptr<BaseWindow>& ptr) {
            return ptr.get() == mainWindow;
        });

    YUCHEN_ASSERT_MSG (it != m_mainWindows.end(), "Main window not found in list");

    unregisterWindow (static_cast<Window*>(mainWindow));
    (*it)->destroy();
    m_mainWindows.erase (it);

    // Quit when last main window closes
    if (m_mainWindows.empty())
    {
        quit();
    }
}

bool WindowManager::isMainWindow (const BaseWindow* window) const
{
    if (!window)
        return false;

    return std::any_of (m_mainWindows.begin(), m_mainWindows.end(),
        [window](const std::unique_ptr<BaseWindow>& ptr) {
            return ptr.get() == window;
        });
}

//==========================================================================================
// Dialog Management

void WindowManager::closeDialog (BaseWindow* dialog)
{
    YUCHEN_ASSERT (dialog);

    auto it = std::find_if (m_dialogs.begin(), m_dialogs.end(),
        [dialog](const std::unique_ptr<BaseWindow>& ptr) {
            return ptr.get() == dialog;
        });

    if (it != m_dialogs.end())
    {
        unregisterWindow (static_cast<Window*>(dialog));
        (*it)->destroy();
        m_dialogs.erase (it);
    }
}

void WindowManager::scheduleDialogDestruction (BaseWindow* dialog)
{
    YUCHEN_ASSERT (dialog);
    m_scheduledDialogDestructions.push_back (dialog);
}

void WindowManager::processScheduledDestructions()
{
    if (m_scheduledDialogDestructions.empty())
        return;

    std::vector<BaseWindow*> toDestroy = std::move (m_scheduledDialogDestructions);
    m_scheduledDialogDestructions.clear();

    for (BaseWindow* dialog : toDestroy)
    {
        closeDialog (dialog);
    }
}

//==========================================================================================
// Tool Window Management

void WindowManager::closeToolWindow (BaseWindow* toolWindow)
{
    YUCHEN_ASSERT (toolWindow);

    auto it = std::find_if (m_toolWindows.begin(), m_toolWindows.end(),
        [toolWindow](const std::unique_ptr<BaseWindow>& ptr) {
            return ptr.get() == toolWindow;
        });

    if (it != m_toolWindows.end())
    {
        unregisterWindow (static_cast<Window*>(toolWindow));
        (*it)->destroy();
        m_toolWindows.erase (it);
    }
}

//==========================================================================================
// Window Registry

void WindowManager::registerWindow (Window* window)
{
    YUCHEN_ASSERT (window);
    
    auto it = std::find (m_allWindows.begin(), m_allWindows.end(), window);
    if (it == m_allWindows.end())
    {
        m_allWindows.push_back (window);
    }
}

void WindowManager::unregisterWindow (Window* window)
{
    YUCHEN_ASSERT (window);
    
    auto it = std::find (m_allWindows.begin(), m_allWindows.end(), window);
    if (it != m_allWindows.end())
    {
        m_allWindows.erase (it);
    }
}

void WindowManager::closeAllWindows()
{
    // Close main windows
    for (auto& mainWindow : m_mainWindows)
    {
        if (mainWindow)
        {
            unregisterWindow (static_cast<Window*>(mainWindow.get()));
            mainWindow->destroy();
        }
    }
    m_mainWindows.clear();

    // Close dialogs
    for (auto& dialog : m_dialogs)
    {
        if (dialog)
        {
            unregisterWindow (static_cast<Window*>(dialog.get()));
            dialog->destroy();
        }
    }
    m_dialogs.clear();

    // Close tool windows
    for (auto& toolWindow : m_toolWindows)
    {
        if (toolWindow)
        {
            unregisterWindow (static_cast<Window*>(toolWindow.get()));
            toolWindow->destroy();
        }
    }
    m_toolWindows.clear();

    m_allWindows.clear();
}

} // namespace YuchenUI
