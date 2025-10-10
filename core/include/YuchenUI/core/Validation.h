#pragma once

#include "YuchenUI/core/Types.h"
#include "YuchenUI/core/Assert.h"
#include <cmath>
#include <cstring>

namespace YuchenUI {

class Validation {
public:
    static inline bool ValidateColor(const Vec4& color)
    {
        if (!color.isValid()) return false;
        if (color.x < 0.0f || color.x > 1.0f) return false;
        if (color.y < 0.0f || color.y > 1.0f) return false;
        if (color.z < 0.0f || color.z > 1.0f) return false;
        if (color.w < 0.0f || color.w > 1.0f) return false;
        return true;
    }
    
    static inline void AssertColor(const Vec4& color)
    {
        YUCHEN_ASSERT(ValidateColor(color));
    }
    
    static inline bool ValidateRect(const Rect& rect)
    {
        if (!rect.isValid()) return false;
        if (rect.width < 0.0f || rect.height < 0.0f) return false;
        return true;
    }
    
    static inline void AssertRect(const Rect& rect)
    {
        YUCHEN_ASSERT(rect.isValid());
    }
    
    static inline bool ValidateCornerRadius(const CornerRadius& cornerRadius)
    {
        return cornerRadius.isValid();
    }
    
    static inline void AssertCornerRadius(const CornerRadius& cornerRadius)
    {
        YUCHEN_ASSERT(cornerRadius.isValid());
    }
    
    static inline bool ValidateCornerRadiusForRect(const CornerRadius& cornerRadius, const Rect& rect)
    {
        if (!ValidateCornerRadius(cornerRadius)) return false;
        if (!ValidateRect(rect)) return false;
        
        if (cornerRadius.topLeft > rect.width * 0.5f || cornerRadius.topLeft > rect.height * 0.5f) return false;
        if (cornerRadius.topRight > rect.width * 0.5f || cornerRadius.topRight > rect.height * 0.5f) return false;
        if (cornerRadius.bottomLeft > rect.width * 0.5f || cornerRadius.bottomLeft > rect.height * 0.5f) return false;
        if (cornerRadius.bottomRight > rect.width * 0.5f || cornerRadius.bottomRight > rect.height * 0.5f) return false;
        
        if (cornerRadius.topLeft + cornerRadius.topRight > rect.width) return false;
        if (cornerRadius.bottomLeft + cornerRadius.bottomRight > rect.width) return false;
        if (cornerRadius.topLeft + cornerRadius.bottomLeft > rect.height) return false;
        if (cornerRadius.topRight + cornerRadius.bottomRight > rect.height) return false;
        
        return true;
    }
    
    static inline void AssertCornerRadiusForRect(const CornerRadius& cornerRadius, const Rect& rect)
    {
        YUCHEN_ASSERT(cornerRadius.isValid());
        cornerRadius.validateForRect(rect);
    }
    
    static inline bool ValidateBorderWidth(float borderWidth, const Rect& rect, float minWidth = 0.0f, float maxWidth = 50.0f)
    {
        if (!std::isfinite(borderWidth) || borderWidth < minWidth) return false;
        if (borderWidth > maxWidth) return false;
        if (borderWidth > rect.width * 0.5f || borderWidth > rect.height * 0.5f) return false;
        return true;
    }
    
    static inline void AssertBorderWidth(float borderWidth, const Rect& rect, float minWidth = 0.0f, float maxWidth = 50.0f)
    {
        YUCHEN_ASSERT(std::isfinite(borderWidth) && borderWidth >= minWidth && borderWidth <= maxWidth);
        YUCHEN_ASSERT_MSG(borderWidth <= rect.width * 0.5f, "Border width too large for rect width");
        YUCHEN_ASSERT_MSG(borderWidth <= rect.height * 0.5f, "Border width too large for rect height");
    }
    
    static inline bool ValidatePosition(float x, float y)
    {
        return std::isfinite(x) && std::isfinite(y);
    }
    
    static inline void AssertPosition(float x, float y)
    {
        YUCHEN_ASSERT(ValidatePosition(x, y));
    }
    
private:
    Validation() = delete;
};

}
