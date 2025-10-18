#pragma once

#include "IResourceProvider.h"
#include <unordered_map>
#include <string>
#include <vector>

namespace YuchenUI {

class EmbeddedResourceProvider : public IResourceProvider {
public:
    EmbeddedResourceProvider(const Resources::ResourceData* resources, size_t count);
    
    const Resources::ResourceData* find(const char* path) override;
    
    std::vector<const Resources::ResourceData*> matchResources(
        const char* pathPrefix,
        const char* extension = nullptr
    ) override;
    
private:
    const Resources::ResourceData* m_resources;
    size_t m_count;
    std::unordered_map<std::string, const Resources::ResourceData*> m_cache;
    
    void buildCache();
    
    bool pathStartsWith(const std::string& path, const char* prefix) const;
    bool pathEndsWith(const std::string& path, const char* extension) const;
};

}
