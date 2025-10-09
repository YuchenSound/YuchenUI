#include "Win32MenuImpl.h"
#include "YuchenUI/menu/Menu.h"
#include "YuchenUI/menu/MenuItem.h"
#include "YuchenUI/core/Assert.h"
#include <string>
#include <algorithm>

namespace YuchenUI {

// MARK: - Static Members

std::unordered_map<UINT, Win32MenuImpl*> Win32MenuImpl::s_menuMap;
std::unordered_map<UINT, MenuItem*> Win32MenuImpl::s_itemMap;
UINT Win32MenuImpl::s_globalMenuId = 1000;

// MARK: - Constructor and Destructor

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

// MARK: - Menu Lifecycle

void Win32MenuImpl::setOwnerMenu(Menu* menu)
{
    m_ownerMenu = menu;
}

bool Win32MenuImpl::createNativeMenu()
{
    YUCHEN_ASSERT_MSG(m_hMenu == nullptr, "Menu already created");
    
    m_hMenu = CreatePopupMenu();
    YUCHEN_ASSERT_MSG(m_hMenu != nullptr, "Failed to create popup menu");
    
    return m_hMenu != nullptr;
}

void Win32MenuImpl::destroyNativeMenu()
{
    if (m_hMenu)
    {
        for (const auto& pair : m_indexToId)
        {
            UINT menuId = pair.second;
            s_menuMap.erase(menuId);
            s_itemMap.erase(menuId);
        }
        
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

// MARK: - Menu Item Management

void Win32MenuImpl::addNativeItem(const MenuItem* item, size_t index)
{
    YUCHEN_ASSERT_MSG(m_hMenu != nullptr, "Menu not created");
    YUCHEN_ASSERT_MSG(item != nullptr, "MenuItem is null");
    
    UINT menuId = m_nextMenuId++;
    
    std::wstring wText;
    int len = MultiByteToWideChar(CP_UTF8, 0, item->getText().c_str(), -1, nullptr, 0);
    if (len > 0)
    {
        wText.resize(len);
        MultiByteToWideChar(CP_UTF8, 0, item->getText().c_str(), -1, &wText[0], len);
    }
    
    UINT flags = MF_STRING;
    if (!item->isEnabled()) flags |= MF_GRAYED;
    if (item->isChecked()) flags |= MF_CHECKED;
    
    AppendMenuW(m_hMenu, flags, menuId, wText.c_str());
    
    m_indexToId[index] = menuId;
    m_idToIndex[menuId] = index;
    s_menuMap[menuId] = this;
    s_itemMap[menuId] = const_cast<MenuItem*>(item);
}

void Win32MenuImpl::addNativeSeparator(size_t index)
{
    YUCHEN_ASSERT_MSG(m_hMenu != nullptr, "Menu not created");
    
    AppendMenuW(m_hMenu, MF_SEPARATOR, 0, nullptr);
    
    m_indexToId[index] = 0;
}

void Win32MenuImpl::addNativeSubmenu(const MenuItem* item, Menu* submenu, size_t index)
{
    YUCHEN_ASSERT_MSG(m_hMenu != nullptr, "Menu not created");
    YUCHEN_ASSERT_MSG(item != nullptr, "MenuItem is null");
    YUCHEN_ASSERT_MSG(submenu != nullptr, "Submenu is null");
    
    std::wstring wText;
    int len = MultiByteToWideChar(CP_UTF8, 0, item->getText().c_str(), -1, nullptr, 0);
    if (len > 0)
    {
        wText.resize(len);
        MultiByteToWideChar(CP_UTF8, 0, item->getText().c_str(), -1, &wText[0], len);
    }
    
    Win32MenuImpl* submenuImpl = static_cast<Win32MenuImpl*>(submenu->getImpl());
    HMENU hSubmenu = static_cast<HMENU>(submenuImpl->getNativeHandle());
    
    UINT flags = MF_POPUP;
    if (!item->isEnabled()) flags |= MF_GRAYED;
    
    AppendMenuW(m_hMenu, flags, reinterpret_cast<UINT_PTR>(hSubmenu), wText.c_str());
    
    m_indexToId[index] = 0;
}

// MARK: - Menu Item Updates

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
        std::wstring wText;
        int len = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
        if (len > 0)
        {
            wText.resize(len);
            MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &wText[0], len);
        }
        
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
        int count = GetMenuItemCount(m_hMenu);
        for (int i = count - 1; i >= 0; --i)
        {
            RemoveMenu(m_hMenu, i, MF_BYPOSITION);
        }
        
        for (const auto& pair : m_indexToId)
        {
            s_menuMap.erase(pair.second);
            s_itemMap.erase(pair.second);
        }
        
        m_indexToId.clear();
        m_idToIndex.clear();
    }
}

// MARK: - Menu Display

void Win32MenuImpl::popupNativeMenu(float screenX, float screenY)
{
    YUCHEN_ASSERT_MSG(m_hMenu != nullptr, "Menu not created");
    
    HWND hwnd = GetActiveWindow();
    if (!hwnd) hwnd = GetForegroundWindow();
    
    TrackPopupMenu(m_hMenu, TPM_LEFTALIGN | TPM_TOPALIGN,
                   static_cast<int>(screenX), static_cast<int>(screenY),
                   0, hwnd, nullptr);
}

// MARK: - Command Handling

void Win32MenuImpl::handleMenuCommand(UINT menuId)
{
    auto menuIt = s_menuMap.find(menuId);
    auto itemIt = s_itemMap.find(menuId);
    
    if (menuIt == s_menuMap.end() || itemIt == s_itemMap.end()) return;
    
    Win32MenuImpl* menuImpl = menuIt->second;
    MenuItem* item = itemIt->second;
    
    if (!item || !menuImpl->m_ownerMenu) return;
    
    if (item->getType() == MenuItemType::Radio)
    {
        int radioGroup = item->getRadioGroup();
        const auto& items = menuImpl->m_ownerMenu->getItems();
        
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
    else if (item->getType() == MenuItemType::Checkable)
    {
        item->setChecked(!item->isChecked());
    }
    
    item->triggerCallback();
}

// MARK: - Factory

MenuImpl* MenuImplFactory::create()
{
    return new Win32MenuImpl();
}

}
