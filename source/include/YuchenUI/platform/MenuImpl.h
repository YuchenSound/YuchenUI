#pragma once

#include "YuchenUI/core/Types.h"
#include <string>

namespace YuchenUI {

    class Menu;
    class MenuItem;

    // Platform menu implementation abstract interface
    class MenuImpl {
    public:
        virtual ~MenuImpl() = default;

        // Create/destroy native menu
        virtual bool createNativeMenu() = 0;
        virtual void destroyNativeMenu() = 0;

        // Add menu items to native menu
        virtual void addNativeItem(const MenuItem* item, size_t index) = 0;
        virtual void addNativeSeparator(size_t index) = 0;
        virtual void addNativeSubmenu(const MenuItem* item, Menu* submenu, size_t index) = 0;

        // Update menu item state
        virtual void updateItemEnabled(size_t index, bool enabled) = 0;
        virtual void updateItemChecked(size_t index, bool checked) = 0;
        virtual void updateItemText(size_t index, const std::string& text) = 0;

        // Clear menu
        virtual void clearNativeMenu() = 0;

        // Popup menu
        virtual void popupNativeMenu(float screenX, float screenY) = 0;

        // Get native handle
        virtual void* getNativeHandle() const = 0;

        // Set owner menu
        virtual void setOwnerMenu(Menu* menu) = 0;
    };

    // Platform menu factory
    class MenuImplFactory {
    public:
        static MenuImpl* create();
    };

}