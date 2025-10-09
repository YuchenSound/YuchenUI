#pragma once

#include "YuchenUI/rendering/GraphicsContext.h"
#include "YuchenUI/rendering/RenderList.h"
#include <memory>
#include <vector>
#include <map>

#ifdef __OBJC__
    #import <Metal/Metal.h>
    #import <MetalKit/MetalKit.h>
    #import <QuartzCore/QuartzCore.h>
    @class CAMetalLayer;
    @class TextureWrapper;
#else
    typedef void CAMetalLayer;
    typedef void id;
    typedef void TextureWrapper;
#endif

namespace YuchenUI {

class TextRenderer;
class TextureCache;

struct ViewportUniforms {
    Vec2 viewportSize;
};

struct ShapeVertex {
    Vec2 position;
    Vec4 color;
    
    ShapeVertex() : position(), color() {}
    ShapeVertex(const Vec2& pos, const Vec4& col) : position(pos), color(col) {}
};

struct CircleVertex {
    Vec2 position;
    Vec2 center;
    float radius;
    float borderWidth;
    Vec4 color;
    
    CircleVertex() : position(), center(), radius(0.0f), borderWidth(0.0f), color() {}
    CircleVertex(const Vec2& pos, const Vec2& cen, float r, float bw, const Vec4& col)
        : position(pos), center(cen), radius(r), borderWidth(bw), color(col) {}
};

enum class ActivePipeline {
    None,
    Rect,
    Text,
    Image,
    Shape,
    Circle
};

class MetalRenderer : public GraphicsContext {
public:
    MetalRenderer();
    virtual ~MetalRenderer();
    
    bool initialize(int width, int height, float dpiScale = 1.0f) override;
    void beginFrame() override;
    void endFrame() override;
    void resize(int width, int height) override;
    bool isInitialized() const override;
    Vec2 getRenderSize() const override;
    
    void setSurface(void* surface) override;
    void setSharedDevice(void* device) override;
    
    void* getCurrentAtlasTexture() const;
    float getDPIScale() const { return m_dpiScale; }
    
    void* createTexture2D(uint32_t width, uint32_t height, TextureFormat format) override;
    void updateTexture2D(void* texture, uint32_t x, uint32_t y,
                        uint32_t width, uint32_t height,
                        const void* data, size_t bytesPerRow) override;
    void destroyTexture(void* texture) override;
private:
    bool createDevice();
    bool createCommandQueue();
    bool setupRenderPipeline();
    void setupVertexDescriptor();
    bool setupTextRenderPipeline();
    void createTextSampler();
    bool createTextBuffers();
    bool setupImageRenderPipeline();
    void createImageSampler();
    bool setupShapePipeline();
    bool setupCirclePipeline();
    
    void executeRenderCommands(const RenderList& commandList) override;
    
    void setPipeline(ActivePipeline pipeline);
    void applyScissorRect(const Rect& clipRect);
    MTLScissorRect computeScissorRect(const Rect& clipRect) const;
    void applyFullScreenScissor();
    
    void renderRectangle(const Rect& rect, const Vec4& color, const CornerRadius& cornerRadius, float borderWidth);
    void renderRectBatch(const std::vector<RenderCommand>& commands, const Rect& clipRect, bool hasClip);
    void renderImageBatch(const std::vector<size_t>& commandIndices, const std::vector<RenderCommand>& commands, void* texture, const Rect& clipRect, bool hasClip);
    void renderTextBatches(const std::vector<TextVertex>& allVertices, const std::vector<size_t>& batchStarts, const std::vector<size_t>& batchCounts, const std::vector<Rect>& batchClips, const std::vector<bool>& batchHasClips);
    void renderLine(const Vec2& start, const Vec2& end, const Vec4& color, float width);
    void renderTriangle(const Vec2& p1, const Vec2& p2, const Vec2& p3, const Vec4& color, float borderWidth, bool filled);
    void renderCircle(const Vec2& center, float radius, const Vec4& color, float borderWidth, bool filled);
    
    void generateImageVertices(const Rect& destRect, const Rect& sourceRect, uint32_t texWidth, uint32_t texHeight, std::vector<float>& outVertices);
    void generateNineSliceVertices(void* texture, const Rect& destRect, const Rect& sourceRect, const NineSliceMargins& margins, float designScale, uint32_t texWidth, uint32_t texHeight, std::vector<float>& outVertices);
    void computeNineSliceRects(const Rect& destRect, const Rect& sourceRect, const NineSliceMargins& margins, float designScale, Rect outSlices[9]);
    
    void convertToNDC(float x, float y, float& ndcX, float& ndcY) const;
    ViewportUniforms getViewportUniforms() const;
    void cleanupTextResources();
    void cleanupImageResources();
    void cleanupShapeResources();
    void releaseMetalObjects();

    bool m_usingSharedDevice;
    
#ifdef __OBJC__
    id<MTLDevice> m_device;
    id<MTLCommandQueue> m_commandQueue;
    id<MTLRenderPipelineState> m_renderPipeline;
    MTLVertexDescriptor* m_vertexDescriptor;
    id<MTLCommandBuffer> m_commandBuffer;
    id<MTLRenderCommandEncoder> m_renderEncoder;
    CAMetalLayer* m_metalLayer;
    id<CAMetalDrawable> m_drawable;
    MTLRenderPassDescriptor* m_renderPass;
    id<MTLRenderPipelineState> m_textRenderPipeline;
    id<MTLSamplerState> m_textSampler;
    id<MTLBuffer> m_textVertexBuffer;
    id<MTLBuffer> m_textIndexBuffer;
    id<MTLRenderPipelineState> m_imageRenderPipeline;
    id<MTLSamplerState> m_imageSampler;
    id<MTLRenderPipelineState> m_shapePipeline;
    id<MTLRenderPipelineState> m_circlePipeline;
#else
    void* m_device;
    void* m_commandQueue;
    void* m_renderPipeline;
    void* m_vertexDescriptor;
    void* m_commandBuffer;
    void* m_renderEncoder;
    CAMetalLayer* m_metalLayer;
    void* m_drawable;
    void* m_renderPass;
    void* m_textRenderPipeline;
    void* m_textSampler;
    void* m_textVertexBuffer;
    void* m_textIndexBuffer;
    void* m_imageRenderPipeline;
    void* m_imageSampler;
    void* m_shapePipeline;
    void* m_circlePipeline;
#endif

    ActivePipeline m_currentPipeline;
    std::unique_ptr<TextRenderer> m_textRenderer;
    std::unique_ptr<TextureCache> m_textureCache;
    size_t m_maxTextVertices;
    bool m_isInitialized;
    int m_width;
    int m_height;
    float m_dpiScale;
    Vec4 m_clearColor;
};

}
