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

//==========================================================================================
/** @file Win32MenuImpl.cpp
    
    Implementation notes:
    - Each menu item is assigned a unique global menu ID
    - Static maps track all menus and items for command routing
    - Radio button groups are handled by checking/unchecking related items
    - UTF-8 strings are converted to UTF-16 for Windows API calls
    - Separators are assigned ID 0 and cannot be clicked
*/

#include "Win32MenuImpl.h"
#include "YuchenUI/menu/Menu.h"
#include "YuchenUI/menu/MenuItem.h"
#include "YuchenUI/core/Assert.h"
#include <string>
#include <algorithm>

namespace YuchenUI {

//==========================================================================================
// Static Member Initialization

std::unordered_map<UINT, Win32MenuImpl*> Win32MenuImpl::s_menuMap;
std::unordered_map<UINT, MenuItem*> Win32MenuImpl::s_itemMap;
UINT Win32MenuImpl::s_globalMenuId = 1000;

//==========================================================================================
// Construction and Destruction

Win32MenuImpl::Win32MenuImpl()
    : m_hMenu(nullptr)
    , m_ownerMenu(nullptr)
    , m_nextMenuId(s_globalMenuId++)
{
}

Win32MenuImpl::~Win32MenuImpl()
{
    destroyNativeMenu();
}

//==========================================================================================
// Menu Lifecycle

void Win32MenuImpl::setOwnerMenu(Menu* menu)
{
    m_ownerMenu = menu;
}

bool Win32MenuImpl::createNativeMenu()
{
    YUCHEN_ASSERT_MSG(m_hMenu == nullptr, "Menu already created");
    
    // Create a popup menu (used for context menus)
    m_hMenu = CreatePopupMenu();
    YUCHEN_ASSERT_MSG(m_hMenu != nullptr, "Failed to create popup menu");
    
    return m_hMenu != nullptr;
}

void Win32MenuImpl::destroyNativeMenu()
{
    if (m_hMenu)
    {
        // Unregister all menu items from global maps
        for (const auto& pair : m_indexToId)
        {
            UINT menuId = pair.second;
            s_menuMap.erase(menuId);
            s_itemMap.erase(menuId);
        }
        
        // Destroy the native menu
        DestroyMenu(m_hMenu);
        m_hMenu = nullptr;
    }
    
    m_indexToId.clear();
    m_idToIndex.clear();
}

void* Win32MenuImpl::getNativeHandle() const
{
    return m_hMenu;
}

//==========================================================================================
// Menu Item Management

void Win32MenuImpl::addNativeItem(const MenuItem* item, size_t index)
{
    YUCHEN_ASSERT_MSG(m_hMenu != nullptr, "Menu not created");
    YUCHEN_ASSERT_MSG(item != nullptr, "MenuItem is null");
    
    // Assign a unique menu ID
    UINT menuId = m_nextMenuId++;
    
    // Convert UTF-8 text to UTF-16 for Windows API
    std::wstring wText;
    int len = MultiByteToWideChar(CP_UTF8, 0, item->getText().c_str(), -1, nullptr, 0);
    if (len > 0)
    {
        wText.resize(len);
        MultiByteToWideChar(CP_UTF8, 0, item->getText().c_str(), -1, &wText[0], len);
    }
    
    // Set menu item flags based on state
    UINT flags = MF_STRING;
    if (!item->isEnabled()) flags |= MF_GRAYED;
    if (item->isChecked()) flags |= MF_CHECKED;
    
    // Append the menu item
    AppendMenuW(m_hMenu, flags, menuId, wText.c_str());
    
    // Store bidirectional mappings
    m_indexToId[index] = menuId;
    m_idToIndex[menuId] = index;
    s_menuMap[menuId] = this;
    s_itemMap[menuId] = const_cast<MenuItem*>(item);
}

void Win32MenuImpl::addNativeSeparator(size_t index)
{
    YUCHEN_ASSERT_MSG(m_hMenu != nullptr, "Menu not created");
    
    // Add a separator line (non-selectable)
    AppendMenuW(m_hMenu, MF_SEPARATOR, 0, nullptr);
    
    // Separators are assigned ID 0
    m_indexToId[index] = 0;
}

void Win32MenuImpl::addNativeSubmenu(const MenuItem* item, Menu* submenu, size_t index)
{
    YUCHEN_ASSERT_MSG(m_hMenu != nullptr, "Menu not created");
    YUCHEN_ASSERT_MSG(item != nullptr, "MenuItem is null");
    YUCHEN_ASSERT_MSG(submenu != nullptr, "Submenu is null");
    
    // Convert UTF-8 text to UTF-16
    std::wstring wText;
    int len = MultiByteToWideChar(CP_UTF8, 0, item->getText().c_str(), -1, nullptr, 0);
    if (len > 0)
    {
        wText.resize(len);
        MultiByteToWideChar(CP_UTF8, 0, item->getText().c_str(), -1, &wText[0], len);
    }
    
    // Get the submenu's native handle
    Win32MenuImpl* submenuImpl = static_cast<Win32MenuImpl*>(submenu->getImpl());
    HMENU hSubmenu = static_cast<HMENU>(submenuImpl->getNativeHandle());
    
    // Set flags for submenu item
    UINT flags = MF_POPUP;
    if (!item->isEnabled()) flags |= MF_GRAYED;
    
    // Append as a popup menu
    AppendMenuW(m_hMenu, flags, reinterpret_cast<UINT_PTR>(hSubmenu), wText.c_str());
    
    // Submenus don't have a command ID
    m_indexToId[index] = 0;
}

//==========================================================================================
// Menu Item Updates

void Win32MenuImpl::updateItemEnabled(size_t index, bool enabled)
{
    auto it = m_indexToId.find(index);
    if (it != m_indexToId.end() && it->second != 0)
    {
        UINT flags = enabled ? MF_ENABLED : MF_GRAYED;
        EnableMenuItem(m_hMenu, it->second, MF_BYCOMMAND | flags);
    }
}

void Win32MenuImpl::updateItemChecked(size_t index, bool checked)
{
    auto it = m_indexToId.find(index);
    if (it != m_indexToId.end() && it->second != 0)
    {
        UINT flags = checked ? MF_CHECKED : MF_UNCHECKED;
        CheckMenuItem(m_hMenu, it->second, MF_BYCOMMAND | flags);
    }
}

void Win32MenuImpl::updateItemText(size_t index, const std::string& text)
{
    auto it = m_indexToId.find(index);
    if (it != m_indexToId.end() && it->second != 0)
    {
        // Convert UTF-8 to UTF-16
        std::wstring wText;
        int len = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
        if (len > 0)
        {
            wText.resize(len);
            MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &wText[0], len);
        }
        
        // Update menu item info
        MENUITEMINFOW mii = {};
        mii.cbSize = sizeof(MENUITEMINFOW);
        mii.fMask = MIIM_STRING;
        mii.dwTypeData = &wText[0];
        
        SetMenuItemInfoW(m_hMenu, it->second, FALSE, &mii);
    }
}

void Win32MenuImpl::clearNativeMenu()
{
    if (m_hMenu)
    {
        // Remove all menu items
        int count = GetMenuItemCount(m_hMenu);
        for (int i = count - 1; i >= 0; --i)
        {
            RemoveMenu(m_hMenu, i, MF_BYPOSITION);
        }
        
        // Unregister from global maps
        for (const auto& pair : m_indexToId)
        {
            s_menuMap.erase(pair.second);
            s_itemMap.erase(pair.second);
        }
        
        m_indexToId.clear();
        m_idToIndex.clear();
    }
}

//==========================================================================================
// Menu Display

void Win32MenuImpl::popupNativeMenu(float screenX, float screenY)
{
    YUCHEN_ASSERT_MSG(m_hMenu != nullptr, "Menu not created");
    
    // Get the active window for menu ownership
    HWND hwnd = GetActiveWindow();
    if (!hwnd) hwnd = GetForegroundWindow();
    
    // Display the popup menu at the specified screen coordinates
    // TPM_LEFTALIGN: Menu extends to the right
    // TPM_TOPALIGN: Menu extends downward
    TrackPopupMenu(m_hMenu, TPM_LEFTALIGN | TPM_TOPALIGN,
                   static_cast<int>(screenX), static_cast<int>(screenY),
                   0, hwnd, nullptr);
}

//==========================================================================================
// Command Handling

void Win32MenuImpl::handleMenuCommand(UINT menuId)
{
    // Look up the menu and item from global maps
    auto menuIt = s_menuMap.find(menuId);
    auto itemIt = s_itemMap.find(menuId);
    
    if (menuIt == s_menuMap.end() || itemIt == s_itemMap.end()) return;
    
    Win32MenuImpl* menuImpl = menuIt->second;
    MenuItem* item = itemIt->second;
    
    if (!item || !menuImpl->m_ownerMenu) return;
    
    // Handle radio button groups
    if (item->getType() == MenuItemType::Radio)
    {
        int radioGroup = item->getRadioGroup();
        const auto& items = menuImpl->m_ownerMenu->getItems();
        
        // Uncheck all other radio items in the same group
        for (size_t i = 0; i < items.size(); ++i)
        {
            MenuItem* otherItem = items[i].get();
            if (otherItem->getType() == MenuItemType::Radio &&
                otherItem->getRadioGroup() == radioGroup)
            {
                otherItem->setChecked(otherItem == item);
            }
        }
    }
    // Handle checkable items
    else if (item->getType() == MenuItemType::Checkable)
    {
        item->setChecked(!item->isChecked());
    }
    
    // Invoke the menu item's callback
    item->triggerCallback();
}

//==========================================================================================
// Factory

MenuImpl* MenuImplFactory::create()
{
    return new Win32MenuImpl();
}

}
