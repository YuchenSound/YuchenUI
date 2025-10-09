#import <Cocoa/Cocoa.h>
#include "MacMenuImpl.h"
#include "YuchenUI/menu/Menu.h"
#include "YuchenUI/menu/MenuItem.h"
#include "YuchenUI/core/Assert.h"

@interface MenuItemTarget : NSObject
@property (nonatomic, assign) YuchenUI::MenuItem* menuItem;
@property (nonatomic, assign) YuchenUI::Menu* ownerMenu;
@property (nonatomic, assign) size_t itemIndex;
- (void)menuItemClicked:(id)sender;
@end

@implementation MenuItemTarget
- (void)menuItemClicked:(id)sender {
    if (self.menuItem && self.menuItem->getCallback()) {
        if (self.menuItem->getType() == YuchenUI::MenuItemType::Radio) {
            int radioGroup = self.menuItem->getRadioGroup();
            if (self.ownerMenu) {
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
        else if (self.menuItem->getType() == YuchenUI::MenuItemType::Checkable) {
            self.menuItem->setChecked(!self.menuItem->isChecked());
        }
        
        self.menuItem->triggerCallback();
    }
}
@end

namespace YuchenUI {

MacMenuImpl::MacMenuImpl()
    : m_nativeMenu(nil)
    , m_target(nil)
    , m_ownerMenu(nullptr)
{
}

MacMenuImpl::~MacMenuImpl() {
    destroyNativeMenu();
}

void MacMenuImpl::setOwnerMenu(Menu* menu) {
    m_ownerMenu = menu;
}

bool MacMenuImpl::createNativeMenu() {
    YUCHEN_ASSERT_MSG(m_nativeMenu == nil, "Menu already created");
    
    @autoreleasepool {
        m_nativeMenu = [[NSMenu alloc] init];
        [m_nativeMenu setAutoenablesItems:NO];
        
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

void MacMenuImpl::addNativeSubmenu(const MenuItem* item, Menu* submenu, size_t index) {
    YUCHEN_ASSERT_MSG(m_nativeMenu != nil, "Menu not created");
    YUCHEN_ASSERT_MSG(item != nullptr, "MenuItem is null");
    YUCHEN_ASSERT_MSG(submenu != nullptr, "Submenu is null");
    
    @autoreleasepool {
        NSString* title = [NSString stringWithUTF8String:item->getText().c_str()];
        NSMenuItem* nsItem = [[NSMenuItem alloc] initWithTitle:title
                                                        action:nil
                                                 keyEquivalent:@""];
        
        // 获取子菜单的原生NSMenu
        MacMenuImpl* submenuImpl = static_cast<MacMenuImpl*>(submenu->getImpl());
        if (submenuImpl && submenuImpl->getNativeHandle()) {
            NSMenu* nativeSubmenu = (__bridge NSMenu*)submenuImpl->getNativeHandle();
            [nsItem setSubmenu:nativeSubmenu];
        }
        
        [nsItem setEnabled:item->isEnabled()];
        
        [m_nativeMenu addItem:nsItem];
        m_itemMap[index] = nsItem;
    }
}

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

void MacMenuImpl::popupNativeMenu(float screenX, float screenY) {
    YUCHEN_ASSERT_MSG(m_nativeMenu != nil, "Menu not created");
    
    @autoreleasepool {
        NSPoint popupPoint = NSMakePoint(screenX, screenY);
        
        [m_nativeMenu popUpMenuPositioningItem:nil
                                     atLocation:popupPoint
                                         inView:nil];
    }
}

void* MacMenuImpl::getNativeHandle() const {
    return (__bridge void*)m_nativeMenu;
}

NSMenuItem* MacMenuImpl::createNSMenuItem(const MenuItem* item, size_t index) {
    @autoreleasepool {
        NSString* title = [NSString stringWithUTF8String:item->getText().c_str()];
        NSString* keyEquivalent = @"";
        
        NSMenuItem* nsItem = [[NSMenuItem alloc] initWithTitle:title
                                                        action:@selector(menuItemClicked:)
                                                 keyEquivalent:keyEquivalent];
        
        MenuItemTarget* target = [[MenuItemTarget alloc] init];
        target.menuItem = const_cast<MenuItem*>(item);
        target.ownerMenu = m_ownerMenu;
        target.itemIndex = index;
        [nsItem setTarget:target];
        
        m_targetMap[index] = target;
        
        if (!item->getShortcut().empty()) {
            NSString* shortcut = [NSString stringWithUTF8String:item->getShortcut().c_str()];
            NSString* fullTitle = [NSString stringWithFormat:@"%@\t%@", title, shortcut];
            [nsItem setTitle:fullTitle];
        }
        
        [nsItem setEnabled:item->isEnabled()];
        
        if (item->getType() == MenuItemType::Checkable ||
            item->getType() == MenuItemType::Radio) {
            [nsItem setState:item->isChecked() ? NSControlStateValueOn : NSControlStateValueOff];
        }
        
        return nsItem;
    }
}

void MacMenuImpl::updateRadioGroupState(size_t index) {
    // Radio group 状态在回调中处理
}

MenuImpl* MenuImplFactory::create() {
    return new MacMenuImpl();
}

}
