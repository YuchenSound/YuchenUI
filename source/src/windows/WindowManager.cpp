#include "YuchenUI/windows/WindowManager.h"
#include "YuchenUI/windows/BaseWindow.h"
#include "YuchenUI/windows/Window.h"
#include "YuchenUI/platform/PlatformBackend.h"
#include "YuchenUI/core/Assert.h"
#include "YuchenUI/core/Config.h"
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

    WindowManager& WindowManager::getInstance()
    {
        if (!s_instance) s_instance = new WindowManager();
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
        if (s_instance == this) s_instance = nullptr;
    }

    bool WindowManager::initialize()
    {
        YUCHEN_ASSERT(!m_isInitialized);
        YUCHEN_ASSERT(createSharedRenderDevice());

        m_isInitialized = true;
        return true;
    }

    void WindowManager::destroy()
    {
        if (!m_isInitialized) return;

        closeAllWindows();
        cleanupResources();
        m_isInitialized = false;
    }

    void WindowManager::run()
    {
        YUCHEN_ASSERT_MSG(m_isInitialized, "WindowManager not initialized");

        if (m_isRunning)
        {
            std::cerr << "[WindowManager] Already running" << std::endl;
            return;
        }

        m_isRunning = true;

#ifdef __APPLE__
        @autoreleasepool{
            while (m_isRunning)
            {
                @autoreleasepool
                {
                    NSEvent * event = [NSApp nextEventMatchingMask : NSEventMaskAny
                                                        untilDate : [NSDate distantFuture]
                                                           inMode : NSDefaultRunLoopMode
                                                          dequeue : YES];
                    if (event)
                    {
                        [NSApp sendEvent : event] ;
                    }

                    processScheduledDestructions();
                }
            }
        }
#elif defined(_WIN32)
        MSG msg;
        while (GetMessageW(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            processScheduledDestructions();
        }
#endif

        m_isRunning = false;
    }

    void WindowManager::quit()
    {
        std::cout << "[WindowManager::quit] Called, isRunning=" << m_isRunning << std::endl;

        if (!m_isRunning) {
            std::cout << "[WindowManager::quit] Already stopped" << std::endl;
            return;
        }

        m_isRunning = false;
        std::cout << "[WindowManager::quit] Set isRunning to false" << std::endl;

#ifdef __APPLE__
        @autoreleasepool
        {
            [NSApp stop : nil] ;

            NSEvent* event = [NSEvent otherEventWithType : NSEventTypeApplicationDefined
                                                location : NSMakePoint(0, 0)
                                           modifierFlags : 0
                                               timestamp : 0
                                            windowNumber : 0
                                                 context : nil
                                                 subtype : 0
                                                   data1 : 0
                                                   data2 : 0];
            [NSApp postEvent : event atStart : YES] ;
        }
#elif defined(_WIN32)
        std::cout << "[WindowManager::quit] Posting WM_QUIT message" << std::endl;
        PostQuitMessage(0);
#endif
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
            PlatformBackend::destroySharedDevice(m_sharedRenderDevice);
            m_sharedRenderDevice = nullptr;
        }
    }

    void WindowManager::closeMainWindow(BaseWindow* mainWindow)
    {
        YUCHEN_ASSERT(mainWindow);

        std::cout << "[WindowManager::closeMainWindow] Attempting to close main window" << std::endl;

        auto it = std::find_if(m_mainWindows.begin(), m_mainWindows.end(),
            [mainWindow](const std::unique_ptr<BaseWindow>& ptr) {
                return ptr.get() == mainWindow;
            });

        if (it != m_mainWindows.end())
        {
            std::cout << "[WindowManager::closeMainWindow] Found window in list" << std::endl;
            unregisterWindow(static_cast<Window*>(mainWindow));
            (*it)->destroy();
            m_mainWindows.erase(it);

            std::cout << "[WindowManager::closeMainWindow] Window closed. Remaining windows: "
                << m_mainWindows.size() << std::endl;

            if (m_mainWindows.empty())
            {
                std::cout << "[WindowManager::closeMainWindow] No more windows, calling quit()" << std::endl;
                quit();
            }
        }
        else
        {
            std::cerr << "[WindowManager::closeMainWindow] ERROR: Window not found in main window list!" << std::endl;
        }
    }

    bool WindowManager::isMainWindow(const BaseWindow* window) const
    {
        if (!window) return false;

        return std::any_of(m_mainWindows.begin(), m_mainWindows.end(),
            [window](const std::unique_ptr<BaseWindow>& ptr) {
                return ptr.get() == window;
            });
    }

    void WindowManager::closeDialog(BaseWindow* dialog)
    {
        YUCHEN_ASSERT(dialog);

        auto it = std::find_if(m_dialogs.begin(), m_dialogs.end(),
            [dialog](const std::unique_ptr<BaseWindow>& ptr) { return ptr.get() == dialog; });

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
        if (m_scheduledDialogDestructions.empty()) return;

        std::vector<BaseWindow*> toDestroy = std::move(m_scheduledDialogDestructions);
        m_scheduledDialogDestructions.clear();

        for (BaseWindow* dialog : toDestroy)
        {
            closeDialog(dialog);
        }
    }

    void WindowManager::closeToolWindow(BaseWindow* toolWindow)
    {
        YUCHEN_ASSERT(toolWindow);

        auto it = std::find_if(m_toolWindows.begin(), m_toolWindows.end(),
            [toolWindow](const std::unique_ptr<BaseWindow>& ptr) { return ptr.get() == toolWindow; });

        if (it != m_toolWindows.end())
        {
            unregisterWindow(static_cast<Window*>(toolWindow));
            (*it)->destroy();
            m_toolWindows.erase(it);
        }
    }

    void WindowManager::registerWindow(Window* window)
    {
        YUCHEN_ASSERT(window);
        auto it = std::find(m_allWindows.begin(), m_allWindows.end(), window);
        if (it == m_allWindows.end()) m_allWindows.push_back(window);
    }

    void WindowManager::unregisterWindow(Window* window)
    {
        YUCHEN_ASSERT(window);
        auto it = std::find(m_allWindows.begin(), m_allWindows.end(), window);
        if (it != m_allWindows.end()) m_allWindows.erase(it);
    }

    void WindowManager::closeAllWindows()
    {
        for (auto& mainWindow : m_mainWindows)
        {
            if (mainWindow)
            {
                unregisterWindow(static_cast<Window*>(mainWindow.get()));
                mainWindow->destroy();
            }
        }
        m_mainWindows.clear();

        for (auto& dialog : m_dialogs)
        {
            if (dialog)
            {
                unregisterWindow(static_cast<Window*>(dialog.get()));
                dialog->destroy();
            }
        }
        m_dialogs.clear();

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

}