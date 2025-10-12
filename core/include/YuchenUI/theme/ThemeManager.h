/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Theme module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

#pragma once

#include "YuchenUI/theme/Theme.h"
#include "YuchenUI/theme/IThemeProvider.h"
#include <memory>

namespace YuchenUI {

class IFontProvider;

/**
    Theme manager with style management.
    
    ThemeManager implements the IThemeProvider interface and manages UI styles for the
    application. It provides both a deprecated singleton interface (for backward compatibility)
    and a modern instance-based interface (recommended).
    
    **Migration Guide:**
    Old Code (Deprecated):
        ThemeManager::getInstance().getCurrentStyle()
    
    New Code (Recommended):
        // In Application class:
        ThemeManager themeManager;
        themeManager.setStyle(...);
        
        // Inject into UIContext:
        uiContext.setThemeProvider(&themeManager);
        
        // In widgets:
        UIStyle* style = m_ownerContext->getCurrentStyle();
    
    The singleton pattern is deprecated because it:
    - Makes dependency injection difficult
    - Complicates unit testing
    - Creates hidden global state
    - Prevents multiple theme contexts in the same application
    
    @see IThemeProvider, UIContext, Application
*/
class ThemeManager : public IThemeProvider {
public:
    //======================================================================================
    // Singleton Access (DEPRECATED)
    
    /**
        Returns singleton instance.
        
        @deprecated Singleton pattern is deprecated. Use instance-based API instead:
                    Create a ThemeManager instance in your Application class and inject
                    it via UIContext::setThemeProvider(). This enables proper dependency
                    injection, improves testability, and allows multiple theme contexts.
                    
                    Migration: Replace ThemeManager::getInstance() with a ThemeManager
                    instance owned by your Application, then inject via setThemeProvider().
        
        @returns Reference to singleton instance
    */
    [[deprecated("Singleton pattern is deprecated. Use instance-based API: create ThemeManager in Application and inject via UIContext::setThemeProvider()")]]
    static ThemeManager& getInstance();
    
    //======================================================================================
    // Instance-based API (RECOMMENDED)
    
    /**
        Creates theme manager instance.
        
        The manager will have a default style until setStyle() is called.
    */
    ThemeManager();
    
    /**
        Destructor.
    */
    ~ThemeManager();
    
    //======================================================================================
    // IThemeProvider Implementation
    
    /**
        Returns the current UI style.
        
        @returns Current style pointer (never null)
    */
    UIStyle* getCurrentStyle() const override { return m_currentStyle.get(); }
    
    /**
        Sets the current UI style.
        
        Transfers ownership of the style to this manager. Any previous style will be
        destroyed.
        
        @param style  New style instance (ownership transferred, must not be null)
    */
    void setStyle(std::unique_ptr<UIStyle> style) override;
    
    /**
        Sets font provider for the current style.
        
        Call this after setStyle() or when the font provider changes. The current style
        will be notified of the font provider change.
        
        @param provider  Font provider interface (must not be null)
    */
    void setFontProvider(IFontProvider* provider) override;

private:
    static ThemeManager* s_instance;
    
    std::unique_ptr<UIStyle> m_currentStyle;
    
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;
};

} // namespace YuchenUI
