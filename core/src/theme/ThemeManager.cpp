#include "YuchenUI/theme/ThemeManager.h"
#include "YuchenUI/core/Assert.h"

namespace YuchenUI {

ThemeManager* ThemeManager::s_instance = nullptr;

ThemeManager& ThemeManager::getInstance() {
    if (!s_instance) {
        s_instance = new ThemeManager();
    }
    return *s_instance;
}

ThemeManager::ThemeManager() {
    m_currentStyle = std::make_unique<ProtoolsDarkStyle>();
}

ThemeManager::~ThemeManager() {
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

void ThemeManager::setStyle(std::unique_ptr<UIStyle> style) {
    YUCHEN_ASSERT(style != nullptr);
    m_currentStyle = std::move(style);
}

}
