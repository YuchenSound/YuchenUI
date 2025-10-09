#pragma once

#include "YuchenUI/widgets/Widget.h"
#include "YuchenUI/core/Types.h"

namespace YuchenUI {

class RenderList;

class ScrollArea : public Widget {
public:
    explicit ScrollArea(const Rect& bounds);
    ~ScrollArea() override;

    static constexpr float SCROLLBAR_WIDTH = 15.0f;
    static constexpr float SCROLLBAR_THUMB_WIDTH = 6.0f;
    static constexpr float SCROLLBAR_THUMB_MIN_SIZE = 20.0f;
    
    static constexpr float BUTTON_SIZE = 15.0f;
    static constexpr float TRIANGLE_BASE = 6.0f;
    static constexpr float TRIANGLE_HEIGHT = 4.0f;
    
    static constexpr float BUTTONS_TOTAL_SIZE = BUTTON_SIZE * 2.0f;
    
    static constexpr float AUTO_SCROLL_TRIGGER_ZONE = 25.0f;

    void setContentSize(const Vec2& size);
    Vec2 getContentSize() const { return m_contentSize; }
    
    Vec2 getScrollOffset() const { return Vec2(m_scrollX, m_scrollY); }
    void setScrollOffset(const Vec2& offset);
    void setScrollX(float x);
    void setScrollY(float y);
    
    void setShowVerticalScrollbar(bool show) { m_showVerticalScrollbar = show; }
    void setShowHorizontalScrollbar(bool show) { m_showHorizontalScrollbar = show; }
    bool isVerticalScrollbarVisible() const { return m_showVerticalScrollbar; }
    bool isHorizontalScrollbarVisible() const { return m_showHorizontalScrollbar; }
    
    void setAutoScrollEnabled(bool enabled) { m_autoScrollEnabled = enabled; }
    bool isAutoScrollEnabled() const { return m_autoScrollEnabled; }
    void setAutoScrollSpeed(float speed) { m_autoScrollSpeed = speed; }
    
    void addDrawCommands(RenderList& commandList, const Vec2& offset = Vec2()) const override;
    bool handleMouseMove(const Vec2& position, const Vec2& offset = Vec2()) override;
    bool handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset = Vec2()) override;
    bool handleMouseWheel(const Vec2& delta, const Vec2& position, const Vec2& offset = Vec2()) override;

private:
    Vec2 m_contentSize;
    float m_scrollX;
    float m_scrollY;
    
    bool m_showVerticalScrollbar;
    bool m_showHorizontalScrollbar;
    
    bool m_autoScrollEnabled;
    float m_autoScrollSpeed;
    
    enum class DragMode
    {
        None,
        Content,
        VerticalThumb,
        HorizontalThumb
    };
    
    DragMode m_dragMode;
    Vec2 m_dragStartPos;
    Vec2 m_dragStartScroll;
    
    bool m_verticalThumbHovered;
    bool m_verticalButtonUpHovered;
    bool m_verticalButtonDownHovered;
    
    bool m_horizontalThumbHovered;
    bool m_horizontalButtonLeftHovered;
    bool m_horizontalButtonRightHovered;
    
    bool m_verticalButtonUpPressed;
    bool m_verticalButtonDownPressed;
    bool m_horizontalButtonLeftPressed;
    bool m_horizontalButtonRightPressed;
    
    Rect getContentArea() const;
    Rect getVerticalScrollbarRect(const Vec2& absPos) const;
    Rect getHorizontalScrollbarRect(const Vec2& absPos) const;
    Rect getVerticalThumbRect(const Vec2& absPos) const;
    Rect getHorizontalThumbRect(const Vec2& absPos) const;
    Rect getVerticalButtonUpRect(const Vec2& absPos) const;
    Rect getVerticalButtonDownRect(const Vec2& absPos) const;
    Rect getHorizontalButtonLeftRect(const Vec2& absPos) const;
    Rect getHorizontalButtonRightRect(const Vec2& absPos) const;
    
    void renderScrollbars(RenderList& commandList, const Vec2& absPos) const;
    void renderVerticalScrollbar(RenderList& commandList, const Vec2& absPos) const;
    void renderHorizontalScrollbar(RenderList& commandList, const Vec2& absPos) const;
    
    bool handleScrollbarInteraction(const Vec2& position, bool pressed, const Vec2& offset);
    bool handleAutoScroll(const Vec2& position, const Vec2& offset);
    
    void clampScroll();
    Vec2 transformToContentCoords(const Vec2& screenPos, const Vec2& offset) const;
};

}
