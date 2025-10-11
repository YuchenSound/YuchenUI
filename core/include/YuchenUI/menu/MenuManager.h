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
/** @file MenuManager.h
    
    Singleton manager for menu system lifecycle.
    
    MenuManager provides centralized initialization and factory methods for menus.
    Singleton automatically created on first access. Currently minimal functionality
    but provides extension point for platform-specific initialization.
    
    Usage:
    - Call getInstance() to get singleton
    - Call createMenu() to create new menu instances
    - Menu system automatically initialized on first access
*/

#pragma once

#include "YuchenUI/menu/Menu.h"
#include <memory>

namespace YuchenUI {

//==========================================================================================
/**
    Singleton manager for menu system.
    
    MenuManager provides centralized control over menu system lifecycle and
    factory methods for creating menus. Singleton pattern ensures single
    instance manages global menu state.
    
    Responsibilities:
    - Initialize/destroy menu system
    - Create platform-independent Menu instances
    - Provide extension point for platform-specific initialization
    
    @see Menu, IMenuBackend
*/
class MenuManager {
public:
    //======================================================================================
    /** Returns singleton instance.
        
        Creates and initializes instance on first call. Subsequent calls
        return existing instance.
        
        @returns Reference to singleton instance
    */
    static MenuManager& getInstance();

    //======================================================================================
    /** Initializes menu system.
        
        Called automatically by getInstance() on first access. Can be called
        explicitly for early initialization.
        
        @returns True if initialization succeeded
    */
    bool initialize();
    
    /** Destroys menu system and cleans up resources.
        
        Called automatically by destructor. Can be called explicitly for
        early cleanup.
    */
    void destroy();
    
    /** Returns true if menu system initialized. */
    bool isInitialized() const { return m_isInitialized; }

    //======================================================================================
    /** Creates new menu instance.
        
        Factory method for creating platform-independent menus. Returned
        menu automatically uses appropriate platform backend.
        
        @returns Unique pointer to new menu (caller owns)
    */
    std::unique_ptr<Menu> createMenu();

private:
    //======================================================================================
    /** Private constructor for singleton. */
    MenuManager();
    
    /** Destructor. Calls destroy() if still initialized. */
    ~MenuManager();

    //======================================================================================
    static MenuManager* s_instance;  ///< Singleton instance
    bool m_isInitialized;            ///< Initialization state
    
    /** Creates platform-specific backend (for future use). */
    std::unique_ptr<IMenuBackend> createBackend();

    MenuManager(const MenuManager&) = delete;
    MenuManager& operator=(const MenuManager&) = delete;
};

} // namespace YuchenUI
