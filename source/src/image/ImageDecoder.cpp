#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "YuchenUI/image/ImageDecoder.h"
#include "YuchenUI/core/Assert.h"

namespace YuchenUI {

bool ImageDecoder::decodePNGFromMemory(const unsigned char* data, size_t size, ImageData& outData)
{
    YUCHEN_ASSERT_MSG(data != nullptr, "Image data cannot be null");
    YUCHEN_ASSERT_MSG(size > 0, "Image size must be positive");
    
    outData.pixels.clear();
    outData.width = 0;
    outData.height = 0;
    
    int width, height, channels;
    unsigned char* pixels = stbi_load_from_memory(
        data, static_cast<int>(size),
        &width, &height, &channels, 4
    );
    
    if (!pixels) {
        return false;
    }
    
    outData.width = static_cast<uint32_t>(width);
    outData.height = static_cast<uint32_t>(height);
    outData.pixels.assign(pixels, pixels + width * height * 4);
    
    stbi_image_free(pixels);
    return true;
}

bool ImageDecoder::decodePNG(const Resources::ResourceData& resource, ImageData& outData)
{
    YUCHEN_ASSERT_MSG(resource.data != nullptr, "Resource data cannot be null");
    YUCHEN_ASSERT_MSG(resource.size > 0, "Resource size must be positive");
    
    return decodePNGFromMemory(resource.data, resource.size, outData);
}

}
