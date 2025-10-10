/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Platform module (Windows).
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

//==========================================================================================
/** @file PlatformBackend.cpp
    
    Windows platform backend factory implementation.
    
    This file provides platform-specific factory methods for creating:
    - Graphics backend (D3D11Renderer implementing IGraphicsBackend)
    - Event manager (Win32EventManager)
    - Shared D3D11 device (for resource sharing between windows)
    
    Implementation notes:
    - D3D11 device creation follows Direct3D 11 best practices
    - Debug layer enabled in debug builds for validation
    - Falls back to release mode if debug layer unavailable
    - Shared device allows efficient texture/resource sharing
*/

#include "YuchenUI/platform/PlatformBackend.h"
#include "D3D11Renderer.h"
#include "Win32EventManager.h"
#include <d3d11.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

namespace YuchenUI {

//==========================================================================================
// Graphics Backend Factory

IGraphicsBackend* PlatformBackend::createGraphicsBackend()
{
    return new D3D11Renderer();
}

//==========================================================================================
// Event Manager Factory

EventManager* PlatformBackend::createEventManager(void* nativeWindow)
{
    return new Win32EventManager(static_cast<HWND>(nativeWindow));
}

//==========================================================================================
// Shared Device Management

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

} // namespace YuchenUI
