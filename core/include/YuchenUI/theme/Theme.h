#pragma once

#include "YuchenUI/core/Types.h"
#include "YuchenUI/text/IFontProvider.h"
#include "YuchenUI/widgets/WidgetsType.h"
#include <string>

namespace YuchenUI {

class RenderList;

struct ButtonDrawInfo {
    Rect bounds;
    std::string text;
    FontFallbackChain fallbackChain;
    float fontSize;
    Vec4 textColor;
    bool isHovered;
    bool isPressed;
    bool isEnabled;
};

struct TextLabelDrawInfo {
    Rect bounds;
    std::string text;
    FontFallbackChain fallbackChain;
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
    FontFallbackChain titleFallbackChain;
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
    FontFallbackChain fallbackChain;
    float fontSize;
};

struct SpinBoxDrawInfo {
    Rect bounds;
    std::string displayText;
    FontFallbackChain fallbackChain;
    float fontSize;
    bool isEditing;
    bool isHovered;
    bool isEnabled;
    bool showCursor;
    size_t cursorPosition;
    float paddingLeft;
    float paddingTop;
    float paddingRight;
    float paddingBottom;
};

enum class WindowType;

enum class CheckBoxState;

struct CheckBoxDrawInfo {
    Rect bounds;
    CheckBoxState state;
    bool isHovered;
    bool isEnabled;
};

struct RadioButtonDrawInfo {
    Rect bounds;
    bool isChecked;
    bool isHovered;
    bool isEnabled;
};

struct KnobDrawInfo {
    Rect bounds;
    int currentFrame;
    int frameCount;
    Vec2 frameSize;
    KnobType type;
    bool isActive;
    bool isEnabled;
};

struct LevelMeterColors {
    Vec4 levelNormal;
    Vec4 levelWarning;
    Vec4 levelPeak;
    Vec4 bgNormal;
    Vec4 bgWarning;
    Vec4 bgPeak;
    Vec4 border;
    Vec4 peakIndicatorActive;
    Vec4 peakIndicatorInactive;
    Vec4 scaleColor;
    Vec4 internalScaleNormalActive;
    Vec4 internalScaleNormalInactive;
    Vec4 internalScaleWarningActive;
    Vec4 internalScaleWarningInactive;
    Vec4 internalScalePeakActive;
    Vec4 internalScalePeakInactive;
};

class UIStyle {
public:
    virtual ~UIStyle() = default;
    static constexpr float FOCUS_INDICATOR_BORDER_WIDTH = 1.0f;
    virtual void drawNormalButton(const ButtonDrawInfo& info, RenderList& cmdList) = 0;
    virtual void drawPrimaryButton(const ButtonDrawInfo& info, RenderList& cmdList) = 0;
    virtual void drawDestructiveButton(const ButtonDrawInfo& info, RenderList& cmdList) = 0;
    virtual void drawFrame(const FrameDrawInfo& info, RenderList& cmdList) = 0;
    virtual void drawGroupBox(const GroupBoxDrawInfo& info, RenderList& cmdList) = 0;
    virtual void drawScrollbarTrack(const ScrollbarTrackDrawInfo& info, RenderList& cmdList) = 0;
    virtual void drawScrollbarThumb(const ScrollbarThumbDrawInfo& info, RenderList& cmdList) = 0;
    virtual void drawScrollbarButton(const ScrollbarButtonDrawInfo& info, RenderList& cmdList) = 0;
    virtual void drawTextInput(const TextInputDrawInfo& info, RenderList& cmdList) = 0;
    virtual void drawSpinBox(const SpinBoxDrawInfo& info, RenderList& cmdList) = 0;
    virtual void drawComboBox(const ComboBoxDrawInfo& info, RenderList& cmdList) = 0;
    virtual void drawFocusIndicator(const FocusIndicatorDrawInfo& info, RenderList& cmdList) = 0;
    virtual void drawCheckBox(const CheckBoxDrawInfo& info, RenderList& cmdList) = 0;
    virtual void drawRadioButton(const RadioButtonDrawInfo& info, RenderList& cmdList) = 0;
    virtual void drawKnob(const KnobDrawInfo& info, RenderList& cmdList) = 0;
    virtual Vec4 getWindowBackground(WindowType type) const = 0;
    virtual Vec4 getDefaultTextColor() const = 0;
    virtual FontFallbackChain getDefaultButtonFontChain() const = 0;
    virtual FontFallbackChain getDefaultLabelFontChain() const = 0;
    virtual FontFallbackChain getDefaultTitleFontChain() const = 0;
    virtual Vec4 getDefaultFrameBackground() const = 0;
    virtual Vec4 getDefaultFrameBorder() const = 0;
    virtual Vec4 getDefaultGroupBoxBackground() const = 0;
    virtual Vec4 getDefaultGroupBoxBorder() const = 0;
    virtual Vec4 getDefaultScrollAreaBackground() const = 0;
    virtual float getGroupBoxTitleBarHeight() const = 0;
    virtual void setFontProvider(IFontProvider* provider) { m_fontProvider = provider; }

    virtual IFontProvider* getFontProvider() const;
    
    virtual LevelMeterColors getLevelMeterColors() const = 0;
protected:
    IFontProvider* m_fontProvider = nullptr;
};

class ProtoolsDarkStyle : public UIStyle {
public:
    ProtoolsDarkStyle();
    ~ProtoolsDarkStyle() override = default;
    
    void drawNormalButton(const ButtonDrawInfo& info, RenderList& cmdList) override;
    void drawPrimaryButton(const ButtonDrawInfo& info, RenderList& cmdList) override;
    void drawDestructiveButton(const ButtonDrawInfo& info, RenderList& cmdList) override;
    void drawFrame(const FrameDrawInfo& info, RenderList& cmdList) override;
    void drawGroupBox(const GroupBoxDrawInfo& info, RenderList& cmdList) override;
    void drawScrollbarTrack(const ScrollbarTrackDrawInfo& info, RenderList& cmdList) override;
    void drawScrollbarThumb(const ScrollbarThumbDrawInfo& info, RenderList& cmdList) override;
    void drawScrollbarButton(const ScrollbarButtonDrawInfo& info, RenderList& cmdList) override;
    void drawTextInput(const TextInputDrawInfo& info, RenderList& cmdList) override;
    void drawComboBox(const ComboBoxDrawInfo& info, RenderList& cmdList) override;
    void drawFocusIndicator(const FocusIndicatorDrawInfo& info, RenderList& cmdList) override;
    void drawSpinBox(const SpinBoxDrawInfo& info, RenderList& cmdList) override;
    void drawCheckBox(const CheckBoxDrawInfo& info, RenderList& cmdList) override;
    void drawRadioButton(const RadioButtonDrawInfo& info, RenderList& cmdList) override;
    void drawKnob(const KnobDrawInfo& info, RenderList& cmdList) override;
    
    Vec4 getWindowBackground(WindowType type) const override;
    Vec4 getDefaultTextColor() const override;
    FontFallbackChain getDefaultButtonFontChain() const override;
    FontFallbackChain getDefaultLabelFontChain() const override;
    FontFallbackChain getDefaultTitleFontChain() const override;
    
    Vec4 getDefaultFrameBackground() const override;
    Vec4 getDefaultFrameBorder() const override;
    Vec4 getDefaultGroupBoxBackground() const override;
    Vec4 getDefaultGroupBoxBorder() const override;
    Vec4 getDefaultScrollAreaBackground() const override;
    LevelMeterColors getLevelMeterColors() const override;
    float getGroupBoxTitleBarHeight() const override;

private:
    Vec4 m_uiTextEnabledColor;
    Vec4 m_uiTextDisabledColor;
    Vec4 m_uiThemeColorText;
};



class ProtoolsClassicStyle : public UIStyle {
public:
    ProtoolsClassicStyle();
    ~ProtoolsClassicStyle() override = default;
    
    void drawNormalButton(const ButtonDrawInfo& info, RenderList& cmdList) override;
    void drawPrimaryButton(const ButtonDrawInfo& info, RenderList& cmdList) override;
    void drawDestructiveButton(const ButtonDrawInfo& info, RenderList& cmdList) override;
    void drawFrame(const FrameDrawInfo& info, RenderList& cmdList) override;
    void drawGroupBox(const GroupBoxDrawInfo& info, RenderList& cmdList) override;
    void drawScrollbarTrack(const ScrollbarTrackDrawInfo& info, RenderList& cmdList) override;
    void drawScrollbarThumb(const ScrollbarThumbDrawInfo& info, RenderList& cmdList) override;
    void drawScrollbarButton(const ScrollbarButtonDrawInfo& info, RenderList& cmdList) override;
    void drawTextInput(const TextInputDrawInfo& info, RenderList& cmdList) override;
    void drawComboBox(const ComboBoxDrawInfo& info, RenderList& cmdList) override;
    void drawFocusIndicator(const FocusIndicatorDrawInfo& info, RenderList& cmdList) override;
    void drawSpinBox(const SpinBoxDrawInfo& info, RenderList& cmdList) override;
    
    void drawCheckBox(const CheckBoxDrawInfo& info, RenderList& cmdList) override;
    void drawRadioButton(const RadioButtonDrawInfo& info, RenderList& cmdList) override;
    void drawKnob(const KnobDrawInfo& info, RenderList& cmdList) override;
    
    Vec4 getWindowBackground(WindowType type) const override;
    Vec4 getDefaultTextColor() const override;
    FontFallbackChain getDefaultButtonFontChain() const override;
    FontFallbackChain getDefaultLabelFontChain() const override;
    FontFallbackChain getDefaultTitleFontChain() const override;
    Vec4 getDefaultFrameBackground() const override;
    Vec4 getDefaultFrameBorder() const override;
    Vec4 getDefaultGroupBoxBackground() const override;
    Vec4 getDefaultGroupBoxBorder() const override;
    Vec4 getDefaultScrollAreaBackground() const override;
    LevelMeterColors getLevelMeterColors() const override;
    float getGroupBoxTitleBarHeight() const override;

private:
    Vec4 m_uiTextEnabledColor;
    Vec4 m_uiTextDisabledColor;
    Vec4 m_uiThemeColorText;
};

}
