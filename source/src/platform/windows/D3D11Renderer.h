/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Rendering module (Windows/Direct3D 11).
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

#pragma once

#include "YuchenUI/rendering/GraphicsContext.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/text/GlyphCache.h"
#include <d3d11.h>
#include <dxgi1_2.h>
#include <memory>
#include <vector>
#include <map>
#include <string>

namespace YuchenUI {

class TextRenderer;
class TextureCache;

/** Constant buffer structure for viewport uniforms passed to shaders. */
struct ViewportUniforms {
    Vec2 viewportSize;    ///< Width and height of the viewport in pixels
    Vec2 _padding;        ///< Padding to meet shader alignment requirements
};

/** Vertex structure for shape rendering (lines, triangles). */
struct ShapeVertex {
    Vec2 position;    ///< Vertex position in screen space
    Vec4 color;       ///< RGBA color for this vertex
};

/** Vertex structure for circle rendering with anti-aliasing support. */
struct CircleVertex {
    Vec2 position;      ///< Vertex position in screen space
    Vec2 center;        ///< Circle center position for distance calculation
    float radius;       ///< Circle radius in pixels
    float borderWidth;  ///< Border width (0 for filled circles)
    Vec4 color;         ///< RGBA color for the circle
};

/** Active graphics pipeline state enumeration. */
enum class ActivePipeline {
    None,     ///< No pipeline bound
    Rect,     ///< Rectangle rendering pipeline
    Text,     ///< Text rendering pipeline
    Image,    ///< Image rendering pipeline
    Shape,    ///< Shape rendering pipeline (lines, triangles)
    Circle    ///< Circle rendering pipeline
};

//==========================================================================================
/**
    Direct3D 11 renderer implementation for Windows platform.
    
    This class provides a complete rendering backend using Direct3D 11, including:
    - Hardware-accelerated 2D graphics
    - Text rendering with font atlas
    - Image rendering with nine-slice support
    - Shape rendering (rectangles, circles, lines, triangles)
    - Scissor rectangle clipping
    - Render command batching for performance
    
    The renderer supports multiple windows through a shared D3D11 device, which improves
    performance and reduces memory overhead. Each window has its own swap chain.
    
    @see GraphicsContext, RenderList, TextRenderer
*/
class D3D11Renderer : public GraphicsContext {
public:
    //======================================================================================
    /** Creates a new D3D11 renderer instance. */
    D3D11Renderer();
    
    /** Destructor. Releases all D3D11 resources. */
    virtual ~D3D11Renderer();
    
    //======================================================================================
    /** Initializes the renderer with the specified dimensions.
        
        @param width      Initial render target width in pixels
        @param height     Initial render target height in pixels
        @param dpiScale   DPI scale factor (1.0 = 96 DPI, 2.0 = 192 DPI, etc.)
        
        @returns True if initialization succeeded, false otherwise
    */
    bool initialize(int width, int height, float dpiScale = 1.0f) override;
    
    /** Begins a new render frame. Must be called before executing render commands. */
    void beginFrame() override;
    
    /** Ends the current render frame and presents the result to screen. */
    void endFrame() override;
    
    /** Resizes the render target.
        
        @param width   New width in pixels
        @param height  New height in pixels
    */
    void resize(int width, int height) override;
    
    /** Returns true if the renderer is initialized and ready to render. */
    bool isInitialized() const override;
    
    /** Returns the current render target size. */
    Vec2 getRenderSize() const override;
    
    //======================================================================================
    /** Sets the window surface for rendering output.
        
        @param surface  Platform-specific window handle (HWND on Windows)
    */
    void setSurface(void* surface) override;
    
    /** Sets the shared D3D11 device to use for rendering.
        
        Multiple renderers can share a single device to reduce overhead.
        Must be called before initialize().
        
        @param device  ID3D11Device pointer
    */
    void setSharedDevice(void* device) override;
    
    /** Returns the current font atlas texture used for text rendering.
        
        @returns ID3D11ShaderResourceView pointer, or nullptr if not available
    */
    void* getCurrentAtlasTexture() const;
    
    /** Returns the DPI scale factor for this renderer. */
    float getDPIScale() const { return m_dpiScale; }
    
    //======================================================================================
    /** Creates a 2D texture resource.
        
        @param width   Texture width in pixels
        @param height  Texture height in pixels
        @param format  Pixel format (R8 or RGBA8)
        
        @returns ID3D11ShaderResourceView pointer, or nullptr on failure
    */
    void* createTexture2D(uint32_t width, uint32_t height, TextureFormat format) override;
    
    /** Updates a region of a 2D texture.
        
        @param texture      Texture handle returned from createTexture2D()
        @param x            X offset in pixels
        @param y            Y offset in pixels
        @param width        Region width in pixels
        @param height       Region height in pixels
        @param data         Source pixel data
        @param bytesPerRow  Number of bytes per row in source data
    */
    void updateTexture2D(void* texture, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                        const void* data, size_t bytesPerRow) override;
    
    /** Destroys a texture created with createTexture2D().
        
        @param texture  Texture handle to destroy
    */
    void destroyTexture(void* texture) override;

private:
    //======================================================================================
    // Device and Resource Creation
    
    /** Creates the D3D11 device and immediate context. */
    bool createDevice();
    
    /** Creates the swap chain for the window.
        
        @param hwnd  Window handle
        @returns True on success, false on failure
    */
    bool createSwapChain(HWND hwnd);
    
    /** Creates the render target view from the swap chain back buffer. */
    bool createRenderTarget();
    
    /** Loads all shader programs from compiled shader objects (.cso files). */
    bool loadShaders();
    
    /** Loads a compiled shader from a .cso file.
        
        @param path     Path to the .cso file
        @param outBlob  Output parameter for the shader bytecode blob
        @returns True on success, false if file not found or read failed
    */
    bool loadShaderFromFile(const wchar_t* path, ID3DBlob** outBlob);
    
    /** Creates all input layout objects for the various pipelines. */
    bool createInputLayouts();
    
    /** Creates blend states for alpha blending. */
    bool createBlendStates();
    
    /** Creates sampler states for texture sampling. */
    bool createSamplerStates();
    
    /** Creates rasterizer states with scissor test enabled. */
    bool createRasterizerStates();
    
    /** Creates constant buffers for shader uniforms. */
    bool createConstantBuffers();
    
    /** Releases all D3D11 resources. */
    void releaseResources();
    
    //======================================================================================
    // Render Command Execution
    
    /** Executes a list of render commands.
        
        This method batches commands by type and clip state for optimal performance.
        
        @param commandList  The render command list to execute
    */
    void executeRenderCommands(const RenderList& commandList) override;
    
    //======================================================================================
    // Pipeline Management
    
    /** Sets the active rendering pipeline.
        
        @param pipeline  The pipeline to activate
    */
    void setPipeline(ActivePipeline pipeline);
    
    //======================================================================================
    // Scissor Rectangle Management
    
    /** Applies a scissor rectangle for clipping.
        
        @param clipRect  The clip rectangle in window coordinates
    */
    void applyScissorRect(const Rect& clipRect);
    
    /** Applies a full-screen scissor rectangle (no clipping). */
    void applyFullScreenScissor();
    
    /** Computes a D3D11 scissor rectangle from a clip rectangle.
        
        @param clipRect  The clip rectangle in window coordinates
        @returns D3D11_RECT structure clamped to viewport bounds
    */
    D3D11_RECT computeScissorRect(const Rect& clipRect) const;
    
    //======================================================================================
    // Rectangle Rendering
    
    /** Renders a single rectangle with optional rounded corners and border.
        
        @param rect          Rectangle bounds
        @param color         Fill color
        @param cornerRadius  Corner radius for each corner
        @param borderWidth   Border width (0 for filled rectangle)
    */
    void renderRectangle(const Rect& rect, const Vec4& color, const CornerRadius& cornerRadius,
                        float borderWidth);
    
    /** Renders a batch of rectangles with the same clip state.
        
        @param commands  Rectangle render commands
        @param clipRect  Clip rectangle (if hasClip is true)
        @param hasClip   Whether clipping is active
    */
    void renderRectBatch(const std::vector<RenderCommand>& commands, const Rect& clipRect,
                        bool hasClip);
    
    //======================================================================================
    // Image Rendering
    
    /** Generates vertex data for a textured quad.
        
        @param destRect     Destination rectangle in screen space
        @param sourceRect   Source rectangle in texture space (pixels)
        @param texWidth     Texture width in pixels
        @param texHeight    Texture height in pixels
        @param outVertices  Output vertex data (position + texcoord)
    */
    void generateImageVertices(const Rect& destRect, const Rect& sourceRect, uint32_t texWidth,
                               uint32_t texHeight, std::vector<float>& outVertices);
    
    /** Generates vertex data for nine-slice scaled image.
        
        @param texture      Texture handle
        @param destRect     Destination rectangle in screen space
        @param sourceRect   Source rectangle in texture space
        @param margins      Nine-slice margins in texture pixels
        @param designScale  Design-time DPI scale of the texture
        @param texWidth     Texture width in pixels
        @param texHeight    Texture height in pixels
        @param outVertices  Output vertex data
    */
    void generateNineSliceVertices(void* texture, const Rect& destRect, const Rect& sourceRect,
                                   const NineSliceMargins& margins, float designScale,
                                   uint32_t texWidth, uint32_t texHeight,
                                   std::vector<float>& outVertices);
    
    /** Computes the nine destination rectangles for nine-slice scaling.
        
        @param destRect     Destination rectangle
        @param sourceRect   Source rectangle
        @param margins      Nine-slice margins
        @param designScale  Design-time DPI scale
        @param outSlices    Output array of 9 rectangles
    */
    void computeNineSliceRects(const Rect& destRect, const Rect& sourceRect,
                              const NineSliceMargins& margins, float designScale,
                              Rect outSlices[9]);
    
    /** Renders a batch of images with the same texture and clip state.
        
        @param commandIndices  Indices into the commands array
        @param commands        All render commands
        @param texture         Texture to bind
        @param clipRect        Clip rectangle (if hasClip is true)
        @param hasClip         Whether clipping is active
    */
    void renderImageBatch(const std::vector<size_t>& commandIndices,
                         const std::vector<RenderCommand>& commands, void* texture,
                         const Rect& clipRect, bool hasClip);
    
    //======================================================================================
    // Text Rendering
    
    /** Renders batched text with the font atlas texture.
        
        @param allVertices     All text vertices
        @param batchStarts     Start index for each batch
        @param batchCounts     Vertex count for each batch
        @param batchClips      Clip rectangle for each batch
        @param batchHasClips   Whether each batch has clipping
    */
    void renderTextBatches(const std::vector<TextVertex>& allVertices,
                          const std::vector<size_t>& batchStarts,
                          const std::vector<size_t>& batchCounts,
                          const std::vector<Rect>& batchClips,
                          const std::vector<bool>& batchHasClips);
    
    //======================================================================================
    // Shape Rendering
    
    /** Renders a line segment.
        
        @param start  Start point in screen space
        @param end    End point in screen space
        @param color  Line color
        @param width  Line width in pixels
    */
    void renderLine(const Vec2& start, const Vec2& end, const Vec4& color, float width);
    
    /** Renders a triangle (filled or outline).
        
        @param p1            First vertex position
        @param p2            Second vertex position
        @param p3            Third vertex position
        @param color         Triangle color
        @param borderWidth   Border width (0 for filled)
        @param filled        True for filled triangle, false for outline
    */
    void renderTriangle(const Vec2& p1, const Vec2& p2, const Vec2& p3, const Vec4& color,
                       float borderWidth, bool filled);
    
    /** Renders a circle (filled or outline).
        
        @param center        Circle center in screen space
        @param radius        Circle radius in pixels
        @param color         Circle color
        @param borderWidth   Border width (0 for filled)
        @param filled        True for filled circle, false for outline
    */
    void renderCircle(const Vec2& center, float radius, const Vec4& color, float borderWidth,
                     bool filled);
    
    //======================================================================================
    // Utility Functions
    
    /** Converts screen-space coordinates to normalized device coordinates (NDC).
        
        @param x     X coordinate in screen space
        @param y     Y coordinate in screen space
        @param ndcX  Output X in NDC (-1 to 1)
        @param ndcY  Output Y in NDC (-1 to 1)
    */
    void convertToNDC(float x, float y, float& ndcX, float& ndcY) const;
    
    /** Gets viewport uniform data for shader constant buffer. */
    ViewportUniforms getViewportUniforms() const;
    
    /** Updates the constant buffer with new viewport uniforms.
        
        @param uniforms  Uniform data to upload
    */
    void updateConstantBuffer(const ViewportUniforms& uniforms);
    
    //======================================================================================
    // Member Variables
    
    bool m_usingSharedDevice;    ///< True if using a shared device from WindowManager
    
    // Core D3D11 objects
    ID3D11Device* m_device;                    ///< D3D11 device
    ID3D11DeviceContext* m_context;            ///< Immediate rendering context
    IDXGISwapChain1* m_swapChain;             ///< Swap chain for window presentation
    ID3D11RenderTargetView* m_rtv;            ///< Render target view for back buffer
    
    // Rectangle pipeline
    ID3D11VertexShader* m_rectVS;             ///< Rectangle vertex shader
    ID3D11PixelShader* m_rectPS;              ///< Rectangle pixel shader
    ID3D11InputLayout* m_rectInputLayout;     ///< Rectangle input layout
    
    // Text pipeline
    ID3D11VertexShader* m_textVS;             ///< Text vertex shader
    ID3D11PixelShader* m_textPS;              ///< Text pixel shader
    ID3D11InputLayout* m_textInputLayout;     ///< Text input layout
    
    // Image pipeline
    ID3D11VertexShader* m_imageVS;            ///< Image vertex shader
    ID3D11PixelShader* m_imagePS;             ///< Image pixel shader
    ID3D11InputLayout* m_imageInputLayout;    ///< Image input layout
    
    // Shape pipeline
    ID3D11VertexShader* m_shapeVS;            ///< Shape vertex shader
    ID3D11PixelShader* m_shapePS;             ///< Shape pixel shader
    ID3D11InputLayout* m_shapeInputLayout;    ///< Shape input layout
    
    // Circle pipeline
    ID3D11VertexShader* m_circleVS;           ///< Circle vertex shader
    ID3D11PixelShader* m_circlePS;            ///< Circle pixel shader
    ID3D11InputLayout* m_circleInputLayout;   ///< Circle input layout
    
    // Pipeline state objects
    ID3D11BlendState* m_blendState;           ///< Alpha blending state
    ID3D11SamplerState* m_samplerState;       ///< Texture sampler state
    ID3D11RasterizerState* m_rasterizerState; ///< Rasterizer state with scissor test
    ID3D11Buffer* m_constantBuffer;           ///< Constant buffer for viewport uniforms
    
    // Text rendering buffers
    ID3D11Buffer* m_textVertexBuffer;         ///< Dynamic vertex buffer for text
    ID3D11Buffer* m_textIndexBuffer;          ///< Index buffer for text quads
    
    ActivePipeline m_currentPipeline;         ///< Currently bound pipeline
    
    std::unique_ptr<TextRenderer> m_textRenderer;    ///< Text shaping and rendering
    std::unique_ptr<TextureCache> m_textureCache;    ///< Image texture cache
    
    bool m_isInitialized;    ///< True if initialize() succeeded
    int m_width;             ///< Current render target width
    int m_height;            ///< Current render target height
    float m_dpiScale;        ///< DPI scale factor
    Vec4 m_clearColor;       ///< Clear color for beginFrame()
    HWND m_hwnd;             ///< Window handle for swap chain
    
    size_t m_maxTextVertices;    ///< Maximum number of text vertices per batch
};

}
