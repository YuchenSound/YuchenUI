/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Theme module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

#include "YuchenUI/theme/Theme.h"
#include "YuchenUI/text/FontManager.h"

namespace YuchenUI {

IFontProvider* UIStyle::getFontProvider() const
{
    // If provider was injected, use it
    if (m_fontProvider)
    {
        return m_fontProvider;
    }
    
    // Fallback to singleton for backward compatibility
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    #ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4996)
    #endif
    
    return &FontManager::getInstance();
    
    #ifdef _MSC_VER
    #pragma warning(pop)
    #endif
    #pragma GCC diagnostic pop
}

} // namespace YuchenUI
