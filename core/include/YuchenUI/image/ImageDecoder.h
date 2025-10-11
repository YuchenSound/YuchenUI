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
/** @file ImageDecoder.h
    
    PNG image decoding using stb_image library.
    
    Provides static functions for decoding PNG images from memory buffers or
    embedded resources. All images converted to 4-channel RGBA format regardless
    of source format.
    
    Implementation uses stb_image library (single-header public domain library).
    No external dependencies required.
*/

#pragma once

#include "YuchenUI/core/Types.h"
#include "../generated/embedded_resources.h"
#include <vector>

namespace YuchenUI {

//==========================================================================================
/** Decoded image data in RGBA format.
    
    Contains raw pixel data as 8-bit RGBA values with image dimensions.
    Data stored in row-major order, 4 bytes per pixel.
*/
struct ImageData {
    std::vector<uint8_t> pixels;  ///< Raw pixel data (width * height * 4 bytes)
    uint32_t width;               ///< Image width in pixels
    uint32_t height;              ///< Image height in pixels
    
    ImageData() : pixels(), width(0), height(0) {}
    
    /** Returns true if image has valid dimensions and pixel data. */
    bool isValid() const {
        return !pixels.empty() && width > 0 && height > 0;
    }
};

//==========================================================================================
/** Static utility class for PNG image decoding.
    
    Provides platform-independent PNG decoding from memory buffers.
    All methods are static; class cannot be instantiated.
*/
class ImageDecoder {
public:
    //======================================================================================
    /** Decodes PNG image from embedded resource.
        
        Reads PNG data from embedded resource and decodes to RGBA format.
        Output format is always 4-channel 8-bit RGBA regardless of source.
        
        @param resource  Embedded resource containing PNG data
        @param outData   Output structure to receive decoded image
        @returns True if decode succeeded, false on error
    */
    static bool decodePNG(const Resources::ResourceData& resource, ImageData& outData);
    
    /** Decodes PNG image from memory buffer.
        
        Reads PNG data from memory buffer and decodes to RGBA format.
        Output format is always 4-channel 8-bit RGBA regardless of source.
        
        @param data      Pointer to PNG data in memory
        @param size      Size of PNG data in bytes
        @param outData   Output structure to receive decoded image
        @returns True if decode succeeded, false on error
    */
    static bool decodePNGFromMemory(const unsigned char* data, size_t size, ImageData& outData);

private:
    /** Private constructor. Class contains only static methods. */
    ImageDecoder() = delete;
};

} // namespace YuchenUI
