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

#include "YuchenUI/widgets/SpinBox.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/text/FontManager.h"
#include "YuchenUI/theme/ThemeManager.h"
#include "YuchenUI/core/Validation.h"
#include "YuchenUI/core/Config.h"
#include "YuchenUI/core/Assert.h"
#include "YuchenUI/core/UIContext.h"

#include <sstream>
#include <iomanip>
#include <cmath>
#include <limits>
#include <algorithm>

namespace YuchenUI {

//==========================================================================================
// Construction and Destruction

SpinBox::SpinBox(const Rect& bounds)
    : m_value(0.0)
    , m_minValue(-std::numeric_limits<double>::max())
    , m_maxValue(std::numeric_limits<double>::max())
    , m_step(1.0)
    , m_precision(2)
    , m_suffix()
    , m_fontSize(Config::Font::DEFAULT_SIZE)
    , m_fontChain()
    , m_hasCustomFont(false)
    , m_horizontalAlignment(TextAlignment::Center)
    , m_verticalAlignment(VerticalAlignment::Middle)
    , m_isReadOnly(false)
    , m_isEditing(false)
    , m_isHovered(false)
    , m_isDragging(false)
    , m_dragStartPos()
    , m_dragStartValue(0.0)
    , m_inputBuffer()
    , m_cursorPosition(0)
    , m_showCursor(true)
    , m_cursorBlinkTimer(0.0f)
    , m_valueChangedCallback(nullptr)
    , m_paddingLeft(5.0f)
    , m_paddingTop(5.0f)
    , m_paddingRight(5.0f)
    , m_paddingBottom(5.0f)
    , m_hasBackground(true)
{
    Validation::AssertRect(bounds);
    setBounds(bounds);
    setFocusPolicy(FocusPolicy::StrongFocus);
}

SpinBox::~SpinBox() {}

//==========================================================================================
// Rendering

void SpinBox::addDrawCommands(RenderList& commandList, const Vec2& offset) const
{
    if (!m_isVisible) return;
    IFontProvider* fontProvider = m_ownerContext ? m_ownerContext->getFontProvider() : nullptr;
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    YUCHEN_ASSERT(fontProvider);
    YUCHEN_ASSERT(style);
    Rect absRect(m_bounds.x + offset.x, m_bounds.y + offset.y, m_bounds.width, m_bounds.height);
    SpinBoxColors colors = style->getSpinBoxColors();
    if (m_hasBackground) commandList.fillRect(absRect, colors.background, CornerRadius(2.0f));
    std::string displayText;
    if   (m_isEditing) { displayText = m_inputBuffer;}
    else { displayText = formatValueWithSuffix();}
    if (displayText.empty())
    {
        if (m_isEditing) drawFocusIndicator(commandList, offset);
        return;
    }
    FontFallbackChain fallbackChain = m_hasCustomFont ? m_fontChain : style->getDefaultLabelFontChain();
    FontHandle primaryFont = fallbackChain.getPrimary();
    FontMetrics metrics = fontProvider->getFontMetrics(primaryFont, m_fontSize);
    Vec2 textSize = fontProvider->measureText(displayText.c_str(), m_fontSize);
    float contentWidth = absRect.width - m_paddingLeft - m_paddingRight;
    float contentHeight = absRect.height - m_paddingTop - m_paddingBottom;
    float textX = absRect.x + m_paddingLeft;
    float textY = absRect.y + m_paddingTop;
    switch (m_horizontalAlignment)
    {
        case TextAlignment::Left:   textX += 0.0f;                                  break;
        case TextAlignment::Center: textX += (contentWidth - textSize.x) * 0.5f;    break;
        case TextAlignment::Right:  textX += contentWidth - textSize.x;             break;
        case TextAlignment::Justify:textX += 0.0f;                                  break;
    }
    switch (m_verticalAlignment)
    {
        case VerticalAlignment::Top:textY += metrics.ascender;                                                  break;
        case VerticalAlignment::Middle:textY += (contentHeight - metrics.lineHeight) * 0.5f + metrics.ascender; break;
        case VerticalAlignment::Bottom:textY += contentHeight - (metrics.lineHeight - metrics.ascender);        break;
        case VerticalAlignment::Baseline: textY += metrics.ascender;                                            break;
    }
    Vec4 textColor = colors.textColor;
    if (m_isEditing)
    {
        Rect textBgRect(textX - 1.0f,absRect.y + m_paddingTop + (contentHeight - metrics.lineHeight) * 0.5f - 1.0f,textSize.x + 2.0f,metrics.lineHeight + 2.0f);
        commandList.fillRect(textBgRect, colors.textEditingBackground);
        textColor = colors.textEditingColor;
    }
    commandList.drawText(displayText.c_str(), Vec2(textX, textY), fallbackChain, m_fontSize, textColor);
    if (m_isEditing) drawFocusIndicator(commandList, offset);
}

//==========================================================================================
// Mouse Event Handling

bool SpinBox::handleMouseMove(const Vec2& position, const Vec2& offset)
{
    if (!m_isEnabled || !m_isVisible) return false;
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect absRect(absPos.x, absPos.y, m_bounds.width, m_bounds.height);
    
    bool wasHovered = m_isHovered;
    m_isHovered = absRect.contains(position);
    
    if (m_isDragging && m_isEditing)
    {
        adjustValueByDrag(position);
        return true;
    }
    
    return wasHovered != m_isHovered;
}

bool SpinBox::handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset)
{
    if (!m_isEnabled || !m_isVisible || m_isReadOnly) return false;
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect absRect(absPos.x, absPos.y, m_bounds.width, m_bounds.height);
    
    if (pressed)
    {
        if (absRect.contains(position))
        {
            if (!m_isEditing)
            {
                enterEditMode();
                requestFocus();
            }
            m_isDragging = true;
            m_dragStartPos = position;
            m_dragStartValue = m_value;
            captureMouse();
            return true;
        }
        return false;
    }
    else
    {
        if (m_isDragging)
        {
            m_isDragging = false;
            releaseMouse();
            return true;
        }
    }
    return false;
}

bool SpinBox::handleMouseWheel(const Vec2& delta, const Vec2& position, const Vec2& offset)
{
    if (!m_isEnabled || !m_isVisible || !m_isEditing || m_isReadOnly) return false;
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect absRect(absPos.x, absPos.y, m_bounds.width, m_bounds.height);
    if (!absRect.contains(position)) return false;
    adjustValueByStep(-delta.y);
    return true;
}

//==========================================================================================
// Keyboard Event Handling

bool SpinBox::handleKeyPress(const Event& event)
{
    if (!m_isEditing || !m_isEnabled || m_isReadOnly) return false;
    YUCHEN_ASSERT(event.type == EventType::KeyPressed || event.type == EventType::KeyReleased);
    if (event.type == EventType::KeyReleased) return false;
    bool hasCmd = event.key.modifiers.hasCommand();
    bool hasCtrl = event.key.modifiers.hasControl();
    bool hasModifier = hasCmd || hasCtrl;
    switch (event.key.key) {
        case KeyCode::UpArrow:adjustValueByStep(1.0);       return true;
        case KeyCode::DownArrow:adjustValueByStep(-1.0);    return true;
        case KeyCode::LeftArrow:moveCursor(-1);             return true;
        case KeyCode::RightArrow:moveCursor(1);             return true;
        case KeyCode::Home:moveCursorToStart();             return true;
        case KeyCode::End:moveCursorToEnd();                return true;
        case KeyCode::Backspace:deleteCharBeforeCursor();   return true;
        case KeyCode::Delete:deleteCharAfterCursor();       return true;
        case KeyCode::A:
            if (hasModifier)
            {
                selectAll();
                return true;
            }
            break;
        case KeyCode::Return:
        case KeyCode::Enter:
        case KeyCode::KeypadEnter:applyValue();             return true;
        case KeyCode::Escape:
            m_inputBuffer = formatValue();
            m_cursorPosition = m_inputBuffer.length();
            exitEditMode();
            return true;
        default:break;
    }
    return false;
}

bool SpinBox::handleTextInput(uint32_t codepoint)
{
    if (!m_isEditing || !m_isEnabled || m_isReadOnly) return false;
    if (codepoint < 32 || codepoint == 127) return false;
    char c = static_cast<char>(codepoint);
    if (isValidChar(c) && canInsertChar(c))
    {
        insertCharAtCursor(c);
        return true;
    }
    return false;
}

//==========================================================================================
// Update Loop

void SpinBox::update(float deltaTime)
{
    if (!m_isEditing) return;
    
    m_cursorBlinkTimer += deltaTime;
    if (m_cursorBlinkTimer >= CURSOR_BLINK_INTERVAL)
    {
        m_cursorBlinkTimer = 0.0f;
        m_showCursor = !m_showCursor;
    }
}

//==========================================================================================
// Input Method Support

Rect SpinBox::getInputMethodCursorRect() const
{
    return Rect();
}

//==========================================================================================
// Value Management

void SpinBox::setValue(double value)
{
    m_value = value;
    clampValue();
    
    if (m_isEditing) {
        m_inputBuffer = formatValue();
        m_cursorPosition = m_inputBuffer.length();
    }
}

void SpinBox::setMinValue(double min)
{
    m_minValue = min;
    clampValue();
}

void SpinBox::setMaxValue(double max)
{
    m_maxValue = max;
    clampValue();
}

void SpinBox::setStep(double step)
{
    m_step = std::abs(step);
}

void SpinBox::setPrecision(int precision)
{
    m_precision = std::max(0, std::min(10, precision));
}

void SpinBox::setSuffix(const std::string& suffix)
{
    m_suffix = suffix;
}

void SpinBox::setValueChangedCallback(SpinBoxValueChangedCallback callback)
{
    m_valueChangedCallback = callback;
}

//==========================================================================================
// Text Style

void SpinBox::setFontSize(float fontSize) {
    if (fontSize >= Config::Font::MIN_SIZE && fontSize <= Config::Font::MAX_SIZE)
    {
        m_fontSize = fontSize;
    }
}

//==========================================================================================
// Font API

void SpinBox::setFont(FontHandle fontHandle) {
    IFontProvider* fontProvider = m_ownerContext ? m_ownerContext->getFontProvider() : nullptr;
    
    if (!fontProvider || !fontProvider->isValidFont(fontHandle)) return;
    
    FontHandle cjkFont = fontProvider->getDefaultCJKFont();
    m_fontChain = FontFallbackChain(fontHandle, cjkFont);
    m_hasCustomFont = true;
}

void SpinBox::setFontChain(const FontFallbackChain& chain)
{
    if (!chain.isValid()) return;
    
    m_fontChain = chain;
    m_hasCustomFont = true;
}

FontFallbackChain SpinBox::getFontChain() const
{
    if (m_hasCustomFont) return m_fontChain;
    
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    return style ? style->getDefaultLabelFontChain() : FontFallbackChain();
}

void SpinBox::resetFont() {
    m_fontChain.clear();
    m_hasCustomFont = false;
}

//==========================================================================================
// Text Alignment API

void SpinBox::setAlignment(TextAlignment horizontal, VerticalAlignment vertical)
{
    m_horizontalAlignment = horizontal;
    m_verticalAlignment = vertical;
}

void SpinBox::setHorizontalAlignment(TextAlignment alignment)
{
    m_horizontalAlignment = alignment;
}

void SpinBox::setVerticalAlignment(VerticalAlignment alignment)
{
    m_verticalAlignment = alignment;
}

//==========================================================================================
// Visual Configuration

void SpinBox::setHasBackground(bool hasBackground)
{
    m_hasBackground = hasBackground;
}

void SpinBox::setReadOnly(bool readOnly)
{
    if (m_isReadOnly == readOnly) return;
    
    m_isReadOnly = readOnly;
    
    if (readOnly && m_isEditing) exitEditMode();
    
    if (readOnly)
    {
        setFocusPolicy(FocusPolicy::NoFocus);
    }
    else
    {
        setFocusPolicy(FocusPolicy::StrongFocus);
    }
}

void SpinBox::setFocusable(bool focusable)
{
    setFocusPolicy(focusable ? FocusPolicy::StrongFocus : FocusPolicy::NoFocus);
}

//==========================================================================================
// Validation

bool SpinBox::isValid() const
{
    return m_bounds.isValid() &&
           m_fontSize >= Config::Font::MIN_SIZE &&
           m_fontSize <= Config::Font::MAX_SIZE;
}

//==========================================================================================
// Focus Events

void SpinBox::focusInEvent(FocusReason reason)
{
    if (!m_isEditing && !m_isReadOnly) enterEditMode();
}

void SpinBox::focusOutEvent(FocusReason reason)
{
    exitEditMode();
}

//==========================================================================================
// Private: Edit Mode Management

void SpinBox::enterEditMode()
{
    if (m_isEditing) return;
    
    m_isEditing = true;
    m_inputBuffer = formatValue();
    m_cursorPosition = m_inputBuffer.length();
    m_showCursor = true;
    m_cursorBlinkTimer = 0.0f;
}

void SpinBox::exitEditMode()
{
    if (!m_isEditing) return;
    
    applyValue();
    
    m_isEditing = false;
    m_isDragging = false;
    m_inputBuffer.clear();
    m_cursorPosition = 0;
}

void SpinBox::applyValue()
{
    double newValue = parseInputBuffer();
    if (std::isnan(newValue) || std::isinf(newValue)) return;
    if (newValue != m_value)
    {
        m_value = newValue;
        clampValue();
        if (m_valueChangedCallback)  m_valueChangedCallback(m_value);
    }
}

void SpinBox::clampValue()
{
    m_value = std::max(m_minValue, std::min(m_maxValue, m_value));
}

//==========================================================================================
// Private: Value Formatting

std::string SpinBox::formatValue() const
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(m_precision) << m_value;
    std::string result = oss.str();
    
    if (m_precision > 0) {
        size_t dotPos = result.find('.');
        if (dotPos != std::string::npos) {
            size_t end = result.length() - 1;
            while (end > dotPos && result[end] == '0')
            {
                end--;
            }
            if (result[end] == '.')
            {
                end--;
            }
            result = result.substr(0, end + 1);
        }
    }
    return result;
}

std::string SpinBox::formatValueWithSuffix() const
{
    std::string result = formatValue();
    if (!m_suffix.empty()) result += " " + m_suffix;
    return result;
}

//==========================================================================================
// Private: Text Input Validation

bool SpinBox::isValidChar(char c) const
{
    return (c >= '0' && c <= '9') || c == '.' || c == '-';
}

bool SpinBox::canInsertChar(char c) const
{
    if (c == '-')
    {
        return m_cursorPosition == 0 && (m_inputBuffer.empty() || m_inputBuffer[0] != '-') && m_minValue < 0.0;
    }
    
    if (c == '.')
    {
        return m_inputBuffer.find('.') == std::string::npos;
    }
    return true;
}

//==========================================================================================
// Private: Cursor and Text Editing

void SpinBox::insertCharAtCursor(char c)
{
    m_inputBuffer.insert(m_cursorPosition, 1, c);
    m_cursorPosition++;
    
    m_showCursor = true;
    m_cursorBlinkTimer = 0.0f;
}

void SpinBox::deleteCharBeforeCursor()
{
    if (m_cursorPosition > 0) {
        m_inputBuffer.erase(m_cursorPosition - 1, 1);
        m_cursorPosition--;
        
        m_showCursor = true;
        m_cursorBlinkTimer = 0.0f;
    }
}

void SpinBox::deleteCharAfterCursor()
{
    if (m_cursorPosition < m_inputBuffer.length()) {
        m_inputBuffer.erase(m_cursorPosition, 1);
        
        m_showCursor = true;
        m_cursorBlinkTimer = 0.0f;
    }
}

void SpinBox::moveCursor(int delta)
{
    int newPos = static_cast<int>(m_cursorPosition) + delta;
    m_cursorPosition = std::max(0, std::min(newPos, static_cast<int>(m_inputBuffer.length())));
    
    m_showCursor = true;
    m_cursorBlinkTimer = 0.0f;
}

void SpinBox::moveCursorToStart()
{
    m_cursorPosition = 0;
    m_showCursor = true;
    m_cursorBlinkTimer = 0.0f;
}

void SpinBox::moveCursorToEnd()
{
    m_cursorPosition = m_inputBuffer.length();
    m_showCursor = true;
    m_cursorBlinkTimer = 0.0f;
}

void SpinBox::selectAll()
{
    m_cursorPosition = m_inputBuffer.length();
}

//==========================================================================================
// Private: Value Adjustment

void SpinBox::adjustValueByDrag(const Vec2& currentPos)
{
    float deltaX = currentPos.x - m_dragStartPos.x;
    float deltaY = m_dragStartPos.y - currentPos.y;
    float totalDelta = (deltaX + deltaY) * DRAG_SENSITIVITY;
    double newValue = m_dragStartValue + totalDelta * m_step;
    if (newValue != m_value)
    {
        m_value = newValue;
        clampValue();
        m_inputBuffer = formatValue();
        m_cursorPosition = m_inputBuffer.length();
        
        if (m_valueChangedCallback) m_valueChangedCallback(m_value);
    }
}

void SpinBox::adjustValueByStep(double multiplier)
{
    double newValue = m_value + m_step * multiplier;
    
    if (newValue != m_value)
    {
        m_value = newValue;
        clampValue();
        
        m_inputBuffer = formatValue();
        m_cursorPosition = m_inputBuffer.length();
        
        if (m_valueChangedCallback) m_valueChangedCallback(m_value);
    }
}

double SpinBox::parseInputBuffer() const
{
    if (m_inputBuffer.empty()) return 0.0;
    
    try
    {
        size_t pos;
        double value = std::stod(m_inputBuffer, &pos);
        return value;
    }
    catch (...)
    {
        return m_value;
    }
}

} // namespace YuchenUI
