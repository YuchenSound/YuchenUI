#pragma once

#include "YuchenUI/core/Types.h"
#include <string>

namespace YuchenUI {

class RenderList;

struct ButtonDrawInfo {
    Rect bounds;
    std::string text;
    FontHandle westernFont;
    FontHandle chineseFont;
    float fontSize;
    Vec4 textColor;
    bool isHovered;
    bool isPressed;
    bool isEnabled;
};

struct TextLabelDrawInfo {
    Rect bounds;
    std::string text;
    FontHandle westernFont;
    FontHandle chineseFont;
    float fontSize;
    Vec4 textColor;
    TextAlignment horizontalAlignment;
    VerticalAlignment verticalAlignment;
    float paddingLeft;
    float paddingTop;
    float paddingRight;
    float paddingBottom;
};

struct FrameDrawInfo {
    Rect bounds;
    Vec4 backgroundColor;
    Vec4 borderColor;
    float borderWidth;
    CornerRadius cornerRadius;
};

struct GroupBoxDrawInfo {
    Rect bounds;
    std::string title;
    FontHandle titleFont;
    float titleFontSize;
    Vec4 titleColor;
    Vec4 backgroundColor;
    Vec4 borderColor;
    float borderWidth;
    CornerRadius cornerRadius;
};

struct TextInputDrawInfo {
    Rect bounds;
    std::string text;
    std::string placeholder;
    bool hasFocus;
    bool isHovered;
    bool isEnabled;
    bool isEmpty;
    
    bool showCursor;
    float cursorX;
    float cursorHeight;
    
    bool hasSelection;
    float selectionStartX;
    float selectionWidth;
    
    float textX;
    float textY;
    
    float fontSize;
};

struct FocusIndicatorDrawInfo {
    Rect bounds;
    CornerRadius cornerRadius;
    
    FocusIndicatorDrawInfo() : bounds(), cornerRadius() {}
};

enum class ScrollbarOrientation {
    Vertical,
    Horizontal
};

enum class ScrollbarButtonType {
    UpLeft,
    DownRight
};

enum class ScrollbarButtonState {
    Normal,
    Hovered,
    Pressed
};

struct ScrollbarTrackDrawInfo {
    Rect bounds;
    ScrollbarOrientation orientation;
};

struct ScrollbarThumbDrawInfo {
    Rect bounds;
    ScrollbarOrientation orientation;
    bool isHovered;
    bool isDragging;
};

struct ScrollbarButtonDrawInfo {
    Rect bounds;
    ScrollbarOrientation orientation;
    ScrollbarButtonType buttonType;
    ScrollbarButtonState buttonState;
};

enum class ComboBoxTheme;

struct ComboBoxDrawInfo {
    Rect bounds;
    std::string text;
    std::string placeholder;
    bool isEmpty;
    bool isHovered;
    bool isEnabled;
    ComboBoxTheme theme;
    FontHandle westernFont;
    FontHandle chineseFont;
    float fontSize;
};

struct SpinBoxDrawInfo {
    Rect bounds;
    bool isEditing;
    bool isHovered;
    bool isEnabled;
};

enum class WindowType;

class UIStyle {
public:
    virtual ~UIStyle() = default;
    static constexpr float FOCUS_INDICATOR_BORDER_WIDTH = 1.0f;

    virtual void drawButton(const ButtonDrawInfo& info, RenderList& cmdList) = 0;
    virtual void drawConfirmButton(const ButtonDrawInfo& info, RenderList& cmdList) = 0;
    virtual void drawCancelButton(const ButtonDrawInfo& info, RenderList& cmdList) = 0;
    virtual void drawFrame(const FrameDrawInfo& info, RenderList& cmdList) = 0;
    virtual void drawGroupBox(const GroupBoxDrawInfo& info, RenderList& cmdList) = 0;
    
    virtual void drawScrollbarTrack(const ScrollbarTrackDrawInfo& info, RenderList& cmdList) = 0;
    virtual void drawScrollbarThumb(const ScrollbarThumbDrawInfo& info, RenderList& cmdList) = 0;
    virtual void drawScrollbarButton(const ScrollbarButtonDrawInfo& info, RenderList& cmdList) = 0;
    
    virtual void drawTextInput(const TextInputDrawInfo& info, RenderList& cmdList) = 0;
    virtual void drawSpinBox(const SpinBoxDrawInfo& info, RenderList& cmdList) = 0;
    
    virtual void drawComboBox(const ComboBoxDrawInfo& info, RenderList& cmdList) = 0;
    virtual void drawFocusIndicator(const FocusIndicatorDrawInfo& info, RenderList& cmdList) = 0;
    virtual Vec4 getWindowBackground(WindowType type) const = 0;

    virtual Vec4 getDefaultTextColor() const = 0;
    virtual FontHandle getDefaultButtonFont() const = 0;
    virtual FontHandle getDefaultLabelFont() const = 0;
    virtual FontHandle getDefaultTitleFont() const = 0;
    
    virtual Vec4 getDefaultFrameBackground() const = 0;
    virtual Vec4 getDefaultFrameBorder() const = 0;
    virtual Vec4 getDefaultGroupBoxBackground() const = 0;
    virtual Vec4 getDefaultGroupBoxBorder() const = 0;
    virtual Vec4 getDefaultScrollAreaBackground() const = 0;

};

class ProtoolsDarkStyle : public UIStyle {
public:
    ProtoolsDarkStyle();
    ~ProtoolsDarkStyle() override = default;
    
    void drawButton(const ButtonDrawInfo& info, RenderList& cmdList) override;
    void drawConfirmButton(const ButtonDrawInfo& info, RenderList& cmdList) override;
    void drawCancelButton(const ButtonDrawInfo& info, RenderList& cmdList) override;
    void drawFrame(const FrameDrawInfo& info, RenderList& cmdList) override;
    void drawGroupBox(const GroupBoxDrawInfo& info, RenderList& cmdList) override;
    
    void drawScrollbarTrack(const ScrollbarTrackDrawInfo& info, RenderList& cmdList) override;
    void drawScrollbarThumb(const ScrollbarThumbDrawInfo& info, RenderList& cmdList) override;
    void drawScrollbarButton(const ScrollbarButtonDrawInfo& info, RenderList& cmdList) override;
    
    void drawTextInput(const TextInputDrawInfo& info, RenderList& cmdList) override;
    void drawComboBox(const ComboBoxDrawInfo& info, RenderList& cmdList) override;
    void drawFocusIndicator(const FocusIndicatorDrawInfo& info, RenderList& cmdList) override;
    void drawSpinBox(const SpinBoxDrawInfo& info, RenderList& cmdList) override;
    Vec4 getWindowBackground(WindowType type) const override;
    Vec4 getDefaultTextColor() const override;
    FontHandle getDefaultButtonFont() const override;
    FontHandle getDefaultLabelFont() const override;
    FontHandle getDefaultTitleFont() const override;
    
    Vec4 getDefaultFrameBackground() const override;
    Vec4 getDefaultFrameBorder() const override;
    Vec4 getDefaultGroupBoxBackground() const override;
    Vec4 getDefaultGroupBoxBorder() const override;
    Vec4 getDefaultScrollAreaBackground() const override;
private:
    Vec4 m_mainWindowBg;
    Vec4 m_dialogBg;
    Vec4 m_toolWindowBg;
    Vec4 m_textColor;
    Vec4 m_textDisabledColor;
    
    Vec4 m_buttonNormal;
    Vec4 m_buttonHover;
    Vec4 m_buttonPressed;
    Vec4 m_buttonDisabled;
    
    Vec4 m_confirmNormal;
    Vec4 m_confirmHover;
    Vec4 m_confirmPressed;
    
    Vec4 m_borderNormal;
    Vec4 m_borderHover;
    Vec4 m_borderPressed;
    
    Vec4 m_frameBg;
    Vec4 m_frameBorder;
    Vec4 m_groupBoxBg;
    Vec4 m_groupBoxBorder;
    
    Vec4 m_scrollAreaBg;

    Vec4 m_scrollbarBackground;
    Vec4 m_scrollbarThumb;
    Vec4 m_scrollbarThumbHovered;
    Vec4 m_scrollbarButtonNormal;
    Vec4 m_scrollbarButtonHovered;
    Vec4 m_scrollbarButtonPressed;
    Vec4 m_scrollbarButtonBorder;
    Vec4 m_scrollbarTriangleNormal;
    Vec4 m_scrollbarTriangleHovered;
    Vec4 m_scrollbarTrianglePressed;
    
    void drawButtonInternal(const ButtonDrawInfo& info, RenderList& cmdList,
                           const Vec4& normalColor, const Vec4& hoverColor,
                           const Vec4& pressedColor, const Vec4& disabledColor,
                           const Vec4& borderNormal, const Vec4& borderHover,
                           const Vec4& borderPressed);
};

class ProtoolsClassicStyle : public UIStyle {
public:
    ProtoolsClassicStyle();
    ~ProtoolsClassicStyle() override = default;
    
    void drawButton(const ButtonDrawInfo& info, RenderList& cmdList) override;
    void drawConfirmButton(const ButtonDrawInfo& info, RenderList& cmdList) override;
    void drawCancelButton(const ButtonDrawInfo& info, RenderList& cmdList) override;
    void drawFrame(const FrameDrawInfo& info, RenderList& cmdList) override;
    void drawGroupBox(const GroupBoxDrawInfo& info, RenderList& cmdList) override;
    
    void drawScrollbarTrack(const ScrollbarTrackDrawInfo& info, RenderList& cmdList) override;
    void drawScrollbarThumb(const ScrollbarThumbDrawInfo& info, RenderList& cmdList) override;
    void drawScrollbarButton(const ScrollbarButtonDrawInfo& info, RenderList& cmdList) override;
    
    void drawTextInput(const TextInputDrawInfo& info, RenderList& cmdList) override;
    void drawComboBox(const ComboBoxDrawInfo& info, RenderList& cmdList) override;
    void drawFocusIndicator(const FocusIndicatorDrawInfo& info, RenderList& cmdList) override;
    void drawSpinBox(const SpinBoxDrawInfo& info, RenderList& cmdList) override;
    Vec4 getWindowBackground(WindowType type) const override;
    Vec4 getDefaultTextColor() const override;
    FontHandle getDefaultButtonFont() const override;
    FontHandle getDefaultLabelFont() const override;
    FontHandle getDefaultTitleFont() const override;
    
    Vec4 getDefaultFrameBackground() const override;
    Vec4 getDefaultFrameBorder() const override;
    Vec4 getDefaultGroupBoxBackground() const override;
    Vec4 getDefaultGroupBoxBorder() const override;
    Vec4 getDefaultScrollAreaBackground() const override;
private:
    Vec4 m_mainWindowBg;
    Vec4 m_dialogBg;
    Vec4 m_toolWindowBg;
    Vec4 m_textColor;
    Vec4 m_textDisabledColor;
    
    Vec4 m_buttonNormal;
    Vec4 m_buttonHover;
    Vec4 m_buttonPressed;
    Vec4 m_buttonDisabled;
    
    Vec4 m_confirmNormal;
    Vec4 m_confirmHover;
    Vec4 m_confirmPressed;
    
    Vec4 m_borderNormal;
    Vec4 m_borderHover;
    Vec4 m_borderPressed;
    
    Vec4 m_frameBg;
    Vec4 m_frameBorder;
    Vec4 m_groupBoxBg;
    Vec4 m_groupBoxBorder;
    
    Vec4 m_scrollAreaBg;

    Vec4 m_scrollbarBackground;
    Vec4 m_scrollbarThumb;
    Vec4 m_scrollbarThumbHovered;
    Vec4 m_scrollbarButtonNormal;
    Vec4 m_scrollbarButtonHovered;
    Vec4 m_scrollbarButtonPressed;
    Vec4 m_scrollbarButtonBorder;
    Vec4 m_scrollbarTriangleNormal;
    Vec4 m_scrollbarTriangleHovered;
    Vec4 m_scrollbarTrianglePressed;
};

}
