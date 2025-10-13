#include "YuchenUI/widgets/RadioButton.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/text/IFontProvider.h"
#include "YuchenUI/theme/IThemeProvider.h"
#include "YuchenUI/theme/Theme.h"
#include "YuchenUI/core/Validation.h"
#include "YuchenUI/core/Config.h"
#include "YuchenUI/core/Assert.h"
#include "YuchenUI/core/UIContext.h"

namespace YuchenUI {

RadioButton::RadioButton(const Rect& bounds)
    : m_bounds(bounds)
    , m_isChecked(false)
    , m_text()
    , m_fontSize(Config::Font::DEFAULT_SIZE)
    , m_textColor()
    , m_hasCustomTextColor(false)
    , m_isHovered(false)
    , m_isPressed(false)
    , m_group(nullptr)
    , m_checkedCallback(nullptr)
{
    Validation::AssertRect(bounds);
    setFocusPolicy(FocusPolicy::StrongFocus);
}

RadioButton::~RadioButton() {
    if (m_group) {
        m_group->removeButton(this);
    }
}

void RadioButton::addDrawCommands(RenderList& commandList, const Vec2& offset) const {
    if (!m_isVisible) return;
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    YUCHEN_ASSERT(style);
    
    RadioButtonDrawInfo info;
    info.bounds = Rect(absPos.x, absPos.y, RADIO_SIZE, RADIO_SIZE);
    info.isChecked = m_isChecked;
    info.isHovered = m_isHovered;
    info.isEnabled = m_isEnabled;
    
    style->drawRadioButton(info, commandList);
    
    if (!m_text.empty()) {
        IFontProvider* fontProvider = m_ownerContext ? m_ownerContext->getFontProvider() : nullptr;
        YUCHEN_ASSERT(fontProvider);
        
        FontFallbackChain fallbackChain = style->getDefaultLabelFontChain();
        
        Vec4 textColor = m_hasCustomTextColor ? m_textColor : style->getDefaultTextColor();
        
        if (!m_isEnabled) {
            textColor = Vec4::FromRGBA(255,255,255,255);
        }
        
        FontMetrics metrics = fontProvider->getFontMetrics(fallbackChain.getPrimary(), m_fontSize);
        float textX = absPos.x + RADIO_SIZE + TEXT_SPACING;
        float textY = absPos.y + (RADIO_SIZE - metrics.lineHeight) * 0.5f + metrics.ascender;
        
        commandList.drawText(m_text.c_str(), Vec2(textX, textY),
                           fallbackChain, m_fontSize, textColor);
    }
    
    drawFocusIndicator(commandList, offset);
}

bool RadioButton::handleMouseMove(const Vec2& position, const Vec2& offset) {
    if (!m_isEnabled || !m_isVisible) return false;
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect absRect(absPos.x, absPos.y, m_bounds.width, m_bounds.height);
    
    bool wasHovered = m_isHovered;
    m_isHovered = absRect.contains(position);
    
    return wasHovered != m_isHovered;
}

bool RadioButton::handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset) {
    if (!m_isEnabled || !m_isVisible) return false;
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect absRect(absPos.x, absPos.y, m_bounds.width, m_bounds.height);
    
    if (pressed) {
        if (absRect.contains(position)) {
            m_isPressed = true;
            requestFocus(FocusReason::MouseFocusReason);
            return true;
        }
    } else {
        if (m_isPressed) {
            m_isPressed = false;
            if (absRect.contains(position)) {
                setChecked(true);
            }
            return true;
        }
    }
    
    return false;
}

bool RadioButton::handleKeyPress(const Event& event) {
    if (!m_isEnabled || !m_isVisible || !m_group) return false;
    
    YUCHEN_ASSERT(event.type == EventType::KeyPressed || event.type == EventType::KeyReleased);
    if (event.type == EventType::KeyReleased) return false;
    
    switch (event.key.key) {
        case KeyCode::Space:
        case KeyCode::Return:
        case KeyCode::Enter:
        case KeyCode::KeypadEnter:
            setChecked(true);
            return true;
            
        case KeyCode::UpArrow:
        case KeyCode::LeftArrow:
            m_group->selectPrevious(this);
            return true;
            
        case KeyCode::DownArrow:
        case KeyCode::RightArrow:
            m_group->selectNext(this);
            return true;
            
        default:
            break;
    }
    
    return false;
}

void RadioButton::setChecked(bool checked) {
    if (checked && m_group) {
        m_group->setCheckedButton(this);
    } else {
        internalSetChecked(checked);
    }
}

void RadioButton::setText(const std::string& text) {
    m_text = text;
}

void RadioButton::setText(const char* text) {
    YUCHEN_ASSERT(text);
    m_text = text;
}

void RadioButton::setFontSize(float fontSize) {
    if (fontSize >= Config::Font::MIN_SIZE && fontSize <= Config::Font::MAX_SIZE) {
        m_fontSize = fontSize;
    }
}

void RadioButton::setTextColor(const Vec4& color) {
    Validation::AssertColor(color);
    m_textColor = color;
    m_hasCustomTextColor = true;
}

Vec4 RadioButton::getTextColor() const {
    if (m_hasCustomTextColor) {
        return m_textColor;
    }
    // Get default color via UIContext instead of deprecated singleton
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    return style ? style->getDefaultTextColor() : Vec4();
}

void RadioButton::resetTextColor() {
    m_hasCustomTextColor = false;
    m_textColor = Vec4();
}

void RadioButton::setBounds(const Rect& bounds) {
    Validation::AssertRect(bounds);
    m_bounds = bounds;
}

void RadioButton::setCheckedCallback(RadioButtonCheckedCallback callback) {
    m_checkedCallback = callback;
}

void RadioButton::setGroup(RadioButtonGroup* group) {
    if (m_group == group) return;
    
    if (m_group) {
        m_group->removeButton(this);
    }
    
    m_group = group;
    
    if (m_group) {
        m_group->addButton(this);
    }
}

bool RadioButton::isValid() const {
    return m_bounds.isValid() &&
           m_fontSize >= Config::Font::MIN_SIZE &&
           m_fontSize <= Config::Font::MAX_SIZE;
}

void RadioButton::internalSetChecked(bool checked) {
    if (m_isChecked != checked) {
        m_isChecked = checked;
        if (m_checkedCallback) {
            m_checkedCallback(m_isChecked);
        }
    }
}

RadioButtonGroup::RadioButtonGroup()
    : m_buttons()
    , m_selectionCallback(nullptr)
{
}

RadioButtonGroup::~RadioButtonGroup() {
    for (RadioButton* button : m_buttons) {
        if (button) {
            button->m_group = nullptr;
        }
    }
}

void RadioButtonGroup::addButton(RadioButton* button) {
    YUCHEN_ASSERT(button);
    
    auto it = std::find(m_buttons.begin(), m_buttons.end(), button);
    if (it == m_buttons.end()) {
        m_buttons.push_back(button);
        button->m_group = this;
    }
}

void RadioButtonGroup::removeButton(RadioButton* button) {
    YUCHEN_ASSERT(button);
    
    auto it = std::find(m_buttons.begin(), m_buttons.end(), button);
    if (it != m_buttons.end()) {
        m_buttons.erase(it);
        button->m_group = nullptr;
    }
}

void RadioButtonGroup::clearButtons() {
    for (RadioButton* button : m_buttons) {
        if (button) {
            button->m_group = nullptr;
        }
    }
    m_buttons.clear();
}

void RadioButtonGroup::setCheckedButton(RadioButton* button) {
    YUCHEN_ASSERT(button);
    
    auto it = std::find(m_buttons.begin(), m_buttons.end(), button);
    if (it == m_buttons.end()) return;
    
    for (RadioButton* rb : m_buttons) {
        if (rb) {
            rb->internalSetChecked(rb == button);
        }
    }
    
    if (m_selectionCallback) {
        int index = static_cast<int>(it - m_buttons.begin());
        m_selectionCallback(index, button);
    }
}

void RadioButtonGroup::setCheckedIndex(int index) {
    if (index >= 0 && index < static_cast<int>(m_buttons.size())) {
        setCheckedButton(m_buttons[index]);
    }
}

RadioButton* RadioButtonGroup::getCheckedButton() const {
    for (RadioButton* button : m_buttons) {
        if (button && button->isChecked()) {
            return button;
        }
    }
    return nullptr;
}

int RadioButtonGroup::getCheckedIndex() const {
    for (size_t i = 0; i < m_buttons.size(); ++i) {
        if (m_buttons[i] && m_buttons[i]->isChecked()) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

RadioButton* RadioButtonGroup::getButton(size_t index) const {
    if (index < m_buttons.size()) {
        return m_buttons[index];
    }
    return nullptr;
}

void RadioButtonGroup::setSelectionCallback(RadioButtonGroupSelectionCallback callback) {
    m_selectionCallback = callback;
}

void RadioButtonGroup::selectNext(RadioButton* current) {
    YUCHEN_ASSERT(current);
    
    auto it = std::find(m_buttons.begin(), m_buttons.end(), current);
    if (it == m_buttons.end()) return;
    
    size_t currentIndex = it - m_buttons.begin();
    size_t nextIndex = (currentIndex + 1) % m_buttons.size();
    
    size_t attempts = 0;
    while (attempts < m_buttons.size()) {
        RadioButton* nextButton = m_buttons[nextIndex];
        if (nextButton && nextButton->isEnabled() && nextButton->isVisible()) {
            setCheckedButton(nextButton);
            nextButton->requestFocus(FocusReason::OtherFocusReason);
            return;
        }
        nextIndex = (nextIndex + 1) % m_buttons.size();
        attempts++;
    }
}

void RadioButtonGroup::selectPrevious(RadioButton* current) {
    YUCHEN_ASSERT(current);
    
    auto it = std::find(m_buttons.begin(), m_buttons.end(), current);
    if (it == m_buttons.end()) return;
    
    size_t currentIndex = it - m_buttons.begin();
    size_t prevIndex = (currentIndex == 0) ? m_buttons.size() - 1 : currentIndex - 1;
    
    size_t attempts = 0;
    while (attempts < m_buttons.size()) {
        RadioButton* prevButton = m_buttons[prevIndex];
        if (prevButton && prevButton->isEnabled() && prevButton->isVisible()) {
            setCheckedButton(prevButton);
            prevButton->requestFocus(FocusReason::OtherFocusReason);
            return;
        }
        prevIndex = (prevIndex == 0) ? m_buttons.size() - 1 : prevIndex - 1;
        attempts++;
    }
}

}
