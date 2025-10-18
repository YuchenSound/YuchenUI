#pragma once

#include "ResourceData.h"
#include <vector>

namespace YuchenUI {

class IResourceProvider {
public:
    virtual ~IResourceProvider() = default;
    
    virtual const Resources::ResourceData* find(const char* path) = 0;
    
    virtual std::vector<const Resources::ResourceData*> matchResources(
        const char* pathPrefix,
        const char* extension = nullptr
    ) = 0;
};

}
