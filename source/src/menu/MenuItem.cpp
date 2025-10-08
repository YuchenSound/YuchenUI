#include "YuchenUI/menu/MenuItem.h"
#include "YuchenUI/menu/Menu.h"
#include "YuchenUI/core/Assert.h"

namespace YuchenUI {

MenuItem::MenuItem()
    : m_text()
    , m_shortcut()
    , m_enabled(true)
    , m_checked(false)
    , m_type(MenuItemType::Normal)
    , m_radioGroup(0)
    , m_callback(nullptr)
    , m_submenu(nullptr)
{
}

MenuItem::~MenuItem() {
}

void MenuItem::setText(const std::string& text) {
    m_text = text;
}

void MenuItem::setShortcut(const std::string& shortcut) {
    m_shortcut = shortcut;
}

void MenuItem::setEnabled(bool enabled) {
    m_enabled = enabled;
}

void MenuItem::setChecked(bool checked) {
    YUCHEN_ASSERT_MSG(m_type == MenuItemType::Checkable || m_type == MenuItemType::Radio,
                      "Only checkable/radio items can be checked");
    m_checked = checked;
}

void MenuItem::setType(MenuItemType type) {
    m_type = type;
}

void MenuItem::setRadioGroup(int groupId) {
    YUCHEN_ASSERT_MSG(m_type == MenuItemType::Radio, "Only radio items have groups");
    m_radioGroup = groupId;
}

void MenuItem::setCallback(MenuItemCallback callback) {
    m_callback = callback;
}

void MenuItem::setSubmenu(Menu* submenu) {
    YUCHEN_ASSERT_MSG(m_type == MenuItemType::Submenu, "Only submenu items can have submenus");
    m_submenu = submenu;
}

void MenuItem::triggerCallback() {
    if (m_callback) {
        m_callback();
    }
}

bool MenuItem::isValid() const {
    if (m_type == MenuItemType::Separator) {
        return true;
    }
    
    if (m_type == MenuItemType::Submenu) {
        return !m_text.empty() && m_submenu != nullptr;
    }
    
    if (m_text.empty()) {
        return false;
    }
    
    if (m_type == MenuItemType::Radio && m_radioGroup < 0) {
        return false;
    }
    
    return true;
}

}
