#pragma once

#include <cstdint>
#include <vector>

namespace YuchenUI {

struct ImageData {
    uint32_t width;
    uint32_t height;
    std::vector<uint8_t> pixels;
};

class ImageDecoder {
public:
    static bool decodePNGFromMemory(const unsigned char* data, size_t size, ImageData& outData);
};

}
