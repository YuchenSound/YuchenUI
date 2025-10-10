#include "YuchenUI/menu/Menu.h"
#include "YuchenUI/menu/IMenuBackend.h"
#include "YuchenUI/core/Assert.h"

namespace YuchenUI {

Menu::Menu()
    : m_backend(nullptr)
    , m_needsRebuild(true)
{
}

Menu::~Menu() {
}

void Menu::ensureBackend() {
    if (m_backend) {
        return;
    }
    
    m_backend = IMenuBackend::createBackend();
    
    if (m_backend) {
        m_backend->setOwnerMenu(this);
    }
}

MenuItem* Menu::addItem(const std::string& text, MenuItemCallback callback) {
    auto item = std::make_unique<MenuItem>();
    item->setText(text);
    item->setType(MenuItemType::Normal);
    item->setCallback(callback);
    
    MenuItem* ptr = item.get();
    addMenuItem(std::move(item));
    return ptr;
}

MenuItem* Menu::addItem(const std::string& text, const std::string& shortcut, MenuItemCallback callback) {
    auto item = std::make_unique<MenuItem>();
    item->setText(text);
    item->setShortcut(shortcut);
    item->setType(MenuItemType::Normal);
    item->setCallback(callback);
    
    MenuItem* ptr = item.get();
    addMenuItem(std::move(item));
    return ptr;
}

MenuItem* Menu::addSeparator() {
    auto item = std::make_unique<MenuItem>();
    item->setType(MenuItemType::Separator);
    
    MenuItem* ptr = item.get();
    addMenuItem(std::move(item));
    return ptr;
}

MenuItem* Menu::addSubmenu(const std::string& text, Menu* submenu) {
    YUCHEN_ASSERT_MSG(submenu != nullptr, "Submenu cannot be null");
    
    auto item = std::make_unique<MenuItem>();
    item->setText(text);
    item->setType(MenuItemType::Submenu);
    item->setSubmenu(submenu);
    
    MenuItem* ptr = item.get();
    addMenuItem(std::move(item));
    return ptr;
}

MenuItem* Menu::addCheckableItem(const std::string& text, MenuItemCallback callback) {
    auto item = std::make_unique<MenuItem>();
    item->setText(text);
    item->setType(MenuItemType::Checkable);
    item->setCallback(callback);
    item->setChecked(false);
    
    MenuItem* ptr = item.get();
    addMenuItem(std::move(item));
    return ptr;
}

MenuItem* Menu::addCheckableItem(const std::string& text, const std::string& shortcut, MenuItemCallback callback) {
    auto item = std::make_unique<MenuItem>();
    item->setText(text);
    item->setShortcut(shortcut);
    item->setType(MenuItemType::Checkable);
    item->setCallback(callback);
    item->setChecked(false);
    
    MenuItem* ptr = item.get();
    addMenuItem(std::move(item));
    return ptr;
}

MenuItem* Menu::addRadioItem(const std::string& text, int radioGroup, MenuItemCallback callback) {
    YUCHEN_ASSERT_MSG(radioGroup >= 0, "Radio group ID must be non-negative");
    
    auto item = std::make_unique<MenuItem>();
    item->setText(text);
    item->setType(MenuItemType::Radio);
    item->setRadioGroup(radioGroup);
    item->setCallback(callback);
    item->setChecked(false);
    
    MenuItem* ptr = item.get();
    addMenuItem(std::move(item));
    return ptr;
}

MenuItem* Menu::addRadioItem(const std::string& text, const std::string& shortcut, int radioGroup, MenuItemCallback callback) {
    YUCHEN_ASSERT_MSG(radioGroup >= 0, "Radio group ID must be non-negative");
    
    auto item = std::make_unique<MenuItem>();
    item->setText(text);
    item->setShortcut(shortcut);
    item->setType(MenuItemType::Radio);
    item->setRadioGroup(radioGroup);
    item->setCallback(callback);
    item->setChecked(false);
    
    MenuItem* ptr = item.get();
    addMenuItem(std::move(item));
    return ptr;
}

void Menu::clear() {
    m_items.clear();
    
    if (m_backend) {
        m_backend->clearNativeMenu();
    }
    
    m_needsRebuild = true;
}

MenuItem* Menu::getItem(size_t index) {
    YUCHEN_ASSERT_MSG(index < m_items.size(), "Index out of range");
    return m_items[index].get();
}

const MenuItem* Menu::getItem(size_t index) const {
    YUCHEN_ASSERT_MSG(index < m_items.size(), "Index out of range");
    return m_items[index].get();
}

void Menu::popup(float screenX, float screenY) {
    ensureBackend();
    
    if (m_needsRebuild) {
        build();
    }

    YUCHEN_ASSERT_MSG(m_backend != nullptr, "Menu backend not available");
    m_backend->popupNativeMenu(screenX, screenY);
}

void Menu::popup(const Vec2& screenPosition) {
    popup(screenPosition.x, screenPosition.y);
}

void* Menu::getNativeHandle() const {
    return m_backend ? m_backend->getNativeHandle() : nullptr;
}

void Menu::build() {
    ensureBackend();
    
    if (!m_backend) {
        return;
    }

    m_backend->clearNativeMenu();

    for (size_t i = 0; i < m_items.size(); ++i) {
        MenuItem* item = m_items[i].get();

        if (item->isSeparator()) {
            m_backend->addNativeSeparator(i);
        }
        else if (item->hasSubmenu()) {
            m_backend->addNativeSubmenu(item, item->getSubmenu(), i);
        }
        else {
            m_backend->addNativeItem(item, i);
        }
    }

    m_needsRebuild = false;
}

void Menu::rebuild() {
    m_needsRebuild = true;
    build();
}

void Menu::addMenuItem(std::unique_ptr<MenuItem> item) {
    YUCHEN_ASSERT(item);
    YUCHEN_ASSERT(item->isValid());
    
    m_items.push_back(std::move(item));
    m_needsRebuild = true;
}

void Menu::updateRadioGroup(int groupId, MenuItem* checkedItem) {
    for (auto& item : m_items) {
        if (item->getType() == MenuItemType::Radio &&
            item->getRadioGroup() == groupId &&
            item.get() != checkedItem) {
            item->setChecked(false);
        }
    }
}

}
