#pragma once

#include "IResourceProvider.h"
#include "IResourceResolver.h"
#include <unordered_map>
#include <string>

namespace YuchenUI {

class ResourceManager : public IResourceResolver {
public:
    static ResourceManager& getInstance();
    
    void registerProvider(const char* namespaceName, IResourceProvider* provider);
    const Resources::ResourceData* find(const char* namespaceName, const char* path) override;
    IResourceProvider* getProvider(const char* namespaceName);
    
    ~ResourceManager();
    
private:
    ResourceManager() = default;
    
    std::unordered_map<std::string, IResourceProvider*> m_providers;
    
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
};

}
