/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Widgets module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

//==========================================================================================
/** @file Image.cpp
    
    Implementation notes:
    - Resource identifier references embedded resource by path
    - Supports multiple scale modes: Original, Stretch, Fill, NineSlice
    - Nine-slice margins define non-stretchable edges for scalable borders
    - Multi-frame support: frame index is clamped to valid range [0, frameCount-1]
    - Source rectangle calculated in logical pixels, backend handles DPI scaling
    - Rendering delegated to RenderList command
    - Visibility check performed before adding draw command
*/

#include "YuchenUI/widgets/Image.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/core/Validation.h"
#include "YuchenUI/core/Assert.h"
#include <algorithm>

namespace YuchenUI {

//==========================================================================================
// Lifecycle

Image::Image(const Rect& bounds)
    : m_resourceIdentifier()
    , m_bounds(bounds)
    , m_scaleMode(ScaleMode::Stretch)
    , m_nineSliceMargins()
    , m_frameCount(1)
    , m_direction(FrameDirection::Vertical)
    , m_frameSize()
    , m_currentFrame(0)
{
    Validation::AssertRect(bounds);
}

Image::~Image()
{
}

//==========================================================================================
// Rendering

void Image::addDrawCommands(RenderList& commandList, const Vec2& offset) const
{
    // Skip rendering if invisible or no resource set
    if (!m_isVisible || m_resourceIdentifier.empty()) return;
    
    // Calculate absolute rectangle in parent coordinate space
    Rect absRect(
        m_bounds.x + offset.x,
        m_bounds.y + offset.y,
        m_bounds.width,
        m_bounds.height
    );
    
    // Calculate source rectangle (empty for single image, specific region for sprite sheet)
    Rect sourceRect = calculateSourceRect();
    
    // Add appropriate draw command based on whether we have a source rect
    if (sourceRect.width > 0.0f && sourceRect.height > 0.0f)
    {
        // Multi-frame mode: draw specific region
        commandList.drawImageRegion(m_resourceIdentifier.c_str(), absRect, sourceRect, m_scaleMode);
    }
    else
    {
        // Single image mode: draw entire image
        commandList.drawImage(m_resourceIdentifier.c_str(), absRect, m_scaleMode, m_nineSliceMargins);
    }
}

//==========================================================================================
// Basic configuration

void Image::setResource(const char* resourceIdentifier)
{
    YUCHEN_ASSERT(resourceIdentifier);
    m_resourceIdentifier = resourceIdentifier;
}

void Image::setScaleMode(ScaleMode mode)
{
    m_scaleMode = mode;
}

void Image::setNineSliceMargins(float left, float top, float right, float bottom)
{
    m_nineSliceMargins = NineSliceMargins(left, top, right, bottom);
    YUCHEN_ASSERT(m_nineSliceMargins.isValid());
}

void Image::setNineSliceMargins(const NineSliceMargins& margins)
{
    YUCHEN_ASSERT(margins.isValid());
    m_nineSliceMargins = margins;
}

void Image::setBounds(const Rect& bounds)
{
    Validation::AssertRect(bounds);
    m_bounds = bounds;
}

//==========================================================================================
// Multi-frame support

void Image::setFrameConfiguration(int frameCount, FrameDirection direction, const Vec2& frameSize)
{
    YUCHEN_ASSERT(frameCount > 0);
    YUCHEN_ASSERT(frameSize.x > 0.0f && frameSize.y > 0.0f);
    
    m_frameCount = frameCount;
    m_direction = direction;
    m_frameSize = frameSize;
    
    // Clamp current frame to new valid range
    if (m_currentFrame >= m_frameCount) {
        m_currentFrame = m_frameCount - 1;
    }
}

void Image::setCurrentFrame(int frameIndex)
{
    // Clamp frame index to valid range
    m_currentFrame = std::max(0, std::min(frameIndex, m_frameCount - 1));
}

//==========================================================================================
// Validation

bool Image::isValid() const
{
    if (!m_bounds.isValid() || m_resourceIdentifier.empty()) {
        return false;
    }
    
    // Validate multi-frame configuration if enabled
    if (m_frameCount > 1) {
        if (m_frameSize.x <= 0.0f || m_frameSize.y <= 0.0f) {
            return false;
        }
        if (m_currentFrame < 0 || m_currentFrame >= m_frameCount) {
            return false;
        }
    }
    
    return true;
}

//==========================================================================================
// Internal

Rect Image::calculateSourceRect() const
{
    // Single image mode: return empty rect (backend will use full texture)
    if (m_frameCount <= 1 || m_frameSize.x <= 0.0f || m_frameSize.y <= 0.0f) {
        return Rect();
    }
    
    // Multi-frame mode: calculate frame position based on direction and index
    if (m_direction == FrameDirection::Horizontal)
    {
        // Frames arranged left to right
        return Rect(
            m_frameSize.x * m_currentFrame,  // x offset
            0.0f,                             // y offset
            m_frameSize.x,                    // width
            m_frameSize.y                     // height
        );
    }
    else // FrameDirection::Vertical
    {
        // Frames arranged top to bottom
        return Rect(
            0.0f,                             // x offset
            m_frameSize.y * m_currentFrame,  // y offset
            m_frameSize.x,                    // width
            m_frameSize.y                     // height
        );
    }
}

} // namespace YuchenUI
