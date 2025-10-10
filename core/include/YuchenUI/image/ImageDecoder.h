#pragma once

#include "YuchenUI/core/Types.h"
#include "../generated/embedded_resources.h"
#include <vector>

namespace YuchenUI {

struct ImageData {
    std::vector<uint8_t> pixels;
    uint32_t width;
    uint32_t height;
    
    ImageData() : pixels(), width(0), height(0) {}
    
    bool isValid() const {
        return !pixels.empty() && width > 0 && height > 0;
    }
};

class ImageDecoder {
public:
    static bool decodePNG(const Resources::ResourceData& resource, ImageData& outData);
    static bool decodePNGFromMemory(const unsigned char* data, size_t size, ImageData& outData);

private:
    ImageDecoder() = delete;
};

}
