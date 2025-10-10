#pragma once

#include "YuchenUI/core/Types.h"

namespace YuchenUI {

class IScrollable {
public:
    virtual ~IScrollable() = default;
    
    virtual bool scrollRectIntoView(const Rect& rect) = 0;
    virtual Rect getVisibleContentArea() const = 0;
    virtual Vec2 getScrollOffset() const = 0;
};

}
