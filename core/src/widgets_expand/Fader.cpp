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
/** @file Fader.cpp
    
    Implementation notes:
    - Fader cap position calculated from top of track (inverted from typical slider)
    - Drag offset preserved to maintain grab point during interaction
    - Value clamping occurs before mapping to prevent out-of-range positions
    - Major ticks correspond exactly to the 11 control points in FaderMapping
    - Minor tick calculation varies by dB range for optimal visual density
    - Background and cap sprites selected based on theme and enabled state
    - Nine-slice scaling applied to background with 1px margins
    - Cap rendering uses fixed-size sprite centered at calculated position
    - Scale rendering uses narrow bold font with negative letter spacing
    - All rendering performed by fader itself, theme only provides colors
*/

#include "YuchenUI/widgets_expand/Fader.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/theme/Theme.h"
#include "YuchenUI/text/IFontProvider.h"
#include "YuchenUI/core/UIContext.h"
#include "YuchenUI/core/Validation.h"
#include "YuchenUI/core/Assert.h"
#include <algorithm>
#include <cmath>

namespace YuchenUI {

//==========================================================================================
// FaderMapping Implementation

float FaderMapping::positionToDb(float position)
{
    position = std::clamp(position, 0.0f, 1.0f);
    const auto& points = getControlPoints();
    
    if (position >= points[0].first) return points[0].second;
    if (position <= points.back().first) return points.back().second;
    
    for (size_t i = 1; i < points.size(); ++i)
    {
        const auto& upper = points[i-1];
        const auto& lower = points[i];
        if (position <= upper.first && position >= lower.first)
        {
            float t = (position - lower.first) / (upper.first - lower.first);
            return lower.second + t * (upper.second - lower.second);
        }
    }
    
    return 0.0f;
}

float FaderMapping::dbToPosition(float db)
{
    db = std::clamp(db, -144.0f, 12.0f);
    const auto& points = getControlPoints();
    
    if (db >= points[0].second) return points[0].first;
    if (db <= points.back().second) return points.back().first;
    
    for (size_t i = 1; i < points.size(); ++i)
    {
        const auto& upper = points[i-1];
        const auto& lower = points[i];
        
        if (db <= upper.second && db >= lower.second)
        {
            float t = (db - lower.second) / (upper.second - lower.second);
            return lower.first + t * (upper.first - lower.first);
        }
    }
    
    return 0.0f;
}

//==========================================================================================
// Fader Implementation

Fader::Fader(UIContext* context, const Rect& bounds)
    : m_context(context)
    , m_colorTheme(FaderColorTheme::Normal)
    , m_showScale(true)
    , m_value(FaderMapping::dbToPosition(0.0f))  // Default to unity gain
    , m_isDragging(false)
    , m_dragOffset(0.0f)
    , m_majorTicks()
    , m_onValueChanged()
{
    Validation::AssertRect(bounds);
    YUCHEN_ASSERT(context);
    setBounds(bounds);
    setOwnerContext(context);
    setFocusPolicy(FocusPolicy::NoFocus);
    
    initializeMajorTicks();
}

Fader::~Fader()
{
}

//==========================================================================================
// Rendering

void Fader::addDrawCommands(RenderList& commandList, const Vec2& offset) const
{
    if (!m_isVisible) return;
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect trackBounds(absPos.x + SCALE_AREA_WIDTH, absPos.y, TRACK_AREA_WIDTH, m_bounds.height);
    
    // Render background track
    renderBackground(commandList, trackBounds);
    
    // Render scale if visible
    if (m_showScale)
    {
        Rect scaleBounds(absPos.x, absPos.y, SCALE_AREA_WIDTH, m_bounds.height);
        renderScale(commandList, scaleBounds, trackBounds);
    }
    
    // Render fader cap
    Vec2 localCapCenter = getCapCenterPosition();
    Vec2 capCenter(absPos.x + localCapCenter.x, absPos.y + localCapCenter.y);
    renderCap(commandList, capCenter);
    
    // Render focus indicator if needed
    if (hasFocus() && showsFocusIndicator() && m_isEnabled) drawFocusIndicator(commandList, offset);
}

//==========================================================================================
// Interaction

bool Fader::handleMouseMove(const Vec2& position, const Vec2& offset)
{
    if (!m_isEnabled || !m_isVisible) return false;
    
    if (m_isDragging)
    {
        Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
        
        float scaleTopY = absPos.y + CAP_CENTER_FROM_TOP;
        float scaleRangeHeight = m_bounds.height - CAP_CENTER_FROM_TOP - CAP_CENTER_FROM_BOTTOM;
        
        float targetCapY = position.y - m_dragOffset;
        targetCapY = std::clamp(targetCapY, scaleTopY, scaleTopY + scaleRangeHeight);
        
        float relativeY = (targetCapY - scaleTopY) / scaleRangeHeight;
        float newValue = std::clamp(1.0f - relativeY, 0.0f, 1.0f);
        
        setValue(newValue);
        
        return true;
    }
    
    return false;
}

bool Fader::handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset)
{
    if (!m_isEnabled || !m_isVisible) return false;
    
    if (!pressed && m_isDragging)
    {
        m_isDragging = false;
        m_dragOffset = 0.0f;
        releaseMouse();
        return true;
    }
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect trackRect(absPos.x + SCALE_AREA_WIDTH, absPos.y, TRACK_AREA_WIDTH, m_bounds.height);
    
    if (!trackRect.contains(position)) return false;
    if (pressed)
    {
        Vec2 capCenter = getCapCenterPosition();
        float capCenterY = absPos.y + capCenter.y;
        
        Rect capHitRect(absPos.x + SCALE_AREA_WIDTH,capCenterY - CAP_CENTER_FROM_TOP,TRACK_AREA_WIDTH,CAP_HEIGHT);
        
        if (capHitRect.contains(position))
        {
            m_isDragging = true;
            m_dragOffset = position.y - capCenterY;
            captureMouse();
            
            if (acceptsClickFocus()) setFocus(FocusReason::MouseFocusReason);
            
            return true;
        }
    }
    
    return false;
}
//==========================================================================================
// Value management

void Fader::setValue(float linearValue)
{
    float clampedValue = std::clamp(linearValue, 0.0f, 1.0f);
    
    if (std::abs(m_value - clampedValue) < 1e-6f) return;
    
    m_value = clampedValue;
    notifyValueChanged();
}

void Fader::setValueDb(float dbValue)
{
    setValue(FaderMapping::dbToPosition(dbValue));
}

float Fader::getValueDb() const
{
    return FaderMapping::positionToDb(m_value);
}

//==========================================================================================
// Visual appearance

void Fader::setColorTheme(FaderColorTheme theme)
{
    m_colorTheme = theme;
}

void Fader::setShowScale(bool visible)
{
    m_showScale = visible;
}

//==========================================================================================
// Scale data access

std::vector<FaderScaleTick> Fader::calculateMinorTicks(float startDb, float endDb) const
{
    std::vector<FaderScaleTick> minorTicks;
    
    if (startDb > endDb) std::swap(startDb, endDb);
    
    // Only render minor ticks above -40dB for visual clarity
    if (endDb <= -40.0f) return minorTicks;
    
    if (startDb < -40.0f) startDb = -40.0f;
    
    int subTickCount;
    if (startDb >= 0.0f && endDb <= 12.0f)
    { subTickCount = 6; }
    else if (startDb >= -40.0f && endDb <= 0.0f)
    { subTickCount = 5; }
    else { return minorTicks; }
    
    float startPos = FaderMapping::dbToPosition(startDb);
    float endPos = FaderMapping::dbToPosition(endDb);
    
    for (int i = 1; i < subTickCount; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(subTickCount);
        float subPos = startPos + t * (endPos - startPos);
        minorTicks.emplace_back(0.0f, subPos, "", false);
    }
    return minorTicks;
}

//==========================================================================================
// Callbacks

void Fader::setOnValueChanged(ValueChangedCallback callback)
{
    m_onValueChanged = callback;
}

//==========================================================================================
// Component interface

bool Fader::isValid() const
{
    return m_bounds.isValid() && m_value >= 0.0f && m_value <= 1.0f;
}

//==========================================================================================
// Focus events

void Fader::focusInEvent(FocusReason reason)
{
    // Fader visual state handled by active flag in rendering
}

void Fader::focusOutEvent(FocusReason reason)
{
    // Fader visual state handled by active flag in rendering
}

//==========================================================================================
// Internal rendering methods

void Fader::renderBackground(RenderList& cmdList, const Rect& trackBounds) const
{
    // Select background resource based on state
    std::string bgResource = "components/fader/fader_background_large_";
    bgResource += m_isEnabled ? "active" : "inactive";
    bgResource += ".png";
    
    // Background track is 5px wide (original sprite size), centered in 21px fader area
    constexpr float BG_TRACK_WIDTH = 5.0f;
    float centerOffset = (TRACK_AREA_WIDTH - BG_TRACK_WIDTH) * 0.5f;
    
    // Calculate background to cover scale range with 2px vertical extension
    constexpr float VERTICAL_EXTENSION = 2.0f;
    
    float scaleRangeHeight = trackBounds.height - CAP_CENTER_FROM_TOP - CAP_CENTER_FROM_BOTTOM;
    float bgStartY = trackBounds.y + CAP_CENTER_FROM_TOP - VERTICAL_EXTENSION;
    float bgHeight = scaleRangeHeight + VERTICAL_EXTENSION * 2.0f;
    
    Rect bgRect(trackBounds.x + centerOffset,bgStartY,BG_TRACK_WIDTH,bgHeight);
    
    // Render with nine-slice scaling
    NineSliceMargins bgMargins(2.0f, 2.0f, 2.0f, 2.0f);
    cmdList.drawImage(bgResource.c_str(), bgRect, ScaleMode::NineSlice, bgMargins);
}

void Fader::renderCap(RenderList& cmdList, const Vec2& capCenter) const
{
    // Select cap resource based on color theme and state
    std::string capResource = "components/fader/fader_large_";
    
    switch (m_colorTheme)
    {
        case FaderColorTheme::Red:    capResource += "red_";    break;
        case FaderColorTheme::Yellow: capResource += "yellow_"; break;
        default:                                                break;
    }
    capResource += m_isEnabled ? "active" : "inactive";
    capResource += ".png";
    // Calculate cap rectangle centered at specified position
    Rect capRect(capCenter.x - CAP_WIDTH * 0.5f,capCenter.y - CAP_CENTER_FROM_TOP,CAP_WIDTH,CAP_HEIGHT);

    cmdList.drawImage(capResource.c_str(), capRect, ScaleMode::Original);
}

void Fader::renderScale(RenderList& cmdList, const Rect& scaleBounds, const Rect& faderBounds) const
{
    UIStyle* style = m_context ? m_context->getCurrentStyle() : nullptr;
    IFontProvider* fontProvider = m_context ? m_context->getFontProvider() : nullptr;
    if (!style || !fontProvider) return;
    
    FaderColors colors = style->getFaderColors();
    FontFallbackChain fallbackChain(fontProvider->getDefaultNarrowBoldFont());
    
    constexpr float fontSize = 9.0f;
    constexpr float letterSpacing = -50.0f;
    constexpr float majorTickLength = 3.0f;
    constexpr float minorTickLength = 2.0f;
    constexpr float tickThickness = 0.5f;
    constexpr float tickEndOffset = 8.0f;
    
    float scaleRangeHeight = faderBounds.height - CAP_CENTER_FROM_TOP - CAP_CENTER_FROM_BOTTOM;
    
    // Render major tick marks and labels
    for (const auto& tick : m_majorTicks)
    {
        float tickY = faderBounds.y + CAP_CENTER_FROM_TOP + scaleRangeHeight * (1.0f - tick.position);
        
        // Draw major tick line
        float lineEndX = faderBounds.x + tickEndOffset;
        float lineStartX = lineEndX - majorTickLength;
        cmdList.drawLine(Vec2(lineStartX, tickY), Vec2(lineEndX, tickY),colors.scaleLineColor, tickThickness);
        
        // Draw label if present
        if (!tick.label.empty())
        {
            FontHandle primaryFont = fallbackChain.getPrimary();
            FontMetrics metrics = fontProvider->getFontMetrics(primaryFont, fontSize);
            Vec2 textSize = fontProvider->measureText(tick.label.c_str(), fontSize);
            
            // Calculate adjusted width accounting for letter spacing
            size_t charCount = std::count_if(tick.label.begin(), tick.label.end(),[](char c) { return (c & 0xC0) != 0x80; });
            
            float adjustedWidth = textSize.x;
            if (charCount > 1) adjustedWidth += (letterSpacing / 1000.0f) * fontSize * (charCount - 1);
            
            // Position text right-aligned to tick line
            float textX = lineStartX - 2.0f - adjustedWidth;
            float textY = tickY - textSize.y * 0.5f + metrics.ascender + 0.5f;
            
            cmdList.drawText(tick.label.c_str(), Vec2(std::round(textX), std::round(textY)),fallbackChain, fontSize, colors.scaleColor, letterSpacing);
        }
    }
    
    // Render minor tick marks between major ticks
    for (size_t i = 0; i < m_majorTicks.size() - 1; ++i)
    {
        const auto& currentTick = m_majorTicks[i];
        const auto& nextTick = m_majorTicks[i + 1];
        
        // Calculate minor ticks for this segment
        std::vector<FaderScaleTick> minorTicks = calculateMinorTicks(currentTick.db, nextTick.db);
        
        // Draw each minor tick
        for (const auto& minorTick : minorTicks)
        {
            float tickY = faderBounds.y + CAP_CENTER_FROM_TOP + scaleRangeHeight * (1.0f - minorTick.position);
            
            // Draw minor tick line
            float lineEndX = faderBounds.x + tickEndOffset;
            float lineStartX = lineEndX - minorTickLength;
            cmdList.drawLine(Vec2(lineStartX, tickY), Vec2(lineEndX, tickY),colors.subScaleColor, tickThickness);
        }
    }
}

Vec2 Fader::getCapCenterPosition() const
{
    float scaleRangeHeight = m_bounds.height - CAP_CENTER_FROM_TOP - CAP_CENTER_FROM_BOTTOM;
    float capCenterY = CAP_CENTER_FROM_TOP + scaleRangeHeight * (1.0f - m_value);
    return Vec2(SCALE_AREA_WIDTH + TRACK_AREA_WIDTH * 0.5f, capCenterY);
}

void Fader::initializeMajorTicks()
{
    m_majorTicks.clear();
    
    const auto& controlPoints = FaderMapping::getControlPoints();
    
    for (const auto& point : controlPoints)
    {
        float db = point.second;
        float position = point.first;
        
        std::string label;
        if (db == -144.0f)
        {
            label = "\xE2\x88\x9E";  // UTF-8 infinity symbol
        } else
        {
            label = std::to_string(static_cast<int>(std::abs(db)));
        }
        m_majorTicks.emplace_back(db, position, label, true);
    }
}

void Fader::notifyValueChanged()
{
    if (m_onValueChanged) m_onValueChanged(getValueDb());
}

} // namespace YuchenUI
