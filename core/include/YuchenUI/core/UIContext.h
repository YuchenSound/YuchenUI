/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Core module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

#pragma once

#include "YuchenUI/core/Types.h"
#include "YuchenUI/events/Event.h"
#include <memory>

namespace YuchenUI {

class IUIContent;
class IGraphicsBackend;
class FocusManager;
class Widget;
class ITextInputHandler;
class ICoordinateMapper;
class RenderList;
class IFontProvider;
class IThemeProvider;
class UIStyle;

//==========================================================================================
/**
    Central UI management context.
    
    UIContext manages the UI subsystem for a window or UI container. It provides
    dependency injection support for both font and theme services.
    
    **Dependency Injection Pattern:**
    UIContext accepts optional IFontProvider and IThemeProvider interfaces during
    construction or via setter methods. This allows the Desktop layer to inject
    concrete implementations (FontManager, ThemeManager) into the Core layer
    without creating tight coupling.
    
    **Usage Patterns:**
    
    New Usage (Recommended - Full Dependency Injection):
        IFontProvider* fontProvider = ...;      // from Application
        IThemeProvider* themeProvider = ...;    // from Application
        UIContext context(fontProvider, themeProvider);
    
    Alternative (Setter-based Injection):
        UIContext context();
        context.setFontProvider(&myFontProvider);
        context.setThemeProvider(&myThemeProvider);
    
    Legacy Usage (Deprecated - Singleton Fallback):
        UIContext context();  // Falls back to FontManager/ThemeManager singletons
    
    **Why Dependency Injection?**
    - Enables unit testing with mock providers
    - Allows multiple independent UI contexts
    - Follows SOLID principles (dependency inversion)
    - Avoids global state and singleton problems
    
    @see IFontProvider, IThemeProvider, Application
*/
class UIContext {
public:
    //======================================================================================
    // Construction
    
    /**
        Creates UI context with optional providers.
        
        If providers are not specified (nullptr), the context will fall back to using
        FontManager::getInstance() and ThemeManager::getInstance() for backward
        compatibility. However, this is deprecated - new code should always inject
        providers explicitly.
        
        @param fontProvider   Font provider interface (nullptr = use FontManager singleton)
        @param themeProvider  Theme provider interface (nullptr = use ThemeManager singleton)
    */
    UIContext(IFontProvider* fontProvider = nullptr, IThemeProvider* themeProvider = nullptr);
    
    ~UIContext();
    
    //======================================================================================
    // Content management
    
    void setContent(std::unique_ptr<IUIContent> content);
    IUIContent* getContent() const;
    
    //======================================================================================
    // Frame lifecycle
    
    void beginFrame();
    void render(RenderList& outCommandList);
    void endFrame();
    
    //======================================================================================
    // Mouse event handling
    
    bool handleMouseMove(const Vec2& position);
    bool handleMouseClick(const Vec2& position, bool pressed);
    bool handleMouseWheel(const Vec2& delta, const Vec2& position);
    
    //======================================================================================
    // Keyboard event handling
    
    bool handleKeyEvent(KeyCode key, bool pressed, const KeyModifiers& mods, bool isRepeat);
    bool handleTextInput(uint32_t codepoint);
    bool handleTextComposition(const char* text, int cursorPos, int selectionLength);
    
    //======================================================================================
    // Viewport and scaling
    
    void setViewportSize(const Vec2& size);
    Vec2 getViewportSize() const;
    void setDPIScale(float scale);
    float getDPIScale() const;
    
    //======================================================================================
    // Focus management
    
    FocusManager& getFocusManager();
    const FocusManager& getFocusManager() const;
    
    //======================================================================================
    // Mouse capture
    
    void captureMouse(Widget* component);
    void releaseMouse();
    Widget* getCapturedComponent() const;
    
    //======================================================================================
    // IME support
    
    Rect getInputMethodCursorRect() const;
    
    //======================================================================================
    // Component management
    
    void addComponent(Widget* component);
    void removeComponent(Widget* component);
    
    //======================================================================================
    // Text input control
    
    void setTextInputHandler(ITextInputHandler* handler);
    void requestTextInput(bool enable);
    void setIMEEnabled(bool enabled);
    
    //======================================================================================
    // Coordinate mapping
    
    void setCoordinateMapper(ICoordinateMapper* mapper);
    Vec2 mapToScreen(const Vec2& windowPos) const;
    
    //======================================================================================
    // Font Provider Access
    
    /**
        Returns the font provider for this context.
        
        If no provider was injected during construction or via setFontProvider(),
        falls back to FontManager::getInstance() for backward compatibility.
        
        @returns Font provider interface (never null)
    */
    IFontProvider* getFontProvider() const;
    
    /**
        Sets the font provider for this context.
        
        Can be called after construction to inject or change the font provider.
        This is the recommended approach for setter-based dependency injection.
        
        @param provider  Font provider interface (must not be null)
    */
    void setFontProvider(IFontProvider* provider);
    
    //======================================================================================
    // Theme Provider Access
    
    /**
        Returns the theme provider for this context.
        
        If no provider was injected during construction or via setThemeProvider(),
        falls back to ThemeManager::getInstance() for backward compatibility.
        
        @returns Theme provider interface (never null)
    */
    IThemeProvider* getThemeProvider() const;
    
    /**
        Sets the theme provider for this context.
        
        Can be called after construction to inject or change the theme provider.
        This is the recommended approach for setter-based dependency injection.
        
        @param provider  Theme provider interface (must not be null)
    */
    void setThemeProvider(IThemeProvider* provider);
    
    /**
        Returns the current UI style from the theme provider.
        
        Convenience method equivalent to getThemeProvider()->getCurrentStyle().
        Useful for widgets that need quick access to style information.
        
        @returns Current style pointer (never null)
    */
    UIStyle* getCurrentStyle() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    
    UIContext(const UIContext&) = delete;
    UIContext& operator=(const UIContext&) = delete;
};

} // namespace YuchenUI
