/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Focus module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

//==========================================================================================
/** @file FocusManager.h
    
    Focus management system for keyboard navigation between UI components.
    
    Implementation notes:
    - Maintains registry of all focusable components
    - Builds focus chain sorted by TabOrder property
    - Supports Tab/Shift+Tab linear navigation
    - Supports directional navigation based on geometric position
    - Notifies components of focus in/out events
    - Lazy rebuilds focus chain when marked dirty
*/

#pragma once

#include "YuchenUI/focus/FocusPolicy.h"
#include "YuchenUI/core/Types.h"
#include <vector>
#include <algorithm>

namespace YuchenUI {

class Widget;

//==========================================================================================
/**
    Manages focus transfer and navigation between UI components.
    
    FocusManager maintains a registry of focusable components and builds a focus
    chain sorted by TabOrder. Supports Tab navigation, directional navigation, and
    automatic focus event notification. Focus chain is rebuilt lazily when marked
    dirty by component registration/unregistration or TabOrder changes.
    
    Key responsibilities:
    - Track currently focused component
    - Maintain registry of all focusable components
    - Build sorted focus chain by TabOrder
    - Handle Tab/Shift+Tab linear navigation
    - Handle directional key navigation based on component positions
    - Notify components of focus enter/exit events
    
    @see UIComponent, FocusPolicy, FocusDirection
*/
class FocusManager {
public:
    //======================================================================================
    /** Creates a focus manager with no focused component. */
    FocusManager();
    
    /** Destructor. Clears focus and releases resources. */
    ~FocusManager();
    
    //======================================================================================
    /** Sets focus to the specified component.
        
        If component already has focus, does nothing. Validates component can accept
        focus and resolves focus proxy if present. Notifies previous and new focused
        components of focus out/in events.
        
        @param component  Component to receive focus, or nullptr to clear focus
        @param reason     Reason for focus change
    */
    void setFocus(Widget* component, FocusReason reason);
    
    /** Clears focus from all components. */
    void clearFocus();
    
    /** Returns the currently focused component.
        
        @returns Pointer to focused component, or nullptr if no component has focus
    */
    Widget* getFocusedComponent() const { return m_focused; }
    
    //======================================================================================
    /** Moves focus in the specified direction.
        
        Delegates to appropriate navigation method based on direction:
        - Next/Previous: Tab navigation through focus chain
        - First/Last: Jump to chain endpoints
        - Up/Down/Left/Right: Directional navigation based on geometry
        
        @param direction  Direction to move focus
        @returns True if focus was moved to another component
    */
    bool moveFocus(FocusDirection direction);
    
    //======================================================================================
    /** Registers a component as focusable.
        
        Adds component to registry if not already present and marks focus chain
        dirty for rebuild. Component must remain valid until unregistered.
        
        @param component  Component to register
    */
    void registerComponent(Widget* component);
    
    /** Unregisters a previously registered component.
        
        Removes component from registry and clears focus if it was focused.
        Marks focus chain dirty for rebuild.
        
        @param component  Component to unregister
    */
    void unregisterComponent(Widget* component);
    
    //======================================================================================
    /** Handles Tab key press for focus navigation.
        
        @param shift  True if Shift modifier is held (navigate backwards)
        @returns True if focus was moved
    */
    bool handleTabKey(bool shift);
    
    /** Handles directional key press for spatial focus navigation.
        
        @param direction  Direction key pressed (Up/Down/Left/Right)
        @returns True if focus was moved
    */
    bool handleDirectionKey(FocusDirection direction);
    
    //======================================================================================
    /** Marks the focus chain as dirty, requiring rebuild.
        
        Called when component TabOrder changes or components are added/removed.
    */
    void markDirty() { m_dirty = true; }

private:
    //======================================================================================
    /** Rebuilds the focus chain sorted by TabOrder.
        
        Copies all registered components and stable-sorts by TabOrder. Components
        with negative TabOrder are sorted to end. Clears dirty flag.
    */
    void rebuildFocusChain();
    
    /** Moves focus to next component in focus chain.
        
        @param reason  Reason for focus change
        @returns True if focus moved to another component
    */
    bool focusNext(FocusReason reason);
    
    /** Moves focus to previous component in focus chain.
        
        @param reason  Reason for focus change
        @returns True if focus moved to another component
    */
    bool focusPrevious(FocusReason reason);
    
    /** Moves focus to first component in focus chain.
        
        @returns True if focus moved to a component
    */
    bool focusFirst();
    
    /** Moves focus to last component in focus chain.
        
        @returns True if focus moved to a component
    */
    bool focusLast();
    
    /** Moves focus in the specified direction based on geometry.
        
        Finds best candidate component in the given direction from currently
        focused component's position.
        
        @param direction  Directional key pressed
        @returns True if focus moved to another component
    */
    bool focusByDirection(FocusDirection direction);
    
    //======================================================================================
    /** Finds the best focus candidate in the specified direction.
        
        Searches all focusable components for the one closest to the given
        rectangle in the specified direction.
        
        @param fromBounds  Source rectangle (current focused component bounds)
        @param direction   Direction to search
        @returns Best candidate component, or nullptr if none found
    */
    Widget* findBestCandidate(const Rect& fromBounds, FocusDirection direction);
    
    /** Calculates distance score for directional navigation.
        
        Computes Euclidean distance from source to target center points.
        Returns -1.0 if target is in wrong direction.
        
        @param from       Source rectangle
        @param to         Target rectangle
        @param direction  Navigation direction
        @returns Distance score, or -1.0 if invalid direction
    */
    float calculateScore(const Rect& from, const Rect& to, FocusDirection direction);
    
    //======================================================================================
    Widget* m_focused;                 ///< Currently focused component
    std::vector<Widget*> m_all;        ///< All registered components
    std::vector<Widget*> m_chain;      ///< Focus chain sorted by TabOrder
    bool m_dirty;                           ///< Focus chain needs rebuild
};

} // namespace YuchenUI
