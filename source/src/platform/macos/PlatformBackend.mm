#import <Metal/Metal.h>
#import <Cocoa/Cocoa.h>
#include "YuchenUI/platform/PlatformBackend.h"
#include "MetalRenderer.h"
#include "MacEventManager.h"

namespace YuchenUI {

GraphicsContext* PlatformBackend::createRenderer() {
    return new MetalRenderer();
}

EventManager* PlatformBackend::createEventManager(void* nativeWindow) {
    return new MacEventManager((__bridge NSWindow*)nativeWindow);
}

void* PlatformBackend::createSharedDevice() {
    @autoreleasepool {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        return (__bridge_retained void*)device;
    }
}

void PlatformBackend::destroySharedDevice(void* device) {
    if (device) {
        CFBridgingRelease(device);
    }
}

}
