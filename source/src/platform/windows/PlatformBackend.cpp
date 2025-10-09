#include "YuchenUI/platform/PlatformBackend.h"
#include "D3D11Renderer.h"
#include "Win32EventManager.h"
#include "Win32MenuImpl.h"
#include <d3d11.h>

namespace YuchenUI {

GraphicsContext* PlatformBackend::createRenderer()
{
    return new D3D11Renderer();
}

EventManager* PlatformBackend::createEventManager(void* nativeWindow)
{
    return new Win32EventManager(static_cast<HWND>(nativeWindow));
}

void* PlatformBackend::createSharedDevice()
{
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;

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
        &device,
        &featureLevel,
        &context
    );

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
            &device,
            &featureLevel,
            &context
        );
    }

    if (context)
    {
        context->Release();
        context = nullptr;
    }

    return SUCCEEDED(hr) ? device : nullptr;
}

void PlatformBackend::destroySharedDevice(void* device)
{
    if (device)
    {
        ID3D11Device* d3dDevice = static_cast<ID3D11Device*>(device);
        d3dDevice->Release();
    }
}

}
