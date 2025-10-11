/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Image module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

//==========================================================================================
/** @file TextureCache.h
    
    GPU texture cache with automatic resolution selection based on DPI scale.
    
    Manages lifecycle of GPU textures loaded from embedded resources. Automatically
    selects best resolution variant (@1x, @2x, @3x) based on current display DPI.
    Caches textures to avoid redundant decoding and GPU uploads.
    
    Resolution selection algorithm:
    - Prefers variants meeting or exceeding DPI scale (avoids upscaling artifacts)
    - Among matching variants, selects smallest to minimize memory usage
    - Falls back to largest available if all variants below target DPI
    
    Thread safety: Not thread-safe. Must be used from single thread (typically main/render thread).
*/

#pragma once

#include "YuchenUI/core/Types.h"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>

namespace YuchenUI {

class IGraphicsBackend;

//==========================================================================================
/**
    GPU texture cache with DPI-aware resolution selection.
    
    TextureCache manages GPU textures loaded from embedded PNG resources.
    Automatically selects optimal resolution variant based on display DPI scale,
    and caches textures to avoid redundant operations.
    
    Key features:
    - Automatic @1x/@2x/@3x variant selection
    - Texture deduplication (multiple paths may reference same texture)
    - Lazy loading (textures created on first access)
    - Batch cleanup (destroys all textures at once)
    
    @see IGraphicsBackend, ImageDecoder
*/
class TextureCache {
public:
    //======================================================================================
    /** Creates a texture cache using the specified graphics backend.
        
        Backend must remain valid for lifetime of TextureCache. Backend is
        not owned by TextureCache.
        
        @param backend  Graphics backend for GPU texture operations
    */
    explicit TextureCache(IGraphicsBackend* backend);
    
    /** Destructor. Destroys all cached textures via backend. */
    ~TextureCache();
    
    //======================================================================================
    /** Initializes the texture cache.
        
        Must be called before any texture operations.
        
        @returns True if initialization succeeded
    */
    bool initialize();
    
    /** Destroys all cached textures and resets to uninitialized state. */
    void destroy();
    
    /** Returns true if cache has been initialized. */
    bool isInitialized() const { return m_isInitialized; }
    
    //======================================================================================
    /** Sets current display DPI scale for resolution selection.
        
        Used to select optimal @Nx variant when loading textures.
        Should be updated when display DPI changes or window moves to different display.
        
        @param dpiScale  Current DPI scale factor (1.0 = 96 DPI, 2.0 = 192 DPI, etc.)
    */
    void setCurrentDPI(float dpiScale);
    
    /** Retrieves or creates GPU texture for the specified resource.
        
        Searches cache for existing texture. If not found, automatically:
        1. Selects best resolution variant based on current DPI
        2. Decodes PNG image from embedded resource
        3. Creates GPU texture via backend
        4. Uploads pixel data to GPU
        5. Caches texture handle for future requests
        
        Returns dimensions of actual texture (may differ from design size if
        using @2x/@3x variant).
        
        @param resourcePath    Path to embedded resource (e.g., "button.png")
        @param outWidth        Receives actual texture width in pixels
        @param outHeight       Receives actual texture height in pixels
        @param outDesignScale  Optional output for resource's design scale (@1x=1.0, @2x=2.0, etc.)
        @returns Opaque texture handle, or nullptr on error
    */
    void* getTexture(const char* resourcePath, uint32_t& outWidth, uint32_t& outHeight, float* outDesignScale = nullptr);
    
    /** Destroys all cached textures and clears cache.
        
        Invalidates all previously returned texture handles.
    */
    void clearAll();

private:
    //======================================================================================
    /** Cached texture entry. */
    struct TextureEntry {
        void* handle;         ///< Opaque GPU texture handle
        uint32_t width;       ///< Texture width in pixels
        uint32_t height;      ///< Texture height in pixels
        float designScale;    ///< Design scale of resource (@1x=1.0, @2x=2.0, etc.)
        
        TextureEntry() : handle(nullptr), width(0), height(0), designScale(1.0f) {}
        TextureEntry(void* h, uint32_t w, uint32_t ht, float ds)
            : handle(h), width(w), height(ht), designScale(ds) {}
    };
    
    //======================================================================================
    /** Extracts base resource name by removing @Nx suffix.
        
        Example: "button@2x.png" -> "button.png"
        
        @param path  Resource path possibly containing scale suffix
        @returns Base path without scale suffix
    */
    std::string extractBaseName(const std::string& path) const;
    
    /** Finds all resolution variants of a base resource path.
        
        Searches embedded resources for all @1x/@2x/@3x variants of the base path.
        
        @param basePath  Base resource path (e.g., "button.png")
        @returns List of variant paths (e.g., ["button.png", "button@2x.png"])
    */
    std::vector<std::string> findAllVariants(const std::string& basePath) const;
    
    /** Selects best resolution variant for current DPI scale.
        
        Selection algorithm:
        - Prefers variants >= current DPI (avoids upscaling)
        - Among >= variants, selects smallest (minimal memory)
        - If all < DPI, selects largest (minimal upscaling)
        
        @param basePath  Base resource path
        @returns Selected variant path, or empty string if none found
    */
    std::string selectBestResource(const std::string& basePath) const;
    
    /** Creates GPU texture from embedded resource.
        
        Decodes PNG, creates GPU texture, uploads pixels, and caches result.
        
        @param resourcePath   Path to embedded resource
        @param outWidth       Receives texture width
        @param outHeight      Receives texture height
        @param outDesignScale Receives resource design scale
        @returns Texture handle, or nullptr on error
    */
    void* createTextureFromResource(const char* resourcePath, uint32_t& outWidth, uint32_t& outHeight, float& outDesignScale);
    
    //======================================================================================
    IGraphicsBackend* m_backend;                              ///< Graphics backend (not owned)
    std::unordered_map<std::string, TextureEntry> m_textureCache;  ///< Path -> texture mapping
    bool m_isInitialized;                                     ///< Initialization state
    float m_currentDPI;                                       ///< Current DPI scale factor
    
    TextureCache(const TextureCache&) = delete;
    TextureCache& operator=(const TextureCache&) = delete;
};

} // namespace YuchenUI
