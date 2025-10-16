/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

#pragma once

#include "YuchenUI/widgets/UIComponent.h"
#include "YuchenUI/widgets/IScrollable.h"
#include "YuchenUI/core/Types.h"

namespace YuchenUI {

class RenderList;

/**
    Scrollable container widget with scrollbars.
    
    ScrollArea provides a viewport into a larger content area, with vertical and
    horizontal scrollbars for navigation. It supports:
    - Mouse wheel scrolling
    - Scrollbar thumb dragging
    - Scrollbar button clicking
    - Auto-scroll when dragging near edges
    - Programmatic scrolling
    - Content clipping to viewport
    
    Visual layout:
    ┌─────────────┬─┐
    │             │↑│  ← Vertical scrollbar with up/down buttons
    │  Content    │█│
    │  Area       │↓│
    ├─────────────┼─┤
    │←  █      →  │ │  ← Horizontal scrollbar with left/right buttons
    └─────────────┴─┘
    
    Scrollbar dimensions:
    - Scrollbar track width: 15 pixels
    - Thumb width: 6 pixels (centered in track)
    - Minimum thumb size: 20 pixels
    - Button size: 15x15 pixels
    
    Auto-scroll feature:
    When dragging content near the viewport edges (within 25 pixels), the content
    automatically scrolls in that direction. The scroll speed increases with proximity
    to the edge.
    
    Example usage:
    @code
    // Create scroll area
    ScrollArea* scrollArea = parent->addChild<ScrollArea>(Rect(10, 10, 400, 300));
    scrollArea->setContentSize(Vec2(800, 600));  // Content is 800x600
    
    // Add content as children
    Frame* content = scrollArea->addChild<Frame>(Rect(0, 0, 800, 600));
    
    // Programmatic scrolling
    scrollArea->setScrollY(100.0f);
    
    // Scroll to show a specific rectangle
    scrollArea->scrollRectIntoView(Rect(200, 200, 100, 50));
    @endcode
    
    Implementation of IScrollable:
    ScrollArea implements the IScrollable interface, allowing child components to
    request scrolling (e.g., when gaining focus) via scrollRectIntoView().
    
    @see IScrollable, UIComponent
*/
class ScrollArea : public UIComponent, public IScrollable {
public:
    /**
        Constructs a scroll area with the specified bounds.
        
        The scroll area is created with:
        - Zero content size (must be set with setContentSize())
        - Zero scroll offset
        - Both scrollbars enabled
        - Auto-scroll enabled with default speed
        
        @param bounds  Initial bounding rectangle (viewport size)
    */
    explicit ScrollArea(const Rect& bounds);
    
    ~ScrollArea() override;
    
    // Scrollbar dimensions
    static constexpr float SCROLLBAR_WIDTH = 15.0f;         ///< Width of scrollbar track
    static constexpr float SCROLLBAR_THUMB_WIDTH = 6.0f;    ///< Width of scrollbar thumb
    static constexpr float SCROLLBAR_THUMB_MIN_SIZE = 20.0f;///< Minimum thumb size
    
    static constexpr float BUTTON_SIZE = 15.0f;             ///< Size of arrow buttons
    static constexpr float TRIANGLE_BASE = 6.0f;            ///< Base of arrow triangle
    static constexpr float TRIANGLE_HEIGHT = 4.0f;          ///< Height of arrow triangle
    
    static constexpr float BUTTONS_TOTAL_SIZE = BUTTON_SIZE * 2.0f;  ///< Total size of both buttons
    
    static constexpr float AUTO_SCROLL_TRIGGER_ZONE = 25.0f;  ///< Distance from edge for auto-scroll
    
    //======================================================================================
    // UIComponent Interface Implementation
    
    void addDrawCommands(RenderList& commandList, const Vec2& offset = Vec2()) const override;
    bool handleMouseMove(const Vec2& position, const Vec2& offset = Vec2()) override;
    bool handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset = Vec2()) override;
    bool handleMouseWheel(const Vec2& delta, const Vec2& position, const Vec2& offset = Vec2()) override;
    
    //======================================================================================
    // IScrollable Interface Implementation
    
    /**
        Scrolls to make the specified rectangle visible.
        
        Adjusts the scroll offset to ensure the given rectangle (in content coordinates)
        is visible in the viewport. If the rectangle is already visible, no scrolling occurs.
        
        @param rect  Rectangle in content coordinates to make visible
        @return true if scrolling occurred, false if already visible
    */
    bool scrollRectIntoView(const Rect& rect) override;
    
    /**
        Returns the visible content area rectangle.
        
        @return Rectangle defining the viewport (excludes scrollbars)
    */
    Rect getVisibleContentArea() const override;
    
    /**
        Returns the current scroll offset.
        
        @return Scroll offset as (scrollX, scrollY)
    */
    Vec2 getScrollOffset() const override { return Vec2(m_scrollX, m_scrollY); }
    
    //======================================================================================
    // Content Size API
    
    /**
        Sets the size of the scrollable content.
        
        This defines the virtual size of the content area. If content size is larger
        than the viewport, scrollbars will appear.
        
        @param size  Content size in pixels (must be non-negative)
    */
    void setContentSize(const Vec2& size);
    
    /**
        Returns the current content size.
        
        @return Content size in pixels
    */
    Vec2 getContentSize() const { return m_contentSize; }
    
    //======================================================================================
    // Scroll Offset API
    
    /**
        Sets the scroll offset for both axes.
        
        The offset is automatically clamped to valid range [0, maxScroll].
        
        @param offset  Scroll offset as (scrollX, scrollY)
    */
    void setScrollOffset(const Vec2& offset);
    
    /**
        Sets the horizontal scroll offset.
        
        @param x  Horizontal scroll position in pixels
    */
    void setScrollX(float x);
    
    /**
        Sets the vertical scroll offset.
        
        @param y  Vertical scroll position in pixels
    */
    void setScrollY(float y);
    
    //======================================================================================
    // Scrollbar Visibility API
    
    /**
        Sets whether the vertical scrollbar is shown.
        
        Even when enabled, the scrollbar only appears if content height exceeds viewport.
        
        @param show  true to enable, false to disable
    */
    void setShowVerticalScrollbar(bool show) { m_showVerticalScrollbar = show; }
    
    /**
        Sets whether the horizontal scrollbar is shown.
        
        Even when enabled, the scrollbar only appears if content width exceeds viewport.
        
        @param show  true to enable, false to disable
    */
    void setShowHorizontalScrollbar(bool show) { m_showHorizontalScrollbar = show; }
    
    /**
        Returns whether the vertical scrollbar is enabled.
        
        @return true if enabled, false otherwise
    */
    bool isVerticalScrollbarVisible() const { return m_showVerticalScrollbar; }
    
    /**
        Returns whether the horizontal scrollbar is enabled.
        
        @return true if enabled, false otherwise
    */
    bool isHorizontalScrollbarVisible() const { return m_showHorizontalScrollbar; }
    
    //======================================================================================
    // Auto-Scroll API
    
    /**
        Sets whether auto-scroll is enabled.
        
        When enabled, dragging near viewport edges automatically scrolls in that direction.
        
        @param enabled  true to enable, false to disable
    */
    void setAutoScrollEnabled(bool enabled) { m_autoScrollEnabled = enabled; }
    
    /**
        Returns whether auto-scroll is enabled.
        
        @return true if enabled, false otherwise
    */
    bool isAutoScrollEnabled() const { return m_autoScrollEnabled; }
    
    /**
        Sets the auto-scroll speed in pixels per second.
        
        @param speed  Maximum scroll speed in pixels/second (default 500)
    */
    void setAutoScrollSpeed(float speed) { m_autoScrollSpeed = speed; }

private:
    /**
        Drag mode for mouse interactions.
    */
    enum class DragMode
    {
        None,              ///< No dragging
        Content,           ///< Dragging content (for auto-scroll)
        VerticalThumb,     ///< Dragging vertical scrollbar thumb
        HorizontalThumb    ///< Dragging horizontal scrollbar thumb
    };
    
    /**
        Returns the content area rectangle (viewport minus scrollbars).
        
        @return Rectangle defining the visible content area
    */
    Rect getContentArea() const;
    
    /**
        Returns the vertical scrollbar track rectangle.
        
        @param absPos  Absolute position of the scroll area
        @return Rectangle for vertical scrollbar track
    */
    Rect getVerticalScrollbarRect(const Vec2& absPos) const;
    
    /**
        Returns the horizontal scrollbar track rectangle.
        
        @param absPos  Absolute position of the scroll area
        @return Rectangle for horizontal scrollbar track
    */
    Rect getHorizontalScrollbarRect(const Vec2& absPos) const;
    
    /**
        Returns the vertical scrollbar thumb rectangle.
        
        @param absPos  Absolute position of the scroll area
        @return Rectangle for vertical thumb
    */
    Rect getVerticalThumbRect(const Vec2& absPos) const;
    
    /**
        Returns the horizontal scrollbar thumb rectangle.
        
        @param absPos  Absolute position of the scroll area
        @return Rectangle for horizontal thumb
    */
    Rect getHorizontalThumbRect(const Vec2& absPos) const;
    
    /**
        Returns the vertical up button rectangle.
        
        @param absPos  Absolute position of the scroll area
        @return Rectangle for up button
    */
    Rect getVerticalButtonUpRect(const Vec2& absPos) const;
    
    /**
        Returns the vertical down button rectangle.
        
        @param absPos  Absolute position of the scroll area
        @return Rectangle for down button
    */
    Rect getVerticalButtonDownRect(const Vec2& absPos) const;
    
    /**
        Returns the horizontal left button rectangle.
        
        @param absPos  Absolute position of the scroll area
        @return Rectangle for left button
    */
    Rect getHorizontalButtonLeftRect(const Vec2& absPos) const;
    
    /**
        Returns the horizontal right button rectangle.
        
        @param absPos  Absolute position of the scroll area
        @return Rectangle for right button
    */
    Rect getHorizontalButtonRightRect(const Vec2& absPos) const;
    
    /**
        Renders both scrollbars.
        
        @param commandList  Render command list
        @param absPos       Absolute position of the scroll area
    */
    void renderScrollbars(RenderList& commandList, const Vec2& absPos) const;
    
    /**
        Renders the vertical scrollbar.
        
        @param commandList  Render command list
        @param absPos       Absolute position of the scroll area
    */
    void renderVerticalScrollbar(RenderList& commandList, const Vec2& absPos) const;
    
    /**
        Renders the horizontal scrollbar.
        
        @param commandList  Render command list
        @param absPos       Absolute position of the scroll area
    */
    void renderHorizontalScrollbar(RenderList& commandList, const Vec2& absPos) const;
    
    /**
        Handles scrollbar interaction (clicking/dragging).
        
        @param position  Mouse position in window coordinates
        @param pressed   true for button down, false for button up
        @param offset    Cumulative offset in parent space
        @return true if scrollbar was interacted with, false otherwise
    */
    bool handleScrollbarInteraction(const Vec2& position, bool pressed, const Vec2& offset);
    
    /**
        Handles auto-scroll when dragging near edges.
        
        @param position  Mouse position in window coordinates
        @param offset    Cumulative offset in parent space
        @return true if auto-scroll occurred, false otherwise
    */
    bool handleAutoScroll(const Vec2& position, const Vec2& offset);
    
    /**
        Clamps scroll offset to valid range.
    */
    void clampScroll();
    
    /**
        Transforms screen position to content coordinates.
        
        @param screenPos  Position in window coordinates
        @param offset     Cumulative offset in parent space
        @return Position in content coordinates
    */
    Vec2 transformToContentCoords(const Vec2& screenPos, const Vec2& offset) const;
    
    Vec2 m_contentSize;                   ///< Size of scrollable content
    float m_scrollX;                      ///< Horizontal scroll position
    float m_scrollY;                      ///< Vertical scroll position
    
    bool m_showVerticalScrollbar;         ///< Whether vertical scrollbar is enabled
    bool m_showHorizontalScrollbar;       ///< Whether horizontal scrollbar is enabled
    
    bool m_autoScrollEnabled;             ///< Whether auto-scroll is enabled
    float m_autoScrollSpeed;              ///< Auto-scroll speed in pixels/second
    
    DragMode m_dragMode;                  ///< Current drag mode
    Vec2 m_dragStartPos;                  ///< Mouse position when drag started
    Vec2 m_dragStartScroll;               ///< Scroll position when drag started
    
    bool m_verticalThumbHovered;          ///< Whether vertical thumb is hovered
    bool m_verticalButtonUpHovered;       ///< Whether up button is hovered
    bool m_verticalButtonDownHovered;     ///< Whether down button is hovered
    
    bool m_horizontalThumbHovered;        ///< Whether horizontal thumb is hovered
    bool m_horizontalButtonLeftHovered;   ///< Whether left button is hovered
    bool m_horizontalButtonRightHovered;  ///< Whether right button is hovered
    
    bool m_verticalButtonUpPressed;       ///< Whether up button is pressed
    bool m_verticalButtonDownPressed;     ///< Whether down button is pressed
    bool m_horizontalButtonLeftPressed;   ///< Whether left button is pressed
    bool m_horizontalButtonRightPressed;  ///< Whether right button is pressed
};

} // namespace YuchenUI
