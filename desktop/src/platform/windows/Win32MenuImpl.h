/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Menu module (Windows).
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

#pragma once

#include "YuchenUI/menu/IMenuBackend.h"
#include "YuchenUI/core/Types.h"
#include <Windows.h>
#include <unordered_map>

namespace YuchenUI {

//==========================================================================================
/**
    Windows platform implementation of IMenuBackend using Win32 menu API.
    
    This class provides native popup menu functionality on Windows, including:
    - Creation of popup menus with items, separators, and submenus
    - Menu item state management (enabled, checked, text)
    - Radio button groups within menus
    - Context menu display at specified screen coordinates
    - Command routing from menu selections back to MenuItem callbacks
    
    The implementation maintains bidirectional mappings between menu items and Win32
    menu IDs to enable proper event routing. Global static maps track all menus and
    items across the application for command handling.
    
    @see IMenuBackend, Menu, MenuItem
*/
class Win32MenuImpl : public IMenuBackend {
public:
    //======================================================================================
    /** Creates a new Win32 menu implementation instance. */
    Win32MenuImpl();
    
    /** Destructor. Destroys the native menu and cleans up resources. */
    virtual ~Win32MenuImpl();
    
    //======================================================================================
    /** Creates the native Windows popup menu.
        
        @returns True if menu creation succeeded, false otherwise
    */
    bool createNativeMenu() override;
    
    /** Destroys the native menu and releases all associated resources.
        
        Also unregisters this menu and all its items from the global maps.
    */
    void destroyNativeMenu() override;
    
    //======================================================================================
    /** Adds a menu item to the native menu.
        
        @param item   MenuItem to add
        @param index  Position in the menu (0-based)
    */
    void addNativeItem(const MenuItem* item, size_t index) override;
    
    /** Adds a separator line to the native menu.
        
        @param index  Position in the menu (0-based)
    */
    void addNativeSeparator(size_t index) override;
    
    /** Adds a submenu to the native menu.
        
        @param item     MenuItem that represents the submenu
        @param submenu  The Menu to attach as a submenu
        @param index    Position in the menu (0-based)
    */
    void addNativeSubmenu(const MenuItem* item, Menu* submenu, size_t index) override;
    
    //======================================================================================
    /** Updates the enabled state of a menu item.
        
        @param index    Index of the menu item
        @param enabled  True to enable, false to disable (grayed)
    */
    void updateItemEnabled(size_t index, bool enabled) override;
    
    /** Updates the checked state of a menu item.
        
        @param index   Index of the menu item
        @param checked True to check, false to uncheck
    */
    void updateItemChecked(size_t index, bool checked) override;
    
    /** Updates the text of a menu item.
        
        @param index  Index of the menu item
        @param text   New text to display
    */
    void updateItemText(size_t index, const std::string& text) override;
    
    //======================================================================================
    /** Removes all items from the native menu.
        
        Also unregisters all items from the global maps.
    */
    void clearNativeMenu() override;
    
    /** Displays the menu as a popup at the specified screen coordinates.
        
        The menu tracks the active window automatically.
        
        @param screenX  X coordinate in screen space
        @param screenY  Y coordinate in screen space
    */
    void popupNativeMenu(float screenX, float screenY) override;
    
    /** Returns the native menu handle (HMENU). */
    void* getNativeHandle() const override;
    
    //======================================================================================
    /** Sets the owning Menu instance.
        
        The owning menu is used to access menu items for radio group processing.
        
        @param menu  The Menu that owns this implementation
    */
    void setOwnerMenu(Menu* menu) override;
    
    /** Handles a menu command from Windows.
        
        This static method is called when a menu item is selected. It looks up the
        corresponding MenuItem and invokes its callback. For radio items, it manages
        the mutual exclusion within the radio group.
        
        @param menuId  The Win32 menu command ID
    */
    static void handleMenuCommand(UINT menuId);

private:
    //======================================================================================
    HMENU m_hMenu;                                   ///< Native menu handle
    Menu* m_ownerMenu;                               ///< Owning Menu instance
    std::unordered_map<size_t, UINT> m_indexToId;   ///< Maps menu item index to Win32 ID
    std::unordered_map<UINT, size_t> m_idToIndex;   ///< Maps Win32 ID to menu item index
    UINT m_nextMenuId;                               ///< Next menu ID to assign
    
    static std::unordered_map<UINT, Win32MenuImpl*> s_menuMap;    ///< Global map of menu IDs to implementations
    static std::unordered_map<UINT, MenuItem*> s_itemMap;         ///< Global map of menu IDs to menu items
    static UINT s_globalMenuId;                                    ///< Global menu ID counter
};

}
