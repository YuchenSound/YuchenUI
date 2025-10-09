/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Platform module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

//==========================================================================================
/** @file PlatformBackend.mm
    
    macOS platform backend factory implementations.
    
    Provides factory methods for creating platform-specific implementations:
    - Graphics renderer (MetalRenderer)
    - Event manager (MacEventManager)
    - Shared Metal device for efficient resource sharing
    
    All windows in the application share a single Metal device to enable
    efficient resource management and inter-window texture sharing.
*/

#import <Metal/Metal.h>
#import <Cocoa/Cocoa.h>
#include "YuchenUI/platform/PlatformBackend.h"
#include "MetalRenderer.h"
#include "MacEventManager.h"

namespace YuchenUI {

//==========================================================================================
/**
    Creates a platform-specific graphics renderer.
    
    On macOS, this returns a MetalRenderer instance.
    
    @returns Pointer to a new MetalRenderer instance
*/
GraphicsContext* PlatformBackend::createRenderer() {
    return new MetalRenderer();
}

//==========================================================================================
/**
    Creates a platform-specific event manager.
    
    On macOS, this returns a MacEventManager for the specified window.
    
    @param nativeWindow  Pointer to NSWindow (passed as void*)
    @returns Pointer to a new MacEventManager instance
*/
EventManager* PlatformBackend::createEventManager(void* nativeWindow) {
    return new MacEventManager((__bridge NSWindow*)nativeWindow);
}

//==========================================================================================
/**
    Creates a shared Metal device.
    
    All windows in the application use this single device to enable
    efficient GPU resource management. Textures and buffers can be
    shared between windows without costly transfers.
    
    The device is created with a retain count, so the caller is responsible
    for calling destroySharedDevice() when done.
    
    @returns Pointer to id<MTLDevice> (bridged to void* for C++ compatibility)
*/
void* PlatformBackend::createSharedDevice() {
    @autoreleasepool {
        // Create the system default Metal device
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        
        // Return with retained ownership (caller must release)
        return (__bridge_retained void*)device;
    }
}

//==========================================================================================
/**
    Destroys a shared Metal device.
    
    Releases the Metal device created by createSharedDevice().
    This should be called when the application is shutting down.
    
    @param device  Pointer to id<MTLDevice> (from createSharedDevice)
*/
void PlatformBackend::destroySharedDevice(void* device) {
    if (device) {
        // Transfer ownership back to ARC and release
        CFBridgingRelease(device);
    }
}

} // namespace YuchenUI
