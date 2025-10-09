#include "YuchenUI/widgets/ScrollArea.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/theme/ThemeManager.h"
#include "YuchenUI/core/Colors.h"
#include "YuchenUI/core/Assert.h"
#include <algorithm>
#include <cmath>

namespace YuchenUI {

ScrollArea::ScrollArea(const Rect& bounds)
    : Widget(bounds)
    , m_contentSize()
    , m_scrollX(0.0f)
    , m_scrollY(0.0f)
    , m_showVerticalScrollbar(true)
    , m_showHorizontalScrollbar(true)
    , m_autoScrollEnabled(true)
    , m_autoScrollSpeed(500.0f)
    , m_dragMode(DragMode::None)
    , m_dragStartPos()
    , m_dragStartScroll()
    , m_verticalThumbHovered(false)
    , m_verticalButtonUpHovered(false)
    , m_verticalButtonDownHovered(false)
    , m_horizontalThumbHovered(false)
    , m_horizontalButtonLeftHovered(false)
    , m_horizontalButtonRightHovered(false)
    , m_verticalButtonUpPressed(false)
    , m_verticalButtonDownPressed(false)
    , m_horizontalButtonLeftPressed(false)
    , m_horizontalButtonRightPressed(false)
{
}

ScrollArea::~ScrollArea() {
}

void ScrollArea::setContentSize(const Vec2& size) {
    YUCHEN_ASSERT(size.x >= 0.0f && size.y >= 0.0f);
    m_contentSize = size;
    clampScroll();
}

void ScrollArea::setScrollOffset(const Vec2& offset) {
    m_scrollX = offset.x;
    m_scrollY = offset.y;
    clampScroll();
}

void ScrollArea::setScrollX(float x) {
    m_scrollX = x;
    clampScroll();
}

void ScrollArea::setScrollY(float y) {
    m_scrollY = y;
    clampScroll();
}

bool ScrollArea::scrollRectIntoView(const Rect& rect) {
    Rect contentArea = getContentArea();
    Vec2 scrollOffset = getScrollOffset();
    
    float scrollX = scrollOffset.x;
    float scrollY = scrollOffset.y;
    bool needsScroll = false;
    
    if (rect.y < scrollY) {
        scrollY = rect.y;
        needsScroll = true;
    } else if (rect.y + rect.height > scrollY + contentArea.height) {
        scrollY = rect.y + rect.height - contentArea.height;
        needsScroll = true;
    }
    
    if (rect.x < scrollX) {
        scrollX = rect.x;
        needsScroll = true;
    } else if (rect.x + rect.width > scrollX + contentArea.width) {
        scrollX = rect.x + rect.width - contentArea.width;
        needsScroll = true;
    }
    
    if (needsScroll) {
        setScrollOffset(Vec2(scrollX, scrollY));
        return true;
    }
    
    return false;
}

Rect ScrollArea::getVisibleContentArea() const {
    return getContentArea();
}

Rect ScrollArea::getContentArea() const {
    float width = m_bounds.width;
    float height = m_bounds.height;
    
    if (m_showVerticalScrollbar && m_contentSize.y > height) {
        width -= SCROLLBAR_WIDTH;
    }
    if (m_showHorizontalScrollbar && m_contentSize.x > width) {
        height -= SCROLLBAR_WIDTH;
    }
    
    return Rect(0, 0, std::max(0.0f, width), std::max(0.0f, height));
}

Rect ScrollArea::getVerticalScrollbarRect(const Vec2& absPos) const {
    Rect contentArea = getContentArea();
    bool needsScrolling = (m_contentSize.y > contentArea.height);
    float mainScrollbarHeight = needsScrolling ? contentArea.height - BUTTONS_TOTAL_SIZE : contentArea.height;
    
    return Rect(
        absPos.x + contentArea.width,
        absPos.y,
        SCROLLBAR_WIDTH,
        mainScrollbarHeight
    );
}

Rect ScrollArea::getHorizontalScrollbarRect(const Vec2& absPos) const {
    Rect contentArea = getContentArea();
    bool needsScrolling = (m_contentSize.x > contentArea.width);
    float mainScrollbarWidth = needsScrolling ? contentArea.width - BUTTONS_TOTAL_SIZE : contentArea.width;
    
    return Rect(
        absPos.x,
        absPos.y + contentArea.height,
        mainScrollbarWidth,
        SCROLLBAR_WIDTH
    );
}

Rect ScrollArea::getVerticalThumbRect(const Vec2& absPos) const {
    Rect scrollbarRect = getVerticalScrollbarRect(absPos);
    Rect contentArea = getContentArea();
    
    if (m_contentSize.y <= contentArea.height) {
        return Rect();
    }
    
    float effectiveScrollHeight = scrollbarRect.height;
    float ratio = contentArea.height / m_contentSize.y;
    float thumbSize = std::max(SCROLLBAR_THUMB_MIN_SIZE, effectiveScrollHeight * ratio);
    thumbSize = std::min(thumbSize, effectiveScrollHeight);
    
    float maxScroll = m_contentSize.y - contentArea.height;
    float scrollRatio = maxScroll > 0.0f ? (m_scrollY / maxScroll) : 0.0f;
    float maxThumbPos = effectiveScrollHeight - thumbSize;
    float thumbY = scrollbarRect.y + scrollRatio * maxThumbPos;
    
    return Rect(
        scrollbarRect.x,
        thumbY,
        SCROLLBAR_WIDTH,
        thumbSize
    );
}

Rect ScrollArea::getHorizontalThumbRect(const Vec2& absPos) const {
    Rect scrollbarRect = getHorizontalScrollbarRect(absPos);
    Rect contentArea = getContentArea();
    
    if (m_contentSize.x <= contentArea.width) {
        return Rect();
    }
    
    float effectiveScrollWidth = scrollbarRect.width;
    float ratio = contentArea.width / m_contentSize.x;
    float thumbSize = std::max(SCROLLBAR_THUMB_MIN_SIZE, effectiveScrollWidth * ratio);
    thumbSize = std::min(thumbSize, effectiveScrollWidth);
    
    float maxScroll = m_contentSize.x - contentArea.width;
    float scrollRatio = maxScroll > 0.0f ? (m_scrollX / maxScroll) : 0.0f;
    float maxThumbPos = effectiveScrollWidth - thumbSize;
    float thumbX = scrollbarRect.x + scrollRatio * maxThumbPos;
    
    return Rect(
        thumbX,
        scrollbarRect.y,
        thumbSize,
        SCROLLBAR_WIDTH
    );
}

Rect ScrollArea::getVerticalButtonUpRect(const Vec2& absPos) const {
    Rect contentArea = getContentArea();
    Rect scrollbarRect = getVerticalScrollbarRect(absPos);
    
    return Rect(
        absPos.x + contentArea.width,
        scrollbarRect.y + scrollbarRect.height,
        BUTTON_SIZE,
        BUTTON_SIZE
    );
}

Rect ScrollArea::getVerticalButtonDownRect(const Vec2& absPos) const {
    Rect buttonUpRect = getVerticalButtonUpRect(absPos);
    
    return Rect(
        buttonUpRect.x,
        buttonUpRect.y + BUTTON_SIZE,
        BUTTON_SIZE,
        BUTTON_SIZE
    );
}

Rect ScrollArea::getHorizontalButtonLeftRect(const Vec2& absPos) const {
    Rect contentArea = getContentArea();
    Rect scrollbarRect = getHorizontalScrollbarRect(absPos);
    
    return Rect(
        scrollbarRect.x + scrollbarRect.width,
        absPos.y + contentArea.height,
        BUTTON_SIZE,
        BUTTON_SIZE
    );
}

Rect ScrollArea::getHorizontalButtonRightRect(const Vec2& absPos) const {
    Rect buttonLeftRect = getHorizontalButtonLeftRect(absPos);
    
    return Rect(
        buttonLeftRect.x + BUTTON_SIZE,
        buttonLeftRect.y,
        BUTTON_SIZE,
        BUTTON_SIZE
    );
}

void ScrollArea::addDrawCommands(RenderList& commandList, const Vec2& offset) const {
    if (!m_isVisible) return;
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect contentArea = getContentArea();
    
    UIStyle* style = ThemeManager::getInstance().getCurrentStyle();
    Vec4 backgroundColor = style->getDefaultScrollAreaBackground();
    
    Rect bgRect(absPos.x, absPos.y, m_bounds.width, m_bounds.height);
    commandList.fillRect(bgRect, backgroundColor);
    
    Rect clipRect(absPos.x, absPos.y, contentArea.width, contentArea.height);
    commandList.pushClipRect(clipRect);
    
    Vec2 contentOffset(absPos.x - m_scrollX, absPos.y - m_scrollY);
    renderChildren(commandList, contentOffset);
    
    commandList.popClipRect();
    
    renderScrollbars(commandList, absPos);
}

void ScrollArea::renderScrollbars(RenderList& commandList, const Vec2& absPos) const {
    Rect contentArea = getContentArea();
    
    if (m_showVerticalScrollbar && m_contentSize.y > contentArea.height) {
        renderVerticalScrollbar(commandList, absPos);
    }
    
    if (m_showHorizontalScrollbar && m_contentSize.x > contentArea.width) {
        renderHorizontalScrollbar(commandList, absPos);
    }
}

void ScrollArea::renderVerticalScrollbar(RenderList& commandList, const Vec2& absPos) const {
    UIStyle* style = ThemeManager::getInstance().getCurrentStyle();
    
    Rect scrollbarRect = getVerticalScrollbarRect(absPos);
    ScrollbarTrackDrawInfo trackInfo;
    trackInfo.bounds = scrollbarRect;
    trackInfo.orientation = ScrollbarOrientation::Vertical;
    style->drawScrollbarTrack(trackInfo, commandList);
    
    Rect thumbRect = getVerticalThumbRect(absPos);
    if (thumbRect.isValid() && thumbRect.height > 0.0f) {
        ScrollbarThumbDrawInfo thumbInfo;
        thumbInfo.bounds = thumbRect;
        thumbInfo.orientation = ScrollbarOrientation::Vertical;
        thumbInfo.isHovered = m_verticalThumbHovered;
        thumbInfo.isDragging = (m_dragMode == DragMode::VerticalThumb);
        style->drawScrollbarThumb(thumbInfo, commandList);
    }
    
    Rect buttonUpRect = getVerticalButtonUpRect(absPos);
    ScrollbarButtonDrawInfo buttonUpInfo;
    buttonUpInfo.bounds = buttonUpRect;
    buttonUpInfo.orientation = ScrollbarOrientation::Vertical;
    buttonUpInfo.buttonType = ScrollbarButtonType::UpLeft;
    buttonUpInfo.buttonState = m_verticalButtonUpPressed ? ScrollbarButtonState::Pressed :
                               (m_verticalButtonUpHovered ? ScrollbarButtonState::Hovered : ScrollbarButtonState::Normal);
    style->drawScrollbarButton(buttonUpInfo, commandList);
    
    Rect buttonDownRect = getVerticalButtonDownRect(absPos);
    ScrollbarButtonDrawInfo buttonDownInfo;
    buttonDownInfo.bounds = buttonDownRect;
    buttonDownInfo.orientation = ScrollbarOrientation::Vertical;
    buttonDownInfo.buttonType = ScrollbarButtonType::DownRight;
    buttonDownInfo.buttonState = m_verticalButtonDownPressed ? ScrollbarButtonState::Pressed :
                                 (m_verticalButtonDownHovered ? ScrollbarButtonState::Hovered : ScrollbarButtonState::Normal);
    style->drawScrollbarButton(buttonDownInfo, commandList);
}

void ScrollArea::renderHorizontalScrollbar(RenderList& commandList, const Vec2& absPos) const {
    UIStyle* style = ThemeManager::getInstance().getCurrentStyle();
    
    Rect scrollbarRect = getHorizontalScrollbarRect(absPos);
    ScrollbarTrackDrawInfo trackInfo;
    trackInfo.bounds = scrollbarRect;
    trackInfo.orientation = ScrollbarOrientation::Horizontal;
    style->drawScrollbarTrack(trackInfo, commandList);
    
    Rect thumbRect = getHorizontalThumbRect(absPos);
    if (thumbRect.isValid() && thumbRect.width > 0.0f) {
        ScrollbarThumbDrawInfo thumbInfo;
        thumbInfo.bounds = thumbRect;
        thumbInfo.orientation = ScrollbarOrientation::Horizontal;
        thumbInfo.isHovered = m_horizontalThumbHovered;
        thumbInfo.isDragging = (m_dragMode == DragMode::HorizontalThumb);
        style->drawScrollbarThumb(thumbInfo, commandList);
    }
    
    Rect buttonLeftRect = getHorizontalButtonLeftRect(absPos);
    ScrollbarButtonDrawInfo buttonLeftInfo;
    buttonLeftInfo.bounds = buttonLeftRect;
    buttonLeftInfo.orientation = ScrollbarOrientation::Horizontal;
    buttonLeftInfo.buttonType = ScrollbarButtonType::UpLeft;
    buttonLeftInfo.buttonState = m_horizontalButtonLeftPressed ? ScrollbarButtonState::Pressed :
                                 (m_horizontalButtonLeftHovered ? ScrollbarButtonState::Hovered : ScrollbarButtonState::Normal);
    style->drawScrollbarButton(buttonLeftInfo, commandList);
    
    Rect buttonRightRect = getHorizontalButtonRightRect(absPos);
    ScrollbarButtonDrawInfo buttonRightInfo;
    buttonRightInfo.bounds = buttonRightRect;
    buttonRightInfo.orientation = ScrollbarOrientation::Horizontal;
    buttonRightInfo.buttonType = ScrollbarButtonType::DownRight;
    buttonRightInfo.buttonState = m_horizontalButtonRightPressed ? ScrollbarButtonState::Pressed :
                                  (m_horizontalButtonRightHovered ? ScrollbarButtonState::Hovered : ScrollbarButtonState::Normal);
    style->drawScrollbarButton(buttonRightInfo, commandList);
}

bool ScrollArea::handleMouseWheel(const Vec2& delta, const Vec2& position, const Vec2& offset) {
    if (!m_isEnabled || !m_isVisible) return false;
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect contentArea = getContentArea();
    Rect absContentRect(absPos.x, absPos.y, contentArea.width, contentArea.height);
    
    if (!absContentRect.contains(position)) return false;
    
    float scrollSpeed = 30.0f;
    m_scrollY -= delta.y * scrollSpeed;
    m_scrollX -= delta.x * scrollSpeed;
    
    clampScroll();
    return true;
}

bool ScrollArea::handleMouseMove(const Vec2& position, const Vec2& offset) {
    if (!m_isEnabled || !m_isVisible) return false;
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect contentArea = getContentArea();
    
    bool needsVerticalScrolling = (m_contentSize.y > contentArea.height);
    bool needsHorizontalScrolling = (m_contentSize.x > contentArea.width);
    
    m_verticalThumbHovered = false;
    m_verticalButtonUpHovered = false;
    m_verticalButtonDownHovered = false;
    m_horizontalThumbHovered = false;
    m_horizontalButtonLeftHovered = false;
    m_horizontalButtonRightHovered = false;
    
    if (needsVerticalScrolling) {
        Rect vThumb = getVerticalThumbRect(absPos);
        Rect buttonUp = getVerticalButtonUpRect(absPos);
        Rect buttonDown = getVerticalButtonDownRect(absPos);
        
        if (vThumb.isValid() && vThumb.contains(position)) {
            m_verticalThumbHovered = true;
        } else if (buttonUp.contains(position)) {
            m_verticalButtonUpHovered = true;
        } else if (buttonDown.contains(position)) {
            m_verticalButtonDownHovered = true;
        }
    }
    
    if (needsHorizontalScrolling) {
        Rect hThumb = getHorizontalThumbRect(absPos);
        Rect buttonLeft = getHorizontalButtonLeftRect(absPos);
        Rect buttonRight = getHorizontalButtonRightRect(absPos);
        
        if (hThumb.isValid() && hThumb.contains(position)) {
            m_horizontalThumbHovered = true;
        } else if (buttonLeft.contains(position)) {
            m_horizontalButtonLeftHovered = true;
        } else if (buttonRight.contains(position)) {
            m_horizontalButtonRightHovered = true;
        }
    }
    
    if (m_dragMode == DragMode::VerticalThumb) {
        Rect scrollbarRect = getVerticalScrollbarRect(absPos);
        Rect thumbRect = getVerticalThumbRect(absPos);
        
        float maxThumbPos = scrollbarRect.height - thumbRect.height;
        float maxScroll = m_contentSize.y - contentArea.height;
        
        if (maxThumbPos > 0.0f && maxScroll > 0.0f) {
            float deltaY = position.y - m_dragStartPos.y;
            float scrollChange = (deltaY / maxThumbPos) * maxScroll;
            m_scrollY = m_dragStartScroll.y + scrollChange;
            clampScroll();
        }
        return true;
    }
    else if (m_dragMode == DragMode::HorizontalThumb) {
        Rect scrollbarRect = getHorizontalScrollbarRect(absPos);
        Rect thumbRect = getHorizontalThumbRect(absPos);
        
        float maxThumbPos = scrollbarRect.width - thumbRect.width;
        float maxScroll = m_contentSize.x - contentArea.width;
        
        if (maxThumbPos > 0.0f && maxScroll > 0.0f) {
            float deltaX = position.x - m_dragStartPos.x;
            float scrollChange = (deltaX / maxThumbPos) * maxScroll;
            m_scrollX = m_dragStartScroll.x + scrollChange;
            clampScroll();
        }
        return true;
    }
    else if (m_dragMode == DragMode::Content) {
        if (m_autoScrollEnabled) {
            handleAutoScroll(position, offset);
        }
        return true;
    }
    
    Rect absContentRect(absPos.x, absPos.y, contentArea.width, contentArea.height);
    
    if (absContentRect.contains(position)) {
        Vec2 contentPos = transformToContentCoords(position, offset);
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            if ((*it) && (*it)->isVisible() && (*it)->handleMouseMove(contentPos, Vec2())) {
                return true;
            }
        }
    }
    
    return false;
}

bool ScrollArea::handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset) {
    if (!m_isEnabled || !m_isVisible) return false;
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect absRect(absPos.x, absPos.y, m_bounds.width, m_bounds.height);
    
    if (!pressed) {
        if (m_dragMode != DragMode::None) {
            releaseMouse();
            m_dragMode = DragMode::None;
        }
        m_verticalButtonUpPressed = false;
        m_verticalButtonDownPressed = false;
        m_horizontalButtonLeftPressed = false;
        m_horizontalButtonRightPressed = false;
        
        if (absRect.contains(position)) {
            return true;
        }
        return false;
    }
    
    if (!absRect.contains(position)) {
        return false;
    }
    
    if (handleScrollbarInteraction(position, pressed, offset)) {
        if (m_dragMode == DragMode::Content) {
            releaseMouse();
            m_dragMode = DragMode::None;
        }
        
        if (pressed && m_dragMode != DragMode::None) {
            captureMouse();
        }
        return true;
    }
    
    Rect contentArea = getContentArea();
    Rect absContentRect(absPos.x, absPos.y, contentArea.width, contentArea.height);
    
    if (absContentRect.contains(position)) {
        Vec2 contentPos = transformToContentCoords(position, offset);
        
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            if ((*it) && (*it)->isVisible() && (*it)->handleMouseClick(contentPos, pressed, Vec2())) {
                return true;
            }
        }
        
        if (pressed) {
            m_dragMode = DragMode::Content;
            m_dragStartPos = position;
            m_dragStartScroll = Vec2(m_scrollX, m_scrollY);
            captureMouse();
        }
    }
    
    return false;
}

bool ScrollArea::handleScrollbarInteraction(const Vec2& position, bool pressed, const Vec2& offset) {
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect contentArea = getContentArea();
    
    bool needsVerticalScrolling = (m_contentSize.y > contentArea.height);
    bool needsHorizontalScrolling = (m_contentSize.x > contentArea.width);
    
    if (!pressed) {
        return false;
    }
    
    if (needsVerticalScrolling) {
        Rect vThumb = getVerticalThumbRect(absPos);
        if (vThumb.isValid() && vThumb.contains(position)) {
            m_dragMode = DragMode::VerticalThumb;
            m_dragStartPos = position;
            m_dragStartScroll = Vec2(m_scrollX, m_scrollY);
            return true;
        }
        
        Rect buttonUp = getVerticalButtonUpRect(absPos);
        if (buttonUp.contains(position)) {
            m_verticalButtonUpPressed = true;
            m_scrollY -= contentArea.height * 0.1f;
            clampScroll();
            return true;
        }
        
        Rect buttonDown = getVerticalButtonDownRect(absPos);
        if (buttonDown.contains(position)) {
            m_verticalButtonDownPressed = true;
            m_scrollY += contentArea.height * 0.1f;
            clampScroll();
            return true;
        }
        
        Rect vScrollbar = getVerticalScrollbarRect(absPos);
        Rect scrollbarInteractionRect(vScrollbar.x, vScrollbar.y, vScrollbar.width, vScrollbar.height);
        if (scrollbarInteractionRect.contains(position)) {
            float clickRatio = (position.y - scrollbarInteractionRect.y) / scrollbarInteractionRect.height;
            float maxScroll = m_contentSize.y - contentArea.height;
            m_scrollY = clickRatio * maxScroll;
            clampScroll();
            return true;
        }
    }
    
    if (needsHorizontalScrolling) {
        Rect hThumb = getHorizontalThumbRect(absPos);
        if (hThumb.isValid() && hThumb.contains(position)) {
            m_dragMode = DragMode::HorizontalThumb;
            m_dragStartPos = position;
            m_dragStartScroll = Vec2(m_scrollX, m_scrollY);
            return true;
        }
        
        Rect buttonLeft = getHorizontalButtonLeftRect(absPos);
        if (buttonLeft.contains(position)) {
            m_horizontalButtonLeftPressed = true;
            m_scrollX -= contentArea.width * 0.1f;
            clampScroll();
            return true;
        }
        
        Rect buttonRight = getHorizontalButtonRightRect(absPos);
        if (buttonRight.contains(position)) {
            m_horizontalButtonRightPressed = true;
            m_scrollX += contentArea.width * 0.1f;
            clampScroll();
            return true;
        }
        
        Rect hScrollbar = getHorizontalScrollbarRect(absPos);
        Rect scrollbarInteractionRect(hScrollbar.x, hScrollbar.y, hScrollbar.width, hScrollbar.height);
        if (scrollbarInteractionRect.contains(position)) {
            float clickRatio = (position.x - scrollbarInteractionRect.x) / scrollbarInteractionRect.width;
            float maxScroll = m_contentSize.x - contentArea.width;
            m_scrollX = clickRatio * maxScroll;
            clampScroll();
            return true;
        }
    }
    
    return false;
}

bool ScrollArea::handleAutoScroll(const Vec2& position, const Vec2& offset) {
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect contentArea = getContentArea();
    Rect absContentRect(absPos.x, absPos.y, contentArea.width, contentArea.height);
    
    float scrollSpeedX = 0.0f;
    float scrollSpeedY = 0.0f;
    
    if (position.y < absContentRect.y + AUTO_SCROLL_TRIGGER_ZONE) {
        float dist = absContentRect.y + AUTO_SCROLL_TRIGGER_ZONE - position.y;
        scrollSpeedY = -m_autoScrollSpeed * (dist / AUTO_SCROLL_TRIGGER_ZONE);
    }
    else if (position.y > absContentRect.y + absContentRect.height - AUTO_SCROLL_TRIGGER_ZONE) {
        float dist = position.y - (absContentRect.y + absContentRect.height - AUTO_SCROLL_TRIGGER_ZONE);
        scrollSpeedY = m_autoScrollSpeed * (dist / AUTO_SCROLL_TRIGGER_ZONE);
    }
    
    if (position.x < absContentRect.x + AUTO_SCROLL_TRIGGER_ZONE) {
        float dist = absContentRect.x + AUTO_SCROLL_TRIGGER_ZONE - position.x;
        scrollSpeedX = -m_autoScrollSpeed * (dist / AUTO_SCROLL_TRIGGER_ZONE);
    }
    else if (position.x > absContentRect.x + absContentRect.width - AUTO_SCROLL_TRIGGER_ZONE) {
        float dist = position.x - (absContentRect.x + absContentRect.width - AUTO_SCROLL_TRIGGER_ZONE);
        scrollSpeedX = m_autoScrollSpeed * (dist / AUTO_SCROLL_TRIGGER_ZONE);
    }
    
    if (scrollSpeedX != 0.0f || scrollSpeedY != 0.0f) {
        float deltaTime = 1.0f / 60.0f;
        m_scrollX += scrollSpeedX * deltaTime;
        m_scrollY += scrollSpeedY * deltaTime;
        clampScroll();
        return true;
    }
    
    return false;
}

void ScrollArea::clampScroll() {
    Rect contentArea = getContentArea();
    
    float maxScrollX = std::max(0.0f, m_contentSize.x - contentArea.width);
    float maxScrollY = std::max(0.0f, m_contentSize.y - contentArea.height);
    
    m_scrollX = std::clamp(m_scrollX, 0.0f, maxScrollX);
    m_scrollY = std::clamp(m_scrollY, 0.0f, maxScrollY);
}

Vec2 ScrollArea::transformToContentCoords(const Vec2& screenPos, const Vec2& offset) const {
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    return Vec2(
        screenPos.x - absPos.x + m_scrollX,
        screenPos.y - absPos.y + m_scrollY
    );
}

}
