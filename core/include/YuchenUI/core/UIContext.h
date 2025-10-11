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
class UIComponent;
class ITextInputHandler;
class ICoordinateMapper;
class RenderList;

//==========================================================================================
/** Central UI management context.
    
    Manages UI content lifecycle, input event distribution, focus management, and
    coordinate mapping. Acts as the bridge between platform layer and UI content.
    
    Responsibilities:
    - Content lifecycle (create, update, render, destroy)
    - Event routing (mouse, keyboard, text input)
    - Focus management and navigation
    - Viewport and DPI scaling
    - Component registration
    - Input method (IME) coordination
    
    Usage:
    1. Create UIContext
    2. Set content with setContent()
    3. Each frame: beginFrame() -> render() -> endFrame()
    4. Handle events with handleMouseMove(), handleKeyEvent(), etc.
    
    @see IUIContent, FocusManager, UIComponent
*/
class UIContext {
public:
    UIContext();
    ~UIContext();
    
    //======================================================================================
    // Content management
    
    /** Sets the UI content to be displayed and managed.
        
        Destroys previous content if any, then creates the new content.
        
        @param content  New content (ownership transferred)
    */
    void setContent(std::unique_ptr<IUIContent> content);
    
    IUIContent* getContent() const;
    
    //======================================================================================
    // Frame lifecycle
    
    /** Begins a new frame - updates content and calculates delta time */
    void beginFrame();
    
    /** Renders current content into the render command list.
        
        @param outCommandList  Command list to populate
    */
    void render(RenderList& outCommandList);
    
    /** Ends the current frame */
    void endFrame();
    
    //======================================================================================
    // Mouse event handling
    
    /** Handles mouse movement.
        
        Routes to captured component or active content.
        
        @param position  Mouse position in window coordinates
        @returns True if event was handled
    */
    bool handleMouseMove(const Vec2& position);
    
    /** Handles mouse button press/release.
        
        @param position  Mouse position in window coordinates
        @param pressed   True for press, false for release
        @returns True if event was handled
    */
    bool handleMouseClick(const Vec2& position, bool pressed);
    
    /** Handles mouse wheel scrolling.
        
        @param delta     Scroll delta (positive = up/right)
        @param position  Mouse position in window coordinates
        @returns True if event was handled
    */
    bool handleMouseWheel(const Vec2& delta, const Vec2& position);
    
    //======================================================================================
    // Keyboard event handling
    
    /** Handles keyboard key press/release.
        
        @param key       Key code
        @param pressed   True for press, false for release
        @param mods      Active keyboard modifiers
        @param isRepeat  True if this is a key repeat event
        @returns True if event was handled
    */
    bool handleKeyEvent(KeyCode key, bool pressed, const KeyModifiers& mods, bool isRepeat);
    
    /** Handles text input from keyboard.
        
        @param codepoint  Unicode codepoint of entered character
        @returns True if event was handled
    */
    bool handleTextInput(uint32_t codepoint);
    
    /** Handles IME composition updates.
        
        @param text             Composition text (UTF-8)
        @param cursorPos        Cursor position within composition
        @param selectionLength  Selection length (if any)
        @returns True if event was handled
    */
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
    
    /** Captures mouse input to a specific component.
        
        All mouse events will be routed to this component until released.
        
        @param component  Component to capture mouse, or nullptr to release
    */
    void captureMouse(UIComponent* component);
    
    /** Releases mouse capture */
    void releaseMouse();
    
    UIComponent* getCapturedComponent() const;
    
    //======================================================================================
    // IME support
    
    /** Returns cursor rectangle for IME candidate window positioning.
        
        @returns Cursor rect in window coordinates, or empty if no active input
    */
    Rect getInputMethodCursorRect() const;
    
    //======================================================================================
    // Component management
    
    void addComponent(UIComponent* component);
    void removeComponent(UIComponent* component);
    
    //======================================================================================
    // Text input control
    
    void setTextInputHandler(ITextInputHandler* handler);
    void requestTextInput(bool enable);
    void setIMEEnabled(bool enabled);
    
    //======================================================================================
    // Coordinate mapping
    
    void setCoordinateMapper(ICoordinateMapper* mapper);
    Vec2 mapToScreen(const Vec2& windowPos) const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    
    UIContext(const UIContext&) = delete;
    UIContext& operator=(const UIContext&) = delete;
};

} // namespace YuchenUI

