#include "YuchenUI/menu/MenuManager.h"
#include "YuchenUI/core/Assert.h"

namespace YuchenUI {

MenuManager* MenuManager::s_instance = nullptr;

MenuManager& MenuManager::getInstance() {
    if (!s_instance) {
        s_instance = new MenuManager();
        s_instance->initialize();
    }
    return *s_instance;
}

MenuManager::MenuManager()
    : m_isInitialized(false)
{
}

MenuManager::~MenuManager() {
    destroy();
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

bool MenuManager::initialize() {
    YUCHEN_ASSERT_MSG(!m_isInitialized, "MenuManager already initialized");
    
    m_isInitialized = true;
    return true;
}

void MenuManager::destroy() {
    if (!m_isInitialized) return;
    
    m_isInitialized = false;
}

std::unique_ptr<Menu> MenuManager::createMenu() {
    YUCHEN_ASSERT_MSG(m_isInitialized, "MenuManager not initialized");
    return std::make_unique<Menu>();
}

}
