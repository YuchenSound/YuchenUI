#pragma once

#include "YuchenUI/core/Types.h"

namespace YuchenUI {

/**
 * Interface for converting window-relative coordinates to screen coordinates.
 *
 * Implemented by platform-specific window classes (e.g., BaseWindow).
 */
class ICoordinateMapper {
public:
    virtual ~ICoordinateMapper() = default;
    
    /**
     * Converts window-relative coordinates to screen coordinates.
     *
     * @param windowPos  Position relative to window's content area origin
     * @returns Position in screen coordinates
     */
    virtual Vec2 mapToScreen(const Vec2& windowPos) const = 0;
};

}
