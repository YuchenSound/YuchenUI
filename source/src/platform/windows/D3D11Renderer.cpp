#include "D3D11Renderer.h"
#include "YuchenUI/core/Validation.h"
#include "YuchenUI/core/Config.h"
#include "YuchenUI/core/Colors.h"
#include "YuchenUI/core/Assert.h"
#include "YuchenUI/text/TextRenderer.h"
#include "YuchenUI/image/TextureCache.h"
#include <d3dcompiler.h>
#include <fstream>
#include <algorithm>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#ifdef DrawText
#undef DrawText
#endif

#ifdef DrawTextW
#undef DrawTextW
#endif

namespace {

using namespace YuchenUI;

struct ImageVertex {
    Vec2 position;
    Vec2 texCoord;
};

struct ClipState {
    YuchenUI::Rect clipRect;
    bool hasClip;
};

uint64_t computeClipHash(const ClipState& state) {
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

D3D11Renderer::D3D11Renderer()
    : m_usingSharedDevice(false)
    , m_device(nullptr)
    , m_context(nullptr)
    , m_swapChain(nullptr)
    , m_rtv(nullptr)
    , m_rectVS(nullptr)
    , m_rectPS(nullptr)
    , m_rectInputLayout(nullptr)
    , m_textVS(nullptr)
    , m_textPS(nullptr)
    , m_textInputLayout(nullptr)
    , m_imageVS(nullptr)
    , m_imagePS(nullptr)
    , m_imageInputLayout(nullptr)
    , m_shapeVS(nullptr)
    , m_shapePS(nullptr)
    , m_shapeInputLayout(nullptr)
    , m_circleVS(nullptr)
    , m_circlePS(nullptr)
    , m_circleInputLayout(nullptr)
    , m_blendState(nullptr)
    , m_samplerState(nullptr)
    , m_rasterizerState(nullptr)
    , m_constantBuffer(nullptr)
    , m_textVertexBuffer(nullptr)
    , m_textIndexBuffer(nullptr)
    , m_currentPipeline(ActivePipeline::None)
    , m_textRenderer(nullptr)
    , m_textureCache(nullptr)
    , m_isInitialized(false)
    , m_width(0)
    , m_height(0)
    , m_dpiScale(1.0f)
    , m_clearColor(Colors::DEFAULT_CLEAR_COLOR)
    , m_hwnd(nullptr)
    , m_maxTextVertices(Config::Rendering::MAX_TEXT_VERTICES)
{
}

D3D11Renderer::~D3D11Renderer()
{
    releaseResources();
}

void D3D11Renderer::setSharedDevice(void* sharedDevice)
{
    YUCHEN_ASSERT_MSG(sharedDevice != nullptr, "Shared device cannot be null");
    YUCHEN_ASSERT_MSG(!m_isInitialized, "Cannot set shared device after initialization");
    
    m_device = static_cast<ID3D11Device*>(sharedDevice);
    m_device->AddRef();
    m_device->GetImmediateContext(&m_context);
    m_usingSharedDevice = true;
}

bool D3D11Renderer::initialize(int width, int height, float dpiScale)
{
    YUCHEN_ASSERT_MSG(!m_isInitialized, "Already initialized");

    std::cout << "[D3D11Renderer::initialize] Starting initialization..." << std::endl;
    std::cout << "[D3D11Renderer::initialize] Size: " << width << "x" << height
        << ", DPI: " << dpiScale << std::endl;
    std::cout << "[D3D11Renderer::initialize] Using shared device: "
        << (m_usingSharedDevice ? "YES" : "NO") << std::endl;

    m_width = width;
    m_height = height;
    m_dpiScale = dpiScale;

    if (!m_usingSharedDevice)
    {
        std::cout << "[D3D11Renderer::initialize] Creating device..." << std::endl;
        if (!createDevice()) {
            std::cerr << "[D3D11Renderer::initialize] Failed to create device" << std::endl;
            return false;
        }
        std::cout << "[D3D11Renderer::initialize] Device created" << std::endl;
    }

    std::cout << "[D3D11Renderer::initialize] Loading shaders..." << std::endl;
    if (!loadShaders()) {
        std::cerr << "[D3D11Renderer::initialize] Failed to load shaders" << std::endl;
        return false;
    }

    std::cout << "[D3D11Renderer::initialize] Creating input layouts..." << std::endl;
    if (!createInputLayouts()) {
        std::cerr << "[D3D11Renderer::initialize] Failed to create input layouts" << std::endl;
        return false;
    }

    std::cout << "[D3D11Renderer::initialize] Creating blend states..." << std::endl;
    if (!createBlendStates()) {
        std::cerr << "[D3D11Renderer::initialize] Failed to create blend states" << std::endl;
        return false;
    }

    std::cout << "[D3D11Renderer::initialize] Creating sampler states..." << std::endl;
    if (!createSamplerStates()) {
        std::cerr << "[D3D11Renderer::initialize] Failed to create sampler states" << std::endl;
        return false;
    }

    std::cout << "[D3D11Renderer::initialize] Creating rasterizer states..." << std::endl;
    if (!createRasterizerStates()) {
        std::cerr << "[D3D11Renderer::initialize] Failed to create rasterizer states" << std::endl;
        return false;
    }

    std::cout << "[D3D11Renderer::initialize] Creating constant buffers..." << std::endl;
    if (!createConstantBuffers()) {
        std::cerr << "[D3D11Renderer::initialize] Failed to create constant buffers" << std::endl;
        return false;
    }

    std::cout << "[D3D11Renderer::initialize] Creating text buffers..." << std::endl;
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = static_cast<UINT>(m_maxTextVertices * sizeof(TextVertex));
    vbDesc.Usage = D3D11_USAGE_DYNAMIC;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    if (FAILED(m_device->CreateBuffer(&vbDesc, nullptr, &m_textVertexBuffer))) {
        std::cerr << "[D3D11Renderer::initialize] Failed to create text vertex buffer" << std::endl;
        return false;
    }

    std::vector<uint16_t> indices((m_maxTextVertices / 4) * 6);
    for (size_t i = 0; i < m_maxTextVertices / 4; ++i)
    {
        uint16_t baseVertex = static_cast<uint16_t>(i * 4);
        size_t baseIndex = i * 6;
        indices[baseIndex + 0] = baseVertex + 0;
        indices[baseIndex + 1] = baseVertex + 1;
        indices[baseIndex + 2] = baseVertex + 2;
        indices[baseIndex + 3] = baseVertex + 1;
        indices[baseIndex + 4] = baseVertex + 3;
        indices[baseIndex + 5] = baseVertex + 2;
    }

    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.ByteWidth = static_cast<UINT>(indices.size() * sizeof(uint16_t));
    ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices.data();

    if (FAILED(m_device->CreateBuffer(&ibDesc, &ibData, &m_textIndexBuffer))) {
        std::cerr << "[D3D11Renderer::initialize] Failed to create text index buffer" << std::endl;
        return false;
    }

    m_isInitialized = true;

    std::cout << "[D3D11Renderer::initialize] Creating TextRenderer..." << std::endl;
    m_textRenderer = std::make_unique<TextRenderer>(this);
    if (!m_textRenderer->initialize(m_dpiScale)) {
        std::cerr << "[D3D11Renderer::initialize] Failed to initialize TextRenderer" << std::endl;
        return false;
    }

    std::cout << "[D3D11Renderer::initialize] Creating TextureCache..." << std::endl;
    m_textureCache = std::make_unique<TextureCache>(this);
    if (!m_textureCache->initialize()) {
        std::cerr << "[D3D11Renderer::initialize] Failed to initialize TextureCache" << std::endl;
        return false;
    }
    m_textureCache->setCurrentDPI(m_dpiScale);

    std::cout << "[D3D11Renderer::initialize] Initialization complete" << std::endl;
    return true;
}

void D3D11Renderer::setSurface(void* surface)
{
    YUCHEN_ASSERT_MSG(surface != nullptr, "Surface cannot be null");
    YUCHEN_ASSERT_MSG(m_width > 0 && m_height > 0, "Dimensions not initialized");
    
    m_hwnd = static_cast<HWND>(surface);
    
    if (m_swapChain)
    {
        if (m_rtv)
        {
            m_rtv->Release();
            m_rtv = nullptr;
        }
        m_swapChain->Release();
        m_swapChain = nullptr;
    }
    
    if (!createSwapChain(m_hwnd)) return;
    if (!createRenderTarget()) return;
}

void D3D11Renderer::resize(int width, int height)
{
    YUCHEN_ASSERT_MSG(width >= Config::Window::MIN_SIZE && width <= Config::Window::MAX_SIZE, "Invalid width");
    YUCHEN_ASSERT_MSG(height >= Config::Window::MIN_SIZE && height <= Config::Window::MAX_SIZE, "Invalid height");
    
    m_width = width;
    m_height = height;
    
    if (m_swapChain)
    {
        if (m_rtv)
        {
            m_rtv->Release();
            m_rtv = nullptr;
        }
        
        m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
        createRenderTarget();
    }
}

Vec2 D3D11Renderer::getRenderSize() const
{
    return Vec2(static_cast<float>(m_width), static_cast<float>(m_height));
}

bool D3D11Renderer::isInitialized() const
{
    return m_isInitialized;
}

void D3D11Renderer::releaseResources()
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
    
    if (m_textIndexBuffer) { m_textIndexBuffer->Release(); m_textIndexBuffer = nullptr; }
    if (m_textVertexBuffer) { m_textVertexBuffer->Release(); m_textVertexBuffer = nullptr; }
    if (m_constantBuffer) { m_constantBuffer->Release(); m_constantBuffer = nullptr; }
    if (m_rasterizerState) { m_rasterizerState->Release(); m_rasterizerState = nullptr; }
    if (m_samplerState) { m_samplerState->Release(); m_samplerState = nullptr; }
    if (m_blendState) { m_blendState->Release(); m_blendState = nullptr; }
    
    if (m_circleInputLayout) { m_circleInputLayout->Release(); m_circleInputLayout = nullptr; }
    if (m_circlePS) { m_circlePS->Release(); m_circlePS = nullptr; }
    if (m_circleVS) { m_circleVS->Release(); m_circleVS = nullptr; }
    
    if (m_shapeInputLayout) { m_shapeInputLayout->Release(); m_shapeInputLayout = nullptr; }
    if (m_shapePS) { m_shapePS->Release(); m_shapePS = nullptr; }
    if (m_shapeVS) { m_shapeVS->Release(); m_shapeVS = nullptr; }
    
    if (m_imageInputLayout) { m_imageInputLayout->Release(); m_imageInputLayout = nullptr; }
    if (m_imagePS) { m_imagePS->Release(); m_imagePS = nullptr; }
    if (m_imageVS) { m_imageVS->Release(); m_imageVS = nullptr; }
    
    if (m_textInputLayout) { m_textInputLayout->Release(); m_textInputLayout = nullptr; }
    if (m_textPS) { m_textPS->Release(); m_textPS = nullptr; }
    if (m_textVS) { m_textVS->Release(); m_textVS = nullptr; }
    
    if (m_rectInputLayout) { m_rectInputLayout->Release(); m_rectInputLayout = nullptr; }
    if (m_rectPS) { m_rectPS->Release(); m_rectPS = nullptr; }
    if (m_rectVS) { m_rectVS->Release(); m_rectVS = nullptr; }
    
    if (m_rtv) { m_rtv->Release(); m_rtv = nullptr; }
    if (m_swapChain) { m_swapChain->Release(); m_swapChain = nullptr; }
    if (m_context) { m_context->Release(); m_context = nullptr; }
    if (m_device && !m_usingSharedDevice) { m_device->Release(); m_device = nullptr; }
    
    m_isInitialized = false;
}

bool D3D11Renderer::createDevice()
{
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };

    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        flags,
        featureLevels,
        _countof(featureLevels),
        D3D11_SDK_VERSION,
        &m_device,
        &featureLevel,
        &m_context
    );

    // Retry without debug flag if it failed
    if (FAILED(hr) && (flags & D3D11_CREATE_DEVICE_DEBUG))
    {
        flags &= ~D3D11_CREATE_DEVICE_DEBUG;

        hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            flags,
            featureLevels,
            _countof(featureLevels),
            D3D11_SDK_VERSION,
            &m_device,
            &featureLevel,
            &m_context
        );
    }

    return SUCCEEDED(hr);
}

bool D3D11Renderer::createSwapChain(HWND hwnd)
{
    IDXGIDevice* dxgiDevice = nullptr;
    m_device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    
    IDXGIAdapter* dxgiAdapter = nullptr;
    dxgiDevice->GetAdapter(&dxgiAdapter);
    
    IDXGIFactory2* dxgiFactory = nullptr;
    dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), (void**)&dxgiFactory);
    
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    
    HRESULT hr = dxgiFactory->CreateSwapChainForHwnd(
        m_device,
        hwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &m_swapChain
    );
    
    dxgiFactory->Release();
    dxgiAdapter->Release();
    dxgiDevice->Release();
    
    return SUCCEEDED(hr);
}

bool D3D11Renderer::createRenderTarget()
{
    ID3D11Texture2D* backBuffer = nullptr;
    m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    
    HRESULT hr = m_device->CreateRenderTargetView(backBuffer, nullptr, &m_rtv);
    backBuffer->Release();
    
    return SUCCEEDED(hr);
}

bool D3D11Renderer::loadShaderFromFile(const wchar_t* path, ID3DBlob** outBlob)
{
    std::wstring fullPath = path;
    std::ifstream file(fullPath, std::ios::binary);

    if (!file.is_open()) {
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(NULL, exePath, MAX_PATH);
        std::wstring exeDir = exePath;
        size_t lastSlash = exeDir.find_last_of(L"\\/");
        if (lastSlash != std::wstring::npos) {
            exeDir = exeDir.substr(0, lastSlash + 1);
        }

        fullPath = exeDir + path;
        file.open(fullPath, std::ios::binary);

        if (!file.is_open()) {
            std::wcerr << L"[D3D11Renderer] Failed to open shader file: " << path << std::endl;
            std::wcerr << L"[D3D11Renderer] Tried: " << fullPath << std::endl;
            return false;
        }
    }

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> data(size);
    file.read(data.data(), size);

    if (FAILED(D3DCreateBlob(size, outBlob))) {
        std::cerr << "[D3D11Renderer] Failed to create blob for shader" << std::endl;
        return false;
    }

    memcpy((*outBlob)->GetBufferPointer(), data.data(), size);
    return true;
}

bool D3D11Renderer::loadShaders()
{
    ID3DBlob* blob = nullptr;
    
    if (!loadShaderFromFile(L"shaders/BasicVS.cso", &blob)) return false;
    if (FAILED(m_device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_rectVS))) { blob->Release(); return false; }
    blob->Release();
    
    if (!loadShaderFromFile(L"shaders/BasicPS.cso", &blob)) return false;
    if (FAILED(m_device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_rectPS))) { blob->Release(); return false; }
    blob->Release();
    
    if (!loadShaderFromFile(L"shaders/TextVS.cso", &blob)) return false;
    if (FAILED(m_device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_textVS))) { blob->Release(); return false; }
    blob->Release();
    
    if (!loadShaderFromFile(L"shaders/TextPS.cso", &blob)) return false;
    if (FAILED(m_device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_textPS))) { blob->Release(); return false; }
    blob->Release();
    
    if (!loadShaderFromFile(L"shaders/ImageVS.cso", &blob)) return false;
    if (FAILED(m_device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_imageVS))) { blob->Release(); return false; }
    blob->Release();
    
    if (!loadShaderFromFile(L"shaders/ImagePS.cso", &blob)) return false;
    if (FAILED(m_device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_imagePS))) { blob->Release(); return false; }
    blob->Release();
    
    if (!loadShaderFromFile(L"shaders/ShapeVS.cso", &blob)) return false;
    if (FAILED(m_device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_shapeVS))) { blob->Release(); return false; }
    blob->Release();
    
    if (!loadShaderFromFile(L"shaders/ShapePS.cso", &blob)) return false;
    if (FAILED(m_device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_shapePS))) { blob->Release(); return false; }
    blob->Release();
    
    if (!loadShaderFromFile(L"shaders/CircleVS.cso", &blob)) return false;
    if (FAILED(m_device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_circleVS))) { blob->Release(); return false; }
    blob->Release();
    
    if (!loadShaderFromFile(L"shaders/CirclePS.cso", &blob)) return false;
    if (FAILED(m_device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_circlePS))) { blob->Release(); return false; }
    blob->Release();
    
    return true;
}

bool D3D11Renderer::createInputLayouts()
{
    D3D11_INPUT_ELEMENT_DESC rectLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "POSITION", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 2, DXGI_FORMAT_R32_FLOAT, 0, 56, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    
    ID3DBlob* vsBlob = nullptr;
    if (!loadShaderFromFile(L"shaders/BasicVS.cso", &vsBlob)) return false;
    HRESULT hr = m_device->CreateInputLayout(rectLayout, 6, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_rectInputLayout);
    vsBlob->Release();
    if (FAILED(hr)) return false;
    
    D3D11_INPUT_ELEMENT_DESC textLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    
    if (!loadShaderFromFile(L"shaders/TextVS.cso", &vsBlob)) return false;
    hr = m_device->CreateInputLayout(textLayout, 3, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_textInputLayout);
    vsBlob->Release();
    if (FAILED(hr)) return false;
    
    D3D11_INPUT_ELEMENT_DESC imageLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    
    if (!loadShaderFromFile(L"shaders/ImageVS.cso", &vsBlob)) return false;
    hr = m_device->CreateInputLayout(imageLayout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_imageInputLayout);
    vsBlob->Release();
    if (FAILED(hr)) return false;
    
    D3D11_INPUT_ELEMENT_DESC shapeLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    
    if (!loadShaderFromFile(L"shaders/ShapeVS.cso", &vsBlob)) return false;
    hr = m_device->CreateInputLayout(shapeLayout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_shapeInputLayout);
    vsBlob->Release();
    if (FAILED(hr)) return false;
    
    D3D11_INPUT_ELEMENT_DESC circleLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "POSITION", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 1, DXGI_FORMAT_R32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    
    if (!loadShaderFromFile(L"shaders/CircleVS.cso", &vsBlob)) return false;
    hr = m_device->CreateInputLayout(circleLayout, 5, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_circleInputLayout);
    vsBlob->Release();
    if (FAILED(hr)) return false;
    
    return true;
}

bool D3D11Renderer::createBlendStates()
{
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    
    return SUCCEEDED(m_device->CreateBlendState(&blendDesc, &m_blendState));
}

bool D3D11Renderer::createSamplerStates()
{
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    
    return SUCCEEDED(m_device->CreateSamplerState(&samplerDesc, &m_samplerState));
}

bool D3D11Renderer::createRasterizerStates()
{
    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    rasterizerDesc.ScissorEnable = TRUE;
    
    return SUCCEEDED(m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerState));
}

bool D3D11Renderer::createConstantBuffers()
{
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(ViewportUniforms);
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    return SUCCEEDED(m_device->CreateBuffer(&cbDesc, nullptr, &m_constantBuffer));
}

void D3D11Renderer::beginFrame()
{
    if (!m_rtv) return;
    
    m_context->OMSetRenderTargets(1, &m_rtv, nullptr);
    
    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<FLOAT>(m_width);
    viewport.Height = static_cast<FLOAT>(m_height);
    viewport.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &viewport);
    
    float clearColor[4] = { m_clearColor.x, m_clearColor.y, m_clearColor.z, m_clearColor.w };
    m_context->ClearRenderTargetView(m_rtv, clearColor);
    
    applyFullScreenScissor();
    m_context->RSSetState(m_rasterizerState);
    m_context->OMSetBlendState(m_blendState, nullptr, 0xFFFFFFFF);
    
    ViewportUniforms uniforms = getViewportUniforms();
    updateConstantBuffer(uniforms);
    
    m_currentPipeline = ActivePipeline::None;
    
    if (m_textRenderer) m_textRenderer->beginFrame();
}

void D3D11Renderer::endFrame()
{
    if (m_swapChain)
    {
        m_swapChain->Present(1, 0);
    }
}

void D3D11Renderer::setPipeline(ActivePipeline pipeline)
{
    if (m_currentPipeline == pipeline) return;
    
    switch (pipeline)
    {
        case ActivePipeline::Rect:
            m_context->VSSetShader(m_rectVS, nullptr, 0);
            m_context->PSSetShader(m_rectPS, nullptr, 0);
            m_context->IASetInputLayout(m_rectInputLayout);
            break;
        case ActivePipeline::Text:
            m_context->VSSetShader(m_textVS, nullptr, 0);
            m_context->PSSetShader(m_textPS, nullptr, 0);
            m_context->IASetInputLayout(m_textInputLayout);
            break;
        case ActivePipeline::Image:
            m_context->VSSetShader(m_imageVS, nullptr, 0);
            m_context->PSSetShader(m_imagePS, nullptr, 0);
            m_context->IASetInputLayout(m_imageInputLayout);
            break;
        case ActivePipeline::Shape:
            m_context->VSSetShader(m_shapeVS, nullptr, 0);
            m_context->PSSetShader(m_shapePS, nullptr, 0);
            m_context->IASetInputLayout(m_shapeInputLayout);
            break;
        case ActivePipeline::Circle:
            m_context->VSSetShader(m_circleVS, nullptr, 0);
            m_context->PSSetShader(m_circlePS, nullptr, 0);
            m_context->IASetInputLayout(m_circleInputLayout);
            break;
        default:
            return;
    }
    
    m_currentPipeline = pipeline;
}

D3D11_RECT D3D11Renderer::computeScissorRect(const Rect& clipRect) const
{
    D3D11_RECT scissor;
    scissor.left = static_cast<LONG>(clipRect.x);
    scissor.top = static_cast<LONG>(clipRect.y);
    scissor.right = static_cast<LONG>(clipRect.x + clipRect.width);
    scissor.bottom = static_cast<LONG>(clipRect.y + clipRect.height);
    
    scissor.left = std::max(scissor.left, 0L);
    scissor.top = std::max(scissor.top, 0L);
    scissor.right = std::min(scissor.right, static_cast<LONG>(m_width));
    scissor.bottom = std::min(scissor.bottom, static_cast<LONG>(m_height));
    
    return scissor;
}

void D3D11Renderer::applyScissorRect(const Rect& clipRect)
{
    D3D11_RECT scissor = computeScissorRect(clipRect);
    m_context->RSSetScissorRects(1, &scissor);
}

void D3D11Renderer::applyFullScreenScissor()
{
    D3D11_RECT scissor;
    scissor.left = 0;
    scissor.top = 0;
    scissor.right = m_width;
    scissor.bottom = m_height;
    m_context->RSSetScissorRects(1, &scissor);
}

void D3D11Renderer::executeRenderCommands(const RenderList& commandList)
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

void D3D11Renderer::renderRectangle(const Rect& rect, const Vec4& color, const CornerRadius& cornerRadius, float borderWidth)
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "Not initialized");
    
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
    
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    
    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices;
    
    ID3D11Buffer* vertexBuffer = nullptr;
    m_device->CreateBuffer(&vbDesc, &vbData, &vertexBuffer);
    
    UINT stride = sizeof(RectVertex);
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    m_context->Draw(6, 0);
    
    vertexBuffer->Release();
}

void D3D11Renderer::renderRectBatch(const std::vector<RenderCommand>& commands, const Rect& clipRect, bool hasClip)
{
    if (commands.empty()) return;
    
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

void D3D11Renderer::generateImageVertices(const Rect& destRect, const Rect& sourceRect, uint32_t texWidth, uint32_t texHeight, std::vector<float>& outVertices)
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

void D3D11Renderer::computeNineSliceRects(const Rect& destRect, const Rect& sourceRect, const NineSliceMargins& margins, float designScale, Rect outSlices[9])
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

void D3D11Renderer::generateNineSliceVertices(void* texture, const Rect& destRect, const Rect& sourceRect, const NineSliceMargins& margins, float designScale, uint32_t texWidth, uint32_t texHeight, std::vector<float>& outVertices)
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

void D3D11Renderer::renderImageBatch(const std::vector<size_t>& commandIndices, const std::vector<RenderCommand>& commands, void* texture, const Rect& clipRect, bool hasClip)
{
    if (commandIndices.empty()) return;
    
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
            generateNineSliceVertices(texture, destRect, sourceRect, cmd.nineSliceMargins, designScale, texWidth, texHeight, vertexData);
        }
        else
        {
            generateImageVertices(destRect, sourceRect, texWidth, texHeight, vertexData);
        }
    }
    
    if (vertexData.empty()) return;
    
    size_t vertexCount = vertexData.size() / 4;
    
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = static_cast<UINT>(vertexData.size() * sizeof(float));
    vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    
    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertexData.data();
    
    ID3D11Buffer* vertexBuffer = nullptr;
    m_device->CreateBuffer(&vbDesc, &vbData, &vertexBuffer);
    
    ID3D11ShaderResourceView* srv = static_cast<ID3D11ShaderResourceView*>(texture);
    m_context->PSSetShaderResources(0, 1, &srv);
    m_context->PSSetSamplers(0, 1, &m_samplerState);
    
    UINT stride = 16;
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    m_context->Draw(static_cast<UINT>(vertexCount), 0);
    
    vertexBuffer->Release();
}

void D3D11Renderer::renderTextBatches(const std::vector<TextVertex>& allVertices, const std::vector<size_t>& batchStarts, const std::vector<size_t>& batchCounts, const std::vector<Rect>& batchClips, const std::vector<bool>& batchHasClips)
{
    if (allVertices.empty() || batchStarts.empty()) return;
    
    setPipeline(ActivePipeline::Text);
    
    void* atlasTexture = getCurrentAtlasTexture();
    if (!atlasTexture) return;
    
    ID3D11ShaderResourceView* srv = static_cast<ID3D11ShaderResourceView*>(atlasTexture);
    m_context->PSSetShaderResources(0, 1, &srv);
    m_context->PSSetSamplers(0, 1, &m_samplerState);
    
    D3D11_MAPPED_SUBRESOURCE mapped;
    m_context->Map(m_textVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, allVertices.data(), allVertices.size() * sizeof(TextVertex));
    m_context->Unmap(m_textVertexBuffer, 0);
    
    UINT stride = sizeof(TextVertex);
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, &m_textVertexBuffer, &stride, &offset);
    m_context->IASetIndexBuffer(m_textIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
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
        
        UINT indexCount = static_cast<UINT>((batchCounts[i] / 4) * 6);
        UINT startIndexLocation = static_cast<UINT>((batchStarts[i] / 4) * 6);
        
        m_context->DrawIndexed(indexCount, startIndexLocation, 0);
    }
}

void D3D11Renderer::renderLine(const Vec2& start, const Vec2& end, const Vec4& color, float width)
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
    vertices[0] = ShapeVertex{p1, color};
    vertices[1] = ShapeVertex{p2, color};
    vertices[2] = ShapeVertex{p3, color};
    vertices[3] = ShapeVertex{p1, color};
    vertices[4] = ShapeVertex{p3, color};
    vertices[5] = ShapeVertex{p4, color};
    
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    
    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices;
    
    ID3D11Buffer* vertexBuffer = nullptr;
    m_device->CreateBuffer(&vbDesc, &vbData, &vertexBuffer);
    
    UINT stride = sizeof(ShapeVertex);
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    m_context->Draw(6, 0);
    
    vertexBuffer->Release();
}

void D3D11Renderer::renderTriangle(const Vec2& p1, const Vec2& p2, const Vec2& p3, const Vec4& color, float borderWidth, bool filled)
{
    setPipeline(ActivePipeline::Shape);
    
    if (filled)
    {
        ShapeVertex vertices[3];
        vertices[0] = ShapeVertex{p1, color};
        vertices[1] = ShapeVertex{p2, color};
        vertices[2] = ShapeVertex{p3, color};
        
        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.ByteWidth = sizeof(vertices);
        vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        
        D3D11_SUBRESOURCE_DATA vbData = {};
        vbData.pSysMem = vertices;
        
        ID3D11Buffer* vertexBuffer = nullptr;
        m_device->CreateBuffer(&vbDesc, &vbData, &vertexBuffer);
        
        UINT stride = sizeof(ShapeVertex);
        UINT offset = 0;
        m_context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        
        m_context->Draw(3, 0);
        
        vertexBuffer->Release();
    }
    else
    {
        renderLine(p1, p2, color, borderWidth);
        renderLine(p2, p3, color, borderWidth);
        renderLine(p3, p1, color, borderWidth);
    }
}

void D3D11Renderer::renderCircle(const Vec2& center, float radius, const Vec4& color, float borderWidth, bool filled)
{
    setPipeline(ActivePipeline::Circle);
    
    float left = center.x - radius - 2.0f;
    float right = center.x + radius + 2.0f;
    float top = center.y - radius - 2.0f;
    float bottom = center.y + radius + 2.0f;
    
    float bw = filled ? 0.0f : borderWidth;
    
    CircleVertex vertices[6];
    vertices[0] = CircleVertex{Vec2(left, top), center, radius, bw, color};
    vertices[1] = CircleVertex{Vec2(left, bottom), center, radius, bw, color};
    vertices[2] = CircleVertex{Vec2(right, bottom), center, radius, bw, color};
    vertices[3] = CircleVertex{Vec2(left, top), center, radius, bw, color};
    vertices[4] = CircleVertex{Vec2(right, bottom), center, radius, bw, color};
    vertices[5] = CircleVertex{Vec2(right, top), center, radius, bw, color};
    
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    
    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices;
    
    ID3D11Buffer* vertexBuffer = nullptr;
    m_device->CreateBuffer(&vbDesc, &vbData, &vertexBuffer);
    
    UINT stride = sizeof(CircleVertex);
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    m_context->Draw(6, 0);
    
    vertexBuffer->Release();
}

void* D3D11Renderer::getCurrentAtlasTexture() const
{
    if (!m_textRenderer) return nullptr;
    return m_textRenderer->getCurrentAtlasTexture();
}

void* D3D11Renderer::createTexture2D(uint32_t width, uint32_t height, TextureFormat format)
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "Renderer not initialized");
    
    DXGI_FORMAT dxgiFormat = (format == TextureFormat::R8_Unorm) ? DXGI_FORMAT_R8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM;
    
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = dxgiFormat;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    
    ID3D11Texture2D* texture = nullptr;
    if (FAILED(m_device->CreateTexture2D(&texDesc, nullptr, &texture))) return nullptr;
    
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = dxgiFormat;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    
    ID3D11ShaderResourceView* srv = nullptr;
    if (FAILED(m_device->CreateShaderResourceView(texture, &srvDesc, &srv)))
    {
        texture->Release();
        return nullptr;
    }
    
    texture->Release();
    return srv;
}

void D3D11Renderer::updateTexture2D(void* texture, uint32_t x, uint32_t y, uint32_t width, uint32_t height, const void* data, size_t bytesPerRow)
{
    YUCHEN_ASSERT_MSG(texture != nullptr, "Texture is null");
    YUCHEN_ASSERT_MSG(data != nullptr, "Data is null");
    
    ID3D11ShaderResourceView* srv = static_cast<ID3D11ShaderResourceView*>(texture);
    
    ID3D11Resource* resource = nullptr;
    srv->GetResource(&resource);
    
    D3D11_BOX box = {};
    box.left = x;
    box.top = y;
    box.right = x + width;
    box.bottom = y + height;
    box.front = 0;
    box.back = 1;
    
    m_context->UpdateSubresource(resource, 0, &box, data, static_cast<UINT>(bytesPerRow), 0);
    
    resource->Release();
}

void D3D11Renderer::destroyTexture(void* texture)
{
    if (!texture) return;
    
    ID3D11ShaderResourceView* srv = static_cast<ID3D11ShaderResourceView*>(texture);
    srv->Release();
}

void D3D11Renderer::convertToNDC(float x, float y, float& ndcX, float& ndcY) const
{
    ndcX = (x / static_cast<float>(m_width)) * 2.0f - 1.0f;
    ndcY = 1.0f - (y / static_cast<float>(m_height)) * 2.0f;
}

ViewportUniforms D3D11Renderer::getViewportUniforms() const
{
    ViewportUniforms uniforms;
    uniforms.viewportSize = Vec2(static_cast<float>(m_width), static_cast<float>(m_height));
    uniforms._padding = Vec2(0.0f, 0.0f);
    return uniforms;
}

void D3D11Renderer::updateConstantBuffer(const ViewportUniforms& uniforms)
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    m_context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, &uniforms, sizeof(ViewportUniforms));
    m_context->Unmap(m_constantBuffer, 0);
    
    m_context->VSSetConstantBuffers(0, 1, &m_constantBuffer);
}

}
