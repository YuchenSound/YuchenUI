#pragma once

#include "YuchenUI/core/Types.h"

namespace YuchenUI {

class IInputMethodSupport {
public:
    virtual ~IInputMethodSupport() = default;

    virtual Rect getInputMethodCursorRect() const = 0;
};

}
