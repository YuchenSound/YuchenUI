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
/** @file WindowManager_macOS.mm
    
    macOS platform implementation for WindowManager event loop.
    
    This file provides the Cocoa/AppKit event loop integration for WindowManager.
    It uses NSApplication's event dispatch mechanism with proper autorelease pool
    management for each event iteration.
    
    Implementation notes:
    - Uses NSApp (shared NSApplication instance) for event dispatch
    - Wraps event loop in @autoreleasepool for memory management
    - Creates nested autorelease pools for each event iteration
    - Blocks on [NSApp nextEventMatchingMask:] waiting for events
    - Dispatches events through [NSApp sendEvent:]
    - Renders visible windows through WindowManager callback
    - Processes scheduled dialog destructions after each event
    - Registers MacMenuImpl as the menu backend
    
    Event loop termination:
    - Loop continues while isRunning flag remains true
    - quitEventLoop() sets [NSApp stop:nil] and posts dummy event
    - Dummy event wakes up the blocking nextEventMatchingMask call
    - Loop exits cleanly on next iteration check
    
    Memory management:
    - Outer @autoreleasepool wraps entire event loop
    - Inner @autoreleasepool created/destroyed per event
    - Prevents autorelease accumulation during long-running sessions
    - Critical for applications that run for extended periods
*/

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#include "WindowManagerPlatform.h"
#include "YuchenUI/windows/WindowManager.h"
#include "YuchenUI/windows/BaseWindow.h"
#include "YuchenUI/windows/Window.h"
#include "YuchenUI/menu/IMenuBackend.h"
#include "MacMenuImpl.h"

namespace YuchenUI {
namespace Platform {

//==========================================================================================
// Event Loop Implementation

void runEventLoop(bool& isRunning, WindowManager* manager)
{
    @autoreleasepool
    {
        // Main event loop continues until isRunning becomes false
        while (isRunning)
        {
            @autoreleasepool
            {
                // Wait for next event with no timeout (blocks until event arrives)
                NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                                     untilDate:[NSDate distantFuture]
                                                        inMode:NSDefaultRunLoopMode
                                                       dequeue:YES];
                
                // Dispatch event to appropriate window and handlers
                if (event)
                {
                    [NSApp sendEvent:event];
                }
                
                // Render all visible windows
                // Note: On macOS, this provides fallback rendering. Most rendering
                // happens through CVDisplayLink callbacks in the window implementation.
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
                
                // Process any dialogs scheduled for destruction
                // Dialogs must be destroyed outside their modal event loop
                manager->processScheduledDestructions();
            }
        }
    }
}

//==========================================================================================
// Event Loop Termination

void quitEventLoop()
{
    @autoreleasepool
    {
        // Request NSApplication to stop its event loop
        [NSApp stop:nil];
        
        // Post a dummy event to wake up the event loop
        // Without this, nextEventMatchingMask continues blocking until next real event
        NSEvent* event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                            location:NSMakePoint(0, 0)
                                       modifierFlags:0
                                           timestamp:0
                                        windowNumber:0
                                             context:nil
                                             subtype:0
                                               data1:0
                                               data2:0];
        
        // Post event at start of queue to ensure immediate processing
        [NSApp postEvent:event atStart:YES];
    }
}

//==========================================================================================
// Menu Backend Registration

void registerMenuBackend()
{
    IMenuBackend::registerFactory([]() -> std::unique_ptr<IMenuBackend> {
        auto backend = std::make_unique<MacMenuImpl>();
        if (backend->createNativeMenu()) {
            return backend;
        }
        return nullptr;
    });
}

} // namespace Platform
} // namespace YuchenUI
