// CheckBox.cpp
#include "YuchenUI/widgets/CheckBox.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/text/IFontProvider.h"
#include "YuchenUI/theme/ThemeManager.h"
#include "YuchenUI/core/Validation.h"
#include "YuchenUI/core/Config.h"
#include "YuchenUI/core/Assert.h"
#include "YuchenUI/core/UIContext.h"

namespace YuchenUI {

CheckBox::CheckBox(const Rect& bounds)
    : m_bounds(bounds)
    , m_state(CheckBoxState::Unchecked)
    , m_text()
    , m_fontSize(Config::Font::DEFAULT_SIZE)
    , m_textColor()
    , m_hasCustomTextColor(false)
    , m_isHovered(false)
    , m_isPressed(false)
    , m_stateChangedCallback(nullptr)
{
    Validation::AssertRect(bounds);
    setFocusPolicy(FocusPolicy::StrongFocus);
}

CheckBox::~CheckBox() {
}

void CheckBox::addDrawCommands(RenderList& commandList, const Vec2& offset) const {
    if (!m_isVisible) return;
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    
    // Get style via UIContext instead of deprecated singleton
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    YUCHEN_ASSERT(style);
    
    CheckBoxDrawInfo info;
    info.bounds = Rect(absPos.x, absPos.y, CHECKBOX_SIZE, CHECKBOX_SIZE);
    info.state = m_state;
    info.isHovered = m_isHovered;
    info.isEnabled = m_isEnabled;
    
    style->drawCheckBox(info, commandList);
    
    if (!m_text.empty()) {
        IFontProvider* fontProvider = m_ownerContext ? m_ownerContext->getFontProvider() : nullptr;
        YUCHEN_ASSERT(fontProvider);
        
        FontHandle westernFont = style->getDefaultLabelFont();
        FontHandle chineseFont = fontProvider->getDefaultCJKFont();
        Vec4 textColor = m_hasCustomTextColor ? m_textColor : style->getDefaultTextColor();
        
        if (!m_isEnabled) {
            textColor = Vec4::FromRGBA(255,255,255,255);
        }
        
        FontMetrics metrics = fontProvider->getFontMetrics(westernFont, m_fontSize);
        float textX = absPos.x + CHECKBOX_SIZE + TEXT_SPACING;
        float textY = absPos.y + (CHECKBOX_SIZE - metrics.lineHeight) * 0.5f + metrics.ascender;
        
        commandList.drawText(m_text.c_str(), Vec2(textX, textY),
                           westernFont, chineseFont, m_fontSize, textColor);
    }
    
    drawFocusIndicator(commandList, offset);
}

bool CheckBox::handleMouseMove(const Vec2& position, const Vec2& offset) {
    if (!m_isEnabled || !m_isVisible) return false;
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect absRect(absPos.x, absPos.y, m_bounds.width, m_bounds.height);
    
    bool wasHovered = m_isHovered;
    m_isHovered = absRect.contains(position);
    
    return wasHovered != m_isHovered;
}

bool CheckBox::handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset) {
    if (!m_isEnabled || !m_isVisible) return false;
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect absRect(absPos.x, absPos.y, m_bounds.width, m_bounds.height);
    
    if (pressed) {
        if (absRect.contains(position)) {
            m_isPressed = true;
            requestFocus();
            return true;
        }
    } else {
        if (m_isPressed) {
            m_isPressed = false;
            if (absRect.contains(position)) {
                toggleState();
            }
            return true;
        }
    }
    
    return false;
}

bool CheckBox::handleKeyPress(const Event& event) {
    if (!m_isEnabled || !m_isVisible) return false;
    
    YUCHEN_ASSERT(event.type == EventType::KeyPressed || event.type == EventType::KeyReleased);
    if (event.type == EventType::KeyReleased) return false;
    
    if (event.key.key == KeyCode::Space) {
        toggleState();
        return true;
    }
    
    return false;
}

void CheckBox::setState(CheckBoxState state) {
    if (m_state != state) {
        m_state = state;
        if (m_stateChangedCallback) {
            m_stateChangedCallback(m_state);
        }
    }
}

void CheckBox::setChecked(bool checked) {
    setState(checked ? CheckBoxState::Checked : CheckBoxState::Unchecked);
}

void CheckBox::setText(const std::string& text) {
    m_text = text;
}

void CheckBox::setText(const char* text) {
    YUCHEN_ASSERT(text);
    m_text = text;
}

void CheckBox::setFontSize(float fontSize) {
    if (fontSize >= Config::Font::MIN_SIZE && fontSize <= Config::Font::MAX_SIZE) {
        m_fontSize = fontSize;
    }
}

void CheckBox::setTextColor(const Vec4& color) {
    Validation::AssertColor(color);
    m_textColor = color;
    m_hasCustomTextColor = true;
}

Vec4 CheckBox::getTextColor() const {
    if (m_hasCustomTextColor) {
        return m_textColor;
    }
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    return style ? style->getDefaultTextColor() : Vec4();
}

void CheckBox::resetTextColor() {
    m_hasCustomTextColor = false;
    m_textColor = Vec4();
}

void CheckBox::setBounds(const Rect& bounds) {
    Validation::AssertRect(bounds);
    m_bounds = bounds;
}

void CheckBox::setStateChangedCallback(CheckBoxStateChangedCallback callback) {
    m_stateChangedCallback = callback;
}

bool CheckBox::isValid() const {
    return m_bounds.isValid() &&
           m_fontSize >= Config::Font::MIN_SIZE &&
           m_fontSize <= Config::Font::MAX_SIZE;
}

void CheckBox::toggleState() {
    if (m_state == CheckBoxState::Indeterminate || m_state == CheckBoxState::Unchecked) {
        setState(CheckBoxState::Checked);
    } else {
        setState(CheckBoxState::Unchecked);
    }
}

Rect CheckBox::getCheckBoxRect(const Vec2& absPos) const {
    return Rect(absPos.x, absPos.y, CHECKBOX_SIZE, CHECKBOX_SIZE);
}

Rect CheckBox::getTextRect(const Vec2& absPos) const {
    float textX = absPos.x + CHECKBOX_SIZE + TEXT_SPACING;
    float textWidth = m_bounds.width - CHECKBOX_SIZE - TEXT_SPACING;
    return Rect(textX, absPos.y, textWidth, CHECKBOX_SIZE);
}

}
