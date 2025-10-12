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

#include <memory>

namespace YuchenUI {

class UIStyle;
class IFontProvider;

/**
    Abstract interface for theme management.
    
    IThemeProvider defines the contract for theme management services. This interface
    allows Core layer components (like UIContext) to access theme functionality without
    depending on concrete implementations.
    
    This follows the same pattern as IFontProvider, enabling dependency injection and
    improving testability.
    
    @see ThemeManager, IFontProvider
*/
class IThemeProvider {
public:
    virtual ~IThemeProvider() = default;
    
    /**
        Returns the current UI style.
        
        The returned style is guaranteed to be non-null. The provider owns the style
        and manages its lifetime.
        
        @returns Current style pointer (never null)
    */
    virtual UIStyle* getCurrentStyle() const = 0;
    
    /**
        Sets the current UI style.
        
        Transfers ownership of the style to the provider. If a style already exists,
        it will be replaced.
        
        @param style  New style instance (ownership transferred, must not be null)
    */
    virtual void setStyle(std::unique_ptr<UIStyle> style) = 0;
    
    /**
        Sets the font provider for the current style.
        
        Should be called after setStyle() or when the font provider changes.
        The style may use the font provider to access font resources.
        
        @param provider  Font provider interface (must not be null)
    */
    virtual void setFontProvider(IFontProvider* provider) = 0;
};

} // namespace YuchenUI
