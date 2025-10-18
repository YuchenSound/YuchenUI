#include "YuchenUI/image/TextureCache.h"
#include "YuchenUI/image/ImageDecoder.h"
#include "YuchenUI/rendering/IGraphicsBackend.h"
#include "YuchenUI/resource/IResourceResolver.h"
#include "YuchenUI/resource/IResourceProvider.h"
#include "YuchenUI/resource/ResourceManager.h"
#include "YuchenUI/core/Assert.h"
#include <regex>
#include <algorithm>

namespace YuchenUI {

TextureCache::TextureCache(IGraphicsBackend* backend, IResourceResolver* resolver)
    : m_backend(backend)
    , m_resolver(resolver)
    , m_textureCache()
    , m_isInitialized(false)
    , m_currentDPI(1.0f)
{
    YUCHEN_ASSERT_MSG(backend != nullptr, "IGraphicsBackend cannot be null");
    YUCHEN_ASSERT_MSG(resolver != nullptr, "IResourceResolver cannot be null");
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

std::vector<std::string> TextureCache::findAllVariants(const std::string& namespaceName, const std::string& basePath) const
{
    std::vector<std::string> variants;
    
    size_t dotPos = basePath.rfind('.');
    if (dotPos == std::string::npos) return variants;
    
    std::string pathWithoutExt = basePath.substr(0, dotPos);
    std::string extension = basePath.substr(dotPos);
    
    IResourceProvider* provider = ResourceManager::getInstance().getProvider(namespaceName.c_str());
    if (!provider) return variants;
    
    const Resources::ResourceData* resource = provider->find(basePath.c_str());
    if (resource) {
        variants.push_back(basePath);
    }
    
    for (int scale = 2; scale <= 3; ++scale) {
        std::string variantPath = pathWithoutExt + "@" + std::to_string(scale) + "x" + extension;
        const Resources::ResourceData* variantRes = provider->find(variantPath.c_str());
        if (variantRes) {
            variants.push_back(variantPath);
        }
    }
    
    return variants;
}

std::string TextureCache::selectBestResource(const std::string& namespaceName, const std::string& basePath) const
{
    std::vector<std::string> variants = findAllVariants(namespaceName, basePath);
    
    if (variants.empty()) return "";
    if (variants.size() == 1) return variants[0];
    
    struct Candidate {
        std::string path;
        float scale;
        float scoreDiff;
    };
    
    std::vector<Candidate> candidates;
    candidates.reserve(variants.size());
    
    IResourceProvider* provider = ResourceManager::getInstance().getProvider(namespaceName.c_str());
    if (!provider) return "";
    
    for (const auto& variant : variants) {
        const Resources::ResourceData* res = provider->find(variant.c_str());
        if (res) {
            float scaleDiff = std::abs(res->designScale - m_currentDPI);
            candidates.push_back({variant, res->designScale, scaleDiff});
        }
    }
    
    std::sort(candidates.begin(), candidates.end(),
        [this](const Candidate& a, const Candidate& b) {
            if (a.scale >= m_currentDPI && b.scale >= m_currentDPI)
                return a.scale < b.scale;
            if (a.scale >= m_currentDPI)
                return true;
            if (b.scale >= m_currentDPI)
                return false;
            return a.scale > b.scale;
        });
    
    return candidates.empty() ? "" : candidates[0].path;
}

void* TextureCache::getTexture(const char* namespaceName, const char* resourcePath,
                                uint32_t& outWidth, uint32_t& outHeight, float* outDesignScale)
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "Not initialized");
    YUCHEN_ASSERT_MSG(namespaceName != nullptr, "Namespace cannot be null");
    YUCHEN_ASSERT_MSG(resourcePath != nullptr, "Resource path cannot be null");
    
    std::string cacheKey = std::string(namespaceName) + ":" + resourcePath;
    
    auto it = m_textureCache.find(cacheKey);
    if (it != m_textureCache.end()) {
        outWidth = it->second.width;
        outHeight = it->second.height;
        if (outDesignScale) *outDesignScale = it->second.designScale;
        return it->second.handle;
    }
    
    std::string basePath = extractBaseName(resourcePath);
    std::string bestPath = selectBestResource(namespaceName, basePath);
    
    if (bestPath.empty()) bestPath = resourcePath;
    
    float designScale = 1.0f;
    void* handle = createTextureFromResource(namespaceName, bestPath.c_str(), outWidth, outHeight, designScale);
    if (outDesignScale) *outDesignScale = designScale;

    if (handle && bestPath != resourcePath) {
        std::string bestCacheKey = std::string(namespaceName) + ":" + bestPath;
        m_textureCache[cacheKey] = TextureEntry(handle, outWidth, outHeight, designScale);
    }
    
    return handle;
}

void* TextureCache::createTextureFromResource(const char* namespaceName, const char* resourcePath,
                                               uint32_t& outWidth, uint32_t& outHeight, float& outDesignScale)
{
    std::string cacheKey = std::string(namespaceName) + ":" + resourcePath;
    
    auto it = m_textureCache.find(cacheKey);
    if (it != m_textureCache.end()) {
        outWidth = it->second.width;
        outHeight = it->second.height;
        outDesignScale = it->second.designScale;
        return it->second.handle;
    }
    
    const Resources::ResourceData* resource = m_resolver->find(namespaceName, resourcePath);
    if (!resource) return nullptr;
    
    outDesignScale = resource->designScale;
    
    ImageData imageData;
    if (!ImageDecoder::decodePNGFromMemory(resource->data, resource->size, imageData)) return nullptr;
    
    void* textureHandle = m_backend->createTexture2D(
        imageData.width,
        imageData.height,
        TextureFormat::RGBA8_Unorm
    );
    
    if (!textureHandle) return nullptr;
    
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
    
    m_textureCache[cacheKey] = TextureEntry(textureHandle, imageData.width, imageData.height, outDesignScale);
    
    return textureHandle;
}

void TextureCache::clearAll()
{
    std::unordered_set<void*> uniqueHandles;
    
    for (auto& pair : m_textureCache) {
        if (pair.second.handle) uniqueHandles.insert(pair.second.handle);
    }
    
    for (void* handle : uniqueHandles) {
        m_backend->destroyTexture(handle);
    }
    
    m_textureCache.clear();
}

}
