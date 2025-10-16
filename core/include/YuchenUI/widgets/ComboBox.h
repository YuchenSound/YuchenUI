/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

#pragma once

#include "YuchenUI/widgets/Widget.h"
#include "YuchenUI/core/Types.h"
#include "YuchenUI/menu/Menu.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace YuchenUI {

class RenderList;

/**
    ComboBox visual theme.
    
    Currently only Grey theme is implemented. Future versions may add more themes.
*/
enum class ComboBoxTheme {
    Grey  ///< Standard grey combobox theme
};

/**
    Item in a ComboBox dropdown list.
    
    ComboBox items can be:
    - Regular selectable items (with text and optional value)
    - Group headers (non-selectable labels that organize items)
    - Separators (visual dividers between item groups)
    
    Items have an associated integer value that can be used for identification
    or data binding. If not specified, the value defaults to -1.
*/
struct ComboBoxItem {
    std::string text;    ///< Display text
    int value;           ///< Associated integer value
    bool enabled;        ///< Whether item can be selected
    bool isGroup;        ///< Whether this is a group header
    bool isSeparator;    ///< Whether this is a separator
    
    /**
        Default constructor creates an empty item.
    */
    ComboBoxItem()
        : text(), value(-1), enabled(true), isGroup(false), isSeparator(false) {}
    
    /**
        Constructs a regular selectable item.
        
        @param t  Display text
        @param v  Associated value (default -1)
        @param e  Whether item is enabled (default true)
    */
    ComboBoxItem(const std::string& t, int v = -1, bool e = true)
        : text(t), value(v), enabled(e), isGroup(false), isSeparator(false) {}
    
    /**
        Creates a group header item.
        
        Group headers are non-selectable labels used to organize items.
        
        @param groupTitle  Text for the group header
        @return Group header item
    */
    static ComboBoxItem Group(const std::string& groupTitle) {
        ComboBoxItem item;
        item.text = groupTitle;
        item.isGroup = true;
        item.enabled = false;
        return item;
    }
    
    /**
        Creates a separator item.
        
        Separators are visual dividers between items or groups.
        
        @return Separator item
    */
    static ComboBoxItem Separator() {
        ComboBoxItem item;
        item.isSeparator = true;
        item.enabled = false;
        return item;
    }
};

using ComboBoxCallback = std::function<void(int selectedIndex, int value)>;
using MenuPopupHandler = std::function<void(const Vec2& screenPos, Menu* menu)>;

/**
    Dropdown selection widget (ComboBox/Dropdown).
    
    ComboBox allows users to select one item from a dropdown list. It supports:
    - Text items with optional integer values
    - Group headers to organize items
    - Visual separators between groups
    - Placeholder text when nothing is selected
    - Keyboard navigation (arrow keys, Space/Enter to open)
    - Selection callbacks
    
    The dropdown list is shown as a popup menu when the combobox is clicked or
    activated via keyboard.
    
    Visual states:
    - Normal: Default appearance showing selected text or placeholder
    - Hovered: Mouse over the combobox
    - Focused: Has keyboard focus (shows focus indicator)
    - Disabled: Non-interactive state
    
    Keyboard support:
    - Space/Enter: Open dropdown menu
    - Up/Down arrows: Navigate through items (wraps around)
    - Tab: Focus navigation
    
    Example usage:
    @code
    // Create combobox
    ComboBox* combo = parent->addChild<ComboBox>(Rect(10, 10, 200, 24));
    combo->setPlaceholder("Select an option...");
    
    // Add items
    combo->addItem("Small", 1);
    combo->addItem("Medium", 2);
    combo->addItem("Large", 3);
    
    // Add organized groups
    combo->addSeparator();
    combo->addGroup("Premium Options");
    combo->addItem("Extra Large", 4);
    combo->addItem("Custom Size", 5);
    
    // Handle selection
    combo->setCallback([](int index, int value) {
        std::cout << "Selected index: " << index << ", value: " << value << std::endl;
    });
    @endcode
    
    @see ComboBoxItem, ComboBoxTheme, UIComponent
*/
class ComboBox : public Widget {
public:
    /**
        Constructs a combobox with the specified bounds.
        
        The combobox is created with:
        - No items
        - No selection (-1)
        - Grey theme
        - Default placeholder text
        - Strong focus policy (keyboard + mouse focus)
        
        @param bounds  Initial bounding rectangle
    */
    explicit ComboBox(const Rect& bounds);
    
    virtual ~ComboBox();
    
    //======================================================================================
    // UIComponent Interface Implementation
    
    void addDrawCommands(RenderList& commandList, const Vec2& offset = Vec2()) const override;
    bool handleMouseMove(const Vec2& position, const Vec2& offset = Vec2()) override;
    bool handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset = Vec2()) override;
    bool handleKeyPress(const Event& event) override;
    
    //======================================================================================
    // Items Management API
    
    /**
        Adds a selectable item to the dropdown.
        
        @param text     Display text for the item
        @param value    Associated integer value (default -1)
        @param enabled  Whether item can be selected (default true)
    */
    void addItem(const std::string& text, int value = -1, bool enabled = true);
    
    /**
        Adds a group header to organize items.
        
        Group headers are non-selectable labels that visually separate item groups.
        
        @param groupTitle  Text for the group header
    */
    void addGroup(const std::string& groupTitle);
    
    /**
        Adds a visual separator between items.
        
        Separators are horizontal lines that divide the dropdown list.
    */
    void addSeparator();
    
    /**
        Replaces all items with a new list.
        
        If the currently selected index becomes invalid, the selection is cleared.
        
        @param items  Vector of items to set
    */
    void setItems(const std::vector<ComboBoxItem>& items);
    
    /**
        Removes all items from the dropdown.
        
        Also clears the current selection.
    */
    void clearItems();
    
    //======================================================================================
    // Selection API
    
    /**
        Sets the selected item by index.
        
        The index must point to a valid, enabled, selectable item (not a group or separator).
        Pass -1 to clear the selection.
        
        @param index  Index of item to select, or -1 for no selection
    */
    void setSelectedIndex(int index);
    
    /**
        Returns the index of the selected item.
        
        @return Selected index, or -1 if nothing is selected
    */
    int getSelectedIndex() const { return m_selectedIndex; }
    
    /**
        Returns the value of the selected item.
        
        @return Value of selected item, or -1 if nothing is selected
    */
    int getSelectedValue() const;
    
    /**
        Returns the text of the selected item.
        
        @return Text of selected item, or empty string if nothing is selected
    */
    std::string getSelectedText() const;
    
    //======================================================================================
    // Callback API
    
    /**
        Sets the callback invoked when selection changes.
        
        The callback receives both the selected index and the associated value.
        
        @param callback  Function to call on selection (can be nullptr to clear)
    */
    void setCallback(ComboBoxCallback callback);
    
    //======================================================================================
    // Appearance API
    
    /**
        Sets the visual theme.
        
        Currently only Grey theme is supported.
        
        @param theme  Theme to use
    */
    void setTheme(ComboBoxTheme theme);
    
    /**
        Returns the current theme.
        
        @return Current theme
    */
    ComboBoxTheme getTheme() const { return m_theme; }
    
    /**
        Sets the placeholder text shown when nothing is selected.
        
        @param placeholder  Placeholder text (e.g., "Select an option...")
    */
    void setPlaceholder(const std::string& placeholder);
    
    /**
        Returns the current placeholder text.
        
        @return Placeholder text
    */
    const std::string& getPlaceholder() const { return m_placeholder; }
    
    /**
        Sets a custom menu popup handler.
        
        By default, the menu is shown using Menu::popup(). This allows custom
        positioning or animation.
        
        @param handler  Custom popup handler function
    */
    void setMenuPopupHandler(MenuPopupHandler handler) {
        m_menuPopupHandler = handler;
    }
    
    /**
        Validates combobox state.
        
        Checks that bounds are valid.
        
        @return true if valid, false otherwise
    */
    bool isValid() const;
    
protected:
    /**
        Returns corner radius for focus indicator.
        
        ComboBoxes use slightly rounded focus indicators.
        
        @return Corner radius of 2 pixels
    */
    CornerRadius getFocusIndicatorCornerRadius() const override;

private:
    /**
        Builds the dropdown menu from current items.
        
        Called when the menu needs to be shown. Converts ComboBoxItems into MenuItems.
    */
    void buildMenu();
    
    /**
        Called when a menu item is selected.
        
        Updates the selected index and invokes the selection callback.
        
        @param index  Index of the selected item
    */
    void onMenuItemSelected(int index);
    
    /**
        Checks if an index points to a valid selectable item.
        
        @param index  Index to check
        @return true if index is valid and item is selectable
    */
    bool isValidSelectableIndex(int index) const;
    
    /**
        Opens the dropdown menu.
        
        Shows the popup menu below the combobox.
    */
    void openMenu();
    
    /**
        Selects the next item in the list.
        
        Used for Down arrow key navigation. Wraps around to the beginning.
    */
    void selectNextItem();
    
    /**
        Selects the previous item in the list.
        
        Used for Up arrow key navigation. Wraps around to the end.
    */
    void selectPreviousItem();
    
    /**
        Finds the next valid selectable index after the given index.
        
        Wraps around to the beginning if necessary.
        
        @param startIndex  Index to start searching from
        @return Next valid index, or -1 if none found
    */
    int findNextValidIndex(int startIndex) const;
    
    /**
        Finds the previous valid selectable index before the given index.
        
        Wraps around to the end if necessary.
        
        @param startIndex  Index to start searching from
        @return Previous valid index, or -1 if none found
    */
    int findPreviousValidIndex(int startIndex) const;
    
    std::vector<ComboBoxItem> m_items;         ///< List of dropdown items
    int m_selectedIndex;                       ///< Currently selected index (-1 = none)
    ComboBoxTheme m_theme;                     ///< Visual theme
    ComboBoxCallback m_callback;               ///< Selection callback
    std::string m_placeholder;                 ///< Placeholder text
    
    std::unique_ptr<Menu> m_menu;              ///< Popup menu for dropdown
    bool m_isHovered;                          ///< Whether mouse is over combobox
    bool m_menuNeedsRebuild;                   ///< Whether menu needs to be rebuilt
    
    MenuPopupHandler m_menuPopupHandler;       ///< Custom menu popup handler
};

} // namespace YuchenUI
