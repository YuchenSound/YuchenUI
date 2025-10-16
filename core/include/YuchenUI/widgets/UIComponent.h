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
#include "YuchenUI/core/IUIContent.h"
#include "YuchenUI/events/Event.h"
#include "YuchenUI/focus/FocusPolicy.h"
#include <vector>
#include <memory>

namespace YuchenUI {

class RenderList;
class UIContext;
class IUIContent;
class Menu;
class FocusManager;

/**
    Base class for all UI components in the YuchenUI framework.
    
    UIComponent is the unified base class that provides complete functionality for both
    leaf components (buttons, checkboxes) and container components (panels, frames).
    
    This design follows the Qt philosophy where all components inherit from a single
    base class, providing:
    - Consistent API across all component types
    - Maximum flexibility (any component can contain children if needed)
    - Simplified type hierarchy
    
    Key responsibilities:
    - Rendering (addDrawCommands)
    - Event handling (mouse, keyboard, touch)
    - Focus management
    - Visibility and enabled state
    - Geometry (bounds, padding)
    - Child component management
    - Context and ownership
    
    Memory overhead: ~100-150 bytes per instance. On modern hardware (64GB+ RAM),
    this overhead is negligible even for applications with 10,000+ components.
    
    Design rationale:
    While some frameworks separate "Control" and "Container" into different hierarchies
    to save memory, modern hardware makes this optimization unnecessary. The benefits
    of a unified API far outweigh the minimal memory cost.
    
    Usage:
    @code
    // All components inherit from UIComponent
    class Button : public UIComponent { ... };
    class Panel : public UIComponent { ... };
    
    // All components can have children (though leaf components typically don't use this)
    UIComponent* root = new Panel();
    UIComponent* button = root->addChild<Button>();
    @endcode
    
    @note This class must be subclassed. Direct instantiation is not allowed.
    @see Button, CheckBox, Frame, Panel
*/
class UIComponent {
public:
    UIComponent();
    virtual ~UIComponent();
    
    //======================================================================================
    // Pure Virtual Interface - Must be implemented by subclasses
    
    /**
        Adds rendering commands to the render list.
        
        Called by the rendering system to generate draw commands for this component.
        Implementations should:
        1. Check visibility (if (!m_isVisible) return;)
        2. Generate draw commands for this component
        3. Call renderChildren() if this component has children
        4. Apply offset to transform local coordinates to parent space
        
        @param commandList  Render command list to append to
        @param offset       Offset in parent coordinate space (cumulative)
    */
    virtual void addDrawCommands(RenderList& commandList, const Vec2& offset = Vec2()) const = 0;
    
    /**
        Handles mouse movement events.
        
        Called when the mouse moves. Implementations should:
        1. Check if enabled and visible
        2. Transform position using offset
        3. Dispatch to children if this is a container
        4. Update hover state
        
        @param position  Mouse position in window coordinates
        @param offset    Offset in parent coordinate space
        @return true if event was handled, false otherwise
    */
    virtual bool handleMouseMove(const Vec2& position, const Vec2& offset = Vec2()) = 0;
    
    /**
        Handles mouse click events (both press and release).
        
        Called when a mouse button is pressed or released. Implementations should:
        1. Check if enabled and visible
        2. Transform position using offset
        3. Dispatch to children if this is a container
        4. Update pressed state and trigger callbacks
        
        @param position  Mouse position in window coordinates
        @param pressed   true for button down, false for button up
        @param offset    Offset in parent coordinate space
        @return true if event was handled, false otherwise
    */
    virtual bool handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset = Vec2()) = 0;
    
    //======================================================================================
    // Virtual Event Handlers - Optional overrides
    
    /**
        Handles mouse wheel events.
        
        Override to handle scrolling. Default implementation returns false.
        
        @param delta     Scroll delta (positive = scroll up/right)
        @param position  Mouse position in window coordinates
        @param offset    Offset in parent coordinate space
        @return true if event was handled, false otherwise
    */
    virtual bool handleMouseWheel(const Vec2& delta, const Vec2& position, const Vec2& offset = Vec2()) { return false; }
    
    /**
        Handles keyboard press/release events.
        
        Override to handle keyboard input. Default implementation returns false.
        
        @param event  Keyboard event with key code and modifiers
        @return true if event was handled, false otherwise
    */
    virtual bool handleKeyPress(const Event& event) { return false; }
    
    /**
        Handles text input events (for IME and text fields).
        
        Override to handle text entry. Default implementation returns false.
        
        @param codepoint  Unicode codepoint of the character
        @return true if event was handled, false otherwise
    */
    virtual bool handleTextInput(uint32_t codepoint) { return false; }
    
    /**
        Handles IME composition events.
        
        Override to handle Input Method Editor composition (for CJK text input).
        Default implementation returns false.
        
        @param text              Composition string
        @param cursorPos         Cursor position in composition
        @param selectionLength   Length of selection in composition
        @return true if event was handled, false otherwise
    */
    virtual bool handleComposition(const char* text, int cursorPos, int selectionLength) { return false; }
    
    /**
        Called every frame for animations and time-based updates.
        
        Override to implement animations or periodic updates. Default implementation
        calls update() on all children.
        
        @param deltaTime  Time since last frame in seconds
    */
    virtual void update(float deltaTime);
    
    //======================================================================================
    // Geometry Management
    
    /**
        Sets the bounding rectangle of this component.
        
        The bounds are in parent-local coordinates. For root components, this is
        window coordinates.
        
        @param bounds  Rectangle defining position and size
    */
    void setBounds(const Rect& bounds);
    
    /**
        Returns the bounding rectangle of this component.
        
        @return Rectangle in parent-local coordinates
    */
    const Rect& getBounds() const { return m_bounds; }
    
    /**
        Sets uniform padding on all sides.
        
        Padding creates an inner margin between the component bounds and its content area.
        This is primarily useful for container components.
        
        @param padding  Padding in pixels (must be >= 0)
    */
    void setPadding(float padding);
    
    /**
        Sets individual padding for each side.
        
        @param left    Left padding in pixels (must be >= 0)
        @param top     Top padding in pixels (must be >= 0)
        @param right   Right padding in pixels (must be >= 0)
        @param bottom  Bottom padding in pixels (must be >= 0)
    */
    void setPadding(float left, float top, float right, float bottom);
    
    float getPaddingLeft() const { return m_paddingLeft; }
    float getPaddingTop() const { return m_paddingTop; }
    float getPaddingRight() const { return m_paddingRight; }
    float getPaddingBottom() const { return m_paddingBottom; }
    
    /**
        Calculates the content rectangle (bounds minus padding).
        
        Returns the area available for child components or content, after subtracting
        padding from the bounds.
        
        @return Rectangle in local coordinates (relative to this component's origin)
    */
    Rect getContentRect() const;
    
    /**
        Transforms a local rectangle to window coordinates.
        
        Accumulates offsets from this component up through all parent components
        to calculate absolute window position.
        
        @param localRect  Rectangle in this component's local coordinates
        @return Rectangle in window coordinates
    */
    Rect mapToWindow(const Rect& localRect) const;
    
    //======================================================================================
    // Child Management
    
    /**
        Adds a child component with automatic memory management.
        
        The parent takes ownership and will delete the child in its destructor.
        This is the recommended way to add child components.
        
        Example:
            Button* button = new Button(bounds);
            groupBox->addChild(button);
            // groupBox now owns button, will delete it automatically
        
        @param child  Child component (ownership transferred to parent)
        @return The child component pointer
    */
    template<typename T>
    T* addChild(T* child) {
        static_assert(std::is_base_of<UIComponent, T>::value,
            "Child must inherit from UIComponent");
        
        if (!child) return nullptr;
        
        child->setParent(this);
        child->setOwnerContext(m_ownerContext);
        
        m_ownedChildren.push_back(child);
        
        if (m_ownerContent && child->getFocusPolicy() != FocusPolicy::NoFocus) {
            m_ownerContent->registerFocusableComponent(child);
        }
        
        return child;
    }
    
    /**
        Removes a child component.
        
        Unregisters the component from the focus system and removes it from the
        children list. The component is destroyed when its unique_ptr is released.
        
        @param child  Pointer to the child component to remove
    */
    void removeChild(UIComponent* child);
    
    /**
        Removes all child components.
        
        Unregisters all children from the focus system and clears the children list.
    */
    void clearChildren();
    
    /**
        Returns the number of child components.
        
        @return Child count
    */
    size_t getChildCount() const { return m_ownedChildren.size(); }

    /**
        Returns the list of child components.
        
        Provides read-only access to the children for iteration and inspection.
        
        @return Vector of unique pointers to child components
    */
    const std::vector<UIComponent*>& getChildren() const { return m_ownedChildren; }

    //======================================================================================
    // Visibility and Enabled State
    
    /**
        Returns whether this component is visible.
        
        Invisible components do not render and do not receive events.
        
        @return true if visible, false otherwise
    */
    virtual bool isVisible() const { return m_isVisible; }
    
    /**
        Sets the visibility of this component.
        
        When hiding a component that has focus, focus is automatically cleared.
        
        @param visible  true to show, false to hide
    */
    virtual void setVisible(bool visible);
    
    /**
        Returns whether this component is enabled.
        
        Disabled components render in a disabled state and do not respond to user input.
        
        @return true if enabled, false otherwise
    */
    virtual bool isEnabled() const { return m_isEnabled; }
    
    /**
        Sets the enabled state of this component.
        
        When disabling a component that has focus, focus is automatically cleared.
        
        @param enabled  true to enable, false to disable
    */
    virtual void setEnabled(bool enabled);
    
    //======================================================================================
    // Context and Ownership
    
    /**
        Sets the UI context that owns this component.
        
        The context provides access to:
        - Style and theme system
        - Font provider
        - Focus manager
        - Screen coordinate mapping
        
        This method recursively sets the context for all children.
        
        @param context  Pointer to the owning UI context
    */
    virtual void setOwnerContext(UIContext* context);
    
    /**
        Returns the UI context that owns this component.
        
        @return Pointer to the owning context, or nullptr if not set
    */
    UIContext* getOwnerContext() const { return m_ownerContext; }
    
    /**
        Sets the UI content container that owns this component.
        
        The content container manages the focus system registration for this component.
        This method recursively sets the content for all children.
        
        @param content  Pointer to the owning UI content
    */
    virtual void setOwnerContent(IUIContent* content);
    
    /**
        Returns the UI content container that owns this component.
        
        @return Pointer to the owning content, or nullptr if not set
    */
    IUIContent* getOwnerContent() const { return m_ownerContent; }
    
    /**
        Sets the parent component.
        
        Parent relationships are used for coordinate transformations and event bubbling.
        
        @param parent  Pointer to the parent component
    */
    void setParent(UIComponent* parent) { m_parent = parent; }
    
    /**
        Returns the parent component.
        
        @return Pointer to the parent, or nullptr if this is a root component
    */
    UIComponent* getParent() const { return m_parent; }
    
    //======================================================================================
    // Context Menu
    
    /**
        Sets the context menu for this component.
        
        The context menu is typically shown on right-click.
        
        @param menu  Pointer to the menu to show
    */
    void setContextMenu(Menu* menu) { m_contextMenu = menu; }
    
    /**
        Returns the context menu for this component.
        
        @return Pointer to the context menu, or nullptr if not set
    */
    Menu* getContextMenu() const { return m_contextMenu; }
    
    /**
        Returns whether this component has a context menu.
        
        @return true if context menu is set, false otherwise
    */
    bool hasContextMenu() const { return m_contextMenu != nullptr; }
    
    //======================================================================================
    // Focus Management
    
    /**
        Sets the focus policy for this component.
        
        The focus policy determines how the component can receive keyboard focus:
        - NoFocus: Cannot receive focus
        - TabFocus: Can receive focus via Tab key
        - ClickFocus: Can receive focus via mouse click
        - StrongFocus: Can receive focus via both Tab and click
        
        When changing to NoFocus, any existing focus is automatically cleared.
        
        @param policy  The focus policy to set
    */
    void setFocusPolicy(FocusPolicy policy);
    
    /**
        Returns the focus policy for this component.
        
        @return The current focus policy
    */
    FocusPolicy getFocusPolicy() const { return m_focusPolicy; }
    
    /**
        Returns whether this component can accept focus.
        
        A component can accept focus if:
        - Focus policy is not NoFocus
        - Component is enabled
        - Component is visible
        
        @return true if component can accept focus, false otherwise
    */
    bool canAcceptFocus() const;
    
    /**
        Returns whether this component accepts focus via Tab key.
        
        @return true if Tab focus is enabled, false otherwise
    */
    bool acceptsTabFocus() const;
    
    /**
        Returns whether this component accepts focus via mouse click.
        
        @return true if click focus is enabled, false otherwise
    */
    bool acceptsClickFocus() const;
    
    /**
        Returns whether this component currently has focus.
        
        @return true if focused, false otherwise
    */
    bool hasFocus() const { return m_hasFocus; }
    
    /**
        Gives keyboard focus to this component.
        
        @param reason  The reason for the focus change (Tab, Click, etc.)
    */
    void setFocus(FocusReason reason = FocusReason::OtherFocusReason);
    
    /**
        Removes keyboard focus from this component.
        
        If this component does not have focus, this method does nothing.
    */
    void clearFocus();
    
    /**
        Requests keyboard focus for this component.
        
        Convenience method that calls setFocus().
        
        @param reason  The reason for the focus change
    */
    void requestFocus(FocusReason reason = FocusReason::OtherFocusReason) {
        setFocus(reason);
    }
    
    /**
        Sets a focus proxy for this component.
        
        When this component receives focus, the proxy component receives focus instead.
        This is useful for composite components where the container should delegate
        focus to a specific child.
        
        @param proxy  Pointer to the component that should receive focus instead
    */
    void setFocusProxy(UIComponent* proxy);
    
    /**
        Returns the focus proxy for this component.
        
        @return Pointer to the focus proxy, or nullptr if not set
    */
    UIComponent* getFocusProxy() const { return m_focusProxy; }
    
    /**
        Returns the effective focus widget.
        
        If a focus proxy is set, returns the proxy. Otherwise returns this component.
        
        @return Pointer to the component that should actually receive focus
    */
    UIComponent* effectiveFocusWidget();
    
    /**
        Sets the tab order for this component.
        
        Components with lower tab order values receive focus first when pressing Tab.
        Components with the same tab order are ordered by their position in the
        component tree.
        
        @param order  Tab order value (can be negative)
    */
    void setTabOrder(int order);
    
    /**
        Returns the tab order for this component.
        
        @return Tab order value (-1 if not explicitly set)
    */
    int getTabOrder() const { return m_tabOrder; }
    
    /**
        Sets whether the focus indicator should be shown.
        
        The focus indicator is a visual outline drawn around focused components.
        
        @param show  true to show focus indicator, false to hide
    */
    void setShowFocusIndicator(bool show) { m_showFocusIndicator = show; }
    
    /**
        Returns whether the focus indicator is shown for this component.
        
        @return true if focus indicator is shown, false otherwise
    */
    bool showsFocusIndicator() const { return m_showFocusIndicator; }
    
    /**
        Returns whether this component should handle directional key navigation.
        
        Override this to enable arrow key navigation for components like lists, grids, etc.
        Default implementation returns false.
        
        @param direction  Direction of navigation (Up, Down, Left, Right)
        @return true if component handles this direction, false otherwise
    */
    virtual bool shouldHandleDirectionKey(FocusDirection direction) const {
        return false;
    }
    
protected:
    //======================================================================================
    // Protected Helper Methods
    
    /**
        Captures all mouse input to this component.
        
        After capturing, all mouse events go to this component regardless of mouse position.
        This is typically used during drag operations.
    */
    void captureMouse();
    
    /**
        Releases mouse capture.
        
        Returns mouse event handling to normal hit-testing behavior.
    */
    void releaseMouse();
    
    /**
        Renders all child components.
        
        Helper method for container components. Iterates through all children and
        calls their addDrawCommands() method.
        
        @param commandList  Render command list to append to
        @param offset       Cumulative offset in parent space
    */
    void renderChildren(RenderList& commandList, const Vec2& offset) const;
    
    /**
        Dispatches mouse events to child components.
        
        Helper method for container components. Performs hit-testing and dispatches
        mouse events to the appropriate child. Children are tested in reverse order
        (front to back).
        
        @param position  Mouse position in window coordinates
        @param pressed   true for click events, ignored for move events
        @param offset    Cumulative offset in parent space
        @param isMove    true for move events, false for click events
        @return true if a child handled the event, false otherwise
    */
    bool dispatchMouseEvent(const Vec2& position, bool pressed, const Vec2& offset, bool isMove);
    
    /**
        Draws the focus indicator around this component.
        
        Called automatically by subclasses if the component has focus and focus
        indicators are enabled. Subclasses can override getFocusIndicatorCornerRadius()
        to customize the appearance.
        
        @param commandList  Render command list to append to
        @param offset       Cumulative offset in parent space
    */
    virtual void drawFocusIndicator(RenderList& commandList, const Vec2& offset) const;
    
    /**
        Returns the corner radius for the focus indicator.
        
        Override this to match the component's visual style. Default returns no rounding.
        
        @return Corner radius for focus indicator
    */
    virtual CornerRadius getFocusIndicatorCornerRadius() const { return CornerRadius(); }
    
    /**
        Called when this component gains focus.
        
        Override to perform actions when focus is gained. Default implementation
        scrolls the component into view if it's inside a scrollable container.
        
        @param reason  The reason for gaining focus
    */
    virtual void focusInEvent(FocusReason reason) {}
    
    /**
        Called when this component loses focus.
        
        Override to perform actions when focus is lost. Default implementation
        does nothing.
        
        @param reason  The reason for losing focus
    */
    virtual void focusOutEvent(FocusReason reason) {}
    
    //======================================================================================
    // Protected Member Variables
    
    bool m_isVisible;                     ///< Whether the component is visible
    bool m_isEnabled;                     ///< Whether the component is enabled
    UIContext* m_ownerContext;            ///< Pointer to owning UI context
    IUIContent* m_ownerContent;           ///< Pointer to owning UI content
    UIComponent* m_parent;                ///< Pointer to parent component
    Menu* m_contextMenu;                  ///< Pointer to context menu
    
    Rect m_bounds;                        ///< Bounding rectangle in parent-local coordinates
    
    std::vector<UIComponent*> m_ownedChildren;  ///< Child components
    
    float m_paddingLeft;                  ///< Left padding in pixels
    float m_paddingTop;                   ///< Top padding in pixels
    float m_paddingRight;                 ///< Right padding in pixels
    float m_paddingBottom;                ///< Bottom padding in pixels

private:
    //======================================================================================
    // Private Focus System Interface
    // These methods are called by FocusManager and IUIContent
    
    void onFocusIn(FocusReason reason);
    void onFocusOut(FocusReason reason);
    void scrollIntoViewIfNeeded();
    void notifyFocusIn(FocusReason reason);
    void notifyFocusOut(FocusReason reason);
    void setFocusState(bool focused) { m_hasFocus = focused; }
    
    FocusPolicy m_focusPolicy;            ///< How this component receives focus
    bool m_hasFocus;                      ///< Whether this component has focus
    int m_tabOrder;                       ///< Tab order (-1 = use tree order)
    UIComponent* m_focusProxy;            ///< Component to delegate focus to
    bool m_showFocusIndicator;            ///< Whether to show focus indicator
    FocusManager* m_focusManagerAccessor; ///< Direct accessor to focus manager
    
    friend class FocusManager;
    friend class IUIContent;
};

} // namespace YuchenUI
