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
/** @file Menu.h
    
    Platform-independent menu implementation with native backend.
    
    Menu class provides high-level API for creating and managing popup/context menus.
    Items added to menu are owned by menu and destroyed with it. Native menu backend
    created lazily on first use.
    
    Usage pattern:
    - Create menu instance
    - Add items using factory methods (addItem, addCheckableItem, etc.)
    - Call popup() to display at screen coordinates
    - Items trigger callbacks when selected
    
    Menu types:
    - Normal: Regular clickable item with optional callback
    - Separator: Visual divider (no text or callback)
    - Submenu: Item that opens another menu
    - Checkable: Item with on/off state
    - Radio: Mutually exclusive group of checkable items
*/

#pragma once

#include "YuchenUI/menu/MenuItem.h"
#include "YuchenUI/core/Types.h"
#include <vector>
#include <memory>

namespace YuchenUI {

class IMenuBackend;

//==========================================================================================
/**
    Platform-independent menu with native backend.
    
    Menu manages collection of MenuItems and delegates platform-specific operations
    to IMenuBackend implementation. Backend created lazily when menu first used.
    Menu owns all items and handles radio group exclusivity.
    
    Key features:
    - Platform-independent API for all menu operations
    - Automatic native menu synchronization
    - Radio button group management
    - Lazy backend creation
    - Nested submenu support
    
    @see MenuItem, IMenuBackend
*/
class Menu {
public:
    //======================================================================================
    /** Creates empty menu with no backend. */
    Menu();
    
    /** Destructor. Destroys all menu items and native backend. */
    ~Menu();

    //======================================================================================
    /** Adds normal menu item.
        
        @param text      Display text for item
        @param callback  Function called when item selected (optional)
        @returns Pointer to created item (owned by menu)
    */
    MenuItem* addItem(const std::string& text, MenuItemCallback callback = nullptr);
    
    /** Adds normal menu item with keyboard shortcut.
        
        Shortcut displayed but not automatically handled. Application must
        implement shortcut handling separately.
        
        @param text      Display text for item
        @param shortcut  Shortcut text (e.g., "Cmd+C", "Ctrl+V")
        @param callback  Function called when item selected (optional)
        @returns Pointer to created item (owned by menu)
    */
    MenuItem* addItem(const std::string& text, const std::string& shortcut, MenuItemCallback callback = nullptr);
    
    /** Adds visual separator to menu.
        
        Separators displayed as horizontal lines and cannot be selected.
        
        @returns Pointer to separator item (owned by menu)
    */
    MenuItem* addSeparator();
    
    /** Adds submenu item.
        
        Item opens submenu when selected. Submenu must remain valid for
        lifetime of parent menu.
        
        @param text     Display text for submenu item
        @param submenu  Submenu to attach (not owned by menu)
        @returns Pointer to submenu item (owned by menu)
    */
    MenuItem* addSubmenu(const std::string& text, Menu* submenu);
    
    /** Adds checkable menu item.
        
        Checkable items toggle between checked and unchecked when selected.
        Initial state is unchecked.
        
        @param text      Display text for item
        @param callback  Function called when item toggled (optional)
        @returns Pointer to created item (owned by menu)
    */
    MenuItem* addCheckableItem(const std::string& text, MenuItemCallback callback = nullptr);
    
    /** Adds checkable menu item with keyboard shortcut.
        
        @param text      Display text for item
        @param shortcut  Shortcut text (e.g., "Cmd+B")
        @param callback  Function called when item toggled (optional)
        @returns Pointer to created item (owned by menu)
    */
    MenuItem* addCheckableItem(const std::string& text, const std::string& shortcut, MenuItemCallback callback = nullptr);
    
    /** Adds radio button menu item.
        
        Radio items in same group are mutually exclusive. Selecting one
        automatically unchecks others in group.
        
        @param text        Display text for item
        @param radioGroup  Group ID (0 or positive)
        @param callback    Function called when item selected (optional)
        @returns Pointer to created item (owned by menu)
    */
    MenuItem* addRadioItem(const std::string& text, int radioGroup, MenuItemCallback callback = nullptr);
    
    /** Adds radio button menu item with keyboard shortcut.
        
        @param text        Display text for item
        @param shortcut    Shortcut text (e.g., "Cmd+1")
        @param radioGroup  Group ID (0 or positive)
        @param callback    Function called when item selected (optional)
        @returns Pointer to created item (owned by menu)
    */
    MenuItem* addRadioItem(const std::string& text, const std::string& shortcut, int radioGroup, MenuItemCallback callback = nullptr);

    //======================================================================================
    /** Removes all items from menu and clears native menu. */
    void clear();
    
    /** Returns number of items in menu.
        
        @returns Item count including separators
    */
    size_t getItemCount() const { return m_items.size(); }
    
    /** Retrieves item by index.
        
        @param index  Item index (0-based)
        @returns Pointer to item (owned by menu)
    */
    MenuItem* getItem(size_t index);
    
    /** Retrieves item by index (const version).
        
        @param index  Item index (0-based)
        @returns Const pointer to item (owned by menu)
    */
    const MenuItem* getItem(size_t index) const;

    //======================================================================================
    /** Displays menu as popup at screen coordinates.
        
        Blocks until user selects item or dismisses menu. Selected items
        trigger their callbacks before returning.
        
        @param screenX  Screen X coordinate
        @param screenY  Screen Y coordinate
    */
    void popup(float screenX, float screenY);
    
    /** Displays menu as popup at screen position.
        
        @param screenPosition  Screen position vector
    */
    void popup(const Vec2& screenPosition);

    //======================================================================================
    /** Builds native menu from current items.
        
        Clears existing native menu and recreates from item list.
        Called automatically when menu structure changes.
    */
    void build();
    
    /** Forces rebuild of native menu.
        
        Sets rebuild flag and immediately rebuilds native menu.
    */
    void rebuild();
    
    /** Returns platform-specific native menu handle.
        
        @returns Opaque native handle (NSMenu*, HMENU, etc.), or nullptr if no backend
    */
    void* getNativeHandle() const;

    /** Returns all menu items for inspection.
        
        @returns Vector of unique_ptr to all items
    */
    const std::vector<std::unique_ptr<MenuItem>>& getItems() const { return m_items; }

private:
    //======================================================================================
    /** Adds item to menu and marks for rebuild.
        
        Takes ownership of item via unique_ptr.
        
        @param item  Item to add (ownership transferred)
    */
    void addMenuItem(std::unique_ptr<MenuItem> item);
    
    /** Unchecks all other radio items in group.
        
        Called when radio item checked to maintain exclusivity.
        
        @param groupId      Radio group ID
        @param checkedItem  Item that was checked (excluded from unchecking)
    */
    void updateRadioGroup(int groupId, MenuItem* checkedItem);
    
    /** Creates backend if not already created. */
    void ensureBackend();

    //======================================================================================
    std::vector<std::unique_ptr<MenuItem>> m_items;  ///< Owned menu items
    std::unique_ptr<IMenuBackend> m_backend;         ///< Platform-specific backend
    bool m_needsRebuild;                             ///< Native menu needs reconstruction

    Menu(const Menu&) = delete;
    Menu& operator=(const Menu&) = delete;
};

} // namespace YuchenUI
