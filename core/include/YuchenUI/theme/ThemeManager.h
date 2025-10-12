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
    application. It provides instance-based API for managing themes.
    
    The ThemeManager automatically maintains the FontProvider reference and injects it
    into any newly set UIStyle. This ensures that styles always have access to fonts
    even after theme switching.
    
    Usage:
        // In Application class:
        ThemeManager themeManager;
        themeManager.setStyle(std::make_unique<ProtoolsDarkStyle>());
        themeManager.setFontProvider(&fontManager);
        
        // Inject into UIContext:
        uiContext.setThemeProvider(&themeManager);
        
        // Switch themes (FontProvider automatically injected):
        themeManager.setStyle(std::make_unique<ProtoolsClassicStyle>());
        
        // In widgets:
        UIStyle* style = m_ownerContext->getCurrentStyle();
    
    @see IThemeProvider, UIContext, Application
*/
class ThemeManager : public IThemeProvider {
public:
    //======================================================================================
    // Instance-based API
    
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
        destroyed. If a FontProvider was previously set via setFontProvider(), it will
        be automatically injected into the new style.
        
        @param style  New style instance (ownership transferred, must not be null)
    */
    void setStyle(std::unique_ptr<UIStyle> style) override;
    
    /**
        Sets font provider for the current and future styles.
        
        The FontProvider reference is saved and will be automatically injected into
        any style set via setStyle() in the future. This ensures themes can be switched
        without losing font access.
        
        Call this after setStyle() or when the font provider changes. The current style
        will be notified of the font provider change.
        
        @param provider  Font provider interface (must not be null)
    */
    void setFontProvider(IFontProvider* provider) override;

private:
    std::unique_ptr<UIStyle> m_currentStyle;
    IFontProvider* m_fontProvider;  // Saved reference for auto-injection
    
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;
};

} // namespace YuchenUI
