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
/** @file WindowManager_Windows.cpp
    
    Windows platform implementation for WindowManager event loop.
    
    This file provides the Win32 message pump integration for WindowManager.
    It uses PeekMessage for non-blocking event retrieval and manual frame rendering.
    
    Implementation notes:
    - Uses PeekMessageW with PM_REMOVE for non-blocking event retrieval
    - Checks for WM_QUIT message to terminate loop
    - Translates keyboard messages with TranslateMessage
    - Dispatches messages through DispatchMessageW
    - Manually renders all visible windows when no messages pending
    - Processes scheduled dialog destructions after each frame
    - Registers Win32MenuImpl as the menu backend
    
    Event loop flow:
    1. Check for pending Windows messages
    2. If message exists, process it (translate and dispatch)
    3. If WM_QUIT received, set isRunning to false and exit
    4. If no messages, render all visible windows
    5. Process scheduled dialog destructions
    6. Repeat until isRunning becomes false
    
    Event loop termination:
    - quitEventLoop() posts WM_QUIT message to thread's message queue
    - Next PeekMessage call retrieves WM_QUIT
    - Loop checks message type and exits cleanly
    
    Rendering model:
    - Windows platform requires explicit frame rendering
    - Idle time used for rendering when no messages pending
    - Each window's renderContent() called if visible
    - More efficient than timer-based rendering
*/

#ifdef _WIN32

#include <windows.h>
#include "WindowManagerPlatform.h"
#include "YuchenUI/windows/WindowManager.h"
#include "YuchenUI/windows/BaseWindow.h"
#include "YuchenUI/windows/Window.h"
#include "YuchenUI/menu/IMenuBackend.h"
#include "Win32MenuImpl.h"

namespace YuchenUI {
namespace Platform {

//==========================================================================================
// Event Loop Implementation

void runEventLoop(bool& isRunning, WindowManager* manager)
{
    MSG msg;
    
    while (isRunning)
    {
        if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                isRunning = false;
                break;
            }
            
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        else
        {
            const std::vector<Window*>& allWindows = manager->getAllWindows();
            for (Window* window : allWindows)
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
        
        manager->processScheduledDestructions();
    }
}

//==========================================================================================
// Event Loop Termination

void quitEventLoop()
{
    PostQuitMessage(0);
}

//==========================================================================================
// Menu Backend Registration

void registerMenuBackend()
{
    IMenuBackend::registerFactory([]() -> std::unique_ptr<IMenuBackend> {
        auto backend = std::make_unique<Win32MenuImpl>();
        if (backend->createNativeMenu()) {
            return backend;
        }
        return nullptr;
    });
}

} // namespace Platform
} // namespace YuchenUI

#endif // _WIN32
