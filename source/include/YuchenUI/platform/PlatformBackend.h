#pragma once

namespace YuchenUI {

class GraphicsContext;
class EventManager;

class PlatformBackend {
public:
    static GraphicsContext* createRenderer();
    static EventManager* createEventManager(void* nativeWindow);
    static void* createSharedDevice();
    static void destroySharedDevice(void* device);
};

}
