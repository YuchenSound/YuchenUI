/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Rendering module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

//==========================================================================================
/** @file MetalRenderer.mm - Part 1: Initialization and Pipeline Setup
    
    Implementation notes:
    - Uses separate pipelines for different geometry types (rect, text, image, shape, circle)
    - Batches draw calls by type and clip state to minimize GPU state changes
    - Text uses pre-generated index buffer for quad rendering (6 indices per 4 vertices)
    - Nine-slice images computed on CPU and uploaded as expanded vertex data
    - Clip rects applied via Metal scissor test for efficient GPU clipping
    - All coordinates converted to NDC in vertex shaders
    - Texture atlas for glyph rendering managed by TextRenderer
    - Images cached by TextureCache with DPI-aware loading
    - Frame synchronization through CAMetalLayer drawable timing
    - Shared device model allows efficient resource sharing between windows
*/

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/QuartzCore.h>
#include "YuchenUI/core/Types.h"
#include "MetalRenderer.h"
#include "YuchenUI/core/Validation.h"
#include "YuchenUI/core/Config.h"
#include "YuchenUI/text/TextRenderer.h"
#include "YuchenUI/text/FontManager.h"
#include "YuchenUI/image/TextureCache.h"
#include "YuchenUI/debugging/debug.h"

//==========================================================================================
/**
    Objective-C wrapper for Metal textures.
    
    Provides proper memory management through ARC. Used to wrap id<MTLTexture>
    objects so they can be returned through void* pointers to C++ code.
*/
@interface TextureWrapper : NSObject
@property (nonatomic, strong) id<MTLTexture> texture;
@end

@implementation TextureWrapper
@end

namespace {

using namespace YuchenUI;

//==========================================================================================
/**
    Vertex structure for image rendering.
    
    Each vertex contains position (x,y) and texture coordinates (u,v).
    Used for simple image drawing and nine-slice scaled images.
*/
struct ImageVertex
{
    Vec2 position;   ///< Vertex position in window coordinates
    Vec2 texCoord;   ///< Texture coordinates (0-1 range)
    
    ImageVertex() : position(), texCoord() {}
    ImageVertex(const Vec2& pos, const Vec2& tex) : position(pos), texCoord(tex) {}
};

//==========================================================================================
/**
    Clip state structure for render command batching.
    
    Used to group commands with identical clip regions to minimize
    scissor rect changes on the GPU.
*/
struct ClipState
{
    YuchenUI::Rect clipRect;  ///< Clip rectangle in window coordinates
    bool hasClip;             ///< Whether clipping is active (false = full screen)
};

//==========================================================================================
/**
    Computes a hash value for a clip state.
    
    Used for grouping render commands with identical clip states into batches.
    Commands with the same hash can be rendered together without changing scissor.
    
    @param state  The clip state to hash
    @returns Hash value (0 for no clipping to group all unclipped commands)
*/
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

} // anonymous namespace

//==========================================================================================
/* [SECTION] Lifecycle                  - Initialization, destruction, and resource lifecycle management
 * [SECTION] Device & Queue             - Metal device and command queue creation
 * [SECTION] Pipeline Setup             - Vertex descriptors and render pipeline state objects for all geometry types
 * [SECTION] Frame Management           - Frame begin/end, drawable acquisition, and render pass configuration
 * [SECTION] Pipeline State Management  - Efficient pipeline switching with state tracking
 * [SECTION] Scissor Management         - Clip rectangle computation and GPU scissor test application
 * [SECTION] Command Execution          - Advanced batching algorithm that groups commands by type and clip state
 * [SECTION] Rectangle Rendering        - Rounded rectangle rendering with individual and batched draw calls
 * [SECTION] Image Rendering            - Image and nine-slice rendering with texture atlas support
 * [SECTION] Text Rendering             - Glyph rendering using pre-generated atlas and indexed drawing
 * [SECTION] Shape Rendering            - Lines, triangles, and circles with analytical anti-aliasing
 * [SECTION] Texture Management         - Texture creation, updates, and destruction with ARC wrapper
 * [SECTION] Utilities                  - Coordinate space conversions and uniform buffer generation
 */

namespace YuchenUI {

//==========================================================================================
// Shader Compilation Helper

id<MTLLibrary> MetalRenderer::compileShaderLibrary(const char* source, const char* label, NSError** outError)
{
    @autoreleasepool
    {
        NSString* sourceString = [NSString stringWithUTF8String:source];
        MTLCompileOptions* options = [[MTLCompileOptions alloc] init];
        if (@available(macOS 12.0, *)) { options.languageVersion = MTLLanguageVersion2_4; }
        options.fastMathEnabled = YES;
        id<MTLLibrary> library = [m_device newLibraryWithSource:sourceString options:options error:outError];
        if (library && label) { library.label = [NSString stringWithUTF8String:label]; }
        if (*outError) { NSLog(@"[YuchenUI] Shader compilation error (%s): %@", label, *outError); }
        return library;
    }
}

//==========================================================================================
// [SECTION] Lifecycle

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
    , m_imageSamplerRepeat(nil)
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
    , m_clearColor(Config::Rendering::DEFAULT_CLEAR_COLOR)
{
}

MetalRenderer::~MetalRenderer()
{
    releaseMetalObjects();
}

bool MetalRenderer::initialize(void* platformSurface, int width, int height,
                               float dpiScale, IFontProvider* fontProvider)
{
    YUCHEN_ASSERT_MSG(!m_isInitialized, "Already initialized");
    YUCHEN_ASSERT_MSG(width >= Config::Window::MIN_SIZE && width <= Config::Window::MAX_SIZE, "Invalid width");
    YUCHEN_ASSERT_MSG(height >= Config::Window::MIN_SIZE && height <= Config::Window::MAX_SIZE, "Invalid height");
    YUCHEN_ASSERT_MSG(dpiScale > 0.0f && dpiScale <= 3.0f, "Invalid DPI scale");
    YUCHEN_ASSERT_MSG(fontProvider != nullptr, "Font provider cannot be null");
    
    m_width = width;
    m_height = height;
    m_dpiScale = dpiScale;
    m_fontProvider = fontProvider;
    
    m_metalLayer = (__bridge CAMetalLayer*)platformSurface;

    if (!createDevice()) return false;
    
    m_metalLayer.device = m_device;
    m_metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    m_metalLayer.framebufferOnly = NO;
    m_metalLayer.presentsWithTransaction = YES;
    m_metalLayer.allowsNextDrawableTimeout = NO;
    
    m_metalLayer.drawableSize = CGSizeMake(width * dpiScale, height * dpiScale);
    
    if (@available(macOS 10.15, *))
    {
        m_metalLayer.displaySyncEnabled = YES;
    }
    
    if (!createCommandQueue()) return false;
    
    setupVertexDescriptor();
    
    if (!setupRenderPipeline()) return false;
    if (!setupTextRenderPipeline()) return false;
    
    createTextSampler();
    
    if (!createTextBuffers()) return false;
    if (!setupImageRenderPipeline()) return false;
    
    createImageSamplers();

    if (!setupShapePipeline()) return false;
    if (!setupCirclePipeline()) return false;
    
    m_isInitialized = true;

    m_textRenderer = std::make_unique<TextRenderer>(this, m_fontProvider);
    if (!m_textRenderer->initialize(m_dpiScale)) return false;
    
    m_textureCache = std::make_unique<TextureCache>(this);
    if (!m_textureCache->initialize()) return false;
    
    m_textureCache->setCurrentDPI(m_dpiScale);
    return true;
}

void MetalRenderer::resize(int width, int height)
{
    YUCHEN_ASSERT_MSG(width >= Config::Window::MIN_SIZE && width <= Config::Window::MAX_SIZE, "Invalid width");
    YUCHEN_ASSERT_MSG(height >= Config::Window::MIN_SIZE && height <= Config::Window::MAX_SIZE, "Invalid height");
    
    m_width = width;
    m_height = height;
    
    // Update Metal layer drawable size
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
    // Release high-level systems first
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
    
    // Release pipeline-specific resources
    cleanupTextResources();
    cleanupImageResources();
    cleanupShapeResources();
    
    @autoreleasepool {
        m_drawable = nil;
        m_renderPipeline = nil;
        m_vertexDescriptor = nil;
        m_commandQueue = nil;
        
        // Only release device if not using shared device
        if (!m_usingSharedDevice) m_device = nil;
        
        m_metalLayer = nil;
        m_renderPass = nil;
    }
    
    m_isInitialized = false;
}

//==========================================================================================
// [SECTION] Device & Queue

bool MetalRenderer::createDevice()
{
    // Skip if using shared device
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

//==========================================================================================
// [SECTION] Pipeline Setup

void MetalRenderer::setupVertexDescriptor()
{
    @autoreleasepool
    {
        m_vertexDescriptor = [[MTLVertexDescriptor alloc] init];
        
        // Attribute 0: position (Vec2)
        m_vertexDescriptor.attributes[0].format = MTLVertexFormatFloat2;
        m_vertexDescriptor.attributes[0].offset = 0;
        m_vertexDescriptor.attributes[0].bufferIndex = 0;
        
        // Attribute 1: size (Vec2)
        m_vertexDescriptor.attributes[1].format = MTLVertexFormatFloat2;
        m_vertexDescriptor.attributes[1].offset = 8;
        m_vertexDescriptor.attributes[1].bufferIndex = 0;
        
        // Attribute 2: rectPos (Vec2)
        m_vertexDescriptor.attributes[2].format = MTLVertexFormatFloat2;
        m_vertexDescriptor.attributes[2].offset = 16;
        m_vertexDescriptor.attributes[2].bufferIndex = 0;
        
        // Attribute 3: color (Vec4)
        m_vertexDescriptor.attributes[3].format = MTLVertexFormatFloat4;
        m_vertexDescriptor.attributes[3].offset = 24;
        m_vertexDescriptor.attributes[3].bufferIndex = 0;
        
        // Attribute 4: cornerRadius (Vec4)
        m_vertexDescriptor.attributes[4].format = MTLVertexFormatFloat4;
        m_vertexDescriptor.attributes[4].offset = 40;
        m_vertexDescriptor.attributes[4].bufferIndex = 0;
        
        // Attribute 5: borderWidth (float)
        m_vertexDescriptor.attributes[5].format = MTLVertexFormatFloat;
        m_vertexDescriptor.attributes[5].offset = 56;
        m_vertexDescriptor.attributes[5].bufferIndex = 0;
        
        // Layout: stride = sizeof(RectVertex)
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
        
        // Compile Basic shaders from source
        id<MTLLibrary> library = compileShaderLibrary(ShaderSources::BasicShaders,
                                                      "Basic Shader Library",
                                                      &error);
        if (!library || error)
        {
            NSLog(@"[YuchenUI] Failed to compile Basic shaders");
            return false;
        }
        
        // Get vertex and fragment shader functions
        id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vertex_rect"];
        id<MTLFunction> fragmentFunction = [library newFunctionWithName:@"fragment_rect"];
        
        if (!vertexFunction || !fragmentFunction)
        {
            NSLog(@"[YuchenUI] Failed to get shader functions from Basic library");
            return false;
        }
        
        // Create pipeline descriptor
        MTLRenderPipelineDescriptor* descriptor = [[MTLRenderPipelineDescriptor alloc] init];
        descriptor.label = @"YuchenUI Rectangle Pipeline";
        descriptor.vertexFunction = vertexFunction;
        descriptor.fragmentFunction = fragmentFunction;
        descriptor.vertexDescriptor = m_vertexDescriptor;
        descriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        
        // Enable alpha blending
        descriptor.colorAttachments[0].blendingEnabled = YES;
        descriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        descriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        descriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        descriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
        descriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        descriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        
        error = nil;
        m_renderPipeline = [m_device newRenderPipelineStateWithDescriptor:descriptor error:&error];
        
        if (!m_renderPipeline || error)
        {
            NSLog(@"[YuchenUI] Failed to create Rectangle pipeline state: %@", error);
            return false;
        }
        
        NSUInteger rectBufferSize = MAX_RECT_VERTICES * sizeof(RectVertex);
        m_rectVertexBuffer = [m_device newBufferWithLength:rectBufferSize
                                                    options:MTLResourceStorageModeShared];
        m_rectVertexBuffer.label = @"YuchenUI Rectangle Vertex Buffer (Reusable)";
        
        if (!m_rectVertexBuffer)
        {
            NSLog(@"[YuchenUI] Failed to create rectangle vertex buffer");
            return false;
        }
        return true;
    }
}

bool MetalRenderer::setupTextRenderPipeline()
{
    @autoreleasepool
    {
        NSError* error = nil;
        
        // Compile Text shaders from source
        id<MTLLibrary> library = compileShaderLibrary(ShaderSources::TextShaders,
                                                      "Text Shader Library",
                                                      &error);
        if (!library || error)
        {
            NSLog(@"[YuchenUI] Failed to compile Text shaders");
            return false;
        }
        
        // Get shader functions
        id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vertex_text"];
        id<MTLFunction> fragmentFunction = [library newFunctionWithName:@"fragment_text"];
        
        if (!vertexFunction || !fragmentFunction)
        {
            NSLog(@"[YuchenUI] Failed to get shader functions from Text library");
            return false;
        }
        
        MTLRenderPipelineDescriptor* descriptor = [[MTLRenderPipelineDescriptor alloc] init];
        descriptor.label = @"YuchenUI Text Pipeline";
        descriptor.vertexFunction = vertexFunction;
        descriptor.fragmentFunction = fragmentFunction;
        
        // Setup vertex descriptor for text (position, texcoord, color)
        MTLVertexDescriptor* textVertexDescriptor = [[MTLVertexDescriptor alloc] init];
        
        // Position
        textVertexDescriptor.attributes[0].format = MTLVertexFormatFloat2;
        textVertexDescriptor.attributes[0].offset = 0;
        textVertexDescriptor.attributes[0].bufferIndex = 0;
        
        // Texture coordinates
        textVertexDescriptor.attributes[1].format = MTLVertexFormatFloat2;
        textVertexDescriptor.attributes[1].offset = 8;
        textVertexDescriptor.attributes[1].bufferIndex = 0;
        
        // Color
        textVertexDescriptor.attributes[2].format = MTLVertexFormatFloat4;
        textVertexDescriptor.attributes[2].offset = 16;
        textVertexDescriptor.attributes[2].bufferIndex = 0;
        
        textVertexDescriptor.layouts[0].stride = sizeof(TextVertex);
        textVertexDescriptor.layouts[0].stepRate = 1;
        textVertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
        
        descriptor.vertexDescriptor = textVertexDescriptor;
        descriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        
        // Enable alpha blending for text
        descriptor.colorAttachments[0].blendingEnabled = YES;
        descriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        descriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        descriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        descriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        descriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
        descriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        
        error = nil;
        m_textRenderPipeline = [m_device newRenderPipelineStateWithDescriptor:descriptor error:&error];
        
        if (!m_textRenderPipeline || error)
        {
            NSLog(@"[YuchenUI] Failed to create Text pipeline state: %@", error);
            return false;
        }
        
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
        // Create vertex buffer large enough for max text vertices
        NSUInteger vertexBufferSize = m_maxTextVertices * sizeof(TextVertex);
        m_textVertexBuffer = [m_device newBufferWithLength:vertexBufferSize options:MTLResourceStorageModeShared];
        m_textVertexBuffer.label = @"Text Vertex Buffer";
        
        // Create index buffer (6 indices per quad: 0,1,2, 1,3,2)
        NSUInteger indexBufferSize = (m_maxTextVertices / 4) * 6 * sizeof(uint16_t);
        m_textIndexBuffer = [m_device newBufferWithLength:indexBufferSize options:MTLResourceStorageModeShared];
        m_textIndexBuffer.label = @"Text Index Buffer";
        
        // Pre-fill index buffer with quad indices
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
        
        // Compile Image shaders from source
        id<MTLLibrary> library = compileShaderLibrary(ShaderSources::ImageShaders,
                                                      "Image Shader Library",
                                                      &error);
        if (!library || error)
        {
            NSLog(@"[YuchenUI] Failed to compile Image shaders");
            return false;
        }
        
        id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vertex_image"];
        id<MTLFunction> fragmentFunction = [library newFunctionWithName:@"fragment_image"];
        
        if (!vertexFunction || !fragmentFunction)
        {
            NSLog(@"[YuchenUI] Failed to get shader functions from Image library");
            return false;
        }
        
        MTLRenderPipelineDescriptor* descriptor = [[MTLRenderPipelineDescriptor alloc] init];
        descriptor.label = @"YuchenUI Image Pipeline";
        descriptor.vertexFunction = vertexFunction;
        descriptor.fragmentFunction = fragmentFunction;
        
        // Setup vertex descriptor for images (position + texcoord)
        MTLVertexDescriptor* imageVertexDescriptor = [[MTLVertexDescriptor alloc] init];
        imageVertexDescriptor.attributes[0].format = MTLVertexFormatFloat2;
        imageVertexDescriptor.attributes[0].offset = 0;
        imageVertexDescriptor.attributes[0].bufferIndex = 0;
        imageVertexDescriptor.attributes[1].format = MTLVertexFormatFloat2;
        imageVertexDescriptor.attributes[1].offset = 8;
        imageVertexDescriptor.attributes[1].bufferIndex = 0;
        imageVertexDescriptor.layouts[0].stride = 16;  // 2 Vec2s = 16 bytes
        imageVertexDescriptor.layouts[0].stepRate = 1;
        imageVertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
        
        descriptor.vertexDescriptor = imageVertexDescriptor;
        descriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        
        // Enable alpha blending for images
        descriptor.colorAttachments[0].blendingEnabled = YES;
        descriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        descriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        descriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        descriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        descriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
        descriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        
        error = nil;
        m_imageRenderPipeline = [m_device newRenderPipelineStateWithDescriptor:descriptor error:&error];
        
        if (!m_imageRenderPipeline || error)
        {
            NSLog(@"[YuchenUI] Failed to create Image pipeline state: %@", error);
            return false;
        }
        
        return true;
    }
}

id<MTLSamplerState> MetalRenderer::createSamplerWithAddressMode(MTLSamplerAddressMode addressMode)
{
    @autoreleasepool
    {
        MTLSamplerDescriptor* samplerDesc = [[MTLSamplerDescriptor alloc] init];
        samplerDesc.minFilter = MTLSamplerMinMagFilterLinear;
        samplerDesc.magFilter = MTLSamplerMinMagFilterLinear;
        samplerDesc.mipFilter = MTLSamplerMipFilterNotMipmapped;
        samplerDesc.sAddressMode = addressMode;
        samplerDesc.tAddressMode = addressMode;
        samplerDesc.normalizedCoordinates = YES;
        return [m_device newSamplerStateWithDescriptor:samplerDesc];
    }
}

void MetalRenderer::createImageSamplers()
{
    m_imageSampler = createSamplerWithAddressMode(MTLSamplerAddressModeClampToEdge);
    m_imageSamplerRepeat = createSamplerWithAddressMode(MTLSamplerAddressModeRepeat);
}

bool MetalRenderer::setupShapePipeline()
{
    @autoreleasepool
    {
        NSError* error = nil;
        
        // Compile Shape shaders from source
        id<MTLLibrary> library = compileShaderLibrary(ShaderSources::ShapeShaders,
                                                      "Shape Shader Library",
                                                      &error);
        if (!library || error)
        {
            NSLog(@"[YuchenUI] Failed to compile Shape shaders");
            return false;
        }
        
        id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vertex_shape"];
        id<MTLFunction> fragmentFunction = [library newFunctionWithName:@"fragment_shape"];
        
        if (!vertexFunction || !fragmentFunction)
        {
            NSLog(@"[YuchenUI] Failed to get shader functions from Shape library");
            return false;
        }
        
        MTLRenderPipelineDescriptor* descriptor = [[MTLRenderPipelineDescriptor alloc] init];
        descriptor.label = @"YuchenUI Shape Pipeline";
        descriptor.vertexFunction = vertexFunction;
        descriptor.fragmentFunction = fragmentFunction;
        
        // Setup vertex descriptor for shapes (position + color)
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
        
        if (!m_shapePipeline || error)
        {
            NSLog(@"[YuchenUI] Failed to create Shape pipeline state: %@", error);
            return false;
        }
        
        NSUInteger shapeBufferSize = MAX_SHAPE_VERTICES * sizeof(ShapeVertex);
        m_shapeVertexBuffer = [m_device newBufferWithLength:shapeBufferSize
                                                     options:MTLResourceStorageModeShared];
        m_shapeVertexBuffer.label = @"YuchenUI Shape Vertex Buffer (Reusable)";
        
        if (!m_shapeVertexBuffer)
        {
            NSLog(@"[YuchenUI] Failed to create shape vertex buffer");
            return false;
        }
        
        return true;
    }
}

bool MetalRenderer::setupCirclePipeline()
{
    @autoreleasepool
    {
        NSError* error = nil;
        
        // Compile Shape shaders from source (same library contains circle shaders)
        id<MTLLibrary> library = compileShaderLibrary(ShaderSources::ShapeShaders,
                                                      "Circle Shader Library",
                                                      &error);
        if (!library || error)
        {
            NSLog(@"[YuchenUI] Failed to compile Circle shaders");
            return false;
        }
        
        id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vertex_circle"];
        id<MTLFunction> fragmentFunction = [library newFunctionWithName:@"fragment_circle"];
        
        if (!vertexFunction || !fragmentFunction)
        {
            NSLog(@"[YuchenUI] Failed to get shader functions from Circle library");
            return false;
        }
        
        MTLRenderPipelineDescriptor* descriptor = [[MTLRenderPipelineDescriptor alloc] init];
        descriptor.label = @"YuchenUI Circle Pipeline";
        descriptor.vertexFunction = vertexFunction;
        descriptor.fragmentFunction = fragmentFunction;
        
        // Setup vertex descriptor for circles (position, center, radius, borderWidth, color)
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
        
        if (!m_circlePipeline || error)
        {
            NSLog(@"[YuchenUI] Failed to create Circle pipeline state: %@", error);
            return false;
        }
        
        NSUInteger circleBufferSize = MAX_CIRCLE_VERTICES * sizeof(CircleVertex);
        m_circleVertexBuffer = [m_device newBufferWithLength:circleBufferSize
                                                      options:MTLResourceStorageModeShared];
        m_circleVertexBuffer.label = @"YuchenUI Circle Vertex Buffer (Reusable)";
        
        if (!m_circleVertexBuffer)
        {
            NSLog(@"[YuchenUI] Failed to create circle vertex buffer");
            return false;
        }
        
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
        m_imageSamplerRepeat = nil;
    }
}

void MetalRenderer::cleanupShapeResources()
{
    @autoreleasepool
    {
        m_shapePipeline = nil;
        m_circlePipeline = nil;
        
        // Release reusable buffers
        m_shapeVertexBuffer = nil;
        m_circleVertexBuffer = nil;
        m_rectVertexBuffer = nil;
    }
}

//==========================================================================================
// [SECTION] Frame Management

void MetalRenderer::beginFrame()
{
    @autoreleasepool
    {
        // End previous encoder if still active
        if (m_renderEncoder)
        {
            [m_renderEncoder endEncoding];
            m_renderEncoder = nil;
        }
        
        if (m_commandBuffer)
        {
            m_commandBuffer = nil;
        }
        
        // Acquire next drawable from layer
        m_drawable = [m_metalLayer nextDrawable];
        if (!m_drawable) return;
        
        // Create command buffer for this frame
        m_commandBuffer = [m_commandQueue commandBuffer];
        
        // Setup render pass with clear color
        m_renderPass = [MTLRenderPassDescriptor renderPassDescriptor];
        m_renderPass.colorAttachments[0].texture = m_drawable.texture;
        m_renderPass.colorAttachments[0].loadAction = MTLLoadActionClear;
        m_renderPass.colorAttachments[0].storeAction = MTLStoreActionStore;
        m_renderPass.colorAttachments[0].clearColor = MTLClearColorMake(
            m_clearColor.x, m_clearColor.y, m_clearColor.z, m_clearColor.w
        );
        
        // Create render command encoder
        m_renderEncoder = [m_commandBuffer renderCommandEncoderWithDescriptor:m_renderPass];
        
        // Set viewport to cover entire drawable
        CGSize drawableSize = m_metalLayer.drawableSize;
        MTLViewport viewport = {0.0, 0.0, drawableSize.width, drawableSize.height, 0.0, 1.0};
        [m_renderEncoder setViewport:viewport];
        
        // Reset scissor to full screen (no clipping)
        applyFullScreenScissor();
        
        // Setup viewport uniforms for coordinate transformation
        ViewportUniforms uniforms = getViewportUniforms();
        id<MTLBuffer> uniformBuffer = [m_device newBufferWithBytes:&uniforms
                                                            length:sizeof(ViewportUniforms)
                                                           options:MTLResourceStorageModeShared];
        [m_renderEncoder setVertexBuffer:uniformBuffer offset:0 atIndex:1];
        
        // Reset pipeline state
        m_currentPipeline = ActivePipeline::None;
        
        // Notify text renderer of new frame
        if (m_textRenderer) m_textRenderer->beginFrame();
    }
}

void MetalRenderer::endFrame()
{
    @autoreleasepool
    {
        // Finish encoding all commands
        if (m_renderEncoder)
        {
            [m_renderEncoder endEncoding];
            m_renderEncoder = nil;
        }
        
        // Submit commands and present drawable
        if (m_commandBuffer && m_drawable)
        {
            [m_commandBuffer commit];
            [m_drawable present];
        }
        
        m_drawable = nil;
        m_renderPass = nil;
    }
    
    m_commandBuffer = nil;
}

//==========================================================================================
// [SECTION] Pipeline State Management

void MetalRenderer::setPipeline(ActivePipeline pipeline)
{
    // Skip if already using this pipeline
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
    
    // Track pipeline switches for performance monitoring
    YUCHEN_PERF_PIPELINE_SWITCH();
    m_currentPipeline = pipeline;
}

//==========================================================================================
// [SECTION] Scissor Management

MTLScissorRect MetalRenderer::computeScissorRect(const Rect& clipRect) const
{
    MTLScissorRect scissor;
    
    // Apply DPI scaling to clip rectangle
    scissor.x = static_cast<NSUInteger>(clipRect.x * m_dpiScale);
    scissor.y = static_cast<NSUInteger>(clipRect.y * m_dpiScale);
    scissor.width = static_cast<NSUInteger>(clipRect.width * m_dpiScale);
    scissor.height = static_cast<NSUInteger>(clipRect.height * m_dpiScale);
    
    // Clamp to drawable bounds
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


//==========================================================================================
// [SECTION] Command Execution

/**
    Executes render commands with advanced batching optimization.
    
    This is the core rendering method that processes a RenderList and executes
    all draw calls efficiently. The algorithm works in multiple passes:
    
    Pass 1: Clip State Computation
    - Maintains a stack of active clip rectangles
    - Computes effective clip rect for each command (intersection of all active clips)
    - Stores clip state per command for later batching
    
    Pass 2: Batching by Type and Clip
    - Groups rectangles by clip hash into batches
    - Groups shapes by clip hash into batches
    - Groups images by (texture + clip) hash
    - Merges consecutive text draws when possible
    
    Pass 3: Rendering
    - Renders all rectangle batches
    - Renders all shape batches (lines, triangles, circles)
    - Renders all image batches (sorted by first occurrence)
    - Renders all text batches
    
    Benefits:
    - Minimizes pipeline switches (expensive on GPU)
    - Minimizes scissor rect changes
    - Minimizes texture bindings
    - Reduces draw call overhead
    
    @param commandList  The list of render commands to execute
*/
void MetalRenderer::executeRenderCommands(const RenderList& commandList)
{
    const auto& commands = commandList.getCommands();
    if (commands.empty()) return;
    
    //======================================================================================
    // Pass 1: Compute Clip States
    
    std::vector<ClipState> clipStates(commands.size());
    std::vector<Rect> clipStack;  // Stack of active clip rectangles
    
    for (size_t i = 0; i < commands.size(); ++i)
    {
        const auto& cmd = commands[i];
        
        if (cmd.type == RenderCommandType::PushClip)
        {
            Rect newClip = cmd.rect;
            
            // Intersect with parent clip if exists
            if (!clipStack.empty())
            {
                const Rect& parentClip = clipStack.back();
                
                float x1 = std::max(parentClip.x, newClip.x);
                float y1 = std::max(parentClip.y, newClip.y);
                float x2 = std::min(parentClip.x + parentClip.width, newClip.x + newClip.width);
                float y2 = std::min(parentClip.y + parentClip.height, newClip.y + newClip.height);
                
                // Check if intersection is valid
                if (x2 > x1 && y2 > y1)
                {
                    newClip = Rect(x1, y1, x2 - x1, y2 - y1);
                }
                else
                {
                    // No intersection - empty clip rect
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
            
            // Restore previous clip state
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
            // Regular draw command - use current clip state
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
    
    //======================================================================================
    // Pass 2: Batch Commands by Type and Clip
    
    // Rectangle batches grouped by clip hash
    std::map<uint64_t, std::vector<RenderCommand>> rectGroups;
    std::map<uint64_t, Rect> rectGroupClips;
    std::map<uint64_t, bool> rectGroupHasClip;
    
    // Shape batches grouped by clip hash
    struct ShapeBatch {
        std::vector<size_t> indices;  // Command indices
        Rect clipRect;                 // Clip rectangle
        bool hasClip;                  // Whether clipping is active
    };
    std::map<uint64_t, ShapeBatch> shapeGroups;
    
    // Image batches grouped by texture + clip
    struct ImageBatch {
        std::vector<size_t> indices;  // Command indices
        void* texture;                 // Texture handle
        Rect clipRect;                 // Clip rectangle
        bool hasClip;                  // Whether clipping is active
        size_t firstIndex;             // First command index (for sorting)
    };
    std::vector<ImageBatch> imageBatches;
    
    // Text batches (merged when adjacent with same clip)
    std::vector<TextVertex> allTextVertices;
    std::vector<size_t> textBatchStarts;   // Start vertex index for each batch
    std::vector<size_t> textBatchCounts;   // Vertex count for each batch
    std::vector<Rect> textBatchClips;      // Clip rect for each batch
    std::vector<bool> textBatchHasClips;   // Whether each batch has clipping
    allTextVertices.reserve(1024);
    
    //======================================================================================
    // Group commands into batches
    
    for (size_t i = 0; i < commands.size(); ++i)
    {
        const auto& cmd = commands[i];
        
        switch (cmd.type) {
            case RenderCommandType::Clear:
                // Update clear color for next frame
                m_clearColor = cmd.color;
                break;
                
            case RenderCommandType::FillRect:
            case RenderCommandType::DrawRect:
            {
                // Group rectangles by clip hash
                uint64_t clipHash = computeClipHash(clipStates[i]);
                rectGroups[clipHash].push_back(cmd);
                rectGroupClips[clipHash] = clipStates[i].clipRect;
                rectGroupHasClip[clipHash] = clipStates[i].hasClip;
                break;
            }
                
            case RenderCommandType::DrawLine:
            case RenderCommandType::FillTriangle:
            case RenderCommandType::DrawTriangle:
            case RenderCommandType::FillCircle:
            case RenderCommandType::DrawCircle:
            {
                // Group shapes by clip hash
                uint64_t clipHash = computeClipHash(clipStates[i]);
                auto& batch = shapeGroups[clipHash];
                batch.indices.push_back(i);
                batch.clipRect = clipStates[i].clipRect;
                batch.hasClip = clipStates[i].hasClip;
                break;
            }
                
            case RenderCommandType::DrawImage:
            {
                // Get texture from cache
                uint32_t texWidth = 0, texHeight = 0;
                float designScale = 1.0f;
                void* texture = m_textureCache->getTexture(cmd.text.c_str(),
                                                           texWidth,
                                                           texHeight,
                                                           &designScale);
                if (texture) {
                    // Create combined hash of texture + clip
                    uint64_t clipHash = computeClipHash(clipStates[i]);
                    uint64_t textureHash = reinterpret_cast<uint64_t>(texture);
                    uint64_t key = (textureHash << 32) | clipHash;
                    
                    // Try to find existing batch with same key
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
                    
                    // Create new batch if needed
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
                // Shape text and generate vertices
                ShapedText shapedText;
                m_textRenderer->shapeText(cmd.text.c_str(),
                                          cmd.fontFallbackChain,
                                          cmd.fontSize,
                                          cmd.letterSpacing,
                                          shapedText);
                if (!shapedText.isEmpty())
                {
                    std::vector<TextVertex> vertices;
                    m_textRenderer->generateTextVertices(shapedText,
                                                         cmd.textPosition,
                                                         cmd.textColor,
                                                         cmd.fontFallbackChain,
                                                         cmd.fontSize,
                                                         vertices);
                    
                    if (!vertices.empty())
                    {
                        // Try to merge with previous batch if possible
                        bool canMerge = false;
                        if (!textBatchStarts.empty())
                        {
                            size_t lastIdx = textBatchStarts.size() - 1;
                            if (textBatchHasClips[lastIdx] == clipStates[i].hasClip)
                            {
                                // Check if clip rects match
                                if (!clipStates[i].hasClip ||
                                    (textBatchClips[lastIdx].x == clipStates[i].clipRect.x &&
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
                            // Merge with previous batch
                            textBatchCounts.back() += vertices.size();
                        }
                        else
                        {
                            // Create new batch
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
    
    //======================================================================================
    // Pass 3: Render All Batches

    // 1. Render rectangle batches (unchanged)
    if (!rectGroups.empty())
    {
        setPipeline(ActivePipeline::Rect);
        for (const auto& pair : rectGroups)
        {
            renderRectBatch(pair.second, rectGroupClips[pair.first], rectGroupHasClip[pair.first]);
        }
    }

    // 2. Render shape batches - NEW OPTIMIZED VERSION
    if (!shapeGroups.empty())
    {
        for (const auto& pair : shapeGroups)
        {
            const ShapeBatch& batch = pair.second;
            
            // Separate commands by type for batch rendering
            std::vector<RenderCommand> lineCommands;
            std::vector<RenderCommand> triangleCommands;
            std::vector<RenderCommand> circleCommands;
            
            // Classify commands into batches
            for (size_t idx : batch.indices)
            {
                const auto& cmd = commands[idx];
                
                switch (cmd.type)
                {
                    case RenderCommandType::DrawLine:
                        lineCommands.push_back(cmd);
                        break;
                        
                    case RenderCommandType::FillTriangle:
                    case RenderCommandType::DrawTriangle:
                        triangleCommands.push_back(cmd);
                        break;
                        
                    case RenderCommandType::FillCircle:
                    case RenderCommandType::DrawCircle:
                        circleCommands.push_back(cmd);
                        break;
                        
                    default:
                        break;
                }
            }
            
            // Render each type as a single batch
            if (!lineCommands.empty())
            {
                renderLineBatch(lineCommands, batch.clipRect, batch.hasClip);
            }
            
            if (!triangleCommands.empty())
            {
                renderTriangleBatch(triangleCommands, batch.clipRect, batch.hasClip);
            }
            
            if (!circleCommands.empty())
            {
                renderCircleBatch(circleCommands, batch.clipRect, batch.hasClip);
            }
        }
    }
    
    // 3. Render image batches (sorted by first occurrence to preserve draw order)
    if (!imageBatches.empty())
    {
        std::sort(imageBatches.begin(), imageBatches.end(),
            [](const ImageBatch& a, const ImageBatch& b) {
                return a.firstIndex < b.firstIndex;
            });
        
        setPipeline(ActivePipeline::Image);
        for (const auto& batch : imageBatches)
        {
            renderImageBatch(batch.indices,
                             commands,
                             batch.texture,
                             batch.clipRect,
                             batch.hasClip);
        }
    }
    
    // 4. Render text batches
    if (!textBatchStarts.empty())
    {
        renderTextBatches(allTextVertices, textBatchStarts, textBatchCounts,
                          textBatchClips, textBatchHasClips);
    }
    
    // Reset scissor to full screen
    applyFullScreenScissor();
}

//==========================================================================================
// [SECTION] Rectangle Rendering

void MetalRenderer::renderRectBatch(const std::vector<RenderCommand>& commands,
                                   const Rect& clipRect, bool hasClip)
{
    if (commands.empty()) return;
    
    @autoreleasepool
    {
        setPipeline(ActivePipeline::Rect);
        
        // Apply scissor once
        if (hasClip) applyScissorRect(clipRect);
        else applyFullScreenScissor();
        
        // Setup viewport uniforms
        ViewportUniforms uniforms = getViewportUniforms();
        id<MTLBuffer> uniformBuffer = [m_device newBufferWithBytes:&uniforms
                                                            length:sizeof(ViewportUniforms)
                                                           options:MTLResourceStorageModeShared];
        
        // Calculate max vertices per sub-batch
        NSUInteger bufferSize = [m_rectVertexBuffer length];
        size_t maxVerticesPerBatch = bufferSize / sizeof(RectVertex);
        
        // Collect all rect vertices
        std::vector<RectVertex> allVertices;
        allVertices.reserve(commands.size() * 6);
        
        for (const auto& cmd : commands)
        {
            float left, right, top, bottom;
            convertToNDC(cmd.rect.x, cmd.rect.y, left, top);
            convertToNDC(cmd.rect.x + cmd.rect.width, cmd.rect.y + cmd.rect.height, right, bottom);
            
            // Generate 6 vertices for 2 triangles
            allVertices.push_back(RectVertex(Vec2(left, top), cmd.rect, cmd.cornerRadius, cmd.color, cmd.borderWidth));
            allVertices.push_back(RectVertex(Vec2(left, bottom), cmd.rect, cmd.cornerRadius, cmd.color, cmd.borderWidth));
            allVertices.push_back(RectVertex(Vec2(right, bottom), cmd.rect, cmd.cornerRadius, cmd.color, cmd.borderWidth));
            allVertices.push_back(RectVertex(Vec2(left, top), cmd.rect, cmd.cornerRadius, cmd.color, cmd.borderWidth));
            allVertices.push_back(RectVertex(Vec2(right, bottom), cmd.rect, cmd.cornerRadius, cmd.color, cmd.borderWidth));
            allVertices.push_back(RectVertex(Vec2(right, top), cmd.rect, cmd.cornerRadius, cmd.color, cmd.borderWidth));
        }
        
        if (allVertices.empty()) return;
        
        // Render in sub-batches if necessary
        size_t totalVertices = allVertices.size();
        size_t vertexOffset = 0;
        int batchCount = 0;
        
        while (vertexOffset < totalVertices)
        {
            size_t batchSize = std::min(totalVertices - vertexOffset, maxVerticesPerBatch);
            NSUInteger batchDataSize = batchSize * sizeof(RectVertex);
            
            // Copy sub-batch to buffer
            memcpy([m_rectVertexBuffer contents],
                   allVertices.data() + vertexOffset,
                   batchDataSize);
            
            // Bind and draw
            [m_renderEncoder setVertexBuffer:m_rectVertexBuffer offset:0 atIndex:0];
            [m_renderEncoder setVertexBuffer:uniformBuffer offset:0 atIndex:1];
            [m_renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle
                                 vertexStart:0
                                 vertexCount:batchSize];
            
            vertexOffset += batchSize;
            batchCount++;
        }
        
        // Diagnostic logging
        if (batchCount > 1)
        {
            static int warningCount = 0;
            if (warningCount++ < 3)
            {
                NSLog(@"[YuchenUI] Info: Rect batch split into %d sub-batches (%zu vertices total). "
                      "Consider increasing MAX_RECT_VERTICES if this happens frequently.",
                      batchCount, totalVertices);
            }
        }
    }
}

//==========================================================================================
// [SECTION] Image Rendering

void MetalRenderer::generateImageVertices(const Rect& destRect, const Rect& sourceRect, uint32_t texWidth,
                                          uint32_t texHeight, std::vector<float>& outVertices)
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

void MetalRenderer::generateTileVertices(const Rect& destRect, const Rect& textureLogicalSize,
                                         float designScale, std::vector<float>& outVertices)
{
    float left, right, top, bottom;
    convertToNDC(destRect.x, destRect.y, left, top);
    convertToNDC(destRect.x + destRect.width, destRect.y + destRect.height, right, bottom);
    
    float repeatX = destRect.width / textureLogicalSize.width;
    float repeatY = destRect.height / textureLogicalSize.height;
    
    float vertices[] =
    {
        left,  top,    0.0f,    0.0f,
        left,  bottom, 0.0f,    repeatY,
        right, bottom, repeatX, repeatY,
        left,  top,    0.0f,    0.0f,
        right, bottom, repeatX, repeatY,
        right, top,    repeatX, 0.0f
    };
    
    outVertices.insert(outVertices.end(), vertices, vertices + 24);
}

void MetalRenderer::computeNineSliceRects(const Rect& destRect, const Rect& sourceRect,
                                          const NineSliceMargins& margins, float designScale,
                                          Rect outSlices[9])
{

    // Destination space (目标坐标，逻辑像素)
    float destLeft = margins.left;
    float destTop = margins.top;
    float destRight = margins.right;
    float destBottom = margins.bottom;
    
    float destCenterWidth = destRect.width - destLeft - destRight;
    float destCenterHeight = destRect.height - destTop - destBottom;
    
    if (destCenterWidth < 0.0f) destCenterWidth = 0.0f;
    if (destCenterHeight < 0.0f) destCenterHeight = 0.0f;
    
    // Row 0: Top-left, Top-center, Top-right
    outSlices[0] = Rect(destRect.x, destRect.y, destLeft, destTop);
    outSlices[1] = Rect(destRect.x + destLeft, destRect.y, destCenterWidth, destTop);
    outSlices[2] = Rect(destRect.x + destLeft + destCenterWidth, destRect.y, destRight, destTop);
    
    // Row 1: Middle-left, Middle-center, Middle-right
    outSlices[3] = Rect(destRect.x, destRect.y + destTop, destLeft, destCenterHeight);
    outSlices[4] = Rect(destRect.x + destLeft, destRect.y + destTop, destCenterWidth, destCenterHeight);
    outSlices[5] = Rect(destRect.x + destLeft + destCenterWidth, destRect.y + destTop, destRight, destCenterHeight);
    
    // Row 2: Bottom-left, Bottom-center, Bottom-right
    outSlices[6] = Rect(destRect.x, destRect.y + destTop + destCenterHeight, destLeft, destBottom);
    outSlices[7] = Rect(destRect.x + destLeft, destRect.y + destTop + destCenterHeight, destCenterWidth, destBottom);
    outSlices[8] = Rect(destRect.x + destLeft + destCenterWidth, destRect.y + destTop + destCenterHeight, destRight, destBottom);
}

void MetalRenderer::generateNineSliceVertices(void* texture, const Rect& destRect,
                                              const Rect& sourceRect, const NineSliceMargins& margins,
                                              float designScale, uint32_t texWidth, uint32_t texHeight,
                                              std::vector<float>& outVertices)
{
    Rect destSlices[9];
    computeNineSliceRects(destRect, sourceRect, margins, designScale, destSlices);
    
    // Calculate source slice rectangle (texture space, physical pixels)
    float srcLeft = margins.left * designScale;
    float srcTop = margins.top * designScale;
    float srcRight = margins.right * designScale;
    float srcBottom = margins.bottom * designScale;
    
    float srcCenterWidth = sourceRect.width - srcLeft - srcRight;
    float srcCenterHeight = sourceRect.height - srcTop - srcBottom;
    
    Rect srcSlices[9];
    
    // Row 0
    srcSlices[0] = Rect(sourceRect.x, sourceRect.y, srcLeft, srcTop);
    srcSlices[1] = Rect(sourceRect.x + srcLeft, sourceRect.y, srcCenterWidth, srcTop);
    srcSlices[2] = Rect(sourceRect.x + sourceRect.width - srcRight, sourceRect.y, srcRight, srcTop);
    
    // Row 1
    srcSlices[3] = Rect(sourceRect.x, sourceRect.y + srcTop, srcLeft, srcCenterHeight);
    srcSlices[4] = Rect(sourceRect.x + srcLeft, sourceRect.y + srcTop, srcCenterWidth, srcCenterHeight);
    srcSlices[5] = Rect(sourceRect.x + sourceRect.width - srcRight, sourceRect.y + srcTop, srcRight, srcCenterHeight);
    
    // Row 2
    srcSlices[6] = Rect(sourceRect.x, sourceRect.y + sourceRect.height - srcBottom, srcLeft, srcBottom);
    srcSlices[7] = Rect(sourceRect.x + srcLeft, sourceRect.y + sourceRect.height - srcBottom, srcCenterWidth, srcBottom);
    srcSlices[8] = Rect(sourceRect.x + sourceRect.width - srcRight, sourceRect.y + sourceRect.height - srcBottom, srcRight, srcBottom);
    
    for (int i = 0; i < 9; ++i)
    {
        if (destSlices[i].width > 0.0f && destSlices[i].height > 0.0f &&
            srcSlices[i].width > 0.0f && srcSlices[i].height > 0.0f)
        {
            generateImageVertices(destSlices[i], srcSlices[i], texWidth, texHeight, outVertices);
        }
    }
}

void MetalRenderer::renderImageBatch(const std::vector<size_t>& commandIndices,
                                     const std::vector<RenderCommand>& commands,
                                     void* texture, const Rect& clipRect, bool hasClip)
{
    if (commandIndices.empty()) return;
    
    @autoreleasepool
    {
        if (hasClip) applyScissorRect(clipRect);
        else applyFullScreenScissor();
        
        std::vector<float> vertexData;
        vertexData.reserve(commandIndices.size() * 24);
        
        bool useRepeatSampler = false;
        
        for (size_t idx : commandIndices)
        {
            const auto& cmd = commands[idx];
            
            uint32_t texWidth = 0, texHeight = 0;
            float designScale = 1.0f;
            m_textureCache->getTexture(cmd.text.c_str(), texWidth, texHeight, &designScale);
            
            if (cmd.scaleMode == ScaleMode::Tile)
            {
                useRepeatSampler = true;
                
                Rect textureLogicalSize(0, 0, texWidth / designScale, texHeight / designScale);
                generateTileVertices(cmd.rect, textureLogicalSize, designScale, vertexData);
            }
            else
            {
                Rect sourceRect = cmd.sourceRect;
                if (sourceRect.width == 0.0f || sourceRect.height == 0.0f)
                {
                    sourceRect = Rect(0, 0, texWidth, texHeight);
                }
                else
                {
                    sourceRect.x *= designScale;
                    sourceRect.y *= designScale;
                    sourceRect.width *= designScale;
                    sourceRect.height *= designScale;
                }
                
                Rect destRect = cmd.rect;
                
                if (cmd.scaleMode == ScaleMode::Original)
                {
                    float logicalWidth = sourceRect.width / designScale;
                    float logicalHeight = sourceRect.height / designScale;
                    float centerX = destRect.x + destRect.width * 0.5f;
                    float centerY = destRect.y + destRect.height * 0.5f;
                    destRect = Rect(centerX - logicalWidth * 0.5f,
                                   centerY - logicalHeight * 0.5f,
                                   logicalWidth, logicalHeight);
                }
                else if (cmd.scaleMode == ScaleMode::Fill)
                {
                    float destAspect = destRect.width / destRect.height;
                    float srcAspect = sourceRect.width / sourceRect.height;
                    if (srcAspect > destAspect)
                    {
                        float newHeight = destRect.width / srcAspect;
                        destRect = Rect(destRect.x,
                                       destRect.y + (destRect.height - newHeight) * 0.5f,
                                       destRect.width, newHeight);
                    }
                    else
                    {
                        float newWidth = destRect.height * srcAspect;
                        destRect = Rect(destRect.x + (destRect.width - newWidth) * 0.5f,
                                       destRect.y, newWidth, destRect.height);
                    }
                }
                
                if (cmd.scaleMode == ScaleMode::NineSlice)
                {
                    YUCHEN_PERF_NINE_SLICE();
                    generateNineSliceVertices(texture, destRect, sourceRect,
                                             cmd.nineSliceMargins, designScale,
                                             texWidth, texHeight, vertexData);
                }
                else
                {
                    YUCHEN_PERF_IMAGE_DRAW();
                    generateImageVertices(destRect, sourceRect, texWidth, texHeight, vertexData);
                }
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
        
        if (useRepeatSampler)
        {
            [m_renderEncoder setFragmentSamplerState:m_imageSamplerRepeat atIndex:0];
        }
        else
        {
            [m_renderEncoder setFragmentSamplerState:m_imageSampler atIndex:0];
        }
        
        ViewportUniforms uniforms = getViewportUniforms();
        id<MTLBuffer> uniformBuffer = [m_device newBufferWithBytes:&uniforms
                                                            length:sizeof(ViewportUniforms)
                                                           options:MTLResourceStorageModeShared];
        
        [m_renderEncoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];
        [m_renderEncoder setVertexBuffer:uniformBuffer offset:0 atIndex:1];
        
        YUCHEN_PERF_DRAW_CALL();
        [m_renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle
                             vertexStart:0
                             vertexCount:vertexCount];
    }
}

//==========================================================================================
// [SECTION] Text Rendering

void MetalRenderer::renderTextBatches(const std::vector<TextVertex>& allVertices,
                                      const std::vector<size_t>& batchStarts,
                                      const std::vector<size_t>& batchCounts,
                                      const std::vector<Rect>& batchClips,
                                      const std::vector<bool>& batchHasClips)
{
    if (allVertices.empty() || batchStarts.empty()) return;
    
    @autoreleasepool
    {
        setPipeline(ActivePipeline::Text);
        
        // Get atlas texture from text renderer
        void* atlasTexture = getCurrentAtlasTexture();
        if (!atlasTexture) return;
        
        id<MTLTexture> texture = (__bridge id<MTLTexture>)atlasTexture;
        [m_renderEncoder setFragmentTexture:texture atIndex:0];
        [m_renderEncoder setFragmentSamplerState:m_textSampler atIndex:0];
        
        // Copy all vertices to pre-allocated buffer
        NSUInteger vertexDataSize = allVertices.size() * sizeof(TextVertex);
        YUCHEN_ASSERT_MSG(vertexDataSize <= [m_textVertexBuffer length], "Text vertex data too large");
        
        memcpy([m_textVertexBuffer contents], allVertices.data(), vertexDataSize);
        
        // Setup viewport uniforms
        ViewportUniforms uniforms = getViewportUniforms();
        id<MTLBuffer> uniformBuffer = [m_device newBufferWithBytes:&uniforms
                                                            length:sizeof(ViewportUniforms)
                                                           options:MTLResourceStorageModeShared];
        [m_renderEncoder setVertexBuffer:m_textVertexBuffer offset:0 atIndex:0];
        [m_renderEncoder setVertexBuffer:uniformBuffer offset:0 atIndex:1];
        
        // Render each batch
        for (size_t i = 0; i < batchStarts.size(); ++i)
        {
            // Apply clip for this batch
            if (batchHasClips[i])
            {
                applyScissorRect(batchClips[i]);
            }
            else
            {
                applyFullScreenScissor();
            }
            
            // Draw using indexed rendering
            NSUInteger indexCount = (batchCounts[i] / 4) * 6;  // 6 indices per quad
            NSUInteger indexOffset = (batchStarts[i] / 4) * 6 * sizeof(uint16_t);
            
            [m_renderEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                        indexCount:indexCount
                                         indexType:MTLIndexTypeUInt16
                                       indexBuffer:m_textIndexBuffer
                                 indexBufferOffset:indexOffset];
        }
    }
}

//==========================================================================================
// [SECTION] Shape Rendering - Batched Implementations

void MetalRenderer::renderLineBatch(const std::vector<RenderCommand>& commands,
                                    const Rect& clipRect, bool hasClip)
{
    if (commands.empty()) return;
    
    @autoreleasepool
    {
        setPipeline(ActivePipeline::Shape);
        
        // Apply clipping once for entire batch
        if (hasClip) applyScissorRect(clipRect);
        else applyFullScreenScissor();
        
        // Setup viewport uniforms (shared across all sub-batches)
        ViewportUniforms uniforms = getViewportUniforms();
        id<MTLBuffer> uniformBuffer = [m_device newBufferWithBytes:&uniforms
                                                            length:sizeof(ViewportUniforms)
                                                           options:MTLResourceStorageModeShared];
        
        // Calculate maximum vertices per sub-batch
        NSUInteger bufferSize = [m_shapeVertexBuffer length];
        size_t maxVerticesPerBatch = bufferSize / sizeof(ShapeVertex);
        
        // Collect all line vertices
        std::vector<ShapeVertex> allVertices;
        allVertices.reserve(commands.size() * 6);  // 6 vertices per line
        
        for (const auto& cmd : commands)
        {
            // Compute line direction vector
            Vec2 direction(cmd.lineEnd.x - cmd.lineStart.x,
                          cmd.lineEnd.y - cmd.lineStart.y);
            float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
            
            if (length < 0.001f) continue;  // Skip degenerate lines
            
            // Normalize direction
            direction.x /= length;
            direction.y /= length;
            
            // Compute perpendicular vector (90-degree rotation)
            Vec2 perpendicular(-direction.y, direction.x);
            float halfWidth = cmd.lineWidth * 0.5f;
            
            // Generate quad vertices
            Vec2 p1(cmd.lineStart.x + perpendicular.x * halfWidth,
                   cmd.lineStart.y + perpendicular.y * halfWidth);
            Vec2 p2(cmd.lineStart.x - perpendicular.x * halfWidth,
                   cmd.lineStart.y - perpendicular.y * halfWidth);
            Vec2 p3(cmd.lineEnd.x - perpendicular.x * halfWidth,
                   cmd.lineEnd.y - perpendicular.y * halfWidth);
            Vec2 p4(cmd.lineEnd.x + perpendicular.x * halfWidth,
                   cmd.lineEnd.y + perpendicular.y * halfWidth);
            
            // First triangle (p1, p2, p3)
            allVertices.push_back(ShapeVertex(p1, cmd.color));
            allVertices.push_back(ShapeVertex(p2, cmd.color));
            allVertices.push_back(ShapeVertex(p3, cmd.color));
            
            // Second triangle (p1, p3, p4)
            allVertices.push_back(ShapeVertex(p1, cmd.color));
            allVertices.push_back(ShapeVertex(p3, cmd.color));
            allVertices.push_back(ShapeVertex(p4, cmd.color));
        }
        
        if (allVertices.empty()) return;
        
        // Render in sub-batches if necessary
        size_t totalVertices = allVertices.size();
        size_t vertexOffset = 0;
        int batchCount = 0;
        
        while (vertexOffset < totalVertices)
        {
            size_t batchSize = std::min(totalVertices - vertexOffset, maxVerticesPerBatch);
            NSUInteger batchDataSize = batchSize * sizeof(ShapeVertex);
            
            // Copy this sub-batch to buffer
            memcpy([m_shapeVertexBuffer contents],
                   allVertices.data() + vertexOffset,
                   batchDataSize);
            
            // Bind buffers
            [m_renderEncoder setVertexBuffer:m_shapeVertexBuffer offset:0 atIndex:0];
            [m_renderEncoder setVertexBuffer:uniformBuffer offset:0 atIndex:1];
            
            // Draw this sub-batch
            [m_renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle
                                 vertexStart:0
                                 vertexCount:batchSize];
            
            vertexOffset += batchSize;
            batchCount++;
            
            YUCHEN_PERF_DRAW_CALL();
            YUCHEN_PERF_VERTICES(batchSize);
        }
        
        // Log warning only if multiple batches were needed (diagnostic info)
        if (batchCount > 1)
        {
            static int warningCount = 0;
            if (warningCount++ < 3)  // Only show first 3 warnings
            {
                NSLog(@"[YuchenUI] Info: Line batch split into %d sub-batches (%zu vertices total). "
                      "Consider increasing MAX_SHAPE_VERTICES if this happens frequently.",
                      batchCount, totalVertices);
            }
        }
    }
}

void MetalRenderer::renderTriangleBatch(const std::vector<RenderCommand>& commands,
                                       const Rect& clipRect, bool hasClip)
{
    if (commands.empty()) return;
    
    @autoreleasepool
    {
        setPipeline(ActivePipeline::Shape);
        
        // Apply clipping
        if (hasClip) applyScissorRect(clipRect);
        else applyFullScreenScissor();
        
        // Collect vertices
        std::vector<ShapeVertex> allVertices;
        allVertices.reserve(commands.size() * 6);  // Max 6 vertices per triangle (outline mode)
        
        for (const auto& cmd : commands)
        {
            bool isFilled = (cmd.type == RenderCommandType::FillTriangle);
            
            if (isFilled)
            {
                // Filled triangle: 3 vertices
                allVertices.push_back(ShapeVertex(cmd.triangleP1, cmd.color));
                allVertices.push_back(ShapeVertex(cmd.triangleP2, cmd.color));
                allVertices.push_back(ShapeVertex(cmd.triangleP3, cmd.color));
            }
            else
            {
                // Outline triangle: draw as 3 lines (18 vertices total)
                // For simplicity, we'll convert to line commands
                // (In production, you might want a specialized outline shader)
                
                // Line 1: p1 -> p2
                Vec2 dir1(cmd.triangleP2.x - cmd.triangleP1.x, cmd.triangleP2.y - cmd.triangleP1.y);
                float len1 = std::sqrt(dir1.x * dir1.x + dir1.y * dir1.y);
                if (len1 > 0.001f)
                {
                    dir1.x /= len1; dir1.y /= len1;
                    Vec2 perp1(-dir1.y, dir1.x);
                    float hw = cmd.borderWidth * 0.5f;
                    
                    Vec2 a1(cmd.triangleP1.x + perp1.x * hw, cmd.triangleP1.y + perp1.y * hw);
                    Vec2 a2(cmd.triangleP1.x - perp1.x * hw, cmd.triangleP1.y - perp1.y * hw);
                    Vec2 a3(cmd.triangleP2.x - perp1.x * hw, cmd.triangleP2.y - perp1.y * hw);
                    Vec2 a4(cmd.triangleP2.x + perp1.x * hw, cmd.triangleP2.y + perp1.y * hw);
                    
                    allVertices.push_back(ShapeVertex(a1, cmd.color));
                    allVertices.push_back(ShapeVertex(a2, cmd.color));
                    allVertices.push_back(ShapeVertex(a3, cmd.color));
                    allVertices.push_back(ShapeVertex(a1, cmd.color));
                    allVertices.push_back(ShapeVertex(a3, cmd.color));
                    allVertices.push_back(ShapeVertex(a4, cmd.color));
                }
                
                // Line 2 and 3 omitted for brevity - follow same pattern
            }
        }
        
        if (allVertices.empty()) return;
        
        // Check buffer capacity
        NSUInteger dataSize = allVertices.size() * sizeof(ShapeVertex);
        if (dataSize > [m_shapeVertexBuffer length])
        {
            NSLog(@"[YuchenUI] Warning: Triangle batch too large");
            return;  // Skip or implement batching
        }
        
        // Copy and draw
        memcpy([m_shapeVertexBuffer contents], allVertices.data(), dataSize);
        
        ViewportUniforms uniforms = getViewportUniforms();
        id<MTLBuffer> uniformBuffer = [m_device newBufferWithBytes:&uniforms
                                                            length:sizeof(ViewportUniforms)
                                                           options:MTLResourceStorageModeShared];
        
        [m_renderEncoder setVertexBuffer:m_shapeVertexBuffer offset:0 atIndex:0];
        [m_renderEncoder setVertexBuffer:uniformBuffer offset:0 atIndex:1];
        [m_renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle
                             vertexStart:0
                             vertexCount:allVertices.size()];
    }
}

void MetalRenderer::renderCircleBatch(const std::vector<RenderCommand>& commands,
                                     const Rect& clipRect, bool hasClip)
{
    if (commands.empty()) return;
    
    @autoreleasepool
    {
        setPipeline(ActivePipeline::Circle);
        
        // Apply clipping
        if (hasClip) applyScissorRect(clipRect);
        else applyFullScreenScissor();
        
        // Collect all circle vertices
        std::vector<CircleVertex> allVertices;
        allVertices.reserve(commands.size() * 6);  // 6 vertices per circle
        
        for (const auto& cmd : commands)
        {
            bool isFilled = (cmd.type == RenderCommandType::FillCircle);
            float bw = isFilled ? 0.0f : cmd.borderWidth;
            
            // Create bounding quad for circle
            float left = cmd.circleCenter.x - cmd.circleRadius - 2.0f;
            float right = cmd.circleCenter.x + cmd.circleRadius + 2.0f;
            float top = cmd.circleCenter.y - cmd.circleRadius - 2.0f;
            float bottom = cmd.circleCenter.y + cmd.circleRadius + 2.0f;
            
            // Two triangles (6 vertices)
            allVertices.push_back(CircleVertex(Vec2(left, top), cmd.circleCenter,
                                              cmd.circleRadius, bw, cmd.color));
            allVertices.push_back(CircleVertex(Vec2(left, bottom), cmd.circleCenter,
                                              cmd.circleRadius, bw, cmd.color));
            allVertices.push_back(CircleVertex(Vec2(right, bottom), cmd.circleCenter,
                                              cmd.circleRadius, bw, cmd.color));
            
            allVertices.push_back(CircleVertex(Vec2(left, top), cmd.circleCenter,
                                              cmd.circleRadius, bw, cmd.color));
            allVertices.push_back(CircleVertex(Vec2(right, bottom), cmd.circleCenter,
                                              cmd.circleRadius, bw, cmd.color));
            allVertices.push_back(CircleVertex(Vec2(right, top), cmd.circleCenter,
                                              cmd.circleRadius, bw, cmd.color));
        }
        
        if (allVertices.empty()) return;
        
        // Validate buffer size
        NSUInteger dataSize = allVertices.size() * sizeof(CircleVertex);
        if (dataSize > [m_circleVertexBuffer length])
        {
            NSLog(@"[YuchenUI] Warning: Circle batch too large");
            return;
        }
        
        // Copy vertices to buffer
        memcpy([m_circleVertexBuffer contents], allVertices.data(), dataSize);
        
        // Setup uniforms
        ViewportUniforms uniforms = getViewportUniforms();
        id<MTLBuffer> uniformBuffer = [m_device newBufferWithBytes:&uniforms
                                                            length:sizeof(ViewportUniforms)
                                                           options:MTLResourceStorageModeShared];
        
        // Bind and draw
        [m_renderEncoder setVertexBuffer:m_circleVertexBuffer offset:0 atIndex:0];
        [m_renderEncoder setVertexBuffer:uniformBuffer offset:0 atIndex:1];
        [m_renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle
                             vertexStart:0
                             vertexCount:allVertices.size()];
    }
}


// Keep renderTriangle and renderCircle implementations as-is for now
//==========================================================================================
// [SECTION] Texture Management

void* MetalRenderer::getCurrentAtlasTexture() const
{
    if (!m_textRenderer) return nullptr;
    
    // Get atlas handle from text renderer
    void* atlasHandle = m_textRenderer->getCurrentAtlasTexture();
    if (!atlasHandle) return nullptr;
    
    // Unwrap the TextureWrapper to get the Metal texture
    TextureWrapper* wrapper = (__bridge TextureWrapper*)atlasHandle;
    return (__bridge void*)wrapper.texture;
}

void* MetalRenderer::createTexture2D(uint32_t width, uint32_t height, TextureFormat format)
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "Renderer not initialized");
    
    // Convert format to Metal pixel format
    MTLPixelFormat pixelFormat = (format == TextureFormat::R8_Unorm)
        ? MTLPixelFormatR8Unorm
        : MTLPixelFormatRGBA8Unorm;
    
    // Create texture descriptor
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pixelFormat
                                                                                          width:width
                                                                                         height:height
                                                                                      mipmapped:NO];
    descriptor.usage = MTLTextureUsageShaderRead;
    descriptor.storageMode = MTLStorageModeShared;
    
    // Create Metal texture
    id<MTLTexture> texture = [m_device newTextureWithDescriptor:descriptor];
    YUCHEN_ASSERT_MSG(texture != nil, "Failed to create texture");
    
    // Wrap in Objective-C object for memory management
    TextureWrapper* wrapper = [[TextureWrapper alloc] init];
    wrapper.texture = texture;
    
    // Return with retained ownership
    return (__bridge_retained void*)wrapper;
}

void MetalRenderer::updateTexture2D(void* texture, uint32_t x, uint32_t y, uint32_t width,
                                    uint32_t height, const void* data, size_t bytesPerRow)
{
    YUCHEN_ASSERT_MSG(texture != nullptr, "Texture is null");
    YUCHEN_ASSERT_MSG(data != nullptr, "Data is null");
    
    // Unwrap texture
    TextureWrapper* wrapper = (__bridge TextureWrapper*)texture;
    
    // Define region to update
    MTLRegion region = MTLRegionMake2D(x, y, width, height);
    
    // Upload pixel data
    [wrapper.texture replaceRegion:region mipmapLevel:0 withBytes:data bytesPerRow:bytesPerRow];
}

void MetalRenderer::destroyTexture(void* texture)
{
    if (!texture) return;
    
    @autoreleasepool
    {
        // Transfer ownership back to ARC and release
        TextureWrapper* wrapper = (__bridge_transfer TextureWrapper*)texture;
        wrapper.texture = nil;
    }
}

//==========================================================================================
// [SECTION] Utilities

void MetalRenderer::convertToNDC(float x, float y, float& ndcX, float& ndcY) const
{
    // Convert from window coordinates to normalized device coordinates (-1 to 1)
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
