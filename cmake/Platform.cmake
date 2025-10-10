if(WIN32)
    set(YUCHEN_PLATFORM "Windows")
    add_compile_definitions(
        YUCHEN_PLATFORM_WINDOWS=1
        YUCHEN_RENDERER_D3D11=1
    )
    
    if(MSVC)
        add_compile_definitions(
            UNICODE
            _UNICODE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
        )
    endif()
    
    set(YUCHEN_PLATFORM_LIBS d3d11 dxgi d3dcompiler dwrite)
    
elseif(APPLE)
    set(YUCHEN_PLATFORM "macOS")
    add_compile_definitions(
        YUCHEN_PLATFORM_MACOS=1
        YUCHEN_RENDERER_METAL=1
    )
    set(CMAKE_OSX_DEPLOYMENT_TARGET 11.0)
    
else()
    message(FATAL_ERROR "Unsupported platform")
endif()