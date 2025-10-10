#pragma once

#include <string>
#include <memory>
#include <functional>

namespace YuchenUI {

class MenuItem;
class Menu;
class IMenuBackend;

using MenuBackendFactory = std::function<std::unique_ptr<IMenuBackend>()>;

class IMenuBackend {
public:
    virtual ~IMenuBackend() = default;
    
    virtual bool createNativeMenu() = 0;
    virtual void destroyNativeMenu() = 0;
    virtual void addNativeItem(const MenuItem* item, size_t index) = 0;
    virtual void addNativeSeparator(size_t index) = 0;
    virtual void addNativeSubmenu(const MenuItem* item, Menu* submenu, size_t index) = 0;
    virtual void updateItemEnabled(size_t index, bool enabled) = 0;
    virtual void updateItemChecked(size_t index, bool checked) = 0;
    virtual void updateItemText(size_t index, const std::string& text) = 0;
    virtual void clearNativeMenu() = 0;
    virtual void popupNativeMenu(float screenX, float screenY) = 0;
    virtual void* getNativeHandle() const = 0;
    virtual void setOwnerMenu(Menu* menu) = 0;
    
    static void registerFactory(MenuBackendFactory factory);
    static std::unique_ptr<IMenuBackend> createBackend();
};

}
