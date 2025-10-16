#pragma once

#include "YuchenUI/widgets/UIComponent.h"
#include "YuchenUI/platform/IInputMethodSupport.h"
#include "YuchenUI/core/Types.h"
#include <string>
#include <functional>

namespace YuchenUI {

class RenderList;

using SpinBoxValueChangedCallback = std::function<void(double)>;

class SpinBox : public UIComponent, public IInputMethodSupport {
public:
    explicit SpinBox(const Rect& bounds);
    virtual ~SpinBox();
    
    void setValue(double value);
    double getValue() const { return m_value; }
    
    void setMinValue(double min);
    double getMinValue() const { return m_minValue; }
    
    void setMaxValue(double max);
    double getMaxValue() const { return m_maxValue; }
    
    void setStep(double step);
    double getStep() const { return m_step; }
    
    void setPrecision(int precision);
    int getPrecision() const { return m_precision; }
    
    void setSuffix(const std::string& suffix);
    const std::string& getSuffix() const { return m_suffix; }
    
    void setValueChangedCallback(SpinBoxValueChangedCallback callback);

    void setFontSize(float fontSize);
    float getFontSize() const { return m_fontSize; }
    
    void setHasBackground(bool hasBackground);
    bool hasBackground() const { return m_hasBackground; }
    
    void addDrawCommands(RenderList& commandList, const Vec2& offset = Vec2()) const override;
    bool handleMouseMove(const Vec2& position, const Vec2& offset = Vec2()) override;
    bool handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset = Vec2()) override;
    bool handleMouseWheel(const Vec2& delta, const Vec2& position, const Vec2& offset = Vec2()) override;
    bool handleKeyPress(const Event& event) override;
    bool handleTextInput(uint32_t codepoint) override;
    void update(float deltaTime) override;
    
    void setFocusable(bool focusable);

    
    Rect getInputMethodCursorRect() const override;
    
    bool isValid() const;

protected:
    void focusInEvent(FocusReason reason) override;
    void focusOutEvent(FocusReason reason) override;
private:
    void enterEditMode();
    void exitEditMode();
    void applyValue();
    void clampValue();
    std::string formatValue() const;
    std::string formatValueWithSuffix() const;
    bool isValidChar(char c) const;
    bool canInsertChar(char c) const;
    void insertCharAtCursor(char c);
    void deleteCharBeforeCursor();
    void deleteCharAfterCursor();
    void moveCursor(int delta);
    void moveCursorToStart();
    void moveCursorToEnd();
    void selectAll();
    void adjustValueByDrag(const Vec2& currentPos);
    void adjustValueByStep(double multiplier);
    double parseInputBuffer() const;
    
    double m_value;
    double m_minValue;
    double m_maxValue;
    double m_step;
    int m_precision;
    std::string m_suffix;
    float m_fontSize;
    
    bool m_isEditing;
    bool m_isHovered;
    bool m_isDragging;
    Vec2 m_dragStartPos;
    double m_dragStartValue;
    
    std::string m_inputBuffer;
    size_t m_cursorPosition;
    bool m_showCursor;
    float m_cursorBlinkTimer;
    
    SpinBoxValueChangedCallback m_valueChangedCallback;
    
    float m_paddingLeft;
    float m_paddingTop;
    float m_paddingRight[[maybe_unused]];
    float m_paddingBottom;
    
    bool m_hasBackground;
    
    static constexpr float CURSOR_BLINK_INTERVAL = 0.53f;
    static constexpr float DRAG_SENSITIVITY = 0.1f;
};

}
