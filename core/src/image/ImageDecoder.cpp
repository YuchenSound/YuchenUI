/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Image module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

//==========================================================================================
/** @file ImageDecoder.cpp
    
    Implementation notes:
    - Uses stb_image library for PNG decoding
    - Forces 4-channel RGBA output regardless of source format
    - Supports decoding from memory buffer or embedded resource
    - Validates input data before attempting decode
    - Automatically frees stb_image allocated memory
*/

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "YuchenUI/image/ImageDecoder.h"
#include "YuchenUI/core/Assert.h"

namespace YuchenUI {

//==========================================================================================
// Decoding

bool ImageDecoder::decodePNGFromMemory(const unsigned char* data, size_t size, ImageData& outData)
{
    YUCHEN_ASSERT_MSG(data != nullptr, "Image data cannot be null");
    YUCHEN_ASSERT_MSG(size > 0, "Image size must be positive");
    
    // Clear output structure
    outData.pixels.clear();
    outData.width = 0;
    outData.height = 0;
    
    // Decode image from memory
    int width, height, channels;
    unsigned char* pixels = stbi_load_from_memory(
        data, static_cast<int>(size),
        &width, &height, &channels, 4  // Force 4 channels (RGBA)
    );
    
    if (!pixels) return false;
    
    // Copy decoded data to output structure
    outData.width = static_cast<uint32_t>(width);
    outData.height = static_cast<uint32_t>(height);
    outData.pixels.assign(pixels, pixels + width * height * 4);
    
    // Free stb_image allocated memory
    stbi_image_free(pixels);
    return true;
}

bool ImageDecoder::decodePNG(const Resources::ResourceData& resource, ImageData& outData)
{
    YUCHEN_ASSERT_MSG(resource.data != nullptr, "Resource data cannot be null");
    YUCHEN_ASSERT_MSG(resource.size > 0, "Resource size must be positive");
    
    return decodePNGFromMemory(resource.data, resource.size, outData);
}

} // namespace YuchenUI
