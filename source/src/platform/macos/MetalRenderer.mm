#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/QuartzCore.h>
#include "YuchenUI/core/Types.h"
#include "MetalRenderer.h"
#include "YuchenUI/core/Validation.h"
#include "YuchenUI/core/Config.h"
#include "YuchenUI/core/Colors.h"
#include "YuchenUI/text/TextRenderer.h"
#include "YuchenUI/text/FontManager.h"
#include "YuchenUI/image/TextureCache.h"
#include "YuchenUI/debugging/debug.h"

@interface TextureWrapper : NSObject
@property (nonatomic, strong) id<MTLTexture> texture;
@end

@implementation TextureWrapper
@end

namespace {

using namespace YuchenUI;

struct ImageVertex
{
    Vec2 position;
    Vec2 texCoord;
    
    ImageVertex() : position(), texCoord() {}
    ImageVertex(const Vec2& pos, const Vec2& tex) : position(pos), texCoord(tex) {}
};

struct ClipState
{
    YuchenUI::Rect clipRect;
    bool hasClip;
};

uint64_t computeClipHash(const ClipState& state)
{
    if (!state.hasClip) return 0;
    
    uint64_t hash = 0;
    hash ^= std::hash<float>()(state.clipRect.x);
    hash ^= std::hash<float>()(state.clipRect.y) << 1;
    hash ^= std::hash<float>()(state.clipRect.width) << 2;
    hash ^= std::hash<float>()(state.clipRect.height) << 3;
    return hash;
}

}

namespace YuchenUI {

// MARK: - Lifecycle
MetalRenderer::MetalRenderer()
    : m_usingSharedDevice(false)
    , m_device(nil)
    , m_commandQueue(nil)
    , m_renderPipeline(nil)
    , m_vertexDescriptor(nil)
    , m_commandBuffer(nil)
    , m_renderEncoder(nil)
    , m_metalLayer(nil)
    , m_drawable(nil)
    , m_renderPass(nil)
    , m_textRenderPipeline(nil)
    , m_textSampler(nil)
    , m_textVertexBuffer(nil)
    , m_textIndexBuffer(nil)
    , m_imageRenderPipeline(nil)
    , m_imageSampler(nil)
    , m_shapePipeline(nil)
    , m_circlePipeline(nil)
    , m_currentPipeline(ActivePipeline::None)
    , m_textRenderer(nullptr)
    , m_textureCache(nullptr)
    , m_maxTextVertices(Config::Rendering::MAX_TEXT_VERTICES)
    , m_isInitialized(false)
    , m_width(0)
    , m_height(0)
    , m_dpiScale(1.0f)
    , m_clearColor(Colors::DEFAULT_CLEAR_COLOR)
{
}

MetalRenderer::~MetalRenderer()
{
    releaseMetalObjects();
}

void MetalRenderer::setSharedDevice(void* sharedDevice)
{
    YUCHEN_ASSERT_MSG(sharedDevice != nullptr, "Shared device cannot be null");
    YUCHEN_ASSERT_MSG(!m_isInitialized, "Cannot set shared device after initialization");
    
    m_device = (__bridge id<MTLDevice>)sharedDevice;
    m_usingSharedDevice = true;
}

bool MetalRenderer::initialize(int width, int height, float dpiScale)
{
    YUCHEN_ASSERT_MSG(!m_isInitialized, "Already initialized");
    
    m_width = width;
    m_height = height;
    m_dpiScale = dpiScale;
    
    if (!createDevice()) return false;
    if (!createCommandQueue()) return false;
    
    setupVertexDescriptor();
    
    if (!setupRenderPipeline()) return false;
    if (!setupTextRenderPipeline()) return false;
    
    createTextSampler();
    
    if (!createTextBuffers()) return false;
    if (!setupImageRenderPipeline()) return false;
    
    createImageSampler();
    
    if (!setupShapePipeline()) return false;
    if (!setupCirclePipeline()) return false;
    
    FontManager::getInstance();
    
    m_isInitialized = true;

    m_textRenderer = std::make_unique<TextRenderer>(this);
    if (!m_textRenderer->initialize(m_dpiScale)) return false;
    
    m_textureCache = std::make_unique<TextureCache>(this);
    if (!m_textureCache->initialize()) return false;
    
    m_textureCache->setCurrentDPI(m_dpiScale);
    return true;
}

void MetalRenderer::setSurface(void* surface)
{
    YUCHEN_ASSERT_MSG(surface != nullptr, "Surface cannot be null");
    YUCHEN_ASSERT_MSG(m_width > 0 && m_height > 0, "Dimensions not initialized");

    m_metalLayer = (__bridge CAMetalLayer*)surface;
    
    m_metalLayer.device = m_device;
    m_metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    m_metalLayer.framebufferOnly = NO;
    m_metalLayer.presentsWithTransaction = YES;
    m_metalLayer.allowsNextDrawableTimeout = NO;
    
    m_metalLayer.drawableSize = CGSizeMake(m_width * m_dpiScale, m_height * m_dpiScale);
    
    if (@available(macOS 10.15, *))
    {
        m_metalLayer.displaySyncEnabled = YES;
    }
}

void MetalRenderer::resize(int width, int height)
{
    YUCHEN_ASSERT_MSG(width >= Config::Window::MIN_SIZE && width <= Config::Window::MAX_SIZE, "Invalid width");
    YUCHEN_ASSERT_MSG(height >= Config::Window::MIN_SIZE && height <= Config::Window::MAX_SIZE, "Invalid height");
    
    m_width = width;
    m_height = height;
    
    if (m_metalLayer)
        m_metalLayer.drawableSize = CGSizeMake(width * m_dpiScale, height * m_dpiScale);
}

Vec2 MetalRenderer::getRenderSize() const
{
    return Vec2(static_cast<float>(m_width), static_cast<float>(m_height));
}

bool MetalRenderer::isInitialized() const
{
    return m_isInitialized;
}

void MetalRenderer::releaseMetalObjects()
{
    if (m_textRenderer)
    {
        m_textRenderer->destroy();
        m_textRenderer.reset();
    }
    
    if (m_textureCache)
    {
        m_textureCache->destroy();
        m_textureCache.reset();
    }
    
    cleanupTextResources();
    cleanupImageResources();
    cleanupShapeResources();
    
    @autoreleasepool {
        m_drawable = nil;
        m_renderPipeline = nil;
        m_vertexDescriptor = nil;
        m_commandQueue = nil;
        
        if (!m_usingSharedDevice) m_device = nil;
        
        m_metalLayer = nil;
        m_renderPass = nil;
    }
    
    m_isInitialized = false;
}

// MARK: - Device & Queue
bool MetalRenderer::createDevice()
{
    if (m_usingSharedDevice) return true;
    
    @autoreleasepool
    {
        m_device = MTLCreateSystemDefaultDevice();
        return true;
    }
}

bool MetalRenderer::createCommandQueue()
{
    @autoreleasepool
    {
        m_commandQueue = [m_device newCommandQueue];
        m_commandQueue.label = @"YuchenUI Main Command Queue";
        return true;
    }
}

// MARK: - Pipeline Setup
void MetalRenderer::setupVertexDescriptor()
{
    @autoreleasepool
    {
        m_vertexDescriptor = [[MTLVertexDescriptor alloc] init];
        
        m_vertexDescriptor.attributes[0].format = MTLVertexFormatFloat2;
        m_vertexDescriptor.attributes[0].offset = 0;
        m_vertexDescriptor.attributes[0].bufferIndex = 0;
        
        m_vertexDescriptor.attributes[1].format = MTLVertexFormatFloat2;
        m_vertexDescriptor.attributes[1].offset = 8;
        m_vertexDescriptor.attributes[1].bufferIndex = 0;
        
        m_vertexDescriptor.attributes[2].format = MTLVertexFormatFloat2;
        m_vertexDescriptor.attributes[2].offset = 16;
        m_vertexDescriptor.attributes[2].bufferIndex = 0;
        
        m_vertexDescriptor.attributes[3].format = MTLVertexFormatFloat4;
        m_vertexDescriptor.attributes[3].offset = 24;
        m_vertexDescriptor.attributes[3].bufferIndex = 0;
        
        m_vertexDescriptor.attributes[4].format = MTLVertexFormatFloat4;
        m_vertexDescriptor.attributes[4].offset = 40;
        m_vertexDescriptor.attributes[4].bufferIndex = 0;
        
        m_vertexDescriptor.attributes[5].format = MTLVertexFormatFloat;
        m_vertexDescriptor.attributes[5].offset = 56;
        m_vertexDescriptor.attributes[5].bufferIndex = 0;
        
        m_vertexDescriptor.layouts[0].stride = sizeof(RectVertex);
        m_vertexDescriptor.layouts[0].stepRate = 1;
        m_vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
    }
}

bool MetalRenderer::setupRenderPipeline()
{
    @autoreleasepool
    {
        NSError* error = nil;
        NSString* metallibPath = [[NSBundle mainBundle] pathForResource:@"Basic" ofType:@"metallib"];
        NSURL* url = [NSURL fileURLWithPath:metallibPath];
        id<MTLLibrary> library = [m_device newLibraryWithURL:url error:&error];
        
        id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vertex_rect"];
        id<MTLFunction> fragmentFunction = [library newFunctionWithName:@"fragment_rect"];
        
        MTLRenderPipelineDescriptor* descriptor = [[MTLRenderPipelineDescriptor alloc] init];
        descriptor.label = @"YuchenUI Pipeline";
        descriptor.vertexFunction = vertexFunction;
        descriptor.fragmentFunction = fragmentFunction;
        descriptor.vertexDescriptor = m_vertexDescriptor;
        descriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        descriptor.colorAttachments[0].blendingEnabled = YES;
        descriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        descriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        descriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        descriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
        descriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        descriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        
        error = nil;
        m_renderPipeline = [m_device newRenderPipelineStateWithDescriptor:descriptor error:&error];
        return true;
    }
}

bool MetalRenderer::setupTextRenderPipeline()
{
    @autoreleasepool
    {
        NSError* error = nil;
        NSString* metallibPath = [[NSBundle mainBundle] pathForResource:@"Text" ofType:@"metallib"];
        NSURL* url = [NSURL fileURLWithPath:metallibPath];
        id<MTLLibrary> library = [m_device newLibraryWithURL:url error:&error];
        
        id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vertex_text"];
        id<MTLFunction> fragmentFunction = [library newFunctionWithName:@"fragment_text"];
        
        MTLRenderPipelineDescriptor* descriptor = [[MTLRenderPipelineDescriptor alloc] init];
        descriptor.label = @"YuchenUI Text Pipeline";
        descriptor.vertexFunction = vertexFunction;
        descriptor.fragmentFunction = fragmentFunction;
        
        MTLVertexDescriptor* textVertexDescriptor = [[MTLVertexDescriptor alloc] init];
        textVertexDescriptor.attributes[0].format = MTLVertexFormatFloat2;
        textVertexDescriptor.attributes[0].offset = 0;
        textVertexDescriptor.attributes[0].bufferIndex = 0;
        textVertexDescriptor.attributes[1].format = MTLVertexFormatFloat2;
        textVertexDescriptor.attributes[1].offset = 8;
        textVertexDescriptor.attributes[1].bufferIndex = 0;
        textVertexDescriptor.attributes[2].format = MTLVertexFormatFloat4;
        textVertexDescriptor.attributes[2].offset = 16;
        textVertexDescriptor.attributes[2].bufferIndex = 0;
        textVertexDescriptor.layouts[0].stride = sizeof(TextVertex);
        textVertexDescriptor.layouts[0].stepRate = 1;
        textVertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
        
        descriptor.vertexDescriptor = textVertexDescriptor;
        descriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        descriptor.colorAttachments[0].blendingEnabled = YES;
        descriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        descriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        descriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        descriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        descriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
        descriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        
        error = nil;
        m_textRenderPipeline = [m_device newRenderPipelineStateWithDescriptor:descriptor error:&error];
        return true;
    }
}

void MetalRenderer::createTextSampler()
{
    @autoreleasepool
    {
        MTLSamplerDescriptor* samplerDesc = [[MTLSamplerDescriptor alloc] init];
        samplerDesc.minFilter = MTLSamplerMinMagFilterLinear;
        samplerDesc.magFilter = MTLSamplerMinMagFilterLinear;
        samplerDesc.mipFilter = MTLSamplerMipFilterNotMipmapped;
        samplerDesc.sAddressMode = MTLSamplerAddressModeClampToEdge;
        samplerDesc.tAddressMode = MTLSamplerAddressModeClampToEdge;
        samplerDesc.normalizedCoordinates = YES;
        m_textSampler = [m_device newSamplerStateWithDescriptor:samplerDesc];
    }
}

bool MetalRenderer::createTextBuffers()
{
    @autoreleasepool
    {
        NSUInteger vertexBufferSize = m_maxTextVertices * sizeof(TextVertex);
        m_textVertexBuffer = [m_device newBufferWithLength:vertexBufferSize options:MTLResourceStorageModeShared];
        m_textVertexBuffer.label = @"Text Vertex Buffer";
        
        NSUInteger indexBufferSize = (m_maxTextVertices / 4) * 6 * sizeof(uint16_t);
        m_textIndexBuffer = [m_device newBufferWithLength:indexBufferSize options:MTLResourceStorageModeShared];
        m_textIndexBuffer.label = @"Text Index Buffer";
        
        uint16_t* indices = (uint16_t*)[m_textIndexBuffer contents];
        for (NSUInteger i = 0; i < m_maxTextVertices / 4; ++i)
        {
            uint16_t baseVertex = i * 4;
            NSUInteger baseIndex = i * 6;
            indices[baseIndex + 0] = baseVertex + 0;
            indices[baseIndex + 1] = baseVertex + 1;
            indices[baseIndex + 2] = baseVertex + 2;
            indices[baseIndex + 3] = baseVertex + 1;
            indices[baseIndex + 4] = baseVertex + 3;
            indices[baseIndex + 5] = baseVertex + 2;
        }
        return true;
    }
}

bool MetalRenderer::setupImageRenderPipeline()
{
    @autoreleasepool
    {
        NSError* error = nil;
        NSString* metallibPath = [[NSBundle mainBundle] pathForResource:@"Image" ofType:@"metallib"];
        NSURL* url = [NSURL fileURLWithPath:metallibPath];
        id<MTLLibrary> library = [m_device newLibraryWithURL:url error:&error];
        
        id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vertex_image"];
        id<MTLFunction> fragmentFunction = [library newFunctionWithName:@"fragment_image"];
        
        MTLRenderPipelineDescriptor* descriptor = [[MTLRenderPipelineDescriptor alloc] init];
        descriptor.label = @"YuchenUI Image Pipeline";
        descriptor.vertexFunction = vertexFunction;
        descriptor.fragmentFunction = fragmentFunction;
        
        MTLVertexDescriptor* imageVertexDescriptor = [[MTLVertexDescriptor alloc] init];
        imageVertexDescriptor.attributes[0].format = MTLVertexFormatFloat2;
        imageVertexDescriptor.attributes[0].offset = 0;
        imageVertexDescriptor.attributes[0].bufferIndex = 0;
        imageVertexDescriptor.attributes[1].format = MTLVertexFormatFloat2;
        imageVertexDescriptor.attributes[1].offset = 8;
        imageVertexDescriptor.attributes[1].bufferIndex = 0;
        imageVertexDescriptor.layouts[0].stride = 16;
        imageVertexDescriptor.layouts[0].stepRate = 1;
        imageVertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
        
        descriptor.vertexDescriptor = imageVertexDescriptor;
        descriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        descriptor.colorAttachments[0].blendingEnabled = YES;
        descriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        descriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        descriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        descriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        descriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
        descriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        
        error = nil;
        m_imageRenderPipeline = [m_device newRenderPipelineStateWithDescriptor:descriptor error:&error];
        return true;
    }
}

void MetalRenderer::createImageSampler()
{
    @autoreleasepool
    {
        MTLSamplerDescriptor* samplerDesc = [[MTLSamplerDescriptor alloc] init];
        samplerDesc.minFilter = MTLSamplerMinMagFilterLinear;
        samplerDesc.magFilter = MTLSamplerMinMagFilterLinear;
        samplerDesc.mipFilter = MTLSamplerMipFilterNotMipmapped;
        samplerDesc.sAddressMode = MTLSamplerAddressModeClampToEdge;
        samplerDesc.tAddressMode = MTLSamplerAddressModeClampToEdge;
        samplerDesc.normalizedCoordinates = YES;
        m_imageSampler = [m_device newSamplerStateWithDescriptor:samplerDesc];
    }
}

bool MetalRenderer::setupShapePipeline()
{
    @autoreleasepool
    {
        NSError* error = nil;
        NSString* metallibPath = [[NSBundle mainBundle] pathForResource:@"Shape" ofType:@"metallib"];
        NSURL* url = [NSURL fileURLWithPath:metallibPath];
        id<MTLLibrary> library = [m_device newLibraryWithURL:url error:&error];
        
        id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vertex_shape"];
        id<MTLFunction> fragmentFunction = [library newFunctionWithName:@"fragment_shape"];
        
        MTLRenderPipelineDescriptor* descriptor = [[MTLRenderPipelineDescriptor alloc] init];
        descriptor.label = @"YuchenUI Shape Pipeline";
        descriptor.vertexFunction = vertexFunction;
        descriptor.fragmentFunction = fragmentFunction;
        
        MTLVertexDescriptor* shapeVertexDescriptor = [[MTLVertexDescriptor alloc] init];
        shapeVertexDescriptor.attributes[0].format = MTLVertexFormatFloat2;
        shapeVertexDescriptor.attributes[0].offset = 0;
        shapeVertexDescriptor.attributes[0].bufferIndex = 0;
        shapeVertexDescriptor.attributes[1].format = MTLVertexFormatFloat4;
        shapeVertexDescriptor.attributes[1].offset = 8;
        shapeVertexDescriptor.attributes[1].bufferIndex = 0;
        shapeVertexDescriptor.layouts[0].stride = sizeof(ShapeVertex);
        shapeVertexDescriptor.layouts[0].stepRate = 1;
        shapeVertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
        
        descriptor.vertexDescriptor = shapeVertexDescriptor;
        descriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        descriptor.colorAttachments[0].blendingEnabled = YES;
        descriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        descriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        descriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        descriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        descriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
        descriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        
        error = nil;
        m_shapePipeline = [m_device newRenderPipelineStateWithDescriptor:descriptor error:&error];
        return true;
    }
}

bool MetalRenderer::setupCirclePipeline()
{
    @autoreleasepool
    {
        NSError* error = nil;
        NSString* metallibPath = [[NSBundle mainBundle] pathForResource:@"Shape" ofType:@"metallib"];
        NSURL* url = [NSURL fileURLWithPath:metallibPath];
        id<MTLLibrary> library = [m_device newLibraryWithURL:url error:&error];
        
        id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vertex_circle"];
        id<MTLFunction> fragmentFunction = [library newFunctionWithName:@"fragment_circle"];
        
        MTLRenderPipelineDescriptor* descriptor = [[MTLRenderPipelineDescriptor alloc] init];
        descriptor.label = @"YuchenUI Circle Pipeline";
        descriptor.vertexFunction = vertexFunction;
        descriptor.fragmentFunction = fragmentFunction;
        
        MTLVertexDescriptor* circleVertexDescriptor = [[MTLVertexDescriptor alloc] init];
        circleVertexDescriptor.attributes[0].format = MTLVertexFormatFloat2;
        circleVertexDescriptor.attributes[0].offset = 0;
        circleVertexDescriptor.attributes[0].bufferIndex = 0;
        circleVertexDescriptor.attributes[1].format = MTLVertexFormatFloat2;
        circleVertexDescriptor.attributes[1].offset = 8;
        circleVertexDescriptor.attributes[1].bufferIndex = 0;
        circleVertexDescriptor.attributes[2].format = MTLVertexFormatFloat;
        circleVertexDescriptor.attributes[2].offset = 16;
        circleVertexDescriptor.attributes[2].bufferIndex = 0;
        circleVertexDescriptor.attributes[3].format = MTLVertexFormatFloat;
        circleVertexDescriptor.attributes[3].offset = 20;
        circleVertexDescriptor.attributes[3].bufferIndex = 0;
        circleVertexDescriptor.attributes[4].format = MTLVertexFormatFloat4;
        circleVertexDescriptor.attributes[4].offset = 24;
        circleVertexDescriptor.attributes[4].bufferIndex = 0;
        circleVertexDescriptor.layouts[0].stride = sizeof(CircleVertex);
        circleVertexDescriptor.layouts[0].stepRate = 1;
        circleVertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
        
        descriptor.vertexDescriptor = circleVertexDescriptor;
        descriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        descriptor.colorAttachments[0].blendingEnabled = YES;
        descriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        descriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        descriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        descriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        descriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
        descriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        
        error = nil;
        m_circlePipeline = [m_device newRenderPipelineStateWithDescriptor:descriptor error:&error];
        return true;
    }
}

void MetalRenderer::cleanupTextResources()
{
    @autoreleasepool
    {
        m_textRenderPipeline = nil;
        m_textSampler = nil;
        m_textVertexBuffer = nil;
        m_textIndexBuffer = nil;
    }
}

void MetalRenderer::cleanupImageResources()
{
    @autoreleasepool
    {
        m_imageRenderPipeline = nil;
        m_imageSampler = nil;
    }
}

void MetalRenderer::cleanupShapeResources()
{
    @autoreleasepool
    {
        m_shapePipeline = nil;
        m_circlePipeline = nil;
    }
}

// MARK: - Frame Management

void MetalRenderer::beginFrame()
{
    @autoreleasepool
    {
        if (m_renderEncoder)
        {
            [m_renderEncoder endEncoding];
            m_renderEncoder = nil;
        }
        
        if (m_commandBuffer)
        {
            m_commandBuffer = nil;
        }
        
        m_drawable = [m_metalLayer nextDrawable];
        if (!m_drawable) return;
        
        m_commandBuffer = [m_commandQueue commandBuffer];
        
        m_renderPass = [MTLRenderPassDescriptor renderPassDescriptor];
        m_renderPass.colorAttachments[0].texture = m_drawable.texture;
        m_renderPass.colorAttachments[0].loadAction = MTLLoadActionClear;
        m_renderPass.colorAttachments[0].storeAction = MTLStoreActionStore;
        m_renderPass.colorAttachments[0].clearColor = MTLClearColorMake(
            m_clearColor.x, m_clearColor.y, m_clearColor.z, m_clearColor.w
        );
        
        m_renderEncoder = [m_commandBuffer renderCommandEncoderWithDescriptor:m_renderPass];
        
        CGSize drawableSize = m_metalLayer.drawableSize;
        MTLViewport viewport = {0.0, 0.0, drawableSize.width, drawableSize.height, 0.0, 1.0};
        [m_renderEncoder setViewport:viewport];
        
        applyFullScreenScissor();
        
        ViewportUniforms uniforms = getViewportUniforms();
        id<MTLBuffer> uniformBuffer = [m_device newBufferWithBytes:&uniforms
                                                            length:sizeof(ViewportUniforms)
                                                           options:MTLResourceStorageModeShared];
        [m_renderEncoder setVertexBuffer:uniformBuffer offset:0 atIndex:1];
        
        m_currentPipeline = ActivePipeline::None;
        
        if (m_textRenderer) m_textRenderer->beginFrame();
    }
}

void MetalRenderer::endFrame()
{
    @autoreleasepool
    {
        if (m_renderEncoder)
        {
            [m_renderEncoder endEncoding];
            m_renderEncoder = nil;
        }
        
        if (m_commandBuffer && m_drawable)
        {
            [m_commandBuffer commit];
            [m_commandBuffer waitUntilScheduled];
            [m_drawable present];
        }
        
        m_drawable = nil;
        m_renderPass = nil;
    }
    
    m_commandBuffer = nil;
}

// MARK: - Pipeline State Management
void MetalRenderer::setPipeline(ActivePipeline pipeline)
{
    if (m_currentPipeline == pipeline) return;
    
    @autoreleasepool
    {
        switch (pipeline)
        {
            case ActivePipeline::Rect:
                [m_renderEncoder setRenderPipelineState:m_renderPipeline];
                break;
            case ActivePipeline::Text:
                [m_renderEncoder setRenderPipelineState:m_textRenderPipeline];
                break;
            case ActivePipeline::Image:
                [m_renderEncoder setRenderPipelineState:m_imageRenderPipeline];
                break;
            case ActivePipeline::Shape:
                [m_renderEncoder setRenderPipelineState:m_shapePipeline];
                break;
            case ActivePipeline::Circle:
                [m_renderEncoder setRenderPipelineState:m_circlePipeline];
                break;
            default:
                return;
        }
    }
    
    YUCHEN_PERF_PIPELINE_SWITCH();
    m_currentPipeline = pipeline;
}

// MARK: - Scissor Management
MTLScissorRect MetalRenderer::computeScissorRect(const Rect& clipRect) const
{
    MTLScissorRect scissor;
    scissor.x = static_cast<NSUInteger>(clipRect.x * m_dpiScale);
    scissor.y = static_cast<NSUInteger>(clipRect.y * m_dpiScale);
    scissor.width = static_cast<NSUInteger>(clipRect.width * m_dpiScale);
    scissor.height = static_cast<NSUInteger>(clipRect.height * m_dpiScale);
    
    NSUInteger maxWidth = static_cast<NSUInteger>(m_width * m_dpiScale);
    NSUInteger maxHeight = static_cast<NSUInteger>(m_height * m_dpiScale);
    
    scissor.x = std::min(scissor.x, maxWidth);
    scissor.y = std::min(scissor.y, maxHeight);
    scissor.width = std::min(scissor.width, maxWidth - scissor.x);
    scissor.height = std::min(scissor.height, maxHeight - scissor.y);
    
    return scissor;
}

void MetalRenderer::applyScissorRect(const Rect& clipRect)
{
    MTLScissorRect scissor = computeScissorRect(clipRect);
    [m_renderEncoder setScissorRect:scissor];
}

void MetalRenderer::applyFullScreenScissor()
{
    MTLScissorRect scissor;
    scissor.x = 0;
    scissor.y = 0;
    scissor.width = static_cast<NSUInteger>(m_width * m_dpiScale);
    scissor.height = static_cast<NSUInteger>(m_height * m_dpiScale);
    [m_renderEncoder setScissorRect:scissor];
}

// MARK: - Command Execution
void MetalRenderer::executeRenderCommands(const RenderList& commandList)
{
    const auto& commands = commandList.getCommands();
    if (commands.empty()) return;
    
    std::vector<ClipState> clipStates(commands.size());
    std::vector<Rect> clipStack;
    
    for (size_t i = 0; i < commands.size(); ++i)
    {
        const auto& cmd = commands[i];
        
        if (cmd.type == RenderCommandType::PushClip)
        {
            Rect newClip = cmd.rect;
            
            if (!clipStack.empty())
            {
                const Rect& parentClip = clipStack.back();
                
                float x1 = std::max(parentClip.x, newClip.x);
                float y1 = std::max(parentClip.y, newClip.y);
                float x2 = std::min(parentClip.x + parentClip.width, newClip.x + newClip.width);
                float y2 = std::min(parentClip.y + parentClip.height, newClip.y + newClip.height);
                
                if (x2 > x1 && y2 > y1)
                {
                    newClip = Rect(x1, y1, x2 - x1, y2 - y1);
                }
                else
                {
                    newClip = Rect(0, 0, 0, 0);
                }
            }
            
            clipStack.push_back(newClip);
            clipStates[i].clipRect = newClip;
            clipStates[i].hasClip = true;
        }
        else if (cmd.type == RenderCommandType::PopClip)
        {
            if (!clipStack.empty())
            {
                clipStack.pop_back();
            }
            
            if (!clipStack.empty())
            {
                clipStates[i].clipRect = clipStack.back();
                clipStates[i].hasClip = true;
            }
            else
            {
                clipStates[i].hasClip = false;
            }
        }
        else
        {
            if (!clipStack.empty())
            {
                clipStates[i].clipRect = clipStack.back();
                clipStates[i].hasClip = true;
            }
            else
            {
                clipStates[i].hasClip = false;
            }
        }
    }
    
    std::map<uint64_t, std::vector<RenderCommand>> rectGroups;
    std::map<uint64_t, Rect> rectGroupClips;
    std::map<uint64_t, bool> rectGroupHasClip;
    
    struct ImageBatch {
        std::vector<size_t> indices;
        void* texture;
        Rect clipRect;
        bool hasClip;
        size_t firstIndex;
    };
    std::vector<ImageBatch> imageBatches;
    
    std::vector<TextVertex> allTextVertices;
    std::vector<size_t> textBatchStarts;
    std::vector<size_t> textBatchCounts;
    std::vector<Rect> textBatchClips;
    std::vector<bool> textBatchHasClips;
    allTextVertices.reserve(1024);
    
    for (size_t i = 0; i < commands.size(); ++i)
    {
        const auto& cmd = commands[i];
        
        switch (cmd.type) {
            case RenderCommandType::Clear:
                m_clearColor = cmd.color;
                break;
                
            case RenderCommandType::FillRect:
            case RenderCommandType::DrawRect:
            {
                uint64_t clipHash = computeClipHash(clipStates[i]);
                rectGroups[clipHash].push_back(cmd);
                rectGroupClips[clipHash] = clipStates[i].clipRect;
                rectGroupHasClip[clipHash] = clipStates[i].hasClip;
                break;
            }
                
            case RenderCommandType::DrawImage:
            {
                uint32_t texWidth = 0, texHeight = 0;
                float designScale = 1.0f;
                void* texture = m_textureCache->getTexture(cmd.text.c_str(), texWidth, texHeight, &designScale);
                if (texture) {
                    uint64_t clipHash = computeClipHash(clipStates[i]);
                    uint64_t textureHash = reinterpret_cast<uint64_t>(texture);
                    uint64_t key = (textureHash << 32) | clipHash;
                    
                    bool foundBatch = false;
                    for (auto& batch : imageBatches) {
                        uint64_t batchKey = (reinterpret_cast<uint64_t>(batch.texture) << 32) |
                                           computeClipHash(ClipState{batch.clipRect, batch.hasClip});
                        if (batchKey == key) {
                            batch.indices.push_back(i);
                            foundBatch = true;
                            break;
                        }
                    }
                    
                    if (!foundBatch) {
                        ImageBatch newBatch;
                        newBatch.indices.push_back(i);
                        newBatch.texture = texture;
                        newBatch.clipRect = clipStates[i].clipRect;
                        newBatch.hasClip = clipStates[i].hasClip;
                        newBatch.firstIndex = i;
                        imageBatches.push_back(newBatch);
                    }
                    
                    YUCHEN_PERF_TEXTURE_USAGE(cmd.text);
                }
                break;
            }
                
            case RenderCommandType::DrawText:
            {
                ShapedText shapedText;
                m_textRenderer->shapeText(cmd.text.c_str(), cmd.westernFont, cmd.chineseFont, cmd.fontSize, shapedText);
                if (!shapedText.isEmpty())
                {
                    std::vector<TextVertex> vertices;
                    m_textRenderer->generateTextVertices(shapedText, cmd.textPosition, cmd.textColor, cmd.westernFont, cmd.fontSize, vertices);
                    
                    if (!vertices.empty())
                    {
                        bool canMerge = false;
                        if (!textBatchStarts.empty())
                        {
                            size_t lastIdx = textBatchStarts.size() - 1;
                            if (textBatchHasClips[lastIdx] == clipStates[i].hasClip)
                            {
                                if (!clipStates[i].hasClip ||(
                                    textBatchClips[lastIdx].x == clipStates[i].clipRect.x &&
                                    textBatchClips[lastIdx].y == clipStates[i].clipRect.y &&
                                    textBatchClips[lastIdx].width == clipStates[i].clipRect.width &&
                                    textBatchClips[lastIdx].height == clipStates[i].clipRect.height))
                                {
                                    canMerge = true;
                                }
                            }
                        }
                        
                        if (canMerge)
                        {
                            textBatchCounts.back() += vertices.size();
                        }
                        else
                        {
                            textBatchStarts.push_back(allTextVertices.size());
                            textBatchCounts.push_back(vertices.size());
                            textBatchClips.push_back(clipStates[i].clipRect);
                            textBatchHasClips.push_back(clipStates[i].hasClip);
                        }
                        allTextVertices.insert(allTextVertices.end(), vertices.begin(), vertices.end());
                    }
                }
                break;
            }
                
            default:
                break;
        }
    }
    
    if (!rectGroups.empty())
    {
        setPipeline(ActivePipeline::Rect);
        for (const auto& pair : rectGroups)
        {
            renderRectBatch(pair.second, rectGroupClips[pair.first], rectGroupHasClip[pair.first]);
        }
    }
    
    if (!imageBatches.empty())
    {
        std::sort(imageBatches.begin(), imageBatches.end(),
            [](const ImageBatch& a, const ImageBatch& b) {
                return a.firstIndex < b.firstIndex;
            });
        
        setPipeline(ActivePipeline::Image);
        for (const auto& batch : imageBatches)
        {
            renderImageBatch(batch.indices, commands, batch.texture, batch.clipRect, batch.hasClip);
        }
    }
    
    if (!textBatchStarts.empty())
    {
        renderTextBatches(allTextVertices, textBatchStarts, textBatchCounts, textBatchClips, textBatchHasClips);
    }
    
    Rect currentClip;
    bool hasClip = false;
    
    for (size_t i = 0; i < commands.size(); ++i)
    {
        const auto& cmd = commands[i];
        
        if (cmd.type == RenderCommandType::PushClip)
        {
            currentClip = clipStates[i].clipRect;
            hasClip = clipStates[i].hasClip;
            if (hasClip)
            {
                applyScissorRect(currentClip);
            }
            continue;
        }
        else if (cmd.type == RenderCommandType::PopClip)
        {
            hasClip = clipStates[i].hasClip;
            if (hasClip)
            {
                currentClip = clipStates[i].clipRect;
                applyScissorRect(currentClip);
            }
            else
            {
                applyFullScreenScissor();
            }
            continue;
        }
        
        if (clipStates[i].hasClip && (!hasClip ||
            currentClip.x != clipStates[i].clipRect.x ||
            currentClip.y != clipStates[i].clipRect.y ||
            currentClip.width != clipStates[i].clipRect.width ||
            currentClip.height != clipStates[i].clipRect.height))
        {
            currentClip = clipStates[i].clipRect;
            hasClip = true;
            applyScissorRect(currentClip);
        }
        else if (!clipStates[i].hasClip && hasClip)
        {
            hasClip = false;
            applyFullScreenScissor();
        }
        
        switch (cmd.type)
        {
            case RenderCommandType::DrawLine:
                renderLine(cmd.lineStart, cmd.lineEnd, cmd.color, cmd.lineWidth);
                break;
                
            case RenderCommandType::FillTriangle:
                renderTriangle(cmd.triangleP1, cmd.triangleP2, cmd.triangleP3, cmd.color, 0.0f, true);
                break;
                
            case RenderCommandType::DrawTriangle:
                renderTriangle(cmd.triangleP1, cmd.triangleP2, cmd.triangleP3, cmd.color, cmd.borderWidth, false);
                break;
                
            case RenderCommandType::FillCircle:
                renderCircle(cmd.circleCenter, cmd.circleRadius, cmd.color, 0.0f, true);
                break;
                
            case RenderCommandType::DrawCircle:
                renderCircle(cmd.circleCenter, cmd.circleRadius, cmd.color, cmd.borderWidth, false);
                break;
                
            default:
                break;
        }
    }
    
    applyFullScreenScissor();
}

// MARK: - Rectangle Rendering
void MetalRenderer::renderRectangle(const Rect& rect, const Vec4& color, const CornerRadius& cornerRadius, float borderWidth)
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "Not initialized");
    
    @autoreleasepool
    {
        float left, right, top, bottom;
        convertToNDC(rect.x, rect.y, left, top);
        convertToNDC(rect.x + rect.width, rect.y + rect.height, right, bottom);
        
        RectVertex vertices[6];
        vertices[0] = RectVertex(Vec2(left, top), rect, cornerRadius, color, borderWidth);
        vertices[1] = RectVertex(Vec2(left, bottom), rect, cornerRadius, color, borderWidth);
        vertices[2] = RectVertex(Vec2(right, bottom), rect, cornerRadius, color, borderWidth);
        vertices[3] = RectVertex(Vec2(left, top), rect, cornerRadius, color, borderWidth);
        vertices[4] = RectVertex(Vec2(right, bottom), rect, cornerRadius, color, borderWidth);
        vertices[5] = RectVertex(Vec2(right, top), rect, cornerRadius, color, borderWidth);
        
        YUCHEN_PERF_BUFFER_CREATE();
        YUCHEN_PERF_VERTICES(6);
        
        id<MTLBuffer> vertexBuffer = [m_device newBufferWithBytes:vertices
                                                           length:sizeof(vertices)
                                                          options:MTLResourceStorageModeShared];
        
        [m_renderEncoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];
        
        YUCHEN_PERF_DRAW_CALL();
        [m_renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];
    }
}

void MetalRenderer::renderRectBatch(const std::vector<RenderCommand>& commands, const Rect& clipRect, bool hasClip)
{
    if (commands.empty()) return;
    
    @autoreleasepool
    {
        if (hasClip)
        {
            applyScissorRect(clipRect);
        }
        else
        {
            applyFullScreenScissor();
        }
        
        for (const auto& cmd : commands)
        {
            renderRectangle(cmd.rect, cmd.color, cmd.cornerRadius, cmd.borderWidth);
        }
    }
}

// MARK: - Image Rendering
void MetalRenderer::generateImageVertices(const Rect& destRect, const Rect& sourceRect, uint32_t texWidth, uint32_t texHeight, std::vector<float>& outVertices)
{
    float left, right, top, bottom;
    convertToNDC(destRect.x, destRect.y, left, top);
    convertToNDC(destRect.x + destRect.width, destRect.y + destRect.height, right, bottom);
    
    float u0 = sourceRect.x / texWidth;
    float v0 = sourceRect.y / texHeight;
    float u1 = (sourceRect.x + sourceRect.width) / texWidth;
    float v1 = (sourceRect.y + sourceRect.height) / texHeight;
    
    float vertices[] =
    {
        left,  top,    u0, v0,
        left,  bottom, u0, v1,
        right, bottom, u1, v1,
        left,  top,    u0, v0,
        right, bottom, u1, v1,
        right, top,    u1, v0
    };
    
    outVertices.insert(outVertices.end(), vertices, vertices + 24);
}

void MetalRenderer::computeNineSliceRects(const Rect& destRect, const Rect& sourceRect, const NineSliceMargins& margins, float designScale, Rect outSlices[9])
{
    float srcLeft = margins.left;
    float srcTop = margins.top;
    float srcRight = margins.right;
    float srcBottom = margins.bottom;
        
    float destLeft = srcLeft / designScale;
    float destTop = srcTop / designScale;
    float destRight = srcRight / designScale;
    float destBottom = srcBottom / designScale;
    
    float destCenterWidth = destRect.width - destLeft - destRight;
    float destCenterHeight = destRect.height - destTop - destBottom;
    
    if (destCenterWidth < 0.0f) destCenterWidth = 0.0f;
    if (destCenterHeight < 0.0f) destCenterHeight = 0.0f;
    
    outSlices[0] = Rect(destRect.x, destRect.y, destLeft, destTop);
    outSlices[1] = Rect(destRect.x + destLeft, destRect.y, destCenterWidth, destTop);
    outSlices[2] = Rect(destRect.x + destLeft + destCenterWidth, destRect.y, destRight, destTop);
    outSlices[3] = Rect(destRect.x, destRect.y + destTop, destLeft, destCenterHeight);
    outSlices[4] = Rect(destRect.x + destLeft, destRect.y + destTop, destCenterWidth, destCenterHeight);
    outSlices[5] = Rect(destRect.x + destLeft + destCenterWidth, destRect.y + destTop, destRight, destCenterHeight);
    outSlices[6] = Rect(destRect.x, destRect.y + destTop + destCenterHeight, destLeft, destBottom);
    outSlices[7] = Rect(destRect.x + destLeft, destRect.y + destTop + destCenterHeight, destCenterWidth, destBottom);
    outSlices[8] = Rect(destRect.x + destLeft + destCenterWidth, destRect.y + destTop + destCenterHeight, destRight, destBottom);
}

void MetalRenderer::generateNineSliceVertices(void* texture, const Rect& destRect, const Rect& sourceRect, const NineSliceMargins& margins, float designScale, uint32_t texWidth, uint32_t texHeight, std::vector<float>& outVertices)
{
    Rect destSlices[9];
    computeNineSliceRects(destRect, sourceRect, margins, designScale, destSlices);
    
    Rect srcSlices[9];
    srcSlices[0] = Rect(sourceRect.x, sourceRect.y, margins.left, margins.top);
    srcSlices[1] = Rect(sourceRect.x + margins.left, sourceRect.y, sourceRect.width - margins.left - margins.right, margins.top);
    srcSlices[2] = Rect(sourceRect.x + sourceRect.width - margins.right, sourceRect.y, margins.right, margins.top);
    srcSlices[3] = Rect(sourceRect.x, sourceRect.y + margins.top, margins.left, sourceRect.height - margins.top - margins.bottom);
    srcSlices[4] = Rect(sourceRect.x + margins.left, sourceRect.y + margins.top, sourceRect.width - margins.left - margins.right, sourceRect.height - margins.top - margins.bottom);
    srcSlices[5] = Rect(sourceRect.x + sourceRect.width - margins.right, sourceRect.y + margins.top, margins.right, sourceRect.height - margins.top - margins.bottom);
    srcSlices[6] = Rect(sourceRect.x, sourceRect.y + sourceRect.height - margins.bottom, margins.left, margins.bottom);
    srcSlices[7] = Rect(sourceRect.x + margins.left, sourceRect.y + sourceRect.height - margins.bottom, sourceRect.width - margins.left - margins.right, margins.bottom);
    srcSlices[8] = Rect(sourceRect.x + sourceRect.width - margins.right, sourceRect.y + sourceRect.height - margins.bottom, margins.right, margins.bottom);
    
    for (int i = 0; i < 9; ++i)
    {
        if (destSlices[i].width > 0.0f && destSlices[i].height > 0.0f && srcSlices[i].width > 0.0f && srcSlices[i].height > 0.0f)
            generateImageVertices(destSlices[i], srcSlices[i], texWidth, texHeight, outVertices);
    }
}

void MetalRenderer::renderImageBatch(const std::vector<size_t>& commandIndices, const std::vector<RenderCommand>& commands, void* texture, const Rect& clipRect, bool hasClip)
{
    if (commandIndices.empty()) return;
    
    @autoreleasepool
    {
        if (hasClip)
        {
            applyScissorRect(clipRect);
        }
        else
        {
            applyFullScreenScissor();
        }
        
        std::vector<float> vertexData;
        vertexData.reserve(commandIndices.size() * 24);
        
        for (size_t idx : commandIndices)
        {
            const auto& cmd = commands[idx];
            
            uint32_t texWidth = 0, texHeight = 0;
            float designScale = 1.0f;
            m_textureCache->getTexture(cmd.text.c_str(), texWidth, texHeight, &designScale);
            
            Rect sourceRect = cmd.sourceRect;
            if (sourceRect.width == 0.0f || sourceRect.height == 0.0f)
                sourceRect = Rect(0, 0, texWidth, texHeight);
            
            Rect destRect = cmd.rect;
            
            if (cmd.scaleMode == ScaleMode::Original)
            {
                float logicalWidth = sourceRect.width / designScale;
                float logicalHeight = sourceRect.height / designScale;
                float centerX = destRect.x + destRect.width * 0.5f;
                float centerY = destRect.y + destRect.height * 0.5f;
                destRect = Rect(centerX - logicalWidth * 0.5f, centerY - logicalHeight * 0.5f, logicalWidth, logicalHeight);
            }
            else if (cmd.scaleMode == ScaleMode::Fill)
            {
                float destAspect = destRect.width / destRect.height;
                float srcAspect = sourceRect.width / sourceRect.height;
                if (srcAspect > destAspect)
                {
                    float newHeight = destRect.width / srcAspect;
                    destRect = Rect(destRect.x, destRect.y + (destRect.height - newHeight) * 0.5f, destRect.width, newHeight);
                }
                else
                {
                    float newWidth = destRect.height * srcAspect;
                    destRect = Rect(destRect.x + (destRect.width - newWidth) * 0.5f, destRect.y, newWidth, destRect.height);
                }
            }
            
            if (cmd.scaleMode == ScaleMode::NineSlice)
            {
                YUCHEN_PERF_NINE_SLICE();
                generateNineSliceVertices(texture, destRect, sourceRect, cmd.nineSliceMargins, designScale, texWidth, texHeight, vertexData);
            }
            else
            {
                YUCHEN_PERF_IMAGE_DRAW();
                generateImageVertices(destRect, sourceRect, texWidth, texHeight, vertexData);
            }
        }
        
        if (vertexData.empty()) return;
        
        size_t vertexCount = vertexData.size() / 4;
        
        YUCHEN_PERF_BUFFER_CREATE();
        YUCHEN_PERF_VERTICES(vertexCount);
        
        id<MTLBuffer> vertexBuffer = [m_device newBufferWithBytes:vertexData.data()
                                                           length:vertexData.size() * sizeof(float)
                                                          options:MTLResourceStorageModeShared];
        
        TextureWrapper* wrapper = (__bridge TextureWrapper*)texture;
        id<MTLTexture> mtlTexture = wrapper.texture;
        
        YUCHEN_PERF_TEXTURE_SWITCH();
        [m_renderEncoder setFragmentTexture:mtlTexture atIndex:0];
        [m_renderEncoder setFragmentSamplerState:m_imageSampler atIndex:0];
        
        ViewportUniforms uniforms = getViewportUniforms();
        id<MTLBuffer> uniformBuffer = [m_device newBufferWithBytes:&uniforms
                                                            length:sizeof(ViewportUniforms)
                                                           options:MTLResourceStorageModeShared];
        
        [m_renderEncoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];
        [m_renderEncoder setVertexBuffer:uniformBuffer offset:0 atIndex:1];
        
        YUCHEN_PERF_DRAW_CALL();
        [m_renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:vertexCount];
    }
}

// MARK: - Text Rendering

void MetalRenderer::renderTextBatches(const std::vector<TextVertex>& allVertices, const std::vector<size_t>& batchStarts, const std::vector<size_t>& batchCounts, const std::vector<Rect>& batchClips, const std::vector<bool>& batchHasClips)
{
    if (allVertices.empty() || batchStarts.empty()) return;
    
    @autoreleasepool
    {
        setPipeline(ActivePipeline::Text);
        
        void* atlasTexture = getCurrentAtlasTexture();
        if (!atlasTexture) return;
        
        id<MTLTexture> texture = (__bridge id<MTLTexture>)atlasTexture;
        [m_renderEncoder setFragmentTexture:texture atIndex:0];
        [m_renderEncoder setFragmentSamplerState:m_textSampler atIndex:0];
        
        NSUInteger vertexDataSize = allVertices.size() * sizeof(TextVertex);
        YUCHEN_ASSERT_MSG(vertexDataSize <= [m_textVertexBuffer length], "Text vertex data too large");
        
        memcpy([m_textVertexBuffer contents], allVertices.data(), vertexDataSize);
        
        ViewportUniforms uniforms = getViewportUniforms();
        id<MTLBuffer> uniformBuffer = [m_device newBufferWithBytes:&uniforms
                                                            length:sizeof(ViewportUniforms)
                                                           options:MTLResourceStorageModeShared];
        [m_renderEncoder setVertexBuffer:m_textVertexBuffer offset:0 atIndex:0];
        [m_renderEncoder setVertexBuffer:uniformBuffer offset:0 atIndex:1];
        
        for (size_t i = 0; i < batchStarts.size(); ++i)
        {
            if (batchHasClips[i])
            {
                applyScissorRect(batchClips[i]);
            }
            else
            {
                applyFullScreenScissor();
            }
            
            NSUInteger indexCount = (batchCounts[i] / 4) * 6;
            NSUInteger indexOffset = (batchStarts[i] / 4) * 6 * sizeof(uint16_t);
            
            [m_renderEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                        indexCount:indexCount
                                         indexType:MTLIndexTypeUInt16
                                       indexBuffer:m_textIndexBuffer
                                 indexBufferOffset:indexOffset];
        }
    }
}

// MARK: - Shape Rendering

void MetalRenderer::renderLine(const Vec2& start, const Vec2& end, const Vec4& color, float width)
{
    @autoreleasepool
    {
        setPipeline(ActivePipeline::Shape);
        
        Vec2 direction(end.x - start.x, end.y - start.y);
        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        
        if (length < 0.001f) return;
        
        direction.x /= length;
        direction.y /= length;
        
        Vec2 perpendicular(-direction.y, direction.x);
        float halfWidth = width * 0.5f;
        
        Vec2 p1(start.x + perpendicular.x * halfWidth, start.y + perpendicular.y * halfWidth);
        Vec2 p2(start.x - perpendicular.x * halfWidth, start.y - perpendicular.y * halfWidth);
        Vec2 p3(end.x - perpendicular.x * halfWidth, end.y - perpendicular.y * halfWidth);
        Vec2 p4(end.x + perpendicular.x * halfWidth, end.y + perpendicular.y * halfWidth);
        
        ShapeVertex vertices[6];
        vertices[0] = ShapeVertex(p1, color);
        vertices[1] = ShapeVertex(p2, color);
        vertices[2] = ShapeVertex(p3, color);
        vertices[3] = ShapeVertex(p1, color);
        vertices[4] = ShapeVertex(p3, color);
        vertices[5] = ShapeVertex(p4, color);
        
        id<MTLBuffer> vertexBuffer = [m_device newBufferWithBytes:vertices
                                                           length:sizeof(vertices)
                                                          options:MTLResourceStorageModeShared];
        
        ViewportUniforms uniforms = getViewportUniforms();
        id<MTLBuffer> uniformBuffer = [m_device newBufferWithBytes:&uniforms
                                                            length:sizeof(ViewportUniforms)
                                                           options:MTLResourceStorageModeShared];
        
        [m_renderEncoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];
        [m_renderEncoder setVertexBuffer:uniformBuffer offset:0 atIndex:1];
        [m_renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];
    }
}

void MetalRenderer::renderTriangle(const Vec2& p1, const Vec2& p2, const Vec2& p3, const Vec4& color, float borderWidth, bool filled)
{
    @autoreleasepool
    {
        setPipeline(ActivePipeline::Shape);
        
        if (filled)
        {
            ShapeVertex vertices[3];
            vertices[0] = ShapeVertex(p1, color);
            vertices[1] = ShapeVertex(p2, color);
            vertices[2] = ShapeVertex(p3, color);
            
            id<MTLBuffer> vertexBuffer = [m_device newBufferWithBytes:vertices
                                                               length:sizeof(vertices)
                                                              options:MTLResourceStorageModeShared];
            
            ViewportUniforms uniforms = getViewportUniforms();
            id<MTLBuffer> uniformBuffer = [m_device newBufferWithBytes:&uniforms
                                                                length:sizeof(ViewportUniforms)
                                                               options:MTLResourceStorageModeShared];
            
            [m_renderEncoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];
            [m_renderEncoder setVertexBuffer:uniformBuffer offset:0 atIndex:1];
            [m_renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
        }
        else
        {
            renderLine(p1, p2, color, borderWidth);
            renderLine(p2, p3, color, borderWidth);
            renderLine(p3, p1, color, borderWidth);
        }
    }
}

void MetalRenderer::renderCircle(const Vec2& center, float radius, const Vec4& color, float borderWidth, bool filled)
{
    @autoreleasepool
    {
        setPipeline(ActivePipeline::Circle);
        
        float left = center.x - radius - 2.0f;
        float right = center.x + radius + 2.0f;
        float top = center.y - radius - 2.0f;
        float bottom = center.y + radius + 2.0f;
        
        float bw = filled ? 0.0f : borderWidth;
        
        CircleVertex vertices[6];
        vertices[0] = CircleVertex(Vec2(left, top), center, radius, bw, color);
        vertices[1] = CircleVertex(Vec2(left, bottom), center, radius, bw, color);
        vertices[2] = CircleVertex(Vec2(right, bottom), center, radius, bw, color);
        vertices[3] = CircleVertex(Vec2(left, top), center, radius, bw, color);
        vertices[4] = CircleVertex(Vec2(right, bottom), center, radius, bw, color);
        vertices[5] = CircleVertex(Vec2(right, top), center, radius, bw, color);
        
        id<MTLBuffer> vertexBuffer = [m_device newBufferWithBytes:vertices
                                                           length:sizeof(vertices)
                                                          options:MTLResourceStorageModeShared];
        
        ViewportUniforms uniforms = getViewportUniforms();
        id<MTLBuffer> uniformBuffer = [m_device newBufferWithBytes:&uniforms
                                                            length:sizeof(ViewportUniforms)
                                                           options:MTLResourceStorageModeShared];
        
        [m_renderEncoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];
        [m_renderEncoder setVertexBuffer:uniformBuffer offset:0 atIndex:1];
        [m_renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];
    }
}

// MARK: - Texture Management
void* MetalRenderer::getCurrentAtlasTexture() const
{
    if (!m_textRenderer) return nullptr;
    void* atlasHandle = m_textRenderer->getCurrentAtlasTexture();
    if (!atlasHandle) return nullptr;
    
    TextureWrapper* wrapper = (__bridge TextureWrapper*)atlasHandle;
    return (__bridge void*)wrapper.texture;
}

void* MetalRenderer::createTexture2D(uint32_t width, uint32_t height, TextureFormat format)
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "Renderer not initialized");
    
    MTLPixelFormat pixelFormat = (format == TextureFormat::R8_Unorm) ? MTLPixelFormatR8Unorm : MTLPixelFormatRGBA8Unorm;
    
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pixelFormat
                                                                                          width:width
                                                                                         height:height
                                                                                      mipmapped:NO];
    descriptor.usage = MTLTextureUsageShaderRead;
    descriptor.storageMode = MTLStorageModeShared;
    
    id<MTLTexture> texture = [m_device newTextureWithDescriptor:descriptor];
    YUCHEN_ASSERT_MSG(texture != nil, "Failed to create texture");
    
    TextureWrapper* wrapper = [[TextureWrapper alloc] init];
    wrapper.texture = texture;
    
    return (__bridge_retained void*)wrapper;
}

void MetalRenderer::updateTexture2D(void* texture, uint32_t x, uint32_t y, uint32_t width, uint32_t height, const void* data, size_t bytesPerRow)
{
    YUCHEN_ASSERT_MSG(texture != nullptr, "Texture is null");
    YUCHEN_ASSERT_MSG(data != nullptr, "Data is null");
    
    TextureWrapper* wrapper = (__bridge TextureWrapper*)texture;
    MTLRegion region = MTLRegionMake2D(x, y, width, height);
    [wrapper.texture replaceRegion:region mipmapLevel:0 withBytes:data bytesPerRow:bytesPerRow];
}

void MetalRenderer::destroyTexture(void* texture)
{
    if (!texture) return;
    
    @autoreleasepool
    {
        TextureWrapper* wrapper = (__bridge_transfer TextureWrapper*)texture;
        wrapper.texture = nil;
    }
}

// MARK: - Utilities
void MetalRenderer::convertToNDC(float x, float y, float& ndcX, float& ndcY) const
{
    ndcX = (x / static_cast<float>(m_width)) * 2.0f - 1.0f;
    ndcY = 1.0f - (y / static_cast<float>(m_height)) * 2.0f;
}

ViewportUniforms MetalRenderer::getViewportUniforms() const
{
    ViewportUniforms uniforms;
    uniforms.viewportSize = Vec2(static_cast<float>(m_width), static_cast<float>(m_height));
    return uniforms;
}

} // namespace YuchenUI
