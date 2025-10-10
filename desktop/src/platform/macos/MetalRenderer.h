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
/** @file MetalRenderer.h
    
    Metal-based graphics rendering implementation for macOS.
    
    Key features:
    - Hardware-accelerated GPU rendering using Apple's Metal API
    - Batched rendering for rectangles, images, and text
    - Multiple render pipelines for different geometry types
    - Texture atlas support for efficient text rendering
    - Nine-slice image scaling for UI elements
    - Clipping rectangle support with scissor testing
    - DPI-aware rendering with scale factor support
*/

#pragma once

#include "YuchenUI/rendering/IGraphicsBackend.h"
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

//==========================================================================================
/**
    Uniform buffer structure for viewport dimensions.
    
    Passed to vertex shaders for coordinate transformations.
*/
struct ViewportUniforms {
    Vec2 viewportSize;  ///< Viewport size in pixels
};

//==========================================================================================
/**
    Vertex structure for shape rendering (lines, triangles).
    
    Contains position and color per vertex.
*/
struct ShapeVertex {
    Vec2 position;  ///< Vertex position
    Vec4 color;     ///< Vertex color (RGBA)
    
    ShapeVertex() : position(), color() {}
    ShapeVertex(const Vec2& pos, const Vec4& col) : position(pos), color(col) {}
};

//==========================================================================================
/**
    Vertex structure for circle rendering.
    
    Uses signed distance field approach in fragment shader for smooth circles.
*/
struct CircleVertex {
    Vec2 position;      ///< Vertex position
    Vec2 center;        ///< Circle center
    float radius;       ///< Circle radius
    float borderWidth;  ///< Border width (0 for filled)
    Vec4 color;         ///< Circle color (RGBA)
    
    CircleVertex() : position(), center(), radius(0.0f), borderWidth(0.0f), color() {}
    CircleVertex(const Vec2& pos, const Vec2& cen, float r, float bw, const Vec4& col)
        : position(pos), center(cen), radius(r), borderWidth(bw), color(col) {}
};

//==========================================================================================
/**
    Enumeration of active rendering pipelines.
    
    Used to track current pipeline state and minimize pipeline switches.
*/
enum class ActivePipeline {
    None,    ///< No pipeline active
    Rect,    ///< Rectangle rendering pipeline
    Text,    ///< Text rendering pipeline
    Image,   ///< Image rendering pipeline
    Shape,   ///< Shape rendering pipeline (lines, triangles)
    Circle   ///< Circle rendering pipeline
};

//==========================================================================================
/**
    Metal-based graphics context implementation.
    
    MetalRenderer provides hardware-accelerated 2D rendering using Apple's Metal API.
    It supports batched rendering of UI primitives (rectangles, text, images, shapes)
    with efficient state management and GPU resource utilization.
    
    Rendering workflow:
    1. beginFrame() - Starts a new frame, acquires drawable
    2. executeRenderCommands() - Processes render command list
    3. endFrame() - Commits commands and presents drawable
    
    The renderer batches similar draw calls to minimize state changes and
    pipeline switches. It maintains separate pipelines for different geometry types.
*/
class MetalRenderer : public IGraphicsBackend {
public:
    //======================================================================================
    /** Creates a MetalRenderer instance. */
    MetalRenderer();
    
    /** Destructor. Releases all Metal resources. */
    virtual ~MetalRenderer();
    
    //======================================================================================
    // GraphicsContext Interface Implementation
    
    /** Initializes the Metal renderer with all required resources.
        
        This method performs complete initialization including:
        - Metal layer configuration
        - Device and command queue creation
        - All render pipeline setup
        - Text and image rendering subsystems
        
        Must be called before any rendering operations. The metalLayer parameter must be
        a valid CAMetalLayer from the window's content view.
        
        @param metalLayer  Pointer to CAMetalLayer for rendering target
        @param width       Viewport width in points (not pixels)
        @param height      Viewport height in points (not pixels)
        @param dpiScale    Display scale factor (typically 1.0 or 2.0 on macOS)
        
        @returns True if initialization succeeded, false on failure
        
        @note This method combines the functionality of the old initialize(), setSurface(),
              and setSharedDevice() methods into a single initialization call.
    */
    bool initialize(void* platformSurface, int width, int height, float dpiScale) override;

    /** Begins a new render frame.
        
        Acquires the next drawable and creates render encoder.
    */
    void beginFrame() override;
    
    /** Ends the current render frame.
        
        Commits commands and presents the drawable to screen.
    */
    void endFrame() override;
    
    /** Resizes the rendering surface.
        
        @param width   New width in pixels
        @param height  New height in pixels
    */
    void resize(int width, int height) override;
    
    /** Returns true if the renderer has been initialized. */
    bool isInitialized() const;
    
    /** Returns the current render surface size. */
    Vec2 getRenderSize() const override;
    
    //======================================================================================
    /** Returns the current text atlas texture.
        
        @returns Pointer to the Metal texture handle
    */
    void* getCurrentAtlasTexture() const;
    
    /** Returns the DPI scale factor. */
    float getDPIScale() const override { return m_dpiScale; }
    
    //======================================================================================
    /** Creates a 2D texture.
        
        @param width   Texture width in pixels
        @param height  Texture height in pixels
        @param format  Pixel format (R8 or RGBA8)
        @returns Opaque texture handle
    */
    void* createTexture2D(uint32_t width, uint32_t height, TextureFormat format) override;
    
    /** Updates a region of a 2D texture.
        
        @param texture      Texture handle from createTexture2D
        @param x            X offset in pixels
        @param y            Y offset in pixels
        @param width        Update region width
        @param height       Update region height
        @param data         Source pixel data
        @param bytesPerRow  Number of bytes per row in source data
    */
    void updateTexture2D(void* texture, uint32_t x, uint32_t y,
                        uint32_t width, uint32_t height,
                        const void* data, size_t bytesPerRow) override;
    
    /** Destroys a texture.
        
        @param texture  Texture handle to destroy
    */
    void destroyTexture(void* texture) override;

private:
    //======================================================================================
    // Initialization
    
    /** Creates or uses shared Metal device. */
    bool createDevice();
    
    /** Creates the command queue. */
    bool createCommandQueue();
    
    /** Sets up the rectangle rendering pipeline. */
    bool setupRenderPipeline();
    
    /** Sets up the vertex descriptor for rectangle rendering. */
    void setupVertexDescriptor();
    
    /** Sets up the text rendering pipeline. */
    bool setupTextRenderPipeline();
    
    /** Creates sampler state for text rendering. */
    void createTextSampler();
    
    /** Creates vertex and index buffers for text. */
    bool createTextBuffers();
    
    /** Sets up the image rendering pipeline. */
    bool setupImageRenderPipeline();
    
    /** Creates sampler state for image rendering. */
    void createImageSampler();
    
    /** Sets up the shape rendering pipeline. */
    bool setupShapePipeline();
    
    /** Sets up the circle rendering pipeline. */
    bool setupCirclePipeline();
    
    //======================================================================================
    // Command Execution
    
    /** Executes a list of render commands.
        
        Batches commands by type and clip state for efficiency.
        
        @param commandList  The render commands to execute
    */
    void executeRenderCommands(const RenderList& commandList) override;
    
    //======================================================================================
    // Pipeline Management
    
    /** Switches to a different render pipeline if needed.
        
        @param pipeline  The pipeline to activate
    */
    void setPipeline(ActivePipeline pipeline);
    
    /** Applies a scissor rectangle for clipping.
        
        @param clipRect  The clip rectangle in window coordinates
    */
    void applyScissorRect(const Rect& clipRect);
    
    /** Computes Metal scissor rect from window coordinates.
        
        @param clipRect  Clip rectangle in window space
        @returns Metal scissor rectangle
    */
    MTLScissorRect computeScissorRect(const Rect& clipRect) const;
    
    /** Applies full-screen scissor (no clipping). */
    void applyFullScreenScissor();
    
    //======================================================================================
    // Rectangle Rendering
    
    /** Renders a single rectangle.
        
        @param rect           Rectangle bounds
        @param color          Fill/border color
        @param cornerRadius   Corner radius for rounded rectangles
        @param borderWidth    Border width (0 for filled)
    */
    void renderRectangle(const Rect& rect, const Vec4& color, const CornerRadius& cornerRadius, float borderWidth);
    
    /** Renders a batch of rectangles.
        
        @param commands  Rectangle render commands
        @param clipRect  Clip rectangle (if hasClip is true)
        @param hasClip   Whether clipping is active
    */
    void renderRectBatch(const std::vector<RenderCommand>& commands, const Rect& clipRect, bool hasClip);
    
    //======================================================================================
    // Image Rendering
    
    /** Renders a batch of images with the same texture.
        
        @param commandIndices  Indices of image commands
        @param commands        Full command list
        @param texture         Texture handle
        @param clipRect        Clip rectangle (if hasClip is true)
        @param hasClip         Whether clipping is active
    */
    void renderImageBatch(const std::vector<size_t>& commandIndices, const std::vector<RenderCommand>& commands, void* texture, const Rect& clipRect, bool hasClip);
    
    /** Generates vertices for a simple image draw.
        
        @param destRect    Destination rectangle
        @param sourceRect  Source rectangle in texture
        @param texWidth    Texture width
        @param texHeight   Texture height
        @param outVertices Output vertex data (position + texcoord)
    */
    void generateImageVertices(const Rect& destRect, const Rect& sourceRect, uint32_t texWidth, uint32_t texHeight, std::vector<float>& outVertices);
    
    /** Generates vertices for nine-slice scaled image.
        
        @param texture      Texture handle
        @param destRect     Destination rectangle
        @param sourceRect   Source rectangle in texture
        @param margins      Nine-slice margins
        @param designScale  Design-time scale factor
        @param texWidth     Texture width
        @param texHeight    Texture height
        @param outVertices  Output vertex data
    */
    void generateNineSliceVertices(void* texture, const Rect& destRect, const Rect& sourceRect, const NineSliceMargins& margins, float designScale, uint32_t texWidth, uint32_t texHeight, std::vector<float>& outVertices);
    
    /** Computes the nine slice rectangles.
        
        @param destRect    Destination rectangle
        @param sourceRect  Source rectangle
        @param margins     Nine-slice margins
        @param designScale Design scale factor
        @param outSlices   Output array of 9 rectangles
    */
    void computeNineSliceRects(const Rect& destRect, const Rect& sourceRect, const NineSliceMargins& margins, float designScale, Rect outSlices[9]);
    
    //======================================================================================
    // Text Rendering
    
    /** Renders multiple text batches.
        
        @param allVertices    All text vertices
        @param batchStarts    Start index for each batch
        @param batchCounts    Vertex count for each batch
        @param batchClips     Clip rect for each batch
        @param batchHasClips  Whether each batch has clipping
    */
    void renderTextBatches(const std::vector<TextVertex>& allVertices, const std::vector<size_t>& batchStarts, const std::vector<size_t>& batchCounts, const std::vector<Rect>& batchClips, const std::vector<bool>& batchHasClips);
    
    //======================================================================================
    // Shape Rendering
    
    /** Renders a line segment.
        
        @param start  Start point
        @param end    End point
        @param color  Line color
        @param width  Line width
    */
    void renderLine(const Vec2& start, const Vec2& end, const Vec4& color, float width);
    
    /** Renders a triangle.
        
        @param p1           First vertex
        @param p2           Second vertex
        @param p3           Third vertex
        @param color        Color
        @param borderWidth  Border width (0 for filled)
        @param filled       True for filled, false for outline
    */
    void renderTriangle(const Vec2& p1, const Vec2& p2, const Vec2& p3, const Vec4& color, float borderWidth, bool filled);
    
    /** Renders a circle.
        
        @param center       Circle center
        @param radius       Circle radius
        @param color        Color
        @param borderWidth  Border width (0 for filled)
        @param filled       True for filled, false for outline
    */
    void renderCircle(const Vec2& center, float radius, const Vec4& color, float borderWidth, bool filled);
    
    //======================================================================================
    // Utilities
    
    /** Converts window coordinates to normalized device coordinates.
        
        @param x     X coordinate in window space
        @param y     Y coordinate in window space
        @param ndcX  Output X in NDC (-1 to 1)
        @param ndcY  Output Y in NDC (-1 to 1)
    */
    void convertToNDC(float x, float y, float& ndcX, float& ndcY) const;
    
    /** Returns viewport uniforms for current frame. */
    ViewportUniforms getViewportUniforms() const;
    
    /** Cleans up text rendering resources. */
    void cleanupTextResources();
    
    /** Cleans up image rendering resources. */
    void cleanupImageResources();
    
    /** Cleans up shape rendering resources. */
    void cleanupShapeResources();
    
    /** Releases all Metal objects. */
    void releaseMetalObjects();

    //======================================================================================
    bool m_usingSharedDevice;  ///< Whether using shared device

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

    ActivePipeline m_currentPipeline;              ///< Current active pipeline
    std::unique_ptr<TextRenderer> m_textRenderer;  ///< Text rendering system
    std::unique_ptr<TextureCache> m_textureCache;  ///< Image texture cache
    size_t m_maxTextVertices;                      ///< Maximum text vertices per frame
    bool m_isInitialized;                          ///< Initialization state
    int m_width;                                   ///< Render surface width
    int m_height;                                  ///< Render surface height
    float m_dpiScale;                              ///< DPI scale factor
    Vec4 m_clearColor;                             ///< Frame clear color
};

} // namespace YuchenUI
