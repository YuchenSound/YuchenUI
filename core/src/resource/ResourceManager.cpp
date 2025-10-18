#include "YuchenUI/resource/ResourceManager.h"
#include "YuchenUI/resource/IResourceProvider.h"
#include "YuchenUI/core/Assert.h"

namespace YuchenUI {

ResourceManager& ResourceManager::getInstance() {
    static ResourceManager instance;
    return instance;
}

void ResourceManager::registerProvider(const char* namespaceName, IResourceProvider* provider) {
    YUCHEN_ASSERT(namespaceName != nullptr);
    YUCHEN_ASSERT(provider != nullptr);
    
    std::string ns(namespaceName);
    auto it = m_providers.find(ns);
    YUCHEN_ASSERT(it == m_providers.end());
    
    m_providers[ns] = provider;
}

const Resources::ResourceData* ResourceManager::find(const char* namespaceName, const char* path) {
    YUCHEN_ASSERT(namespaceName != nullptr);
    YUCHEN_ASSERT(path != nullptr);
    
    auto it = m_providers.find(namespaceName);
    YUCHEN_ASSERT(it != m_providers.end());
    
    const Resources::ResourceData* result = it->second->find(path);
    YUCHEN_ASSERT(result != nullptr);
    
    return result;
}

IResourceProvider* ResourceManager::getProvider(const char* namespaceName) {
    YUCHEN_ASSERT(namespaceName != nullptr);
    
    auto it = m_providers.find(namespaceName);
    return (it != m_providers.end()) ? it->second : nullptr;
}

ResourceManager::~ResourceManager() {
    for (auto& pair : m_providers) {
        delete pair.second;
    }
}

}
