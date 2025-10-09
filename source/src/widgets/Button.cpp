#include "YuchenUI/widgets/Button.h"
#include "YuchenUI/text/FontManager.h"
#include "YuchenUI/theme/ThemeManager.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/core/Validation.h"
#include "YuchenUI/core/Config.h"

namespace YuchenUI {

Button::Button(const Rect& bounds)
    : m_text()
    , m_westernFontHandle(INVALID_FONT_HANDLE)
    , m_chineseFontHandle(INVALID_FONT_HANDLE)
    , m_fontSize(Config::Font::DEFAULT_SIZE)
    , m_textColor()
    , m_bounds(bounds)
    , m_role(ButtonRole::Normal)
    , m_isHovered(false)
    , m_isPressed(false)
    , m_clickCallback(nullptr)
    , m_hasCustomWesternFont(false)
    , m_hasCustomChineseFont(false)
    , m_hasCustomTextColor(false)
{
    Validation::AssertRect(bounds);
}

Button::~Button() {
}

void Button::addDrawCommands(RenderList& commandList, const Vec2& offset) const {
    YUCHEN_ASSERT(isValid());
    
    if (!m_isVisible) return;
    
    UIStyle* style = ThemeManager::getInstance().getCurrentStyle();
    FontManager& fontManager = FontManager::getInstance();
    
    ButtonDrawInfo info;
    info.bounds = Rect(m_bounds.x + offset.x, m_bounds.y + offset.y,
                      m_bounds.width, m_bounds.height);
    info.text = m_text;
    
    info.westernFont = m_hasCustomWesternFont ? m_westernFontHandle
                                              : style->getDefaultButtonFont();
    info.chineseFont = m_hasCustomChineseFont ? m_chineseFontHandle
                                              : fontManager.getPingFangFont();
    info.textColor = m_hasCustomTextColor ? m_textColor
                                          : style->getDefaultTextColor();
    
    info.fontSize = m_fontSize;
    info.isHovered = m_isHovered;
    info.isPressed = m_isPressed;
    info.isEnabled = m_isEnabled;
    
    switch (m_role) {
        case ButtonRole::Confirm:
            style->drawConfirmButton(info, commandList);
            break;
        case ButtonRole::Cancel:
            style->drawCancelButton(info, commandList);
            break;
        default:
            style->drawButton(info, commandList);
            break;
    }
    drawFocusIndicator(commandList, offset);
}

void Button::setText(const std::string& text) {
    m_text = text;
}

void Button::setText(const char* text) {
    m_text = text;
}

void Button::setWesternFont(FontHandle fontHandle) {
    if (FontManager::getInstance().isValidFont(fontHandle)) {
        m_westernFontHandle = fontHandle;
        m_hasCustomWesternFont = true;
    }
}

FontHandle Button::getWesternFont() const {
    if (m_hasCustomWesternFont) {
        return m_westernFontHandle;
    }
    return ThemeManager::getInstance().getCurrentStyle()->getDefaultButtonFont();
}

void Button::resetWesternFont() {
    m_hasCustomWesternFont = false;
    m_westernFontHandle = INVALID_FONT_HANDLE;
}

void Button::setChineseFont(FontHandle fontHandle) {
    if (FontManager::getInstance().isValidFont(fontHandle)) {
        m_chineseFontHandle = fontHandle;
        m_hasCustomChineseFont = true;
    }
}

FontHandle Button::getChineseFont() const {
    if (m_hasCustomChineseFont) {
        return m_chineseFontHandle;
    }
    return FontManager::getInstance().getPingFangFont();
}

void Button::resetChineseFont() {
    m_hasCustomChineseFont = false;
    m_chineseFontHandle = INVALID_FONT_HANDLE;
}

void Button::setFontSize(float fontSize) {
    if (fontSize >= Config::Font::MIN_SIZE && fontSize <= Config::Font::MAX_SIZE) {
        m_fontSize = fontSize;
    }
}

void Button::setTextColor(const Vec4& color) {
    Validation::AssertColor(color);
    m_textColor = color;
    m_hasCustomTextColor = true;
}

Vec4 Button::getTextColor() const {
    if (m_hasCustomTextColor) {
        return m_textColor;
    }
    return ThemeManager::getInstance().getCurrentStyle()->getDefaultTextColor();
}

void Button::resetTextColor() {
    m_hasCustomTextColor = false;
    m_textColor = Vec4();
}

void Button::setBounds(const Rect& bounds) {
    Validation::AssertRect(bounds);
    m_bounds = bounds;
}

void Button::setRole(ButtonRole role) {
    m_role = role;
}

void Button::setClickCallback(ButtonClickCallback callback) {
    m_clickCallback = callback;
}

bool Button::handleMouseMove(const Vec2& position, const Vec2& offset) {
    if (!m_isEnabled || !m_isVisible) return false;
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect absRect(absPos.x, absPos.y, m_bounds.width, m_bounds.height);
    
    bool wasHovered = m_isHovered;
    m_isHovered = absRect.contains(position);
    
    return wasHovered != m_isHovered;
}

bool Button::handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset) {
    if (!m_isEnabled || !m_isVisible) return false;
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect absRect(absPos.x, absPos.y, m_bounds.width, m_bounds.height);
    
    bool isInBounds = absRect.contains(position);
    
    if (pressed && isInBounds) {
        m_isPressed = true;
        return true;
    } else if (!pressed && m_isPressed) {
        m_isPressed = false;
        if (isInBounds && m_clickCallback) {
            m_clickCallback();
        }
        return true;
    }
    
    return false;
}

bool Button::isValid() const {
    return m_bounds.isValid() &&
           m_fontSize >= Config::Font::MIN_SIZE &&
           m_fontSize <= Config::Font::MAX_SIZE;
}

}
