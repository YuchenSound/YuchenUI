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
/** @file MenuItem.h
    
    Individual menu item with type, state, and callback.
    
    MenuItem represents single entry in menu with various types:
    - Normal: Regular clickable item
    - Separator: Visual divider (no interaction)
    - Submenu: Opens another menu
    - Checkable: Toggle on/off state
    - Radio: Mutually exclusive within group
    
    Items owned by Menu and should not be created directly. Use Menu::addItem()
    factory methods instead.
*/

#pragma once

#include "YuchenUI/core/Types.h"
#include <string>
#include <functional>

namespace YuchenUI {

class Menu;

//==========================================================================================
/** Type of menu item. */
enum class MenuItemType {
    Normal,      ///< Regular clickable menu item
    Separator,   ///< Visual separator (horizontal line)
    Submenu,     ///< Item that opens submenu
    Checkable,   ///< Item with on/off state (independent)
    Radio        ///< Item with on/off state (mutually exclusive group)
};

//==========================================================================================
/** Callback function invoked when menu item selected.
    
    Called synchronously when user selects item. For checkable/radio items,
    called after checked state updated.
*/
using MenuItemCallback = std::function<void()>;

//==========================================================================================
/**
    Individual menu item with type, state, and callback.
    
    MenuItem encapsulates properties and behavior of single menu entry.
    Type determines available properties and validation rules. Items
    owned by Menu and created via Menu factory methods.
    
    Property requirements by type:
    - Normal: text required, callback optional
    - Separator: no properties required
    - Submenu: text and submenu pointer required
    - Checkable: text required, callback optional, supports checked state
    - Radio: text and group ID required, callback optional, supports checked state
    
    @see Menu, MenuItemType
*/
class MenuItem {
public:
    //======================================================================================
    /** Creates menu item with default properties.
        
        Type defaults to Normal. All other properties initialized to defaults.
    */
    MenuItem();
    
    /** Destructor. */
    ~MenuItem();

    //======================================================================================
    /** Sets display text for menu item.
        
        @param text  Text shown in menu (UTF-8)
    */
    void setText(const std::string& text);
    
    /** Returns display text.
        
        @returns Current menu item text
    */
    const std::string& getText() const { return m_text; }

    /** Sets keyboard shortcut text.
        
        Shortcut displayed but not automatically handled. Application must
        implement keyboard handling separately.
        
        @param shortcut  Shortcut text (e.g., "Cmd+C", "Ctrl+Alt+Delete")
    */
    void setShortcut(const std::string& shortcut);
    
    /** Returns keyboard shortcut text.
        
        @returns Shortcut string, or empty if none
    */
    const std::string& getShortcut() const { return m_shortcut; }

    /** Sets enabled state of menu item.
        
        Disabled items displayed grayed out and cannot be selected.
        
        @param enabled  True to enable, false to disable
    */
    void setEnabled(bool enabled);
    
    /** Returns enabled state.
        
        @returns True if enabled, false if disabled
    */
    bool isEnabled() const { return m_enabled; }

    /** Sets checked state for checkable/radio items.
        
        Only valid for Checkable and Radio types. Asserts if called on
        other types.
        
        @param checked  True for checked, false for unchecked
    */
    void setChecked(bool checked);
    
    /** Returns checked state.
        
        Only meaningful for Checkable and Radio types.
        
        @returns True if checked, false if unchecked
    */
    bool isChecked() const { return m_checked; }

    /** Sets menu item type.
        
        Changes validation rules and available properties.
        
        @param type  New item type
    */
    void setType(MenuItemType type);
    
    /** Returns menu item type.
        
        @returns Current type
    */
    MenuItemType getType() const { return m_type; }

    /** Sets radio group ID.
        
        Radio items with same group ID are mutually exclusive. Only valid
        for Radio type. Asserts if called on other types.
        
        @param groupId  Group ID (0 or positive)
    */
    void setRadioGroup(int groupId);
    
    /** Returns radio group ID.
        
        Only meaningful for Radio type.
        
        @returns Group ID
    */
    int getRadioGroup() const { return m_radioGroup; }

    //======================================================================================
    /** Sets callback function.
        
        Callback invoked when user selects item.
        
        @param callback  Function to call, or nullptr for no callback
    */
    void setCallback(MenuItemCallback callback);
    
    /** Returns callback function.
        
        @returns Current callback, or nullptr if none
    */
    MenuItemCallback getCallback() const { return m_callback; }

    //======================================================================================
    /** Sets submenu for Submenu type.
        
        Submenu opened when item selected. Only valid for Submenu type.
        Asserts if called on other types. Menu does not take ownership.
        
        @param submenu  Submenu to attach (not owned)
    */
    void setSubmenu(Menu* submenu);
    
    /** Returns attached submenu.
        
        Only meaningful for Submenu type.
        
        @returns Submenu pointer, or nullptr if none
    */
    Menu* getSubmenu() const { return m_submenu; }

    //======================================================================================
    /** Invokes callback if set.
        
        Called by menu system when item selected. For checkable/radio items,
        checked state already updated before this called.
    */
    void triggerCallback();
    
    /** Returns true if item is separator type. */
    bool isSeparator() const { return m_type == MenuItemType::Separator; }
    
    /** Returns true if item has submenu attached. */
    bool hasSubmenu() const { return m_submenu != nullptr; }

    //======================================================================================
    /** Validates item properties for current type.
        
        Checks type-specific requirements:
        - Separator: always valid
        - Submenu: requires text and submenu pointer
        - Normal/Checkable: requires text
        - Radio: requires text and non-negative group ID
        
        @returns True if item valid for its type
    */
    bool isValid() const;

private:
    //======================================================================================
    std::string m_text;             ///< Display text
    std::string m_shortcut;         ///< Keyboard shortcut text
    bool m_enabled;                 ///< Enabled state
    bool m_checked;                 ///< Checked state (Checkable/Radio only)
    MenuItemType m_type;            ///< Item type
    int m_radioGroup;               ///< Radio group ID (Radio only)
    MenuItemCallback m_callback;    ///< Selection callback
    Menu* m_submenu;                ///< Attached submenu (Submenu only, not owned)

    MenuItem(const MenuItem&) = delete;
    MenuItem& operator=(const MenuItem&) = delete;
};

} // namespace YuchenUI
