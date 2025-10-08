#pragma once

#include "YuchenUI/menu/Menu.h"
#include <memory>

namespace YuchenUI {

    // Menu manager singleton
    class MenuManager {
    public:
        static MenuManager& getInstance();

        bool initialize();
        void destroy();
        bool isInitialized() const { return m_isInitialized; }

        // Create menu
        std::unique_ptr<Menu> createMenu();

    private:
        MenuManager();
        ~MenuManager();

        static MenuManager* s_instance;

        bool m_isInitialized;

        MenuManager(const MenuManager&) = delete;
        MenuManager& operator=(const MenuManager&) = delete;
    };

}