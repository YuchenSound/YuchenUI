#include "YuchenUI/menu/Menu.h"
#include "YuchenUI/platform/MenuImpl.h"
#include "YuchenUI/core/Assert.h"

namespace YuchenUI {

Menu::Menu()
    : m_items()
    , m_impl(nullptr)
    , m_needsRebuild(true)
{
    m_impl.reset(MenuImplFactory::create());
    YUCHEN_ASSERT(m_impl);
    
    // 设置 Menu 指针
    m_impl->setOwnerMenu(this);
    
    bool success = m_impl->createNativeMenu();
    YUCHEN_ASSERT(success);
}

Menu::~Menu() {
    clear();
    
    if (m_impl) {
        m_impl->destroyNativeMenu();
        m_impl.reset();
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
    
    if (m_impl) {
        m_impl->clearNativeMenu();
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
    if (m_needsRebuild) {
        build();
    }
    
    YUCHEN_ASSERT(m_impl);
    m_impl->popupNativeMenu(screenX, screenY);
}

void Menu::popup(const Vec2& screenPosition) {
    popup(screenPosition.x, screenPosition.y);
}

void Menu::build() {
    YUCHEN_ASSERT(m_impl);
    
    m_impl->setOwnerMenu(this);
    
    m_impl->clearNativeMenu();
    
    for (size_t i = 0; i < m_items.size(); ++i) {
        const MenuItem* item = m_items[i].get();
        YUCHEN_ASSERT(item->isValid());
        
        if (item->isSeparator()) {
            m_impl->addNativeSeparator(i);
        } else if (item->hasSubmenu()) {
            Menu* submenu = item->getSubmenu();
            if (submenu->m_needsRebuild) {
                submenu->build();
            }
            m_impl->addNativeSubmenu(item, submenu, i);
        } else {
            m_impl->addNativeItem(item, i);
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
