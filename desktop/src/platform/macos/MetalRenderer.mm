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
    
    // Configure Metal layer as rendering target
    m_metalLayer = (__bridge CAMetalLayer*)platformSurface;

    // Create Metal device (or use shared device if set)
    if (!createDevice()) return false;
    
    // Configure Metal layer with device and pixel format
    m_metalLayer.device = m_device;
    m_metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    m_metalLayer.framebufferOnly = NO;  // Allow reading back for screenshots
    m_metalLayer.presentsWithTransaction = YES;
    m_metalLayer.allowsNextDrawableTimeout = NO;
    
    // Set drawable size accounting for DPI scaling (retina displays)
    m_metalLayer.drawableSize = CGSizeMake(width * dpiScale, height * dpiScale);
    
    // Enable display synchronization if available (macOS 10.15+)
    if (@available(macOS 10.15, *))
    {
        m_metalLayer.displaySyncEnabled = YES;
    }
    
    // Create command queue for submitting GPU work
    if (!createCommandQueue()) return false;
    
    // Setup vertex descriptor for rectangle rendering
    setupVertexDescriptor();
    
    // Create all render pipelines
    if (!setupRenderPipeline()) return false;
    if (!setupTextRenderPipeline()) return false;
    
    createTextSampler();
    
    if (!createTextBuffers()) return false;
    if (!setupImageRenderPipeline()) return false;
    
    createImageSampler();
    
    if (!setupShapePipeline()) return false;
    if (!setupCirclePipeline()) return false;
    
    m_isInitialized = true;

    // Create text rendering system with font provider
    m_textRenderer = std::make_unique<TextRenderer>(this, m_fontProvider);
    if (!m_textRenderer->initialize(m_dpiScale)) return false;
    
    // Create texture cache for DPI-aware image loading
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
            [m_commandBuffer waitUntilScheduled];
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
    - Groups images by (texture + clip) hash
    - Merges consecutive text draws when possible
    - Shapes are rendered individually (less frequent, not worth batching)
    
    Pass 3: Rendering
    - Renders all rectangle batches
    - Renders all image batches (sorted by first occurrence)
    - Renders all text batches
    - Renders individual shapes with per-command clip management
    
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
    
    // Render rectangle batches
    if (!rectGroups.empty())
    {
        setPipeline(ActivePipeline::Rect);
        for (const auto& pair : rectGroups)
        {
            renderRectBatch(pair.second, rectGroupClips[pair.first], rectGroupHasClip[pair.first]);
        }
    }
    
    // Render image batches (sorted by first occurrence to preserve draw order)
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
    
    // Render text batches
    if (!textBatchStarts.empty())
    {
        renderTextBatches(allTextVertices, textBatchStarts, textBatchCounts,
                          textBatchClips, textBatchHasClips);
    }
    
    //======================================================================================
    // Render individual shape commands (with per-command clip management)
    
    Rect currentClip;
    bool hasClip = false;
    
    for (size_t i = 0; i < commands.size(); ++i)
    {
        const auto& cmd = commands[i];
        
        // Handle clip state changes
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
        
        // Update scissor if clip state changed
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
        
        // Render individual shape commands
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
    
    // Reset scissor to full screen
    applyFullScreenScissor();
}

//==========================================================================================
// [SECTION] Rectangle Rendering

void MetalRenderer::renderRectangle(const Rect& rect, const Vec4& color, const CornerRadius& cornerRadius, float borderWidth)
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "Not initialized");
    
    @autoreleasepool
    {
        // Convert to NDC coordinates
        float left, right, top, bottom;
        convertToNDC(rect.x, rect.y, left, top);
        convertToNDC(rect.x + rect.width, rect.y + rect.height, right, bottom);
        
        // Create 6 vertices for 2 triangles (quad)
        RectVertex vertices[6];
        vertices[0] = RectVertex(Vec2(left, top), rect, cornerRadius, color, borderWidth);
        vertices[1] = RectVertex(Vec2(left, bottom), rect, cornerRadius, color, borderWidth);
        vertices[2] = RectVertex(Vec2(right, bottom), rect, cornerRadius, color, borderWidth);
        vertices[3] = RectVertex(Vec2(left, top), rect, cornerRadius, color, borderWidth);
        vertices[4] = RectVertex(Vec2(right, bottom), rect, cornerRadius, color, borderWidth);
        vertices[5] = RectVertex(Vec2(right, top), rect, cornerRadius, color, borderWidth);
        
        YUCHEN_PERF_BUFFER_CREATE();
        YUCHEN_PERF_VERTICES(6);
        
        // Create and bind vertex buffer
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
        // Apply scissor once for entire batch
        if (hasClip)
        {
            applyScissorRect(clipRect);
        }
        else
        {
            applyFullScreenScissor();
        }
        
        // Render each rectangle in the batch
        for (const auto& cmd : commands)
        {
            renderRectangle(cmd.rect, cmd.color, cmd.cornerRadius, cmd.borderWidth);
        }
    }
}

//==========================================================================================
// [SECTION] Image Rendering

void MetalRenderer::generateImageVertices(const Rect& destRect, const Rect& sourceRect, uint32_t texWidth,
                                          uint32_t texHeight, std::vector<float>& outVertices)
{
    // Convert destination rect to NDC
    float left, right, top, bottom;
    convertToNDC(destRect.x, destRect.y, left, top);
    convertToNDC(destRect.x + destRect.width, destRect.y + destRect.height, right, bottom);
    
    // Compute texture coordinates (0-1 range)
    float u0 = sourceRect.x / texWidth;
    float v0 = sourceRect.y / texHeight;
    float u1 = (sourceRect.x + sourceRect.width) / texWidth;
    float v1 = (sourceRect.y + sourceRect.height) / texHeight;
    
    // Generate vertices for two triangles
    float vertices[] =
    {
        // Triangle 1
        left,  top,    u0, v0,
        left,  bottom, u0, v1,
        right, bottom, u1, v1,
        // Triangle 2
        left,  top,    u0, v0,
        right, bottom, u1, v1,
        right, top,    u1, v0
    };
    
    outVertices.insert(outVertices.end(), vertices, vertices + 24);
}

void MetalRenderer::computeNineSliceRects(const Rect& destRect, const Rect& sourceRect, const NineSliceMargins& margins,
                                          float designScale, Rect outSlices[9])
{
    // Source margins in pixels
    float srcLeft = margins.left;
    float srcTop = margins.top;
    float srcRight = margins.right;
    float srcBottom = margins.bottom;
    
    // Destination margins scaled by design scale
    float destLeft = srcLeft / designScale;
    float destTop = srcTop / designScale;
    float destRight = srcRight / designScale;
    float destBottom = srcBottom / designScale;
    
    // Compute center dimensions
    float destCenterWidth = destRect.width - destLeft - destRight;
    float destCenterHeight = destRect.height - destTop - destBottom;
    
    // Clamp to non-negative
    if (destCenterWidth < 0.0f) destCenterWidth = 0.0f;
    if (destCenterHeight < 0.0f) destCenterHeight = 0.0f;
    
    // Create 9 destination rectangles
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

void MetalRenderer::generateNineSliceVertices(void* texture, const Rect& destRect, const Rect& sourceRect, const NineSliceMargins& margins,
                                              float designScale, uint32_t texWidth, uint32_t texHeight, std::vector<float>& outVertices)
{
    // Compute destination slice rectangles
    Rect destSlices[9];
    computeNineSliceRects(destRect, sourceRect, margins, designScale, destSlices);
    
    // Compute source slice rectangles
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
    
    // Generate vertices for each slice
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
        
        for (size_t idx : commandIndices)
        {
            const auto& cmd = commands[idx];
            
            uint32_t texWidth = 0, texHeight = 0;
            float designScale = 1.0f;
            m_textureCache->getTexture(cmd.text.c_str(), texWidth, texHeight, &designScale);
            
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
// [SECTION] Shape Rendering

void MetalRenderer::renderLine(const Vec2& start, const Vec2& end, const Vec4& color, float width)
{
    @autoreleasepool
    {
        setPipeline(ActivePipeline::Shape);
        
        // Compute line direction and perpendicular
        Vec2 direction(end.x - start.x, end.y - start.y);
        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        
        if (length < 0.001f) return;  // Skip zero-length lines
        
        // Normalize direction
        direction.x /= length;
        direction.y /= length;
        
        // Perpendicular vector (rotated 90 degrees)
        Vec2 perpendicular(-direction.y, direction.x);
        float halfWidth = width * 0.5f;
        
        // Generate quad vertices
        Vec2 p1(start.x + perpendicular.x * halfWidth, start.y + perpendicular.y * halfWidth);
        Vec2 p2(start.x - perpendicular.x * halfWidth, start.y - perpendicular.y * halfWidth);
        Vec2 p3(end.x - perpendicular.x * halfWidth, end.y - perpendicular.y * halfWidth);
        Vec2 p4(end.x + perpendicular.x * halfWidth, end.y + perpendicular.y * halfWidth);
        
        // Create two triangles
        ShapeVertex vertices[6];
        vertices[0] = ShapeVertex(p1, color);
        vertices[1] = ShapeVertex(p2, color);
        vertices[2] = ShapeVertex(p3, color);
        vertices[3] = ShapeVertex(p1, color);
        vertices[4] = ShapeVertex(p3, color);
        vertices[5] = ShapeVertex(p4, color);
        
        // Create and bind vertex buffer
        id<MTLBuffer> vertexBuffer = [m_device newBufferWithBytes:vertices
                                                           length:sizeof(vertices)
                                                          options:MTLResourceStorageModeShared];
        
        // Setup uniforms
        ViewportUniforms uniforms = getViewportUniforms();
        id<MTLBuffer> uniformBuffer = [m_device newBufferWithBytes:&uniforms
                                                            length:sizeof(ViewportUniforms)
                                                           options:MTLResourceStorageModeShared];
        
        [m_renderEncoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];
        [m_renderEncoder setVertexBuffer:uniformBuffer offset:0 atIndex:1];
        [m_renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];
    }
}

void MetalRenderer::renderTriangle(const Vec2& p1, const Vec2& p2, const Vec2& p3,
                                   const Vec4& color, float borderWidth, bool filled)
{
    @autoreleasepool
    {
        setPipeline(ActivePipeline::Shape);
        
        if (filled)
        {
            // Render filled triangle
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
            // Render triangle outline using three lines
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
        
        // Create quad that bounds the circle (with small padding for anti-aliasing)
        float left = center.x - radius - 2.0f;
        float right = center.x + radius + 2.0f;
        float top = center.y - radius - 2.0f;
        float bottom = center.y + radius + 2.0f;
        
        float bw = filled ? 0.0f : borderWidth;
        
        // Generate 6 vertices for two triangles
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
