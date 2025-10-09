#pragma once

#include "YuchenUI/menu/MenuItem.h"
#include "YuchenUI/core/Types.h"
#include "YuchenUI/core/Assert.h"
#include <vector>
#include <memory>

namespace YuchenUI {

    class MenuImpl;

    // Native menu wrapper class
    class Menu {
    public:
        Menu();
        ~Menu();

        // Add menu items
        MenuItem* addItem(const std::string& text, MenuItemCallback callback = nullptr);
        MenuItem* addItem(const std::string& text, const std::string& shortcut, MenuItemCallback callback = nullptr);

        MenuItem* addSeparator();

        MenuItem* addSubmenu(const std::string& text, Menu* submenu);

        MenuItem* addCheckableItem(const std::string& text, MenuItemCallback callback = nullptr);
        MenuItem* addCheckableItem(const std::string& text, const std::string& shortcut, MenuItemCallback callback = nullptr);

        MenuItem* addRadioItem(const std::string& text, int radioGroup, MenuItemCallback callback = nullptr);
        MenuItem* addRadioItem(const std::string& text, const std::string& shortcut, int radioGroup, MenuItemCallback callback = nullptr);

        // Menu management
        void clear();
        size_t getItemCount() const { return m_items.size(); }
        MenuItem* getItem(size_t index);
        const MenuItem* getItem(size_t index) const;

        // Popup menu
        void popup(float screenX, float screenY);
        void popup(const Vec2& screenPosition);

        // Build/rebuild native menu
        void build();
        void rebuild();

        // Internal use
        MenuImpl* getImpl() const { return m_impl.get(); }
        const std::vector<std::unique_ptr<MenuItem>>& getItems() const { return m_items; }

    private:
        void addMenuItem(std::unique_ptr<MenuItem> item);
        void updateRadioGroup(int groupId, MenuItem* checkedItem);

        std::vector<std::unique_ptr<MenuItem>> m_items;
        std::unique_ptr<MenuImpl> m_impl;
        bool m_needsRebuild;

        Menu(const Menu&) = delete;
        Menu& operator=(const Menu&) = delete;
    };

}