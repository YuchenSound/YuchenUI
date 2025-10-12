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
#include "YuchenUI/core/Assert.h"

namespace YuchenUI {

//==========================================================================================
// UIStyle Base Implementation

IFontProvider* UIStyle::getFontProvider() const
{
    YUCHEN_ASSERT_MSG(m_fontProvider != nullptr,
        "Font provider not set. Call setFontProvider() after creating UIStyle.");
    return m_fontProvider;
}

} // namespace YuchenUI
