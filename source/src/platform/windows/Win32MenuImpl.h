#pragma once

#include "YuchenUI/platform/MenuImpl.h"
#include "YuchenUI/core/Types.h"
#include <Windows.h>
#include <unordered_map>

namespace YuchenUI {

class Win32MenuImpl : public MenuImpl {
public:
    Win32MenuImpl();
    virtual ~Win32MenuImpl();
    
    bool createNativeMenu() override;
    void destroyNativeMenu() override;
    
    void addNativeItem(const MenuItem* item, size_t index) override;
    void addNativeSeparator(size_t index) override;
    void addNativeSubmenu(const MenuItem* item, Menu* submenu, size_t index) override;
    
    void updateItemEnabled(size_t index, bool enabled) override;
    void updateItemChecked(size_t index, bool checked) override;
    void updateItemText(size_t index, const std::string& text) override;
    
    void clearNativeMenu() override;
    void popupNativeMenu(float screenX, float screenY) override;
    void* getNativeHandle() const override;
    
    void setOwnerMenu(Menu* menu) override;
    
    static void handleMenuCommand(UINT menuId);

private:
    HMENU m_hMenu;
    Menu* m_ownerMenu;
    std::unordered_map<size_t, UINT> m_indexToId;
    std::unordered_map<UINT, size_t> m_idToIndex;
    UINT m_nextMenuId;
    
    static std::unordered_map<UINT, Win32MenuImpl*> s_menuMap;
    static std::unordered_map<UINT, MenuItem*> s_itemMap;
    static UINT s_globalMenuId;
};

}
