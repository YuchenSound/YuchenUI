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
/** @file MenuManager.cpp
    
    Implementation notes:
    - Singleton pattern with automatic initialization on first access
    - Manages lifecycle of platform menu backend system
    - Factory method creates platform-independent Menu instances
    - Singleton cleaned up when instance is destroyed
*/

#include "YuchenUI/menu/MenuManager.h"
#include "YuchenUI/core/Assert.h"

namespace YuchenUI {

//==========================================================================================
// Singleton Instance

MenuManager* MenuManager::s_instance = nullptr;

//==========================================================================================
// Singleton Access

MenuManager& MenuManager::getInstance()
{
    if (!s_instance)
    {
        s_instance = new MenuManager();
        s_instance->initialize();
    }
    return *s_instance;
}

//==========================================================================================
// Lifecycle

MenuManager::MenuManager()
    : m_isInitialized(false)
{
}

MenuManager::~MenuManager()
{
    destroy();
    if (s_instance == this) s_instance = nullptr;
}

bool MenuManager::initialize()
{
    YUCHEN_ASSERT_MSG(!m_isInitialized, "MenuManager already initialized");
    
    m_isInitialized = true;
    return true;
}

void MenuManager::destroy()
{
    if (!m_isInitialized) return;
    
    m_isInitialized = false;
}

//==========================================================================================
// Menu Factory

std::unique_ptr<Menu> MenuManager::createMenu()
{
    YUCHEN_ASSERT_MSG(m_isInitialized, "MenuManager not initialized");
    return std::make_unique<Menu>();
}

} // namespace YuchenUI
