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
/** @file TextureCache.cpp
    
    Implementation notes:
    - Automatically selects best resolution variant based on DPI scale
    - Extracts base name by removing @Nx suffix from resource paths
    - Prefers variants matching or exceeding current DPI scale
    - Caches textures by resource path to avoid redundant GPU uploads
    - Deduplicates texture handles before destruction to avoid double-free
    - Uses regex to identify and match scale variants (@1x, @2x, @3x)
    - Graphics backend handles actual GPU texture creation and destruction
*/

#include "YuchenUI/image/TextureCache.h"
#include "YuchenUI/image/ImageDecoder.h"
#include "YuchenUI/rendering/IGraphicsBackend.h"
#include "YuchenUI/core/Assert.h"
#include "../generated/embedded_resources.h"
#include <regex>
#include <algorithm>

namespace YuchenUI {

//==========================================================================================
// Lifecycle

TextureCache::TextureCache(IGraphicsBackend* backend)
    : m_backend(backend)
    , m_textureCache()
    , m_isInitialized(false)
    , m_currentDPI(1.0f)
{
    YUCHEN_ASSERT_MSG(backend != nullptr, "IGraphicsBackend cannot be null");
}

TextureCache::~TextureCache()
{
    destroy();
}

bool TextureCache::initialize()
{
    YUCHEN_ASSERT_MSG(!m_isInitialized, "Already initialized");
    m_isInitialized = true;
    return true;
}

void TextureCache::destroy()
{
    if (!m_isInitialized) return;
    
    clearAll();
    m_isInitialized = false;
}

//==========================================================================================
// Configuration

void TextureCache::setCurrentDPI(float dpiScale)
{
    YUCHEN_ASSERT_MSG(dpiScale > 0.0f, "DPI scale must be positive");
    m_currentDPI = dpiScale;
}

//==========================================================================================
// Resource Path Processing

std::string TextureCache::extractBaseName(const std::string& path) const
{
    // Remove @Nx suffix to get base resource name
    // Example: "image@2x.png" -> "image.png"
    std::regex scalePattern(R"(@(\d+)x\.(png|jpg|jpeg|bmp))", std::regex::icase);
    std::string result = std::regex_replace(path, scalePattern, ".$2");
    return result;
}

std::vector<std::string> TextureCache::findAllVariants(const std::string& basePath) const
{
    std::vector<std::string> variants;
    
    // Extract base path components
    size_t dotPos = basePath.rfind('.');
    if (dotPos == std::string::npos) return variants;
    
    std::string pathWithoutExt = basePath.substr(0, dotPos);
    std::string extension = basePath.substr(dotPos);
    
    // Search all embedded resources for variants
    const Resources::ResourceData* allResources = Resources::getAllResources();
    size_t resourceCount = Resources::getResourceCount();
    
    for (size_t i = 0; i < resourceCount; ++i)
    {
        std::string resourcePath(allResources[i].path);
        std::string resourceBase = extractBaseName(resourcePath);
        
        // Check if resource matches base path or is exact match
        if (resourceBase == basePath || resourcePath == basePath)
        {
            variants.push_back(resourcePath);
        }
        else
        {
            // Check if resource is a scale variant of base path
            size_t resDotPos = resourcePath.rfind('.');
            if (resDotPos != std::string::npos)
            {
                std::string resPathWithoutExt = resourcePath.substr(0, resDotPos);
                std::string resExt = resourcePath.substr(resDotPos);
                
                // Remove scale suffix for comparison
                std::regex scalePattern(R"(@\d+x)");
                std::string resBaseWithoutScale = std::regex_replace(resPathWithoutExt, scalePattern, "");
                
                // Add if base path and extension match
                if (resBaseWithoutScale == pathWithoutExt && resExt == extension)
                    variants.push_back(resourcePath);
            }
        }
    }
    
    return variants;
}

std::string TextureCache::selectBestResource(const std::string& basePath) const
{
    std::vector<std::string> variants = findAllVariants(basePath);
    
    if (variants.empty()) return "";
    if (variants.size() == 1) return variants[0];
    
    // Build candidate list with design scales
    struct Candidate
    {
        std::string path;
        float scale;
        float scoreDiff;
    };
    
    std::vector<Candidate> candidates;
    candidates.reserve(variants.size());
    
    for (const auto& variant : variants)
    {
        const Resources::ResourceData* res = Resources::findResource(variant);
        if (res)
        {
            float scaleDiff = std::abs(res->designScale - m_currentDPI);
            candidates.push_back({variant, res->designScale, scaleDiff});
        }
    }
    
    // Sort candidates by preference:
    // 1. Prefer variants >= current DPI (avoid upscaling)
    // 2. Among variants >= DPI, prefer smallest (avoid excessive downscaling)
    // 3. Among variants < DPI, prefer largest (minimize upscaling artifacts)
    std::sort(candidates.begin(), candidates.end(),
        [this](const Candidate& a, const Candidate& b) {
            // Both meet or exceed target DPI
            if (a.scale >= m_currentDPI && b.scale >= m_currentDPI)
                return a.scale < b.scale;  // Prefer smaller (less downscaling)
            // Only one meets target DPI
            if (a.scale >= m_currentDPI)
                return true;  // Prefer meeting DPI
            if (b.scale >= m_currentDPI)
                return false;
            // Both below target DPI
            return a.scale > b.scale;  // Prefer larger (less upscaling)
        });
    
    return candidates.empty() ? "" : candidates[0].path;
}

//==========================================================================================
// Texture Retrieval

void* TextureCache::getTexture(const char* resourcePath, uint32_t& outWidth, uint32_t& outHeight, float* outDesignScale)
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "Not initialized");
    YUCHEN_ASSERT_MSG(resourcePath != nullptr, "Resource path cannot be null");
    
    // Check cache for existing texture
    auto it = m_textureCache.find(resourcePath);
    if (it != m_textureCache.end())
    {
        outWidth = it->second.width;
        outHeight = it->second.height;
        if (outDesignScale) *outDesignScale = it->second.designScale;
        return it->second.handle;
    }
    
    // Select best resolution variant for current DPI
    std::string basePath = extractBaseName(resourcePath);
    std::string bestPath = selectBestResource(basePath);
    
    if (bestPath.empty()) bestPath = resourcePath;  // Fallback to requested path
    
    // Create texture from selected resource
    float designScale = 1.0f;
    void* handle = createTextureFromResource(bestPath.c_str(), outWidth, outHeight, designScale);
    if (outDesignScale) *outDesignScale = designScale;

    // Cache under original path if different from selected path
    if (handle && bestPath != resourcePath)
    {
        m_textureCache[resourcePath] = TextureEntry(handle, outWidth, outHeight, designScale);
    }
    
    return handle;
}

void* TextureCache::createTextureFromResource(const char* resourcePath, uint32_t& outWidth, uint32_t& outHeight, float& outDesignScale)
{
    // Check if already cached
    auto it = m_textureCache.find(resourcePath);
    if (it != m_textureCache.end())
    {
        outWidth = it->second.width;
        outHeight = it->second.height;
        outDesignScale = it->second.designScale;
        return it->second.handle;
    }
    
    // Find resource in embedded data
    const Resources::ResourceData* resource = Resources::findResource(resourcePath);
    if (!resource) return nullptr;
    
    outDesignScale = resource->designScale;
    
    // Decode image data
    ImageData imageData;
    if (!ImageDecoder::decodePNG(*resource, imageData)) return nullptr;
    
    // Create GPU texture
    void* textureHandle = m_backend->createTexture2D(
        imageData.width,
        imageData.height,
        TextureFormat::RGBA8_Unorm
    );
    
    if (!textureHandle) return nullptr;
    
    // Upload pixel data to GPU
    m_backend->updateTexture2D(
        textureHandle,
        0, 0,
        imageData.width,
        imageData.height,
        imageData.pixels.data(),
        imageData.width * 4
    );
    
    outWidth = imageData.width;
    outHeight = imageData.height;
    
    // Cache the texture
    m_textureCache[resourcePath] = TextureEntry(textureHandle, imageData.width, imageData.height, outDesignScale);
    
    return textureHandle;
}

//==========================================================================================
// Cache Management

void TextureCache::clearAll()
{
    // Collect unique texture handles (multiple paths may reference same texture)
    std::unordered_set<void*> uniqueHandles;
    
    for (auto& pair : m_textureCache)
    {
        if (pair.second.handle) uniqueHandles.insert(pair.second.handle);
    }
    
    // Destroy each unique texture once
    for (void* handle : uniqueHandles)
    {
        m_backend->destroyTexture(handle);
    }
    
    m_textureCache.clear();
}

} // namespace YuchenUI
