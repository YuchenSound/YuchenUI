#pragma once

#include "YuchenUI/theme/Theme.h"
#include <memory>

namespace YuchenUI {

class ThemeManager {
public:
    static ThemeManager& getInstance();
    
    void setStyle(std::unique_ptr<UIStyle> style);
    UIStyle* getCurrentStyle() const { return m_currentStyle.get(); }

private:
    ThemeManager();
    ~ThemeManager();
    
    static ThemeManager* s_instance;
    
    std::unique_ptr<UIStyle> m_currentStyle;
    
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;
};

}
