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

class MacMenuImpl : public MenuImpl {
public:
    MacMenuImpl();
    virtual ~MacMenuImpl();
    
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
    
    // 新增
    void setOwnerMenu(Menu* menu) override;

private:
    NSMenu* m_nativeMenu;
    MenuItemTarget* m_target;
    std::unordered_map<size_t, NSMenuItem*> m_itemMap;
    std::unordered_map<size_t, MenuItemTarget*> m_targetMap;  // 新增：保存所有 target
    Menu* m_ownerMenu;  // 新增：保存 Menu 指针
    
    NSMenuItem* createNSMenuItem(const MenuItem* item, size_t index);
    void updateRadioGroupState(size_t index);
};

}
