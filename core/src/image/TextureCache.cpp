#include "YuchenUI/image/TextureCache.h"
#include "YuchenUI/image/ImageDecoder.h"
#include "YuchenUI/rendering/IGraphicsBackend.h"
#include "YuchenUI/core/Assert.h"
#include "../generated/embedded_resources.h"
#include <regex>
#include <algorithm>

namespace YuchenUI {

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

void TextureCache::setCurrentDPI(float dpiScale)
{
    YUCHEN_ASSERT_MSG(dpiScale > 0.0f, "DPI scale must be positive");
    m_currentDPI = dpiScale;
}

std::string TextureCache::extractBaseName(const std::string& path) const
{
    std::regex scalePattern(R"(@(\d+)x\.(png|jpg|jpeg|bmp))", std::regex::icase);
    std::string result = std::regex_replace(path, scalePattern, ".$2");
    return result;
}

std::vector<std::string> TextureCache::findAllVariants(const std::string& basePath) const
{
    std::vector<std::string> variants;
    
    size_t dotPos = basePath.rfind('.');
    if (dotPos == std::string::npos) return variants;
    
    std::string pathWithoutExt = basePath.substr(0, dotPos);
    std::string extension = basePath.substr(dotPos);
    
    const Resources::ResourceData* allResources = Resources::getAllResources();
    size_t resourceCount = Resources::getResourceCount();
    
    for (size_t i = 0; i < resourceCount; ++i)
    {
        std::string resourcePath(allResources[i].path);
        std::string resourceBase = extractBaseName(resourcePath);
        
        if (resourceBase == basePath || resourcePath == basePath)
        {
            variants.push_back(resourcePath);
        }
        else
        {
            size_t resDotPos = resourcePath.rfind('.');
            if (resDotPos != std::string::npos)
            {
                std::string resPathWithoutExt = resourcePath.substr(0, resDotPos);
                std::string resExt = resourcePath.substr(resDotPos);
                
                std::regex scalePattern(R"(@\d+x)");
                std::string resBaseWithoutScale = std::regex_replace(resPathWithoutExt, scalePattern, "");
                
                if (resBaseWithoutScale == pathWithoutExt && resExt == extension)
                {
                    variants.push_back(resourcePath);
                }
            }
        }
    }
    
    return variants;
}

std::string TextureCache::selectBestResource(const std::string& basePath) const
{
    std::vector<std::string> variants = findAllVariants(basePath);
    
    if (variants.empty())
    {
        return "";
    }
    
    if (variants.size() == 1)
    {
        return variants[0];
    }
    
    struct Candidate {
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
    
    std::sort(candidates.begin(), candidates.end(),
        [this](const Candidate& a, const Candidate& b) {
            if (a.scale >= m_currentDPI && b.scale >= m_currentDPI)
            {
                return a.scale < b.scale;
            }
            if (a.scale >= m_currentDPI)
            {
                return true;
            }
            if (b.scale >= m_currentDPI)
            {
                return false;
            }
            return a.scale > b.scale;
        });
    
    return candidates.empty() ? "" : candidates[0].path;
}

void* TextureCache::getTexture(const char* resourcePath, uint32_t& outWidth, uint32_t& outHeight, float* outDesignScale)
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "Not initialized");
    YUCHEN_ASSERT_MSG(resourcePath != nullptr, "Resource path cannot be null");
    
    auto it = m_textureCache.find(resourcePath);
    if (it != m_textureCache.end())
    {
        outWidth = it->second.width;
        outHeight = it->second.height;
        if (outDesignScale) *outDesignScale = it->second.designScale;
        return it->second.handle;
    }
    
    std::string basePath = extractBaseName(resourcePath);
    std::string bestPath = selectBestResource(basePath);
    
    if (bestPath.empty())
    {
        bestPath = resourcePath;
    }
    
    float designScale = 1.0f;
    void* handle = createTextureFromResource(bestPath.c_str(), outWidth, outHeight, designScale);
    if (outDesignScale) *outDesignScale = designScale;

    if (handle && bestPath != resourcePath)
    {
        m_textureCache[resourcePath] = TextureEntry(handle, outWidth, outHeight, designScale);
    }
    
    return handle;
}

void* TextureCache::createTextureFromResource(const char* resourcePath, uint32_t& outWidth, uint32_t& outHeight, float& outDesignScale)
{
    auto it = m_textureCache.find(resourcePath);
    if (it != m_textureCache.end())
    {
        outWidth = it->second.width;
        outHeight = it->second.height;
        outDesignScale = it->second.designScale;
        return it->second.handle;
    }
    
    const Resources::ResourceData* resource = Resources::findResource(resourcePath);
    if (!resource)
    {
        return nullptr;
    }
    
    outDesignScale = resource->designScale;
    
    ImageData imageData;
    if (!ImageDecoder::decodePNG(*resource, imageData))
    {
        return nullptr;
    }
    
    void* textureHandle = m_backend->createTexture2D(
        imageData.width,
        imageData.height,
        TextureFormat::RGBA8_Unorm
    );
    
    if (!textureHandle)
    {
        return nullptr;
    }
    
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
    
    m_textureCache[resourcePath] = TextureEntry(textureHandle, imageData.width, imageData.height, outDesignScale);
    
    return textureHandle;
}

void TextureCache::clearAll()
{
    std::unordered_set<void*> uniqueHandles;
    
    for (auto& pair : m_textureCache)
    {
        if (pair.second.handle)
        {
            uniqueHandles.insert(pair.second.handle);
        }
    }
    
    for (void* handle : uniqueHandles)
    {
        m_backend->destroyTexture(handle);
    }
    
    m_textureCache.clear();
}

}
