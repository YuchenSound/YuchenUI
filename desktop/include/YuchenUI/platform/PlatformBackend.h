#pragma once

namespace YuchenUI {

class IGraphicsBackend;
class EventManager;

class PlatformBackend {
public:
    static IGraphicsBackend* createGraphicsBackend();
    static EventManager* createEventManager(void* nativeWindow);
    static void* createSharedDevice();
    static void destroySharedDevice(void* device);
};

}
