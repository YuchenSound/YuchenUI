#pragma once

#include "YuchenUI/core/Types.h"
#include <string>
#include <functional>

namespace YuchenUI {

    class Menu;

    // Menu item type enumeration
    enum class MenuItemType {
        Normal,      // Normal menu item
        Separator,   // Separator
        Submenu,     // Submenu
        Checkable,   // Checkable menu item
        Radio        // Radio menu item (mutually exclusive group)
    };

    // Menu item callback function type
    using MenuItemCallback = std::function<void()>;

    // Menu item class
    class MenuItem {
    public:
        MenuItem();
        ~MenuItem();

        // Basic properties
        void setText(const std::string& text);
        const std::string& getText() const { return m_text; }

        void setShortcut(const std::string& shortcut);
        const std::string& getShortcut() const { return m_shortcut; }

        void setEnabled(bool enabled);
        bool isEnabled() const { return m_enabled; }

        void setChecked(bool checked);
        bool isChecked() const { return m_checked; }

        void setType(MenuItemType type);
        MenuItemType getType() const { return m_type; }

        void setRadioGroup(int groupId);
        int getRadioGroup() const { return m_radioGroup; }

        // Callback
        void setCallback(MenuItemCallback callback);
        MenuItemCallback getCallback() const { return m_callback; }

        // Submenu
        void setSubmenu(Menu* submenu);
        Menu* getSubmenu() const { return m_submenu; }

        // Internal use
        void triggerCallback();
        bool isSeparator() const { return m_type == MenuItemType::Separator; }
        bool hasSubmenu() const { return m_submenu != nullptr; }

        // Validation
        bool isValid() const;

    private:
        std::string m_text;
        std::string m_shortcut;
        bool m_enabled;
        bool m_checked;
        MenuItemType m_type;
        int m_radioGroup;
        MenuItemCallback m_callback;
        Menu* m_submenu;

        MenuItem(const MenuItem&) = delete;
        MenuItem& operator=(const MenuItem&) = delete;
    };

}