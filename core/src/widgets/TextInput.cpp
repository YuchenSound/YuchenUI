#include "YuchenUI/widgets/TextInput.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/core/IUIContent.h"
#include "YuchenUI/core/UIContext.h"
#include "YuchenUI/text/IFontProvider.h"
#include "YuchenUI/theme/IThemeProvider.h"
#include "YuchenUI/theme/Theme.h"
#include "YuchenUI/core/Validation.h"
#include "YuchenUI/core/Config.h"
#include "YuchenUI/core/Assert.h"
#include "YuchenUI/utils/Clipboard.h"
#include <algorithm>

namespace YuchenUI {

TextInput::TextInput(const Rect& bounds)
    : m_text()
    , m_textUTF32()
    , m_placeholder()
    , m_cursorPosition(0)
    , m_selectionStart(0)
    , m_selectionEnd(0)
    , m_scrollOffset(0.0f)
    , m_maxLength(1000)
    , m_hasFocus(false)
    , m_isPasswordMode(false)
    , m_isHovered(false)
    , m_showCursor(true)
    , m_cursorBlinkTimer(0.0f)
    , m_bounds(bounds)
    , m_fontSize(Config::Font::DEFAULT_SIZE)
    , m_textColor()
    , m_hasCustomTextColor(false)
    , m_paddingLeft(5.0f)
    , m_paddingTop(5.0f)
    , m_paddingRight(5.0f)
    , m_paddingBottom(5.0f)
    , m_validator(nullptr)
    , m_changeCallback(nullptr)
    , m_submitCallback(nullptr)
    , m_isDragging(false)
    , m_dragStartPosition(0)
    , m_compositionText{0}
    , m_compositionCursorPos(0)
    , m_compositionSelectionLength(0)
    , m_inputType(TextInputType::Text)
{
    Validation::AssertRect(bounds);
    setFocusPolicy(FocusPolicy::StrongFocus);
}

TextInput::~TextInput() {
}

void TextInput::addDrawCommands(RenderList& commandList, const Vec2& offset) const {
    if (!m_isVisible) return;
    
    // Get style via UIContext instead of deprecated singleton
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    YUCHEN_ASSERT(style);
    
    TextInputDrawInfo info;
    info.bounds = Rect(m_bounds.x + offset.x, m_bounds.y + offset.y, m_bounds.width, m_bounds.height);
    info.hasFocus = m_hasFocus;
    info.isHovered = m_isHovered;
    info.isEnabled = m_isEnabled;
    info.isEmpty = m_text.empty();
    info.placeholder = m_placeholder;
    info.fontSize = m_fontSize;
    
    std::string displayText = m_text;
    size_t visualCursorPos = m_cursorPosition;
    
    bool hasComposition = (m_compositionText[0] != '\0' && m_hasFocus);
    size_t compositionLength = 0;
    
    if (hasComposition) {
        compositionLength = strlen(m_compositionText);
        
        std::u32string textU32 = utf8ToUtf32(m_text);
        std::u32string compU32 = utf8ToUtf32(m_compositionText);
        
        textU32.insert(m_cursorPosition, compU32);
        
        displayText = utf32ToUtf8(textU32);
        
        visualCursorPos = m_cursorPosition + compU32.length();
    }
    
    if (m_isPasswordMode && !displayText.empty()) {
        std::u32string textU32 = utf8ToUtf32(displayText);
        displayText = std::string(textU32.length() * 3, '\xe2');
        for (size_t i = 0; i < textU32.length(); ++i) {
            displayText[i * 3] = '\xe2';
            displayText[i * 3 + 1] = '\x80';
            displayText[i * 3 + 2] = '\xa2';
        }
    }
    
    info.text = displayText;
    
    // Get font provider via UIContext instead of deprecated singleton
    IFontProvider* fontProvider = m_ownerContext ? m_ownerContext->getFontProvider() : nullptr;
    YUCHEN_ASSERT(fontProvider);
    
    FontHandle westernFont = style->getDefaultLabelFont();
    FontMetrics metrics = fontProvider->getFontMetrics(westernFont, m_fontSize);
    
    float contentHeight = m_bounds.height - m_paddingTop - m_paddingBottom;
    float textTopY = m_paddingTop + (contentHeight - metrics.lineHeight) * 0.5f;
    
    info.showCursor = m_showCursor && m_hasFocus;
    if (info.showCursor) {
        std::u32string displayU32 = utf8ToUtf32(displayText);
        size_t measurePos = std::min(visualCursorPos, displayU32.length());
        std::string measureText = utf32ToUtf8(displayU32.substr(0, measurePos));
        
        float cursorX = m_paddingLeft + fontProvider->measureText(measureText.c_str(), m_fontSize).x - m_scrollOffset;
        info.cursorX = info.bounds.x + cursorX;
        info.cursorHeight = metrics.lineHeight;
    } else {
        info.cursorX = 0.0f;
        info.cursorHeight = 0.0f;
    }
    
    info.hasSelection = hasSelection() && m_hasFocus && !hasComposition;
    if (info.hasSelection) {
        size_t selStart = std::min(m_selectionStart, m_selectionEnd);
        size_t selEnd = std::max(m_selectionStart, m_selectionEnd);
        
        float startX = m_paddingLeft + measureTextToPosition(selStart) - m_scrollOffset;
        float endX = m_paddingLeft + measureTextToPosition(selEnd) - m_scrollOffset;
        
        info.selectionStartX = info.bounds.x + startX;
        info.selectionWidth = endX - startX;
    } else {
        info.selectionStartX = 0.0f;
        info.selectionWidth = 0.0f;
    }
    
    info.textX = info.bounds.x + m_paddingLeft - m_scrollOffset;
    info.textY = info.bounds.y + textTopY;
    
    style->drawTextInput(info, commandList);
    
    const float borderWidth = 1.0f;
    Rect contentClip(
        info.bounds.x + borderWidth,
        info.bounds.y + borderWidth,
        info.bounds.width - borderWidth * 2.0f,
        info.bounds.height - borderWidth * 2.0f
    );
    commandList.pushClipRect(contentClip);
    
    if (hasComposition) {
        std::u32string displayU32 = utf8ToUtf32(displayText);
        std::string beforeComp = utf32ToUtf8(displayU32.substr(0, m_cursorPosition));
        std::string withComp = utf32ToUtf8(displayU32.substr(0, m_cursorPosition + compositionLength));
        
        float compStartX = m_paddingLeft + fontProvider->measureText(beforeComp.c_str(), m_fontSize).x - m_scrollOffset;
        float compEndX = m_paddingLeft + fontProvider->measureText(withComp.c_str(), m_fontSize).x - m_scrollOffset;
        
        float underlineY = info.bounds.y + textTopY + metrics.ascender + 2.0f;
        
        commandList.drawLine(
            Vec2(info.bounds.x + compStartX, underlineY),
            Vec2(info.bounds.x + compEndX, underlineY),
            style->getDefaultTextColor(),
            1.0f
        );
    }
    
    commandList.popClipRect();
    
    drawFocusIndicator(commandList, offset);
}

bool TextInput::handleMouseMove(const Vec2& position, const Vec2& offset) {
    if (!m_isEnabled || !m_isVisible) return false;
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect absRect(absPos.x, absPos.y, m_bounds.width, m_bounds.height);
    
    bool wasHovered = m_isHovered;
    m_isHovered = absRect.contains(position);
    
    if (m_isDragging && m_hasFocus) {
        size_t newPos = positionToCharIndex(position.x, offset);
        m_cursorPosition = newPos;
        m_selectionEnd = newPos;
        adjustScrollToCursor();
        m_showCursor = true;
        m_cursorBlinkTimer = 0.0f;
        return true;
    }
    
    return wasHovered != m_isHovered;
}

bool TextInput::handleKeyPress(const Event& event) {
    if (!m_hasFocus || !m_isEnabled) return false;
    
    YUCHEN_ASSERT(event.type == EventType::KeyPressed || event.type == EventType::KeyReleased);
    if (event.type == EventType::KeyReleased) return false;
    
    if (m_compositionText[0] != '\0') {
        return false;
    }
    
    bool handled = false;
    bool hasShift = event.key.modifiers.hasShift();
    bool hasCmd = event.key.modifiers.hasCommand();
    bool hasCtrl = event.key.modifiers.hasControl();
    bool hasModifier = hasCmd || hasCtrl;
    
    switch (event.key.key) {
        case KeyCode::LeftArrow:
            moveCursor(-1, hasShift);
            handled = true;
            break;
            
        case KeyCode::RightArrow:
            moveCursor(1, hasShift);
            handled = true;
            break;
            
        case KeyCode::Home:
            moveCursorToStart(hasShift);
            handled = true;
            break;
            
        case KeyCode::End:
            moveCursorToEnd(hasShift);
            handled = true;
            break;
            
        case KeyCode::Backspace:
            if (hasSelection()) {
                deleteSelection();
            } else {
                deleteCharacterBefore();
            }
            handled = true;
            break;
            
        case KeyCode::Delete:
            if (hasSelection()) {
                deleteSelection();
            } else {
                deleteCharacterAfter();
            }
            handled = true;
            break;
            
        case KeyCode::A:
            if (hasModifier) {
                selectAll();
                handled = true;
            }
            break;
            
        case KeyCode::C:
            if (hasModifier) {
                copy();
                handled = true;
            }
            break;
            
        case KeyCode::X:
            if (hasModifier) {
                cut();
                handled = true;
            }
            break;
            
        case KeyCode::V:
            if (hasModifier) {
                paste();
                handled = true;
            }
            break;
            
        case KeyCode::Return:
        case KeyCode::Enter:
        case KeyCode::KeypadEnter:
            if (m_submitCallback) {
                m_submitCallback(m_text);
            }
            handled = true;
            break;
            
        case KeyCode::Escape:
            blur();
            handled = true;
            break;
            
        default:
            break;
    }
    
    if (handled) {
        m_showCursor = true;
        m_cursorBlinkTimer = 0.0f;
    }
    
    return handled;
}

bool TextInput::handleTextInput(uint32_t codepoint) {
    if (!m_hasFocus || !m_isEnabled) return false;

    if (codepoint < 32 || codepoint == 127) {
        return false;
    }

    if (m_inputType == TextInputType::Number) {
        if (!((codepoint >= '0' && codepoint <= '9') || codepoint == '.')) {
            return false;
        }
    }

    std::u32string u32char(1, codepoint);
    std::string utf8char = utf32ToUtf8(u32char);

    insertTextAtCursor(utf8char);

    m_compositionText[0] = '\0';
    m_compositionCursorPos = 0;
    m_compositionSelectionLength = 0;

    m_showCursor = true;
    m_cursorBlinkTimer = 0.0f;

    return true;
}

bool TextInput::handleComposition(const char* text, int cursorPos, int selectionLength) {
    if (!m_hasFocus || !m_isEnabled) {
        return false;
    }

    if (shouldDisableIME()) {
        return false;
    }

    if (text && text[0] != '\0') {
        strncpy(m_compositionText, text, 255);
        m_compositionText[255] = '\0';
        m_compositionCursorPos = cursorPos;
        m_compositionSelectionLength = selectionLength;
    }
    else {
        m_compositionText[0] = '\0';
        m_compositionCursorPos = 0;
        m_compositionSelectionLength = 0;
    }

    m_showCursor = true;
    m_cursorBlinkTimer = 0.0f;

    return true;
}

void TextInput::setText(const std::string& text) {
    if (!validateText(text)) return;
    
    m_text = text;
    m_textUTF32 = utf8ToUtf32(text);
    
    m_cursorPosition = std::min(m_cursorPosition, m_textUTF32.length());
    m_selectionStart = 0;
    m_selectionEnd = 0;
    m_scrollOffset = 0.0f;
    
    notifyTextChanged();
}

void TextInput::setPlaceholder(const std::string& placeholder) {
    m_placeholder = placeholder;
}

void TextInput::setMaxLength(size_t maxLength) {
    m_maxLength = maxLength;
}

void TextInput::setPasswordMode(bool enabled) {
    m_isPasswordMode = enabled;
    if (enabled) {
        m_inputType = TextInputType::Password;
    } else if (m_inputType == TextInputType::Password) {
        m_inputType = TextInputType::Text;
    }
    
    if (m_hasFocus && m_ownerContext) {
        m_ownerContext->requestTextInput(!shouldDisableIME());
    }
}

void TextInput::setInputType(TextInputType type) {
    m_inputType = type;
    
    if (type == TextInputType::Password) {
        m_isPasswordMode = true;
    } else if (m_inputType == TextInputType::Password) {
        m_isPasswordMode = false;
    }
    
    if (m_hasFocus && m_ownerContext) {
        m_ownerContext->requestTextInput(!shouldDisableIME());
    }
}

bool TextInput::shouldDisableIME() const {
    return m_inputType == TextInputType::Password ||
           m_inputType == TextInputType::Number;
}

void TextInput::setValidator(TextInputValidator validator) {
    m_validator = validator;
}

void TextInput::setChangeCallback(TextInputChangeCallback callback) {
    m_changeCallback = callback;
}

void TextInput::setSubmitCallback(TextInputSubmitCallback callback) {
    m_submitCallback = callback;
}

void TextInput::setBounds(const Rect& bounds) {
    Validation::AssertRect(bounds);
    m_bounds = bounds;
}

void TextInput::setFontSize(float fontSize) {
    if (fontSize >= Config::Font::MIN_SIZE && fontSize <= Config::Font::MAX_SIZE) {
        m_fontSize = fontSize;
    }
}

void TextInput::setTextColor(const Vec4& color) {
    Validation::AssertColor(color);
    m_textColor = color;
    m_hasCustomTextColor = true;
}

Vec4 TextInput::getTextColor() const {
    if (m_hasCustomTextColor) {
        return m_textColor;
    }
    // Get default color via UIContext instead of deprecated singleton
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    return style ? style->getDefaultTextColor() : Vec4();
}

void TextInput::resetTextColor() {
    m_hasCustomTextColor = false;
    m_textColor = Vec4();
}

void TextInput::setPadding(float padding) {
    setPadding(padding, padding, padding, padding);
}

void TextInput::setPadding(float left, float top, float right, float bottom) {
    m_paddingLeft = left;
    m_paddingTop = top;
    m_paddingRight = right;
    m_paddingBottom = bottom;
}

void TextInput::selectAll() {
    m_selectionStart = 0;
    m_selectionEnd = m_textUTF32.length();
    m_cursorPosition = m_selectionEnd;
}

void TextInput::clearSelection() {
    m_selectionStart = 0;
    m_selectionEnd = 0;
}

bool TextInput::hasSelection() const {
    return m_selectionStart != m_selectionEnd;
}

void TextInput::update(float deltaTime) {
    if (!m_hasFocus) return;
    
    m_cursorBlinkTimer += deltaTime;
    if (m_cursorBlinkTimer >= CURSOR_BLINK_INTERVAL) {
        m_cursorBlinkTimer = 0.0f;
        m_showCursor = !m_showCursor;
    }
}

bool TextInput::isValid() const {
    return m_bounds.isValid() &&
           m_fontSize >= Config::Font::MIN_SIZE &&
           m_fontSize <= Config::Font::MAX_SIZE;
}

std::u32string TextInput::utf8ToUtf32(const std::string& utf8) {
    std::u32string result;
    result.reserve(utf8.length());
    
    for (size_t i = 0; i < utf8.length();) {
        unsigned char c = utf8[i];
        char32_t codepoint = 0;
        
        if ((c & 0x80) == 0) {
            codepoint = c;
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            if (i + 1 < utf8.length()) {
                codepoint = ((c & 0x1F) << 6) | (utf8[i + 1] & 0x3F);
                i += 2;
            } else {
                break;
            }
        } else if ((c & 0xF0) == 0xE0) {
            if (i + 2 < utf8.length()) {
                codepoint = ((c & 0x0F) << 12) | ((utf8[i + 1] & 0x3F) << 6) | (utf8[i + 2] & 0x3F);
                i += 3;
            } else {
                break;
            }
        } else if ((c & 0xF8) == 0xF0) {
            if (i + 3 < utf8.length()) {
                codepoint = ((c & 0x07) << 18) | ((utf8[i + 1] & 0x3F) << 12) |
                           ((utf8[i + 2] & 0x3F) << 6) | (utf8[i + 3] & 0x3F);
                i += 4;
            } else {
                break;
            }
        } else {
            i += 1;
            continue;
        }
        
        result.push_back(codepoint);
    }
    
    return result;
}

std::string TextInput::utf32ToUtf8(const std::u32string& utf32) {
    std::string result;
    result.reserve(utf32.length() * 4);
    
    for (char32_t codepoint : utf32) {
        if (codepoint <= 0x7F) {
            result.push_back(static_cast<char>(codepoint));
        } else if (codepoint <= 0x7FF) {
            result.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
            result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else if (codepoint <= 0xFFFF) {
            result.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
            result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else if (codepoint <= 0x10FFFF) {
            result.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
            result.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
    }
    
    return result;
}

void TextInput::insertTextAtCursor(const std::string& text) {
    if (hasSelection()) {
        deleteSelection();
    }
    
    std::u32string u32text = utf8ToUtf32(text);
    
    if (m_textUTF32.length() + u32text.length() > m_maxLength) {
        return;
    }
    
    m_textUTF32.insert(m_cursorPosition, u32text);
    m_text = utf32ToUtf8(m_textUTF32);
    
    if (!validateText(m_text)) {
        m_textUTF32.erase(m_cursorPosition, u32text.length());
        m_text = utf32ToUtf8(m_textUTF32);
        return;
    }
    
    m_cursorPosition += u32text.length();
    adjustScrollToCursor();
    notifyTextChanged();
}

void TextInput::deleteSelection() {
    if (!hasSelection()) return;
    
    size_t start = std::min(m_selectionStart, m_selectionEnd);
    size_t end = std::max(m_selectionStart, m_selectionEnd);
    
    m_textUTF32.erase(start, end - start);
    m_text = utf32ToUtf8(m_textUTF32);
    
    m_cursorPosition = start;
    m_selectionStart = 0;
    m_selectionEnd = 0;
    
    adjustScrollToCursor();
    notifyTextChanged();
}

void TextInput::deleteCharacterBefore() {
    if (m_cursorPosition == 0) return;
    
    m_textUTF32.erase(m_cursorPosition - 1, 1);
    m_text = utf32ToUtf8(m_textUTF32);
    
    m_cursorPosition--;
    adjustScrollToCursor();
    notifyTextChanged();
}

void TextInput::deleteCharacterAfter() {
    if (m_cursorPosition >= m_textUTF32.length()) return;
    
    m_textUTF32.erase(m_cursorPosition, 1);
    m_text = utf32ToUtf8(m_textUTF32);
    
    adjustScrollToCursor();
    notifyTextChanged();
}

void TextInput::moveCursor(int delta, bool extendSelection) {
    if (!extendSelection) {
        if (hasSelection() && delta != 0) {
            if (delta < 0) {
                m_cursorPosition = std::min(m_selectionStart, m_selectionEnd);
            } else {
                m_cursorPosition = std::max(m_selectionStart, m_selectionEnd);
            }
            clearSelection();
            adjustScrollToCursor();
            return;
        }
    }
    
    int newPos = static_cast<int>(m_cursorPosition) + delta;
    newPos = std::max(0, std::min(newPos, static_cast<int>(m_textUTF32.length())));
    
    if (extendSelection) {
        if (!hasSelection()) {
            m_selectionStart = m_cursorPosition;
        }
        m_selectionEnd = newPos;
    } else {
        clearSelection();
    }
    
    m_cursorPosition = newPos;
    adjustScrollToCursor();
}

void TextInput::moveCursorToStart(bool extendSelection) {
    if (extendSelection) {
        if (!hasSelection()) {
            m_selectionStart = m_cursorPosition;
        }
        m_selectionEnd = 0;
    } else {
        clearSelection();
    }
    
    m_cursorPosition = 0;
    adjustScrollToCursor();
}

void TextInput::moveCursorToEnd(bool extendSelection) {
    if (extendSelection) {
        if (!hasSelection()) {
            m_selectionStart = m_cursorPosition;
        }
        m_selectionEnd = m_textUTF32.length();
    } else {
        clearSelection();
    }
    
    m_cursorPosition = m_textUTF32.length();
    adjustScrollToCursor();
}

void TextInput::selectWord() {
    if (m_textUTF32.empty()) return;
    
    size_t start = findWordBoundary(m_cursorPosition, false);
    size_t end = findWordBoundary(m_cursorPosition, true);
    
    m_selectionStart = start;
    m_selectionEnd = end;
    m_cursorPosition = end;
}

size_t TextInput::findWordBoundary(size_t pos, bool forward) const {
    if (m_textUTF32.empty()) return 0;
    
    auto isWordChar = [](char32_t c) {
        return (c >= U'a' && c <= U'z') ||
               (c >= U'A' && c <= U'Z') ||
               (c >= U'0' && c <= U'9') ||
               (c >= 0x4E00 && c <= 0x9FFF) ||
               c == U'_';
    };
    
    if (forward) {
        size_t i = pos;
        while (i < m_textUTF32.length() && isWordChar(m_textUTF32[i])) {
            i++;
        }
        return i;
    } else {
        if (pos == 0) return 0;
        size_t i = pos - 1;
        while (i > 0 && isWordChar(m_textUTF32[i])) {
            i--;
        }
        return isWordChar(m_textUTF32[i]) ? i : i + 1;
    }
}

void TextInput::copy() {
    if (!hasSelection()) return;
    
    size_t start = std::min(m_selectionStart, m_selectionEnd);
    size_t end = std::max(m_selectionStart, m_selectionEnd);
    
    std::u32string selected = m_textUTF32.substr(start, end - start);
    std::string selectedUTF8 = utf32ToUtf8(selected);
    
    Clipboard::setText(selectedUTF8);
}

void TextInput::cut() {
    copy();
    deleteSelection();
}

void TextInput::paste() {
    std::string clipboardText = Clipboard::getText();
    if (!clipboardText.empty()) {
        insertTextAtCursor(clipboardText);
    }
}

float TextInput::measureTextToPosition(size_t charIndex) const {
    if (charIndex == 0 || m_textUTF32.empty()) {
        return 0.0f;
    }
    
    charIndex = std::min(charIndex, m_textUTF32.length());
    
    std::u32string substr = m_textUTF32.substr(0, charIndex);
    std::string utf8substr = utf32ToUtf8(substr);
    
    if (m_isPasswordMode) {
        utf8substr = std::string(charIndex * 3, '\xe2');
        for (size_t i = 0; i < charIndex; ++i) {
            utf8substr[i * 3] = '\xe2';
            utf8substr[i * 3 + 1] = '\x80';
            utf8substr[i * 3 + 2] = '\xa2';
        }
    }
    
    // Get font provider via UIContext instead of deprecated singleton
    IFontProvider* fontProvider = m_ownerContext ? m_ownerContext->getFontProvider() : nullptr;
    if (!fontProvider) return 0.0f;
    
    Vec2 size = fontProvider->measureText(utf8substr.c_str(), m_fontSize);
    
    return size.x;
}

size_t TextInput::positionToCharIndex(float x, const Vec2& offset) const {
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    float relativeX = x - absPos.x - m_paddingLeft + m_scrollOffset;
    
    if (relativeX <= 0.0f) return 0;
    if (m_textUTF32.empty()) return 0;
    
    for (size_t i = 0; i <= m_textUTF32.length(); i++) {
        float charX = measureTextToPosition(i);
        
        if (i < m_textUTF32.length()) {
            float nextCharX = measureTextToPosition(i + 1);
            float midPoint = (charX + nextCharX) / 2.0f;
            
            if (relativeX < midPoint) {
                return i;
            }
        } else {
            return i;
        }
    }
    
    return m_textUTF32.length();
}

void TextInput::adjustScrollToCursor() {
    float cursorX = measureTextToPosition(m_cursorPosition);
    float visibleWidth = m_bounds.width - m_paddingLeft - m_paddingRight;
    
    float cursorScreenX = cursorX - m_scrollOffset;
    
    if (cursorScreenX < 0.0f) {
        m_scrollOffset += cursorScreenX;
    } else if (cursorScreenX > visibleWidth) {
        m_scrollOffset += cursorScreenX - visibleWidth;
    }
    
    m_scrollOffset = std::max(0.0f, m_scrollOffset);
}

void TextInput::focus() {
    requestFocus(FocusReason::OtherFocusReason);
}

void TextInput::blur() {
    clearFocus();
}

bool TextInput::handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset) {
    if (!m_isEnabled || !m_isVisible) return false;
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect absRect(absPos.x, absPos.y, m_bounds.width, m_bounds.height);
    
    if (pressed) {
        if (absRect.contains(position)) {
            requestFocus();
            
            size_t clickPos = positionToCharIndex(position.x, offset);
            m_cursorPosition = clickPos;
            m_selectionStart = clickPos;
            m_selectionEnd = clickPos;
            m_isDragging = true;
            m_dragStartPosition = clickPos;
            
            adjustScrollToCursor();
            m_showCursor = true;
            m_cursorBlinkTimer = 0.0f;
            
            static float lastClickTime = 0.0f;
            static size_t lastClickPos = SIZE_MAX;
            float currentTime = m_cursorBlinkTimer;
            
            if (currentTime - lastClickTime < 0.5f && clickPos == lastClickPos) {
                selectWord();
            }
            
            lastClickTime = currentTime;
            lastClickPos = clickPos;
            
            captureMouse();
            return true;
        }
        return false;
    } else {
        if (m_isDragging) {
            m_isDragging = false;
            releaseMouse();
            return true;
        }
    }
    
    return false;
}

Rect TextInput::getInputMethodCursorRect() const {
    if (!m_hasFocus || !m_isEnabled || !m_isVisible) {
        return Rect();
    }

    float cursorX = m_paddingLeft + measureTextToPosition(m_cursorPosition) - m_scrollOffset;
    
    // Get style and font provider via UIContext instead of deprecated singletons
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    IFontProvider* fontProvider = m_ownerContext ? m_ownerContext->getFontProvider() : nullptr;
    
    if (!style || !fontProvider) {
        return Rect();
    }
    
    FontHandle westernFont = style->getDefaultLabelFont();
    FontMetrics metrics = fontProvider->getFontMetrics(westernFont, m_fontSize);
    
    float contentHeight = m_bounds.height - m_paddingTop - m_paddingBottom;
    float textTopY = m_paddingTop + (contentHeight - metrics.lineHeight) * 0.5f;
    float cursorY = textTopY;
    float cursorHeight = metrics.lineHeight;

    Rect result(cursorX, cursorY, 2.0f, cursorHeight);

    return result;
}

bool TextInput::validateText(const std::string& text) const {
    if (m_validator) {
        return m_validator(text);
    }
    return true;
}

void TextInput::notifyTextChanged() {
    if (m_changeCallback) {
        m_changeCallback(m_text);
    }
}


void TextInput::focusInEvent(FocusReason reason) {
    m_hasFocus = true;
    m_cursorBlinkTimer = 0.0f;
    m_showCursor = true;
    
    if (m_ownerContext) {
        bool shouldDisableIME = this->shouldDisableIME();
        m_ownerContext->requestTextInput(!shouldDisableIME);
        m_ownerContext->setIMEEnabled(!shouldDisableIME);
    }
}

void TextInput::focusOutEvent(FocusReason reason) {
    m_hasFocus = false;
    m_showCursor = false;
    clearSelection();
    
    if (m_ownerContext) {
        m_ownerContext->requestTextInput(false);
        m_ownerContext->setIMEEnabled(false);
    }
}

}
