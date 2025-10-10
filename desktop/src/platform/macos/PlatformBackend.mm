#include "YuchenUI/platform/PlatformBackend.h"
#include "MetalRenderer.h"
#include "MacEventManager.h"
#import <Metal/Metal.h>

namespace YuchenUI {

IGraphicsBackend* PlatformBackend::createGraphicsBackend() {
    return new MetalRenderer();
}

EventManager* PlatformBackend::createEventManager(void* nativeWindow) {
    return new MacEventManager((__bridge NSWindow*)nativeWindow);
}

void* PlatformBackend::createSharedDevice() {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    return (__bridge_retained void*)device;
}

void PlatformBackend::destroySharedDevice(void* device) {
    if (device) {
        CFRelease(device);
    }
}

}
