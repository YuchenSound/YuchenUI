/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Platform module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

//==========================================================================================
/** @file MacMenuImpl.mm
    
    Implementation notes:
    - Each menu item gets its own MenuItemTarget for action handling
    - Radio groups are synchronized by finding all items with same radio group ID
    - Checkable items toggle their state on click
    - Shortcuts are displayed as text suffix (not functional keybindings)
    - Submenus are attached using NSMenuItem's setSubmenu: method
    - Factory registration moved to WindowManager for explicit initialization
*/

#import <Cocoa/Cocoa.h>
#include "MacMenuImpl.h"
#include "YuchenUI/menu/Menu.h"
#include "YuchenUI/menu/MenuItem.h"
#include "YuchenUI/core/Assert.h"

//==========================================================================================
/**
    Objective-C target object for handling menu item actions.
    
    Each menu item gets its own MenuItemTarget instance which invokes
    the appropriate callback when the item is clicked.
*/
@interface MenuItemTarget : NSObject

/** The YuchenUI MenuItem this target represents. */
@property (nonatomic, assign) YuchenUI::MenuItem* menuItem;

/** The owner Menu (for radio group synchronization). */
@property (nonatomic, assign) YuchenUI::Menu* ownerMenu;

/** The index of this item in the menu. */
@property (nonatomic, assign) size_t itemIndex;

/** Action method called when menu item is clicked.
    
    @param sender  The NSMenuItem that sent the action
*/
- (void)menuItemClicked:(id)sender;

@end

@implementation MenuItemTarget

- (void)menuItemClicked:(id)sender {
    if (self.menuItem && self.menuItem->getCallback()) {
        // Handle radio button groups
        if (self.menuItem->getType() == YuchenUI::MenuItemType::Radio) {
            int radioGroup = self.menuItem->getRadioGroup();
            if (self.ownerMenu) {
                // Uncheck all other radio items in the same group
                const auto& items = self.ownerMenu->getItems();
                for (size_t i = 0; i < items.size(); ++i) {
                    YuchenUI::MenuItem* item = items[i].get();
                    if (item->getType() == YuchenUI::MenuItemType::Radio &&
                        item->getRadioGroup() == radioGroup) {
                        item->setChecked(item == self.menuItem);
                    }
                }
            }
        }
        // Handle checkable items
        else if (self.menuItem->getType() == YuchenUI::MenuItemType::Checkable) {
            self.menuItem->setChecked(!self.menuItem->isChecked());
        }
        
        // Invoke the user callback
        self.menuItem->triggerCallback();
    }
}

@end

namespace YuchenUI {

//==========================================================================================
// Construction and Destruction

MacMenuImpl::MacMenuImpl()
    : m_nativeMenu(nil)
    , m_target(nil)
    , m_ownerMenu(nullptr)
{
}

MacMenuImpl::~MacMenuImpl() {
    destroyNativeMenu();
}

//==========================================================================================
// Menu Lifecycle

void MacMenuImpl::setOwnerMenu(Menu* menu) {
    m_ownerMenu = menu;
}

bool MacMenuImpl::createNativeMenu() {
    YUCHEN_ASSERT_MSG(m_nativeMenu == nil, "Menu already created");
    
    @autoreleasepool {
        m_nativeMenu = [[NSMenu alloc] init];
        [m_nativeMenu setAutoenablesItems:NO];  // Manual control over enabled state
        
        m_target = [[MenuItemTarget alloc] init];
        
        return m_nativeMenu != nil && m_target != nil;
    }
}

void MacMenuImpl::destroyNativeMenu() {
    @autoreleasepool {
        if (m_nativeMenu) {
            m_nativeMenu = nil;
        }
        
        if (m_target) {
            m_target = nil;
        }
        
        m_itemMap.clear();
        m_targetMap.clear();
    }
}

//==========================================================================================
// Item Management

void MacMenuImpl::addNativeItem(const MenuItem* item, size_t index) {
    YUCHEN_ASSERT_MSG(m_nativeMenu != nil, "Menu not created");
    YUCHEN_ASSERT_MSG(item != nullptr, "MenuItem is null");
    
    @autoreleasepool {
        NSMenuItem* nsItem = createNSMenuItem(item, index);
        
        if (nsItem) {
            [m_nativeMenu addItem:nsItem];
            m_itemMap[index] = nsItem;
        }
    }
}

void MacMenuImpl::addNativeSeparator(size_t index) {
    YUCHEN_ASSERT_MSG(m_nativeMenu != nil, "Menu not created");
    
    @autoreleasepool {
        NSMenuItem* separator = [NSMenuItem separatorItem];
        [m_nativeMenu addItem:separator];
        m_itemMap[index] = separator;
    }
}

void MacMenuImpl::addNativeSubmenu(const MenuItem* item, Menu* submenu, size_t index)
{
    YUCHEN_ASSERT_MSG(m_nativeMenu != nil, "Menu not created");
    YUCHEN_ASSERT_MSG(item != nullptr, "MenuItem is null");
    YUCHEN_ASSERT_MSG(submenu != nullptr, "Submenu is null");
    
    @autoreleasepool
    {
        NSString* title = [NSString stringWithUTF8String:item->getText().c_str()];
        NSMenuItem* nsItem = [[NSMenuItem alloc] initWithTitle:title
                                                        action:nil
                                                 keyEquivalent:@""];
        
        // Get the native NSMenu handle from the submenu
        void* nativeHandle = submenu->getNativeHandle();
        if (nativeHandle)
        {
            NSMenu* nativeSubmenu = (__bridge NSMenu*)nativeHandle;
            [nsItem setSubmenu:nativeSubmenu];
        }
        
        [nsItem setEnabled:item->isEnabled()];
        
        [m_nativeMenu addItem:nsItem];
        m_itemMap[index] = nsItem;
    }
}

//==========================================================================================
// Item Updates

void MacMenuImpl::updateItemEnabled(size_t index, bool enabled) {
    @autoreleasepool {
        auto it = m_itemMap.find(index);
        if (it != m_itemMap.end()) {
            NSMenuItem* nsItem = it->second;
            [nsItem setEnabled:enabled];
        }
    }
}

void MacMenuImpl::updateItemChecked(size_t index, bool checked) {
    @autoreleasepool {
        auto it = m_itemMap.find(index);
        if (it != m_itemMap.end()) {
            NSMenuItem* nsItem = it->second;
            [nsItem setState:checked ? NSControlStateValueOn : NSControlStateValueOff];
        }
    }
}

void MacMenuImpl::updateItemText(size_t index, const std::string& text) {
    @autoreleasepool {
        auto it = m_itemMap.find(index);
        if (it != m_itemMap.end()) {
            NSMenuItem* nsItem = it->second;
            NSString* title = [NSString stringWithUTF8String:text.c_str()];
            [nsItem setTitle:title];
        }
    }
}

void MacMenuImpl::clearNativeMenu() {
    @autoreleasepool {
        if (m_nativeMenu) {
            [m_nativeMenu removeAllItems];
            m_itemMap.clear();
            m_targetMap.clear();
        }
    }
}

//==========================================================================================
// Menu Display

void MacMenuImpl::popupNativeMenu(float screenX, float screenY) {
    YUCHEN_ASSERT_MSG(m_nativeMenu != nil, "Menu not created");
    
    @autoreleasepool {
        NSPoint popupPoint = NSMakePoint(screenX, screenY);
        
        // Display menu as popup at specified coordinates
        [m_nativeMenu popUpMenuPositioningItem:nil
                                     atLocation:popupPoint
                                         inView:nil];
    }
}

void* MacMenuImpl::getNativeHandle() const {
    return (__bridge void*)m_nativeMenu;
}

//==========================================================================================
// Private Methods

NSMenuItem* MacMenuImpl::createNSMenuItem(const MenuItem* item, size_t index) {
    @autoreleasepool {
        NSString* title = [NSString stringWithUTF8String:item->getText().c_str()];
        NSString* keyEquivalent = @"";
        
        // Create menu item with action selector
        NSMenuItem* nsItem = [[NSMenuItem alloc] initWithTitle:title
                                                        action:@selector(menuItemClicked:)
                                                 keyEquivalent:keyEquivalent];
        
        // Create target object for this item
        MenuItemTarget* target = [[MenuItemTarget alloc] init];
        target.menuItem = const_cast<MenuItem*>(item);
        target.ownerMenu = m_ownerMenu;
        target.itemIndex = index;
        [nsItem setTarget:target];
        
        // Store target for later cleanup
        m_targetMap[index] = target;
        
        // Add shortcut text if present (displayed, not functional)
        if (!item->getShortcut().empty()) {
            NSString* shortcut = [NSString stringWithUTF8String:item->getShortcut().c_str()];
            NSString* fullTitle = [NSString stringWithFormat:@"%@\t%@", title, shortcut];
            [nsItem setTitle:fullTitle];
        }
        
        // Set enabled state
        [nsItem setEnabled:item->isEnabled()];
        
        // Set checked state for checkable and radio items
        if (item->getType() == MenuItemType::Checkable ||
            item->getType() == MenuItemType::Radio) {
            [nsItem setState:item->isChecked() ? NSControlStateValueOn : NSControlStateValueOff];
        }
        
        return nsItem;
    }
}

void MacMenuImpl::updateRadioGroupState(size_t index) {
    // Radio group state is handled in the callback
}

} // namespace YuchenUI
