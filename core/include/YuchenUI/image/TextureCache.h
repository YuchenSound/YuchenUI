#pragma once

#include "YuchenUI/core/Types.h"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>

namespace YuchenUI {

class IGraphicsBackend;
class IResourceResolver;

class TextureCache {
public:
    TextureCache(IGraphicsBackend* backend, IResourceResolver* resolver);
    ~TextureCache();
    
    bool initialize();
    void destroy();
    bool isInitialized() const { return m_isInitialized; }
    
    void setCurrentDPI(float dpiScale);
    
    void* getTexture(const char* namespaceName, const char* path,
                     uint32_t& outWidth, uint32_t& outHeight,
                     float* outDesignScale = nullptr);
    
    void clearAll();

private:
    struct TextureEntry {
        void* handle;
        uint32_t width;
        uint32_t height;
        float designScale;
        
        TextureEntry() : handle(nullptr), width(0), height(0), designScale(1.0f) {}
        TextureEntry(void* h, uint32_t w, uint32_t ht, float ds)
            : handle(h), width(w), height(ht), designScale(ds) {}
    };
    
    std::string extractBaseName(const std::string& path) const;
    std::vector<std::string> findAllVariants(const std::string& namespaceName, const std::string& basePath) const;
    std::string selectBestResource(const std::string& namespaceName, const std::string& basePath) const;
    void* createTextureFromResource(const char* namespaceName, const char* resourcePath,
                                     uint32_t& outWidth, uint32_t& outHeight, float& outDesignScale);
    
    IGraphicsBackend* m_backend;
    IResourceResolver* m_resolver;
    std::unordered_map<std::string, TextureEntry> m_textureCache;
    bool m_isInitialized;
    float m_currentDPI;
    
    TextureCache(const TextureCache&) = delete;
    TextureCache& operator=(const TextureCache&) = delete;
};

}
