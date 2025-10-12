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

#include "YuchenUI/theme/ThemeManager.h"
#include "YuchenUI/theme/Theme.h"
#include "YuchenUI/core/Assert.h"

namespace YuchenUI {
//==========================================================================================
// Construction

ThemeManager::ThemeManager()
    : m_currentStyle(nullptr)
    , m_fontProvider(nullptr)
{
    // Create default style
    m_currentStyle = std::make_unique<ProtoolsDarkStyle>();
}

ThemeManager::~ThemeManager()
{
}

//==========================================================================================
// IThemeProvider Implementation

void ThemeManager::setStyle(std::unique_ptr<UIStyle> style)
{
    YUCHEN_ASSERT_MSG(style != nullptr, "Cannot set null style");
    m_currentStyle = std::move(style);
    
    // Auto-inject FontProvider if it was previously set
    if (m_fontProvider && m_currentStyle)
    {
        m_currentStyle->setFontProvider(m_fontProvider);
    }
}

void ThemeManager::setFontProvider(IFontProvider* provider)
{
    YUCHEN_ASSERT_MSG(provider != nullptr, "Cannot set null font provider");
    
    // Save the font provider reference
    m_fontProvider = provider;
    
    // Inject into current style if it exists
    if (m_currentStyle)
    {
        m_currentStyle->setFontProvider(provider);
    }
}

} // namespace YuchenUI
