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
/** @file WindowManagerPlatform.h
    
    Platform abstraction layer for WindowManager event loop implementation.
    
    This header defines the interface between the cross-platform WindowManager code
    and platform-specific event loop implementations. Each platform provides its own
    implementation of these functions in a separate .mm or .cpp file.
    
    Supported platforms:
    - macOS: Uses NSApplication event loop with autoreleasepool management
    - Windows: Uses Win32 message pump with PeekMessage/DispatchMessage
    
    The platform layer is responsible for:
    - Running the main event loop until application termination
    - Dispatching platform events to window instances
    - Handling application quit requests
    - Managing per-frame resource cleanup (autorelease pools on macOS)
    - Registering platform-specific menu backend
*/

#pragma once

namespace YuchenUI {

class WindowManager;

//==========================================================================================
/**
    Platform-specific event loop implementations.
    
    These functions must be implemented for each supported platform in separate
    compilation units. They provide the platform-specific event loop logic while
    allowing the main WindowManager class to remain platform-agnostic.
*/
namespace Platform {

//==========================================================================================
/**
    Runs the platform's main event loop.
    
    This function blocks until the application quits. It continuously processes
    platform events and dispatches them to the appropriate window instances.
    On each iteration, it calls back to the WindowManager to:
    - Render visible windows (Windows only - macOS uses display link)
    - Process scheduled dialog destructions
    - Check if the event loop should terminate
    
    The function must respect the isRunning flag and return when it becomes false.
    
    @param isRunning    Reference to running flag - set to false to terminate loop
    @param manager      WindowManager instance for callbacks during event processing
    
    Platform implementation notes:
    - macOS: Uses [NSApp nextEventMatchingMask] with NSDefaultRunLoopMode
    - Windows: Uses PeekMessageW/DispatchMessageW with manual rendering
*/
void runEventLoop(bool& isRunning, WindowManager* manager);

//==========================================================================================
/**
    Requests termination of the platform event loop.
    
    This function signals the platform to stop its event loop at the next opportunity.
    It must be safe to call from any thread, though it is typically called from the
    main thread in response to the last window closing.
    
    The function should:
    - Set platform-specific quit flags
    - Post a wakeup event if the event loop is blocking
    - Return immediately without waiting for the loop to actually terminate
    
    Platform implementation notes:
    - macOS: Calls [NSApp stop:nil] and posts dummy event to wake event loop
    - Windows: Calls PostQuitMessage(0) to post WM_QUIT message
*/
void quitEventLoop();

//==========================================================================================
/**
    Registers the platform-specific menu backend factory.
    
    This function is called during WindowManager initialization to register
    the menu backend factory for the current platform. It must call
    IMenuBackend::registerFactory() with an appropriate factory function.
    
    This design avoids static initialization order issues and keeps
    platform-specific code isolated.
    
    Platform implementation notes:
    - macOS: Registers MacMenuImpl factory
    - Windows: Registers WindowsMenuImpl factory
*/
void registerMenuBackend();

} // namespace Platform
} // namespace YuchenUI
