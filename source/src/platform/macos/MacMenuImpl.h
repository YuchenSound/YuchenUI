/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Platform module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

//==========================================================================================
/** @file MacMenuImpl.h
    
    macOS-specific menu implementation using Cocoa NSMenu system.
    
    Implementation notes:
    - Wraps NSMenu and NSMenuItem for native menu appearance
    - Supports checkable items, radio groups, and separators
    - Menu items are dynamically created and destroyed
    - Radio button behavior is handled in callback
    - Each menu item has its own target object for action handling
*/

#pragma once

#include "YuchenUI/platform/MenuImpl.h"
#include <unordered_map>

#ifdef __OBJC__
@class NSMenu;
@class NSMenuItem;
@class MenuItemTarget;
#else
typedef void NSMenu;
typedef void NSMenuItem;
typedef void MenuItemTarget;
#endif

namespace YuchenUI {

//==========================================================================================
/**
    macOS implementation of the MenuImpl interface.
    
    MacMenuImpl creates and manages native NSMenu objects and their items.
    It handles menu callbacks through Objective-C target-action pattern.
    
    Features:
    - Native macOS menu appearance and behavior
    - Support for submenus
    - Checkable and radio group menu items
    - Dynamic item enable/disable
    - Popup menu display at arbitrary screen coordinates
    
    @see MenuImpl, Menu, MenuItem
*/
class MacMenuImpl : public MenuImpl {
public:
    //======================================================================================
    /** Creates a MacMenuImpl instance. */
    MacMenuImpl();
    
    /** Destructor. Destroys the native menu and all items. */
    virtual ~MacMenuImpl();
    
    //======================================================================================
    // MenuImpl Interface Implementation
    
    /** Creates the native NSMenu object.
        
        @returns True if menu creation succeeded, false otherwise
    */
    bool createNativeMenu() override;
    
    /** Destroys the native menu and releases all items. */
    void destroyNativeMenu() override;
    
    //======================================================================================
    /** Adds a menu item to the native menu.
        
        @param item   The menu item to add
        @param index  The index in the menu item list
    */
    void addNativeItem(const MenuItem* item, size_t index) override;
    
    /** Adds a separator line to the menu.
        
        @param index  The index in the menu item list
    */
    void addNativeSeparator(size_t index) override;
    
    /** Adds a submenu item to the menu.
        
        @param item     The menu item representing the submenu
        @param submenu  The submenu to attach
        @param index    The index in the menu item list
    */
    void addNativeSubmenu(const MenuItem* item, Menu* submenu, size_t index) override;
    
    //======================================================================================
    /** Updates the enabled state of a menu item.
        
        @param index    The index of the item to update
        @param enabled  True to enable, false to disable
    */
    void updateItemEnabled(size_t index, bool enabled) override;
    
    /** Updates the checked state of a menu item.
        
        @param index   The index of the item to update
        @param checked True to check, false to uncheck
    */
    void updateItemChecked(size_t index, bool checked) override;
    
    /** Updates the text of a menu item.
        
        @param index  The index of the item to update
        @param text   The new text for the item
    */
    void updateItemText(size_t index, const std::string& text) override;
    
    //======================================================================================
    /** Removes all items from the native menu. */
    void clearNativeMenu() override;
    
    /** Displays the menu as a popup at the specified screen coordinates.
        
        @param screenX  X coordinate in screen space
        @param screenY  Y coordinate in screen space
    */
    void popupNativeMenu(float screenX, float screenY) override;
    
    /** Returns the native NSMenu handle.
        
        @returns Pointer to the NSMenu object (void* for C++ compatibility)
    */
    void* getNativeHandle() const override;
    
    //======================================================================================
    /** Sets the owner Menu object.
        
        This is needed for radio group synchronization.
        
        @param menu  The Menu object that owns this implementation
    */
    void setOwnerMenu(Menu* menu) override;

private:
    //======================================================================================
    /** Creates a native NSMenuItem from a MenuItem object.
        
        Sets up the action target and configures appearance.
        
        @param item   The MenuItem to convert
        @param index  The index for tracking purposes
        @returns The created NSMenuItem
    */
    NSMenuItem* createNSMenuItem(const MenuItem* item, size_t index);
    
    /** Updates radio group states after a radio item is selected.
        
        Currently unused - radio logic handled in callback.
        
        @param index  The index of the selected radio item
    */
    void updateRadioGroupState(size_t index);
    
    //======================================================================================
    NSMenu* m_nativeMenu;                                       ///< The native NSMenu object
    MenuItemTarget* m_target;                                   ///< Default target object
    std::unordered_map<size_t, NSMenuItem*> m_itemMap;         ///< Maps indices to menu items
    std::unordered_map<size_t, MenuItemTarget*> m_targetMap;   ///< Maps indices to target objects
    Menu* m_ownerMenu;                                          ///< Owner Menu for callbacks
};

} // namespace YuchenUI
