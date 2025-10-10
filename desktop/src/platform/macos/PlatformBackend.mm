/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Platform module (macOS).
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

//==========================================================================================
/** @file PlatformBackend.mm
    
    macOS platform backend implementation.
    
    This file provides macOS-specific implementations for the platform abstraction layer,
    including graphics backend creation (Metal), event management, and shared device
    lifecycle management.
    
    Key responsibilities:
    - Creating Metal-based graphics backend instances
    - Creating macOS event managers for native window event handling
    - Managing shared Metal device for multi-window rendering
    - Proper Objective-C memory management with ARC bridging
    
    Implementation notes:
    - Uses __bridge and __bridge_retained for ARC/manual memory management interop
    - Shared Metal device is created once and reused across all windows
    - Device lifetime managed through CFRetain/CFRelease to work with C++ RAII
    - Event manager receives NSWindow pointer for native event integration
*/

#include "YuchenUI/platform/PlatformBackend.h"
#include "MetalRenderer.h"
#include "MacEventManager.h"
#import <Metal/Metal.h>

namespace YuchenUI {

//==========================================================================================
// Graphics Backend Creation

IGraphicsBackend* PlatformBackend::createGraphicsBackend() {
    return new MetalRenderer();
}

//==========================================================================================
// Event Manager Creation

EventManager* PlatformBackend::createEventManager(void* nativeWindow) {
    return new MacEventManager((__bridge NSWindow*)nativeWindow);
}

//==========================================================================================
// Shared Device Management

void* PlatformBackend::createSharedDevice() {
    // Create system default Metal device
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    
    // Transfer ownership to C++ side with manual retain
    // The caller becomes responsible for calling destroySharedDevice
    return (__bridge_retained void*)device;
}

void PlatformBackend::destroySharedDevice(void* device) {
    if (device) {
        // Release the manually retained Metal device
        CFRelease(device);
    }
}

} // namespace YuchenUI
