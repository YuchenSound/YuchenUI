#pragma once

#include "YuchenUI/widgets/UIComponent.h"
#include "YuchenUI/windows/IWindowContent.h"
#include "YuchenUI/widgets/IInputMethodSupport.h"

#include "YuchenUI/core/Types.h"
#include <string>
#include <functional>

namespace YuchenUI {

class RenderList;

using TextInputChangeCallback = std::function<void(const std::string&)>;
using TextInputValidator = std::function<bool(const std::string&)>;
using TextInputSubmitCallback = std::function<void(const std::string&)>;

enum class TextInputType {
    Text,
    Password,
    Number
};

class TextInput : public UIComponent, public IInputMethodSupport {
public:
    explicit TextInput(const Rect& bounds);
    virtual ~TextInput();
    
    void addDrawCommands(RenderList& commandList, const Vec2& offset = Vec2()) const override;
    bool handleMouseMove(const Vec2& position, const Vec2& offset = Vec2()) override;
    bool handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset = Vec2()) override;
    
    bool handleKeyPress(const Event& event) override;
    bool handleTextInput(uint32_t codepoint) override;
    bool handleComposition(const char* text, int cursorPos, int selectionLength) override;
    void update(float deltaTime) override;

    void setText(const std::string& text);
    const std::string& getText() const { return m_text; }
    
    void setPlaceholder(const std::string& placeholder);
    const std::string& getPlaceholder() const { return m_placeholder; }
    
    void setMaxLength(size_t maxLength);
    size_t getMaxLength() const { return m_maxLength; }
    
    void setPasswordMode(bool enabled);
    bool isPasswordMode() const { return m_isPasswordMode; }
    
    void setInputType(TextInputType type);
    TextInputType getInputType() const { return m_inputType; }
    
    bool shouldDisableIME() const;
    
    void setValidator(TextInputValidator validator);
    
    void setChangeCallback(TextInputChangeCallback callback);
    void setSubmitCallback(TextInputSubmitCallback callback);
    
    void setBounds(const Rect& bounds);
    const Rect& getBounds() const override { return m_bounds; }

    void setFontSize(float fontSize);
    float getFontSize() const { return m_fontSize; }
    
    void setTextColor(const Vec4& color);
    Vec4 getTextColor() const;
    void resetTextColor();
    
    void setPadding(float padding);
    void setPadding(float left, float top, float right, float bottom);
    
    void focus();
    void blur();
    bool hasFocus() const { return m_hasFocus; }
    
    void selectAll();
    void clearSelection();
    bool hasSelection() const;
        
    bool isValid() const;
    bool canReceiveFocus() const override { return m_isEnabled && m_isVisible; }
    void onFocusGained() override;
    void onFocusLost() override;
    
    Rect getInputMethodCursorRect() const override;

protected:
    CornerRadius getFocusIndicatorCornerRadius() const override;

private:
    static std::u32string utf8ToUtf32(const std::string& utf8);
    static std::string utf32ToUtf8(const std::u32string& utf32);
    
    void insertTextAtCursor(const std::string& text);
    void deleteSelection();
    void deleteCharacterBefore();
    void deleteCharacterAfter();
    
    void moveCursor(int delta, bool extendSelection);
    void moveCursorToStart(bool extendSelection);
    void moveCursorToEnd(bool extendSelection);
    
    void selectWord();
    size_t findWordBoundary(size_t pos, bool forward) const;
    
    void copy();
    void cut();
    void paste();
    
    float measureTextToPosition(size_t charIndex) const;
    size_t positionToCharIndex(float x, const Vec2& offset) const;
    
    void adjustScrollToCursor();
    
    bool validateText(const std::string& text) const;
    
    void notifyTextChanged();
    
    std::string m_text;
    std::u32string m_textUTF32;
    std::string m_placeholder;
    
    size_t m_cursorPosition;
    size_t m_selectionStart;
    size_t m_selectionEnd;
    
    float m_scrollOffset;
    size_t m_maxLength;
    
    bool m_hasFocus;
    bool m_isPasswordMode;
    bool m_isHovered;
    bool m_showCursor;
    float m_cursorBlinkTimer;
    
    Rect m_bounds;
    float m_fontSize;
    Vec4 m_textColor;
    bool m_hasCustomTextColor;
    
    float m_paddingLeft;
    float m_paddingTop;
    float m_paddingRight;
    float m_paddingBottom;
    
    TextInputValidator m_validator;
    TextInputChangeCallback m_changeCallback;
    TextInputSubmitCallback m_submitCallback;
    
    bool m_isDragging;
    size_t m_dragStartPosition;
    
    char m_compositionText[256];
    int m_compositionCursorPos;
    int m_compositionSelectionLength;
    
    TextInputType m_inputType;
    
    static constexpr float CURSOR_BLINK_INTERVAL = 0.53f;
    static constexpr float CURSOR_WIDTH = 1.0f;
};

}
