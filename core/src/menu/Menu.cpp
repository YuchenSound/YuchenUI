/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Menu module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

//==========================================================================================
/** @file Menu.cpp
    
    Implementation notes:
    - Backend created lazily on first menu operation
    - Rebuild flag set when items added/removed, cleared after native menu built
    - Radio items in same group automatically uncheck when another is checked
    - Native menu cleared and rebuilt when structure changes
    - Menu owns all MenuItem instances via unique_ptr
    - Backend handles platform-specific menu implementation
*/

#include "YuchenUI/menu/Menu.h"
#include "YuchenUI/menu/IMenuBackend.h"
#include "YuchenUI/core/Assert.h"

namespace YuchenUI {

//==========================================================================================
// Lifecycle

Menu::Menu()
    : m_backend(nullptr)
    , m_needsRebuild(true)
{
}

Menu::~Menu() {}

//==========================================================================================
// Backend Management

void Menu::ensureBackend()
{
    if (m_backend) return;
    
    // Create platform-specific backend
    m_backend = IMenuBackend::createBackend();
    
    if (m_backend) m_backend->setOwnerMenu(this);
}

//==========================================================================================
// Item Creation

MenuItem* Menu::addItem(const std::string& text, MenuItemCallback callback)
{
    auto item = std::make_unique<MenuItem>();
    item->setText(text);
    item->setType(MenuItemType::Normal);
    item->setCallback(callback);
    
    MenuItem* ptr = item.get();
    addMenuItem(std::move(item));
    return ptr;
}

MenuItem* Menu::addItem(const std::string& text, const std::string& shortcut, MenuItemCallback callback)
{
    auto item = std::make_unique<MenuItem>();
    item->setText(text);
    item->setShortcut(shortcut);
    item->setType(MenuItemType::Normal);
    item->setCallback(callback);
    
    MenuItem* ptr = item.get();
    addMenuItem(std::move(item));
    return ptr;
}

MenuItem* Menu::addSeparator()
{
    auto item = std::make_unique<MenuItem>();
    item->setType(MenuItemType::Separator);
    
    MenuItem* ptr = item.get();
    addMenuItem(std::move(item));
    return ptr;
}

MenuItem* Menu::addSubmenu(const std::string& text, Menu* submenu)
{
    YUCHEN_ASSERT_MSG(submenu != nullptr, "Submenu cannot be null");
    
    auto item = std::make_unique<MenuItem>();
    item->setText(text);
    item->setType(MenuItemType::Submenu);
    item->setSubmenu(submenu);
    
    MenuItem* ptr = item.get();
    addMenuItem(std::move(item));
    return ptr;
}

MenuItem* Menu::addCheckableItem(const std::string& text, MenuItemCallback callback)
{
    auto item = std::make_unique<MenuItem>();
    item->setText(text);
    item->setType(MenuItemType::Checkable);
    item->setCallback(callback);
    item->setChecked(false);
    
    MenuItem* ptr = item.get();
    addMenuItem(std::move(item));
    return ptr;
}

MenuItem* Menu::addCheckableItem(const std::string& text, const std::string& shortcut, MenuItemCallback callback)
{
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

MenuItem* Menu::addRadioItem(const std::string& text, int radioGroup, MenuItemCallback callback)
{
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

MenuItem* Menu::addRadioItem(const std::string& text, const std::string& shortcut, int radioGroup, MenuItemCallback callback)
{
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

//==========================================================================================
// Item Management

void Menu::clear()
{
    m_items.clear();
    
    if (m_backend) m_backend->clearNativeMenu();
    
    m_needsRebuild = true;
}

MenuItem* Menu::getItem(size_t index)
{
    YUCHEN_ASSERT_MSG(index < m_items.size(), "Index out of range");
    return m_items[index].get();
}

const MenuItem* Menu::getItem(size_t index) const
{
    YUCHEN_ASSERT_MSG(index < m_items.size(), "Index out of range");
    return m_items[index].get();
}

//==========================================================================================
// Display

void Menu::popup(float screenX, float screenY)
{
    ensureBackend();
    
    // Rebuild native menu if structure changed
    if (m_needsRebuild) build();

    YUCHEN_ASSERT_MSG(m_backend != nullptr, "Menu backend not available");
    m_backend->popupNativeMenu(screenX, screenY);
}

void Menu::popup(const Vec2& screenPosition)
{
    popup(screenPosition.x, screenPosition.y);
}

void* Menu::getNativeHandle() const
{
    return m_backend ? m_backend->getNativeHandle() : nullptr;
}

//==========================================================================================
// Native Menu Construction

void Menu::build()
{
    ensureBackend();
    
    if (!m_backend) return;

    // Clear existing native menu
    m_backend->clearNativeMenu();

    // Build native menu from items
    for (size_t i = 0; i < m_items.size(); ++i)
    {
        MenuItem* item = m_items[i].get();

        if (item->isSeparator())
        {
            m_backend->addNativeSeparator(i);
        }
        else if (item->hasSubmenu())
        {
            m_backend->addNativeSubmenu(item, item->getSubmenu(), i);
        }
        else
        {
            m_backend->addNativeItem(item, i);
        }
    }

    m_needsRebuild = false;
}

void Menu::rebuild()
{
    m_needsRebuild = true;
    build();
}

//==========================================================================================
// Internal

void Menu::addMenuItem(std::unique_ptr<MenuItem> item)
{
    YUCHEN_ASSERT(item);
    YUCHEN_ASSERT(item->isValid());
    
    m_items.push_back(std::move(item));
    m_needsRebuild = true;
}

void Menu::updateRadioGroup(int groupId, MenuItem* checkedItem)
{
    // Uncheck all other radio items in the same group
    for (auto& item : m_items)
    {
        if (item->getType() == MenuItemType::Radio &&
            item->getRadioGroup() == groupId &&
            item.get() != checkedItem)
        {
            item->setChecked(false);
        }
    }
}

} // namespace YuchenUI
