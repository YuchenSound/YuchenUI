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
/** @file IMenuBackend.h
    
    Platform-independent interface for native menu implementation.
    
    Abstract interface allowing platform-specific menu implementations (Windows, macOS, Linux)
    while maintaining common menu API. Platform implementations register factory function
    at initialization to create appropriate backend instances.
    
    Factory pattern usage:
    1. Platform code calls registerFactory() at startup with platform-specific factory
    2. Menu calls createBackend() to get platform-appropriate implementation
    3. Backend handles all native menu API calls
*/

#pragma once

#include <string>
#include <memory>
#include <functional>

namespace YuchenUI {

class MenuItem;
class Menu;
class IMenuBackend;

//==========================================================================================
/** Factory function type for creating menu backend instances.
    
    Platform implementations provide factory that creates appropriate backend.
*/
using MenuBackendFactory = std::function<std::unique_ptr<IMenuBackend>()>;

//==========================================================================================
/**
    Platform-independent interface for native menu systems.
    
    IMenuBackend abstracts platform-specific menu APIs (NSMenu on macOS,
    HMENU on Windows, etc.) behind common interface. Platform implementations
    create native menu items and handle platform events.
    
    Typical platform responsibilities:
    - Create/destroy native menu handles
    - Add items, separators, submenus to native menu
    - Update item state (enabled, checked, text)
    - Display popup menus at screen coordinates
    - Translate native events to MenuItem callbacks
    
    @see Menu, MenuItem
*/
class IMenuBackend {
public:
    /** Virtual destructor for proper cleanup of platform resources. */
    virtual ~IMenuBackend() = default;
    
    //======================================================================================
    /** Creates platform-specific native menu handle.
        
        @returns True if native menu created successfully
    */
    virtual bool createNativeMenu() = 0;
    
    /** Destroys platform-specific native menu handle. */
    virtual void destroyNativeMenu() = 0;
    
    //======================================================================================
    /** Adds normal menu item to native menu.
        
        @param item   MenuItem containing text, callback, enabled state, etc.
        @param index  Position in menu (0-based)
    */
    virtual void addNativeItem(const MenuItem* item, size_t index) = 0;
    
    /** Adds visual separator to native menu.
        
        @param index  Position in menu (0-based)
    */
    virtual void addNativeSeparator(size_t index) = 0;
    
    /** Adds submenu item to native menu.
        
        @param item     MenuItem containing submenu text
        @param submenu  Submenu to attach
        @param index    Position in menu (0-based)
    */
    virtual void addNativeSubmenu(const MenuItem* item, Menu* submenu, size_t index) = 0;
    
    //======================================================================================
    /** Updates enabled state of native menu item.
        
        @param index    Item index (0-based)
        @param enabled  New enabled state
    */
    virtual void updateItemEnabled(size_t index, bool enabled) = 0;
    
    /** Updates checked state of native menu item.
        
        @param index    Item index (0-based)
        @param checked  New checked state
    */
    virtual void updateItemChecked(size_t index, bool checked) = 0;
    
    /** Updates text of native menu item.
        
        @param index  Item index (0-based)
        @param text   New item text
    */
    virtual void updateItemText(size_t index, const std::string& text) = 0;
    
    /** Removes all items from native menu. */
    virtual void clearNativeMenu() = 0;
    
    //======================================================================================
    /** Displays native menu as popup at screen coordinates.
        
        Blocks until menu dismissed. Platform handles event routing and
        triggers MenuItem callbacks when items selected.
        
        @param screenX  Screen X coordinate
        @param screenY  Screen Y coordinate
    */
    virtual void popupNativeMenu(float screenX, float screenY) = 0;
    
    /** Returns platform-specific native menu handle.
        
        Return type is void* for platform independence. Cast to appropriate
        type (NSMenu*, HMENU, etc.) in platform code.
        
        @returns Opaque native menu handle
    */
    virtual void* getNativeHandle() const = 0;
    
    /** Sets the owning Menu instance for callback routing.
        
        @param menu  Parent Menu object
    */
    virtual void setOwnerMenu(Menu* menu) = 0;
    
    //======================================================================================
    /** Registers factory function for creating backend instances.
        
        Platform implementation calls this at initialization to register
        its factory function. Only one factory can be registered.
        
        @param factory  Factory function returning platform-specific backend
    */
    static void registerFactory(MenuBackendFactory factory);
    
    /** Creates backend instance using registered factory.
        
        @returns Platform-specific backend, or nullptr if no factory registered
    */
    static std::unique_ptr<IMenuBackend> createBackend();
};

} // namespace YuchenUI
