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

struct ViewportUniforms {
    Vec2 viewportSize;
    Vec2 _padding;
};

struct ShapeVertex {
    Vec2 position;
    Vec4 color;
};

struct CircleVertex {
    Vec2 position;
    Vec2 center;
    float radius;
    float borderWidth;
    Vec4 color;
};

enum class ActivePipeline {
    None,
    Rect,
    Text,
    Image,
    Shape,
    Circle
};

class D3D11Renderer : public GraphicsContext {
public:
    D3D11Renderer();
    virtual ~D3D11Renderer();
    
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
    void updateTexture2D(void* texture, uint32_t x, uint32_t y, uint32_t width, uint32_t height, const void* data, size_t bytesPerRow) override;
    void destroyTexture(void* texture) override;

private:
    bool createDevice();
    bool createSwapChain(HWND hwnd);
    bool createRenderTarget();
    bool loadShaders();
    bool loadShaderFromFile(const wchar_t* path, ID3DBlob** outBlob);
    bool createInputLayouts();
    bool createBlendStates();
    bool createSamplerStates();
    bool createRasterizerStates();
    bool createConstantBuffers();
    
    void executeRenderCommands(const RenderList& commandList) override;
    void releaseResources();
    
    void setPipeline(ActivePipeline pipeline);
    void applyScissorRect(const Rect& clipRect);
    void applyFullScreenScissor();
    D3D11_RECT computeScissorRect(const Rect& clipRect) const;
    
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
    void updateConstantBuffer(const ViewportUniforms& uniforms);
    
    bool m_usingSharedDevice;
    
    ID3D11Device* m_device;
    ID3D11DeviceContext* m_context;
    IDXGISwapChain1* m_swapChain;
    ID3D11RenderTargetView* m_rtv;
    
    ID3D11VertexShader* m_rectVS;
    ID3D11PixelShader* m_rectPS;
    ID3D11InputLayout* m_rectInputLayout;
    
    ID3D11VertexShader* m_textVS;
    ID3D11PixelShader* m_textPS;
    ID3D11InputLayout* m_textInputLayout;
    
    ID3D11VertexShader* m_imageVS;
    ID3D11PixelShader* m_imagePS;
    ID3D11InputLayout* m_imageInputLayout;
    
    ID3D11VertexShader* m_shapeVS;
    ID3D11PixelShader* m_shapePS;
    ID3D11InputLayout* m_shapeInputLayout;
    
    ID3D11VertexShader* m_circleVS;
    ID3D11PixelShader* m_circlePS;
    ID3D11InputLayout* m_circleInputLayout;
    
    ID3D11BlendState* m_blendState;
    ID3D11SamplerState* m_samplerState;
    ID3D11RasterizerState* m_rasterizerState;
    ID3D11Buffer* m_constantBuffer;
    
    ID3D11Buffer* m_textVertexBuffer;
    ID3D11Buffer* m_textIndexBuffer;
    
    ActivePipeline m_currentPipeline;
    std::unique_ptr<TextRenderer> m_textRenderer;
    std::unique_ptr<TextureCache> m_textureCache;
    
    bool m_isInitialized;
    int m_width;
    int m_height;
    float m_dpiScale;
    Vec4 m_clearColor;
    HWND m_hwnd;
    
    size_t m_maxTextVertices;
};

}
