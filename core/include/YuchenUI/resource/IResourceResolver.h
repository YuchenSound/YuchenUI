#pragma once

#include "ResourceData.h"

namespace YuchenUI {

class IResourceResolver {
public:
    virtual ~IResourceResolver() = default;
    virtual const Resources::ResourceData* find(const char* namespaceName, const char* path) = 0;
};

}
