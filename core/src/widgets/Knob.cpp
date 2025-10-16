/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Widgets module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

//==========================================================================================
/** @file Knob.cpp
    
    Implementation notes:
    - Default configuration: 29-frame vertical sprite sheet, 34x36 logical pixels
    - Resources automatically selected based on theme (Dark/Classical)
    - Active state determined by focus or dragging
    - Drag direction: upward increases value, downward decreases value
    - Drag distance of 100 pixels spans full value range (scaled by sensitivity)
    - Double-click detection uses simple timing threshold (300ms)
    - Value changes trigger immediate frame update and callback
    - Frame mapping: linear interpolation from value range to frame range
    - Theme integration: rendering delegated to UIStyle::drawKnob()
*/

#include "YuchenUI/widgets/Knob.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/core/Validation.h"
#include "YuchenUI/core/Assert.h"
#include "YuchenUI/core/UIContext.h"
#include <algorithm>
#include <cmath>

namespace YuchenUI {

//==========================================================================================
// Constants

namespace {
    constexpr float DEFAULT_DRAG_RANGE = 100.0f;  // Pixels to drag for full range
}

//==========================================================================================
// Lifecycle

Knob::Knob(const Rect& bounds)
    : m_knobType(KnobType::NoCentered)
    , m_value(0.0f)
    , m_minValue(0.0f)
    , m_maxValue(1.0f)
    , m_defaultValue(0.0f)
    , m_sensitivity(1.0f)
    , m_doubleClickResetEnabled(true)
    , m_isDragging(false)
    , m_dragStartY(0.0f)
    , m_dragStartValue(0.0f)
    , m_onValueChanged()
{
    Validation::AssertRect(bounds);
    setBounds(bounds);
    
    // Enable focus to support active state
    setFocusPolicy(FocusPolicy::ClickFocus);
}

Knob::~Knob()
{
}

//==========================================================================================
// Rendering

void Knob::addDrawCommands(RenderList& commandList, const Vec2& offset) const
{
    if (!m_isVisible) return;
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    YUCHEN_ASSERT(style);
    KnobDrawInfo info;
    info.bounds = Rect(m_bounds.x + offset.x,m_bounds.y + offset.y,m_bounds.width,m_bounds.height);
    info.currentFrame = valueToFrame();
    info.frameCount = DEFAULT_FRAME_COUNT;
    info.frameSize = Vec2(DEFAULT_FRAME_WIDTH, DEFAULT_FRAME_HEIGHT);
    info.type = m_knobType;
    info.isActive = m_isEnabled;
    info.isEnabled = m_isEnabled;
    style->drawKnob(info, commandList);
    if (hasFocus() && showsFocusIndicator() && m_isEnabled) drawFocusIndicator(commandList, offset);
}

//==========================================================================================
// Interaction

bool Knob::handleMouseMove(const Vec2& position, const Vec2& offset)
{
    if (!m_isEnabled || !m_isVisible) return false;
    
    // Handle drag operation
    if (m_isDragging)
    {
        // Calculate delta from drag start
        float deltaY = m_dragStartY - position.y;  // Inverted: up is positive
        
        // Convert pixel delta to value delta
        float valueRange = m_maxValue - m_minValue;
        float valueDelta = (deltaY / DEFAULT_DRAG_RANGE) * valueRange * m_sensitivity;
        
        // Apply new value
        setValue(m_dragStartValue + valueDelta);
        
        return true;
    }
    
    return false;
}

bool Knob::handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset)
{
    if (!m_isEnabled || !m_isVisible) return false;
    
    if (!pressed && m_isDragging)
    {
        m_isDragging = false;
        releaseMouse();
        return true;
    }
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect absRect(absPos.x, absPos.y, m_bounds.width, m_bounds.height);
    bool isInBounds = absRect.contains(position);
    
    if (pressed && isInBounds)
    {
        m_isDragging = true;
        m_dragStartY = position.y;
        m_dragStartValue = m_value;
        captureMouse();
        
        if (acceptsClickFocus()) {
            setFocus(FocusReason::MouseFocusReason);
        }
        
        return true;
    }
    
    return false;
}

//==========================================================================================
// Value management

void Knob::setValue(float value)
{
    // Clamp value to range
    float clampedValue = std::max(m_minValue, std::min(value, m_maxValue));
    
    // Check if value actually changed
    if (std::abs(clampedValue - m_value) < 1e-6f) return;
    
    m_value = clampedValue;
    
    // Notify callback
    notifyValueChanged();
}

void Knob::setValueRange(float minValue, float maxValue)
{
    YUCHEN_ASSERT(minValue < maxValue);
    
    m_minValue = minValue;
    m_maxValue = maxValue;
    
    // Clamp current value to new range
    setValue(m_value);
}

void Knob::setDefaultValue(float value)
{
    m_defaultValue = std::max(m_minValue, std::min(value, m_maxValue));
}

void Knob::resetToDefault()
{
    setValue(m_defaultValue);
}

//==========================================================================================
// Knob type and appearance

void Knob::setKnobType(KnobType type)
{
    m_knobType = type;
}

//==========================================================================================
// Interaction settings

void Knob::setSensitivity(float sensitivity)
{
    YUCHEN_ASSERT(sensitivity > 0.0f);
    m_sensitivity = sensitivity;
}

void Knob::setDoubleClickResetEnabled(bool enabled)
{
    m_doubleClickResetEnabled = enabled;
}

//==========================================================================================
// Callbacks

void Knob::setOnValueChanged(ValueChangedCallback callback)
{
    m_onValueChanged = callback;
}

//==========================================================================================
// Component interface

bool Knob::isValid() const
{
    return m_bounds.isValid() &&
           m_minValue < m_maxValue &&
           m_value >= m_minValue && m_value <= m_maxValue;
}

//==========================================================================================
// Focus events

void Knob::focusInEvent(FocusReason reason)
{
    // Knob becomes active when focused
    // Visual state will update on next render
}

void Knob::focusOutEvent(FocusReason reason)
{
    // Knob becomes inactive when focus lost
    // Visual state will update on next render
}

//==========================================================================================
// Internal

int Knob::valueToFrame() const
{
    // Normalize value to [0, 1]
    float normalizedValue = (m_value - m_minValue) / (m_maxValue - m_minValue);
    
    // Map to frame index [0, frameCount-1]
    int frameIndex = static_cast<int>(normalizedValue * (DEFAULT_FRAME_COUNT - 1) + 0.5f);
    
    // Clamp to valid range
    return std::max(0, std::min(frameIndex, DEFAULT_FRAME_COUNT - 1));
}

void Knob::notifyValueChanged()
{
    if (m_onValueChanged)
    {
        m_onValueChanged(m_value);
    }
}

} // namespace YuchenUI
