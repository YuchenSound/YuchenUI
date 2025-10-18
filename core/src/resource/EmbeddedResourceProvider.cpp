#include "YuchenUI/resource/EmbeddedResourceProvider.h"
#include <algorithm>
#include <cctype>

namespace YuchenUI {

EmbeddedResourceProvider::EmbeddedResourceProvider(const Resources::ResourceData* resources, size_t count)
    : m_resources(resources)
    , m_count(count)
    , m_cache()
{
    buildCache();
}

void EmbeddedResourceProvider::buildCache()
{
    for (size_t i = 0; i < m_count; ++i)
    {
        std::string pathKey(m_resources[i].path.data(), m_resources[i].path.size());
        m_cache[pathKey] = &m_resources[i];
    }
}

const Resources::ResourceData* EmbeddedResourceProvider::find(const char* path)
{
    auto it = m_cache.find(path);
    return (it != m_cache.end()) ? it->second : nullptr;
}

std::vector<const Resources::ResourceData*> EmbeddedResourceProvider::matchResources(
    const char* pathPrefix,
    const char* extension)
{
    std::vector<const Resources::ResourceData*> results;
    results.reserve(m_count / 4);
    
    for (size_t i = 0; i < m_count; ++i)
    {
        const Resources::ResourceData* res = &m_resources[i];
        std::string path(res->path.data(), res->path.size());
        
        if (pathPrefix && !pathStartsWith(path, pathPrefix))
        {
            continue;
        }
        
        if (extension && !pathEndsWith(path, extension))
        {
            continue;
        }
        
        results.push_back(res);
    }
    
    return results;
}

bool EmbeddedResourceProvider::pathStartsWith(const std::string& path, const char* prefix) const
{
    if (!prefix || *prefix == '\0') return true;
    
    size_t prefixLen = std::strlen(prefix);
    if (path.size() < prefixLen) return false;
    
    return path.compare(0, prefixLen, prefix) == 0;
}

bool EmbeddedResourceProvider::pathEndsWith(const std::string& path, const char* extension) const
{
    if (!extension || *extension == '\0') return true;
    
    size_t extLen = std::strlen(extension);
    if (path.size() < extLen) return false;
    
    std::string pathLower = path;
    std::string extLower = extension;
    
    std::transform(pathLower.begin(), pathLower.end(), pathLower.begin(), ::tolower);
    std::transform(extLower.begin(), extLower.end(), extLower.begin(), ::tolower);
    
    return pathLower.compare(pathLower.size() - extLen, extLen, extLower) == 0;
}

}
