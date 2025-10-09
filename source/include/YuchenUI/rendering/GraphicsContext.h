#pragma once

#include "YuchenUI/core/Types.h"

namespace YuchenUI {

class RenderList;

class GraphicsContext {
public:
    virtual ~GraphicsContext() = default;
    
    virtual bool initialize(int width, int height, float dpiScale = 1.0f) = 0;
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
    virtual void resize(int width, int height) = 0;
    virtual bool isInitialized() const = 0;
    virtual Vec2 getRenderSize() const = 0;
    virtual void executeRenderCommands(const RenderList& commandList) = 0;
    
    virtual void* createTexture2D(uint32_t width, uint32_t height, TextureFormat format) = 0;
    virtual void updateTexture2D(void* texture, uint32_t x, uint32_t y,
                                 uint32_t width, uint32_t height,
                                 const void* data, size_t bytesPerRow) = 0;
    virtual void destroyTexture(void* texture) = 0;
    
    virtual void setSurface(void* surface) = 0;
    virtual void setSharedDevice(void* device) = 0;
};

}
