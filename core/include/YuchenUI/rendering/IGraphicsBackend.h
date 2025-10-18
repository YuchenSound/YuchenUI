#pragma once

#include "YuchenUI/core/Types.h"

namespace YuchenUI {

class RenderList;
class IFontProvider;
class IResourceResolver;

class IGraphicsBackend {
public:
    virtual ~IGraphicsBackend() = default;
    
    virtual bool initialize(void* platformSurface, int width, int height, float dpiScale, IFontProvider* fontProvider, IResourceResolver* resourceResolver) = 0;
    virtual void resize(int width, int height) = 0;

    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
    
    virtual void executeRenderCommands(const RenderList& commands) = 0;
    
    virtual void* createTexture2D(uint32_t width, uint32_t height,
                                   TextureFormat format) = 0;
    virtual void updateTexture2D(void* texture, uint32_t x, uint32_t y,
                                 uint32_t width, uint32_t height,
                                 const void* data, size_t bytesPerRow) = 0;
    virtual void destroyTexture(void* texture) = 0;
    
    virtual Vec2 getRenderSize() const = 0;
    virtual float getDPIScale() const = 0;
};

}
