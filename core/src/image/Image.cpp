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
    - Rendering delegated to RenderList command
    - Visibility check performed before adding draw command
*/

#include "YuchenUI/widgets/Image.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/core/Validation.h"
#include "YuchenUI/core/Assert.h"

namespace YuchenUI {

//==========================================================================================
// Lifecycle

Image::Image(const Rect& bounds)
    : m_resourceIdentifier()
    , m_bounds(bounds)
    , m_scaleMode(ScaleMode::Stretch)
    , m_nineSliceMargins()
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
    
    // Add image draw command
    commandList.drawImage(m_resourceIdentifier.c_str(), absRect, m_scaleMode, m_nineSliceMargins);
}

//==========================================================================================
// Configuration

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
// Validation

bool Image::isValid() const
{
    return m_bounds.isValid() && !m_resourceIdentifier.empty();
}

} // namespace YuchenUI
