/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Menu module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

//==========================================================================================
/** @file IMenuBackend.cpp
    
    Implementation notes:
    - Factory pattern allows platform-specific backend registration
    - Single global factory function stored in static storage
    - Platform implementation registers factory at initialization
    - Menu class uses factory to create appropriate backend instance
*/

#include "YuchenUI/menu/IMenuBackend.h"

namespace YuchenUI {

//==========================================================================================
// Factory Storage

static MenuBackendFactory& getFactoryStorage()
{
    static MenuBackendFactory factory = nullptr;
    return factory;
}

//==========================================================================================
// Factory Registration

void IMenuBackend::registerFactory(MenuBackendFactory factory)
{
    getFactoryStorage() = factory;
}

std::unique_ptr<IMenuBackend> IMenuBackend::createBackend()
{
    auto& factory = getFactoryStorage();
    if (factory) return factory();
    return nullptr;
}

} // namespace YuchenUI
