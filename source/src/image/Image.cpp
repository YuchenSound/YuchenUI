#include "YuchenUI/widgets/Image.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/core/Validation.h"
#include "YuchenUI/core/Assert.h"

namespace YuchenUI {

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

void Image::addDrawCommands(RenderList& commandList, const Vec2& offset) const
{
    if (!m_isVisible || m_resourceIdentifier.empty()) return;
    
    Rect absRect(
        m_bounds.x + offset.x,
        m_bounds.y + offset.y,
        m_bounds.width,
        m_bounds.height
    );
    
    commandList.drawImage(m_resourceIdentifier.c_str(), absRect, m_scaleMode, m_nineSliceMargins);
}

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

bool Image::isValid() const
{
    return m_bounds.isValid() && !m_resourceIdentifier.empty();
}

}
