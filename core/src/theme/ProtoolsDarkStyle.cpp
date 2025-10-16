/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Text module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/
//==========================================================================================
/** @file
 
    [SECTION] - Window Background
    [SECTION] - Font Definition
    [SECTION] - Focus Indicator
    **Components**
    [SECTION] - Frame
    [SECTION] - Group Box
    [SECTION] - Push Button
    [SECTION] - Knob
    [SECTION] - Check Box
    [SECTION] - Radio Button
    [SECTION] - Text Input
    [SECTION] - Spin Box
    [SECTION] - Combo Box
    [SECTION] - Scrollbar
    [SECTION] - LevelMeter
*/

#include "YuchenUI/theme/Theme.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/text/FontManager.h"
#include "YuchenUI/core/Types.h"
#include "YuchenUI/widgets/ScrollArea.h"
#include "YuchenUI/widgets/ComboBox.h"
#include "YuchenUI/widgets/CheckBox.h"
#include "YuchenUI/widgets/RadioButton.h"
#include "YuchenUI/core/Assert.h"

namespace YuchenUI {
//==========================================================================================
ProtoolsDarkStyle::ProtoolsDarkStyle()
    : m_uiTextEnabledColor(Vec4::FromRGBA(255, 255, 255, 196))
    , m_uiTextDisabledColor(Vec4::FromRGBA(120, 120, 120, 255))
    , m_uiThemeColorText(Vec4::FromRGBA(56, 209, 119, 255))
{}

//==========================================================================================
// [SECTION] - Window Background
Vec4 ProtoolsDarkStyle::getWindowBackground(WindowType type) const
{
    switch (type)
    {
        case WindowType::Main:          return Vec4::FromRGBA(75, 75, 75, 255);
        case WindowType::Dialog:        return Vec4::FromRGBA(75, 75, 75, 255);
        case WindowType::ToolWindow:    return Vec4::FromRGBA(75, 75, 75, 255);
        default:                        return Vec4::FromRGBA(75, 75, 75, 255);
    }
}


//==========================================================================================
// [SECTION] - Font Definition
// DefaultFont:    Arial Regular
// DefaultCJKFont: PingFang SC / Microsoft YaHei
FontFallbackChain ProtoolsDarkStyle::getDefaultButtonFontChain() const
{
    IFontProvider* provider = getFontProvider();
    return FontFallbackChain(provider->getDefaultBoldFont(),provider->getDefaultCJKFont());
}
FontFallbackChain ProtoolsDarkStyle::getDefaultLabelFontChain() const
{
    IFontProvider* provider = getFontProvider();
    return FontFallbackChain(provider->getDefaultFont(),provider->getDefaultCJKFont());
}
FontFallbackChain ProtoolsDarkStyle::getDefaultTitleFontChain() const
{
    IFontProvider* provider = getFontProvider();
    return FontFallbackChain(provider->getDefaultBoldFont(),provider->getDefaultCJKFont());
}
Vec4 ProtoolsDarkStyle::getDefaultTextColor() const { return m_uiTextEnabledColor; }

//==========================================================================================
// [SECTION] - Focus Indicator
void ProtoolsDarkStyle::drawFocusIndicator(const FocusIndicatorDrawInfo& info, RenderList& cmdList)
{
    Vec4 focusColor = Vec4::FromRGBA(255, 200, 0, 255);
    cmdList.drawRect(info.bounds, focusColor, UIStyle::FOCUS_INDICATOR_BORDER_WIDTH, info.cornerRadius);
}

/**** ====================================== Components ====================================== ** */
// [SECTION] - Frame
void ProtoolsDarkStyle::drawFrame(const FrameDrawInfo& info, RenderList& cmdList)
{
    cmdList.fillRect(info.bounds, info.backgroundColor, info.cornerRadius);
    
    if (info.borderWidth > 0.0f) cmdList.drawRect(info.bounds, info.borderColor, info.borderWidth, info.cornerRadius);
}
Vec4 ProtoolsDarkStyle::getDefaultFrameBackground() const { return Vec4::FromRGBA(255, 255, 255, 51); }
Vec4 ProtoolsDarkStyle::getDefaultFrameBorder()     const { return Vec4::FromRGBA(255, 255, 255, 128); }

//==========================================================================================
// [SECTION] - Group Box
void ProtoolsDarkStyle::drawGroupBox(const GroupBoxDrawInfo& info, RenderList& cmdList)
{
    const float TITLE_HEIGHT = getGroupBoxTitleBarHeight();
    static constexpr float TITLE_PADDING_LEFT = 8.0f;
    static constexpr float CORNER_RADIUS = 2.0f;
    IFontProvider* fontProvider = getFontProvider();
    Vec4 blackColor = Vec4::FromRGBA(0, 0, 0, 76);
    Rect titleRect(info.bounds.x,info.bounds.y,info.bounds.width,TITLE_HEIGHT);
    CornerRadius titleCornerRadius(0.0f, 0.0f, CORNER_RADIUS, CORNER_RADIUS);
    cmdList.fillRect(titleRect, blackColor, titleCornerRadius);
    Rect contentRect(info.bounds.x,info.bounds.y + TITLE_HEIGHT,info.bounds.width,info.bounds.height - TITLE_HEIGHT);
    CornerRadius contentCornerRadius(CORNER_RADIUS, CORNER_RADIUS, 0.0f, 0.0f);
    if (info.borderWidth > 0.0f) cmdList.drawRect(contentRect, blackColor, info.borderWidth, contentCornerRadius);
    if (!info.title.empty())
    {
        FontHandle primaryFont = info.titleFallbackChain.getPrimary();
        FontMetrics metrics = fontProvider->getFontMetrics(primaryFont, info.titleFontSize);
        float textX = info.bounds.x + TITLE_PADDING_LEFT;
        float textY = info.bounds.y + (TITLE_HEIGHT - metrics.lineHeight) * 0.5f + metrics.ascender;
        Vec2 titleTextPos(textX, textY);
        Vec4 titleTextColor = m_uiTextEnabledColor;
        cmdList.drawText(info.title.c_str(), titleTextPos, info.titleFallbackChain,info.titleFontSize, titleTextColor);
    }
}
Vec4  ProtoolsDarkStyle::getDefaultGroupBoxBackground() const { return Vec4::FromRGBA(30, 30, 30, 255); }
Vec4  ProtoolsDarkStyle::getDefaultGroupBoxBorder() const     { return Vec4::FromRGBA(128, 128, 128, 255); }
float ProtoolsDarkStyle::getGroupBoxTitleBarHeight() const    { return 20.0f; }

//==========================================================================================
// [SECTION] - Push Button
void ProtoolsDarkStyle::drawNormalButton(const ButtonDrawInfo& info, RenderList& cmdList) {
    Vec4 bgColor = !info.isEnabled ? Vec4::FromRGBA(255, 255, 255, 64) :    ///< Disabled
                   info.isPressed ? Vec4::FromRGBA(255, 255, 255, 128) :    ///< Pressed
                   info.isHovered ? Vec4::FromRGBA(255, 255, 255, 64) :    ///< Hover
                   Vec4::FromRGBA(0, 0, 0, 0);                      ///< Normal
    Vec4 borderColor = !info.isEnabled ? Vec4::FromRGBA(255, 255, 255, 64) :///< Disabled
                       info.isPressed ? Vec4::FromRGBA(255, 255, 255, 128) :///< BorderPressed
                       info.isHovered ? Vec4::FromRGBA(255, 255, 255, 128) :///< BorderHover
                       Vec4::FromRGBA(255, 255, 255, 128);                  ///< BorderNormal
    cmdList.fillRect(info.bounds, bgColor, CornerRadius(2.0f));
    cmdList.drawRect(info.bounds, borderColor, 1.0f, CornerRadius(2.0f));
    if (!info.text.empty())
    {
        IFontProvider* fontProvider = getFontProvider();
        Vec2 textSize = fontProvider->measureText(info.text.c_str(), info.fontSize);
        FontHandle primaryFont = info.fallbackChain.getPrimary();
        FontMetrics metrics = fontProvider->getFontMetrics(primaryFont, info.fontSize);
        Vec2 textPos(info.bounds.x + (info.bounds.width - textSize.x) * 0.5f,info.bounds.y +
                    (info.bounds.height - metrics.lineHeight) * 0.5f + metrics.ascender);
        Vec4 textColor = info.isEnabled ? info.textColor : m_uiTextDisabledColor;
        cmdList.drawText(info.text.c_str(), textPos, info.fallbackChain,info.fontSize, textColor);
    }
}
void ProtoolsDarkStyle::drawPrimaryButton(const ButtonDrawInfo& info, RenderList& cmdList)
{
    Vec4 bgColor = !info.isEnabled ? Vec4::FromRGBA(0, 0, 0, 128) :          ///< Disabled
                   info.isPressed ? Vec4::FromRGBA(46, 152, 209, 255) :      ///< Pressed
                   info.isHovered ? Vec4::FromRGBA(46, 152, 209, 164) :      ///< Hover
                   Vec4::FromRGBA(46, 152, 209, 128);                        ///< Normal
    Vec4 borderColor = !info.isEnabled ? Vec4::FromRGBA(80, 80, 80, 255) :   ///< Disabled
                       info.isPressed ? Vec4::FromRGBA(46, 152, 209, 255) :  ///< BorderPressed
                       info.isHovered ? Vec4::FromRGBA(46, 152, 209, 255) :  ///< BorderHover
                       Vec4::FromRGBA(46, 152, 209, 255);                    ///< BorderNormal
    cmdList.fillRect(info.bounds, bgColor, CornerRadius(2.0f));
    cmdList.drawRect(info.bounds, borderColor, 1.0f, CornerRadius(2.0f));
    if (!info.text.empty())
    {
        IFontProvider* fontProvider = getFontProvider();
        Vec2 textSize = fontProvider->measureText(info.text.c_str(), info.fontSize);
        FontHandle primaryFont = info.fallbackChain.getPrimary();
        FontMetrics metrics = fontProvider->getFontMetrics(primaryFont, info.fontSize);
        Vec2 textPos(info.bounds.x + (info.bounds.width - textSize.x) * 0.5f,info.bounds.y +
                    (info.bounds.height - metrics.lineHeight) * 0.5f + metrics.ascender);
        Vec4 textColor = info.isEnabled ? info.textColor : m_uiTextDisabledColor;
        cmdList.drawText(info.text.c_str(), textPos, info.fallbackChain,info.fontSize, textColor);
    }
}
void ProtoolsDarkStyle::drawDestructiveButton(const ButtonDrawInfo &info, RenderList &cmdList)
{
    Vec4 bgColor = !info.isEnabled ? Vec4::FromRGBA(0, 0, 0, 128) :         ///< Disabled
                   info.isPressed ? Vec4::FromRGBA(255, 0, 0, 128) :        ///< Pressed
                   info.isHovered ? Vec4::FromRGBA(255, 0, 0, 60) :         ///< Hover
                   Vec4::FromRGBA(255, 0, 0, 30);                           ///< Normal
    Vec4 borderColor = !info.isEnabled ? Vec4::FromRGBA(80, 80, 80, 255) :  ///< Disabled
                       info.isPressed ? Vec4::FromRGBA(255, 0, 0, 128) :    ///< BorderPressed
                       info.isHovered ? Vec4::FromRGBA(255, 0, 0, 128) :    ///< BorderHover
                       Vec4::FromRGBA(255, 0, 0, 128);                      ///< BorderNormal
    cmdList.fillRect(info.bounds, bgColor, CornerRadius(2.0f));
    cmdList.drawRect(info.bounds, borderColor, 1.0f, CornerRadius(2.0f));
    if (!info.text.empty())
    {
        IFontProvider* fontProvider = getFontProvider();
        Vec2 textSize = fontProvider->measureText(info.text.c_str(), info.fontSize);
        FontHandle primaryFont = info.fallbackChain.getPrimary();
        FontMetrics metrics = fontProvider->getFontMetrics(primaryFont, info.fontSize);
        Vec2 textPos(info.bounds.x + (info.bounds.width - textSize.x) * 0.5f,info.bounds.y +
                    (info.bounds.height - metrics.lineHeight) * 0.5f + metrics.ascender);
        Vec4 textColor = info.isEnabled ? info.textColor : m_uiTextDisabledColor;
        cmdList.drawText(info.text.c_str(), textPos, info.fallbackChain,info.fontSize, textColor);
    }
}

//==========================================================================================
// [SECTION] - Knob
void ProtoolsDarkStyle::drawKnob(const KnobDrawInfo& info, RenderList& cmdList)
{
    std::string resourcePath = "components/knob/dark/knob_";
    if   (info.type == KnobType::Centered) {resourcePath += "centered_";}
    else {resourcePath += "no_centered_";}
    if   (info.isActive) {resourcePath += "active_29frames.png";}
    else {resourcePath += "inactive_29frames.png";}
    Rect sourceRect(0.0f,info.frameSize.y * info.currentFrame,info.frameSize.x,info.frameSize.y);
    cmdList.drawImageRegion(resourcePath.c_str(), info.bounds, sourceRect, ScaleMode::Stretch);
}

//==========================================================================================
// [SECTION] - Check Box
void ProtoolsDarkStyle::drawCheckBox(const CheckBoxDrawInfo& info, RenderList& cmdList)
{
    std::string resourcePath = "components/checkbox/dark/checkbox_";
    if (!info.isEnabled)
    {
        resourcePath += (info.state == CheckBoxState::Checked)      ?  "checked_disabled.png" :
                        (info.state == CheckBoxState::Indeterminate) ? "indeterminate_disabled.png" :
                                                                       "unchecked_disabled.png";
    }
    else
    {
        resourcePath += (info.state == CheckBoxState::Checked)      ?  "checked.png" :
                        (info.state == CheckBoxState::Indeterminate) ? "indeterminate.png" :
                                                                       "unchecked.png";
    }
    cmdList.drawImage(resourcePath.c_str(), info.bounds, ScaleMode::Original);
}

//==========================================================================================
// [SECTION] - Radio Button
void ProtoolsDarkStyle::drawRadioButton(const RadioButtonDrawInfo& info, RenderList& cmdList) {
    std::string resourcePath = "components/radio/dark/radio_";
    if (!info.isEnabled){ resourcePath += (info.isChecked) ? "checked_disabled.png" : "unchecked_disabled.png"; }
    else { resourcePath += (info.isChecked) ? "checked.png" : "unchecked.png"; }
    cmdList.drawImage(resourcePath.c_str(), info.bounds, ScaleMode::Original);
}

//==========================================================================================
// [SECTION] - Text Input
void ProtoolsDarkStyle::drawTextInput(const TextInputDrawInfo& info, RenderList& cmdList) {
    Vec4 bgColor = !info.isEnabled ? Vec4::FromRGBA(40, 40, 40, 255) : Vec4::FromRGBA(42, 42, 42, 255);
    Vec4 borderColor = !info.isEnabled ? Vec4::FromRGBA(60, 60, 60, 255) :
                       info.isHovered ? Vec4::FromRGBA(100, 100, 100, 255) :
                       Vec4::FromRGBA(80, 80, 80, 255);
    cmdList.fillRect(info.bounds, bgColor, CornerRadius(2.0f));
    cmdList.drawRect(info.bounds, borderColor, 1.0f, CornerRadius(2.0f));
    cmdList.pushClipRect(info.bounds);
    if (info.hasSelection)
    {
        Rect selectionRect(info.selectionStartX,info.bounds.y + 3.0f,info.selectionWidth,info.bounds.height - 6.0f);
        cmdList.fillRect(selectionRect, Vec4::FromRGBA(46, 152, 209, 128));
    }
    IFontProvider* fontProvider = getFontProvider();
    FontFallbackChain fallbackChain = getDefaultLabelFontChain();
    FontHandle labelFont = fallbackChain.getPrimary();
    if (!info.text.empty())
    {
        FontMetrics metrics = fontProvider->getFontMetrics(labelFont, info.fontSize);
        float textY = info.textY + metrics.ascender;
        cmdList.drawText(info.text.c_str(), Vec2(info.textX, textY),fallbackChain, info.fontSize, m_uiTextEnabledColor);
    }
    else if (!info.placeholder.empty() && !info.hasFocus)
    {
        FontMetrics metrics = fontProvider->getFontMetrics(labelFont, info.fontSize);
        float textY = info.textY + metrics.ascender;
        Vec4 placeholderColor = Vec4::FromRGBA(120, 120, 120, 255);
        cmdList.drawText(info.placeholder.c_str(), Vec2(info.textX, textY),fallbackChain, info.fontSize, placeholderColor);
    }
    if (info.showCursor)
    {
        float cursorY1 = info.bounds.y + (info.bounds.height - info.cursorHeight) * 0.5f;
        float cursorY2 = cursorY1 + info.cursorHeight;
        cmdList.drawLine(Vec2(info.cursorX, cursorY1), Vec2(info.cursorX, cursorY2),m_uiTextEnabledColor, 1.0f);
    }
    cmdList.popClipRect();
}

//==========================================================================================
// [SECTION] - Spin Box
void ProtoolsDarkStyle::drawSpinBox(const SpinBoxDrawInfo& info, RenderList& cmdList)
{
    cmdList.fillRect(info.bounds, Vec4::FromRGBA(0, 0, 0, 255), CornerRadius(2.0f));
    if (info.displayText.empty()) return;
    IFontProvider* fontProvider = getFontProvider();
    FontHandle primaryFont = info.fallbackChain.getPrimary();
    FontMetrics metrics = fontProvider->getFontMetrics(primaryFont, info.fontSize);
    float contentHeight = info.bounds.height - info.paddingTop - info.paddingBottom;
    float textY = info.bounds.y + info.paddingTop + (contentHeight - metrics.lineHeight) * 0.5f + metrics.ascender;
    float textX = info.bounds.x + info.paddingLeft;
    Vec4 textColor = m_uiThemeColorText;
    if (info.isEditing)
    {
        Vec2 textSize = fontProvider->measureText(info.displayText.c_str(), info.fontSize);
        Rect textBgRect(textX-1.0f,info.bounds.y+info.paddingTop+(contentHeight-metrics.lineHeight) * 0.5f-1.0f,textSize.x+2.0f,metrics.lineHeight+2.0f);
        cmdList.fillRect(textBgRect, m_uiThemeColorText);
        textColor = Vec4::FromRGBA(50, 50, 50, 255);
    }
    cmdList.drawText(info.displayText.c_str(), Vec2(textX, textY),info.fallbackChain, info.fontSize, textColor);
}

//==========================================================================================
// [SECTION] - Combo Box
void ProtoolsDarkStyle::drawComboBox(const ComboBoxDrawInfo& info, RenderList& cmdList)
{
    static constexpr float TEXT_PADDING_LEFT  = 4.0f;
    static constexpr float NINE_SLICE_MARGIN  = 2.0f;
    static constexpr float ARROW_BASE_SIZE    = 7.0f;
    static constexpr float ARROW_HIGHT_SIZE   = 4.0f;
    static constexpr float ARROW_MARGIN_RIGHT = 3.0f;
    static constexpr float ARROW_MARGIN_TOP   = 4.0f;
    Vec4 textColor;
    const char* backgroundResource = "components/combobox/combobox_background_black.png";
    textColor = Vec4::FromRGBA(255, 255, 255, 255);
    NineSliceMargins margins(NINE_SLICE_MARGIN, NINE_SLICE_MARGIN, NINE_SLICE_MARGIN, NINE_SLICE_MARGIN);
    cmdList.drawImage(backgroundResource, info.bounds, ScaleMode::NineSlice, margins);
    std::string displayText = info.isEmpty ? info.placeholder : info.text;
    if (!displayText.empty())
    {
        IFontProvider* fontProvider = getFontProvider();
        FontHandle primaryFont = info.fallbackChain.getPrimary();
        FontMetrics metrics = fontProvider->getFontMetrics(primaryFont, info.fontSize);
        float textX = info.bounds.x + TEXT_PADDING_LEFT;
        float textY = info.bounds.y + (info.bounds.height - metrics.lineHeight) * 0.5f + metrics.ascender;
        Vec2 textPosition(textX, textY);
        cmdList.drawText(displayText.c_str(), textPosition,info.fallbackChain, info.fontSize, textColor);
    }
    float arrowX = info.bounds.x + info.bounds.width - ARROW_MARGIN_RIGHT - ARROW_BASE_SIZE;
    float arrowY = info.bounds.y + ARROW_MARGIN_TOP;
    Rect arrowRect(arrowX, arrowY, ARROW_BASE_SIZE, ARROW_HIGHT_SIZE);
    cmdList.drawImage("components/combobox/combobox_triangle.png", arrowRect, ScaleMode::Original);
}

//==========================================================================================
// [SECTION] - Scrollbar
void ProtoolsDarkStyle::drawScrollbarTrack(const ScrollbarTrackDrawInfo& info, RenderList& cmdList)
{
    cmdList.fillRect(info.bounds, Vec4::FromRGBA(31, 31, 31, 255));
}
void ProtoolsDarkStyle::drawScrollbarThumb(const ScrollbarThumbDrawInfo& info, RenderList& cmdList)
{
    cmdList.fillRect(info.bounds, Vec4::FromRGBA(0, 0, 0, 0));
    Rect innerRect;
    if (info.orientation == ScrollbarOrientation::Vertical)
    {
        float margin = (info.bounds.width - 6.0f) / 2.0f;
        innerRect = Rect(info.bounds.x + margin,info.bounds.y + margin,6.0f,info.bounds.height - margin * 2.0f);
    }
    else
    {
        float margin = (info.bounds.height - 6.0f) / 2.0f;
        innerRect = Rect(info.bounds.x + margin,info.bounds.y + margin,info.bounds.width - margin * 2.0f,6.0f);
    }
    Vec4 thumbColor = (info.isDragging || info.isHovered) ? Vec4::FromRGBA(69, 69, 69, 255) : Vec4::FromRGBA(56, 56, 56, 255);
    cmdList.fillRect(innerRect, thumbColor);
}

void ProtoolsDarkStyle::drawScrollbarButton(const ScrollbarButtonDrawInfo& info, RenderList& cmdList) {
    Vec4 buttonColor;
    Vec4 triangleColor;
    
    switch (info.buttonState)
    {
        case ScrollbarButtonState::Pressed: buttonColor = Vec4::FromRGBA(99, 99, 99, 255); triangleColor = Vec4::FromRGBA(255, 255, 255, 255); break;
        case ScrollbarButtonState::Hovered:
            buttonColor = Vec4::FromRGBA(69, 69, 69, 255);
            triangleColor = Vec4::FromRGBA(181, 181, 181, 255);
            break;
        default:
            buttonColor = Vec4::FromRGBA(56, 56, 56, 255);
            triangleColor = Vec4::FromRGBA(150, 150, 150, 255);
            break;
    }
    
    cmdList.fillRect(info.bounds, buttonColor);
    
    float centerX = std::round(info.bounds.x + ScrollArea::BUTTON_SIZE / 2.0f);
    float centerY = std::round(info.bounds.y + ScrollArea::BUTTON_SIZE / 2.0f);
    
    Vec2 p1, p2, p3;
    
    if (info.orientation == ScrollbarOrientation::Vertical) {
        if (info.buttonType == ScrollbarButtonType::UpLeft) {
            p1 = Vec2(centerX, centerY - ScrollArea::TRIANGLE_HEIGHT / 2.0f);
            p2 = Vec2(centerX + ScrollArea::TRIANGLE_BASE / 2.0f, centerY + ScrollArea::TRIANGLE_HEIGHT / 2.0f);
            p3 = Vec2(centerX - ScrollArea::TRIANGLE_BASE / 2.0f, centerY + ScrollArea::TRIANGLE_HEIGHT / 2.0f);
        } else {
            p1 = Vec2(centerX, centerY + ScrollArea::TRIANGLE_HEIGHT / 2.0f);
            p2 = Vec2(centerX - ScrollArea::TRIANGLE_BASE / 2.0f, centerY - ScrollArea::TRIANGLE_HEIGHT / 2.0f);
            p3 = Vec2(centerX + ScrollArea::TRIANGLE_BASE / 2.0f, centerY - ScrollArea::TRIANGLE_HEIGHT / 2.0f);
        }
    } else {
        if (info.buttonType == ScrollbarButtonType::UpLeft) {
            p1 = Vec2(centerX - ScrollArea::TRIANGLE_HEIGHT / 2.0f, centerY);
            p2 = Vec2(centerX + ScrollArea::TRIANGLE_HEIGHT / 2.0f, centerY + ScrollArea::TRIANGLE_HEIGHT / 2.0f);
            p3 = Vec2(centerX + ScrollArea::TRIANGLE_HEIGHT / 2.0f, centerY - ScrollArea::TRIANGLE_HEIGHT / 2.0f);
        } else {
            p1 = Vec2(centerX + ScrollArea::TRIANGLE_HEIGHT / 2.0f, centerY);
            p2 = Vec2(centerX - ScrollArea::TRIANGLE_HEIGHT / 2.0f, centerY - ScrollArea::TRIANGLE_HEIGHT / 2.0f);
            p3 = Vec2(centerX - ScrollArea::TRIANGLE_HEIGHT / 2.0f, centerY + ScrollArea::TRIANGLE_HEIGHT / 2.0f);
        }
    }
    
    cmdList.fillTriangle(p1, p2, p3, triangleColor);
}

Vec4 ProtoolsDarkStyle::getDefaultScrollAreaBackground() const { return Vec4::FromRGBA(47, 47, 47, 255); }

//==========================================================================================
// [SECTION] - Level Meter
LevelMeterColors ProtoolsDarkStyle::getLevelMeterColors() const
{
    LevelMeterColors colors;
    colors.levelNormal                  = Vec4::FromRGBA(37,  173, 0,  255);
    colors.levelWarning                 = Vec4::FromRGBA(109, 250, 0,  255);
    colors.levelPeak                    = Vec4::FromRGBA(253, 190, 0,  255);
    colors.bgNormal                     = Vec4::FromRGBA(55,  52,  3,  255);
    colors.bgWarning                    = Vec4::FromRGBA(39,  40,  3,  255);
    colors.bgPeak                       = Vec4::FromRGBA(36,  33,  15, 255);
    colors.border                       = Vec4::FromRGBA(0,   0,   0,  255);
    colors.peakIndicatorActive          = Vec4::FromRGBA(253, 190, 0,  255);
    colors.peakIndicatorInactive        = Vec4::FromRGBA(49,  4,   1,  255);
    colors.scaleColor                   = Vec4::FromRGBA(255, 255, 255,178); // 70% Transparent white text
    colors.internalScaleNormalActive    = Vec4::FromRGBA(81,  203, 40, 255);
    colors.internalScaleNormalInactive  = Vec4::FromRGBA(37,  71,  4,  255);
    colors.internalScaleWarningActive   = Vec4::FromRGBA(47,  118, 0,  255);
    colors.internalScaleWarningInactive = Vec4::FromRGBA(89,  63,  1,  255);
    colors.internalScalePeakActive      = Vec4::FromRGBA(233, 156, 1,  255);
    colors.internalScalePeakInactive    = Vec4::FromRGBA(92,  50,  5,  255);
    return colors;
}

//==========================================================================================
// [SECTION] - Fader
FaderColors ProtoolsDarkStyle::getFaderColors() const
{
    FaderColors colors;
    colors.scaleColor = Vec4::FromRGBA(255, 255, 255, 178);
    colors.scaleLineColor = Vec4::FromRGBA(255, 255, 255, 178);
    colors.subScaleColor = Vec4::FromRGBA(255, 255, 255, 89);
    return colors;
}
}
