#include "YuchenUI/theme/Theme.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/text/FontManager.h"
#include "YuchenUI/widgets/ScrollArea.h"
#include "YuchenUI/widgets/ComboBox.h"
#include "YuchenUI/widgets/CheckBox.h"
#include "YuchenUI/widgets/RadioButton.h"
#include "YuchenUI/core/Assert.h"
#include "YuchenUI/core/Types.h"

namespace YuchenUI {

//==========================================================================================
// UIStyle Base Implementation

IFontProvider* UIStyle::getFontProvider() const {
    if (m_fontProvider) {
        return m_fontProvider;
    }
    
    // Fallback to singleton for backward compatibility
    // This allows existing code to work without modifications
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    return &FontManager::getInstance();
    #pragma GCC diagnostic pop
}

//==========================================================================================
// ProtoolsClassicStyle Implementation

ProtoolsClassicStyle::ProtoolsClassicStyle()
    : m_mainWindowBg(Vec4::FromRGBA(209, 209, 209, 255))
    , m_dialogBg(Vec4::FromRGBA(60, 60, 60, 255))
    , m_toolWindowBg(Vec4::FromRGBA(250, 250, 250, 255))
    , m_textColor(Vec4::FromRGBA(30, 30, 30, 255))
    , m_textDisabledColor(Vec4::FromRGBA(160, 160, 160, 255))
    , m_buttonNormal(Vec4::FromRGBA(220, 220, 220, 255))
    , m_buttonHover(Vec4::FromRGBA(200, 200, 200, 255))
    , m_buttonPressed(Vec4::FromRGBA(180, 180, 180, 255))
    , m_buttonDisabled(Vec4::FromRGBA(235, 235, 235, 255))
    , m_confirmNormal(Vec4::FromRGBA(0, 122, 255, 255))
    , m_confirmHover(Vec4::FromRGBA(0, 100, 220, 255))
    , m_confirmPressed(Vec4::FromRGBA(0, 80, 200, 255))
    , m_borderNormal(Vec4::FromRGBA(180, 180, 180, 255))
    , m_borderHover(Vec4::FromRGBA(140, 140, 140, 255))
    , m_borderPressed(Vec4::FromRGBA(120, 120, 120, 255))
    , m_frameBg(Vec4::FromRGBA(255, 255, 255, 196))
    , m_frameBorder(Vec4::FromRGBA(255, 255, 255, 196))
    , m_groupBoxBg(Vec4::FromRGBA(255, 255, 255, 255))
    , m_groupBoxBorder(Vec4::FromRGBA(200, 200, 200, 255))
    , m_scrollAreaBg(Vec4::FromRGBA(219, 219, 219, 255))
    , m_scrollbarBackground(Vec4::FromRGBA(219, 219, 219, 219))
    , m_scrollbarThumb(Vec4::FromRGBA(200, 200, 200, 255))
    , m_scrollbarThumbHovered(Vec4::FromRGBA(170, 170, 170, 255))
    , m_scrollbarButtonNormal(Vec4::FromRGBA(230, 230, 230, 255))
    , m_scrollbarButtonHovered(Vec4::FromRGBA(210, 210, 210, 255))
    , m_scrollbarButtonPressed(Vec4::FromRGBA(180, 180, 180, 255))
    , m_scrollbarButtonBorder(Vec4::FromRGBA(190, 190, 190, 255))
    , m_scrollbarTriangleNormal(Vec4::FromRGBA(100, 100, 100, 255))
    , m_scrollbarTriangleHovered(Vec4::FromRGBA(60, 60, 60, 255))
    , m_scrollbarTrianglePressed(Vec4::FromRGBA(30, 30, 30, 255))
    , m_spinBoxTextNormal(Vec4::FromRGBA(169, 231, 0, 255))
    , m_spinBoxTextEditing(Vec4::FromRGBA(50, 50, 50, 255))
    , m_spinBoxEditingBg(Vec4::FromRGBA(169, 231, 0, 255))
    , m_spinBoxCursor(Vec4::FromRGBA(0, 0, 0, 255))
    , m_groupBoxTitleBarHeight(20.0f)
{
}

Vec4 ProtoolsClassicStyle::getDefaultFrameBackground() const {
    return m_frameBg;
}

Vec4 ProtoolsClassicStyle::getDefaultFrameBorder() const {
    return m_frameBorder;
}

Vec4 ProtoolsClassicStyle::getDefaultGroupBoxBackground() const {
    return m_groupBoxBg;
}

Vec4 ProtoolsClassicStyle::getDefaultGroupBoxBorder() const {
    return m_groupBoxBorder;
}

float ProtoolsClassicStyle::getGroupBoxTitleBarHeight() const {
    return m_groupBoxTitleBarHeight;
}

Vec4 ProtoolsClassicStyle::getWindowBackground(WindowType type) const {
    switch (type) {
        case WindowType::Main:
            return m_mainWindowBg;
        case WindowType::Dialog:
            return m_dialogBg;
        case WindowType::ToolWindow:
            return m_toolWindowBg;
        default:
            return m_mainWindowBg;
    }
}

Vec4 ProtoolsClassicStyle::getDefaultTextColor() const {
    return m_textColor;
}

void ProtoolsClassicStyle::drawButton(const ButtonDrawInfo& info, RenderList& cmdList) {
    NineSliceMargins margins(4.0f, 4.0f, 4.0f, 4.0f);
    cmdList.drawImage("components/buttons/btn_grey.png",
                      info.bounds,
                      ScaleMode::NineSlice,
                      margins);
    
    if (!info.text.empty()) {
        IFontProvider* fontProvider = getFontProvider();
        Vec2 textSize = fontProvider->measureText(info.text.c_str(), info.fontSize);
        FontMetrics metrics = fontProvider->getFontMetrics(info.westernFont, info.fontSize);
        
        Vec2 textPos(
            info.bounds.x + (info.bounds.width - textSize.x) * 0.5f,
            info.bounds.y + (info.bounds.height - metrics.lineHeight) * 0.5f + metrics.ascender
        );
        
        Vec4 textColor = info.isEnabled ? Vec4::FromRGBA(255, 255, 255, 255) : m_textDisabledColor;
        cmdList.drawText(info.text.c_str(), textPos, info.westernFont,
                        info.chineseFont, info.fontSize, textColor);
    }
}

void ProtoolsClassicStyle::drawConfirmButton(const ButtonDrawInfo& info, RenderList& cmdList) {
    NineSliceMargins margins(4.0f, 4.0f, 4.0f, 4.0f);
    cmdList.drawImage("components/buttons/btn_blue.png",
                      info.bounds,
                      ScaleMode::NineSlice,
                      margins);
    
    if (!info.text.empty()) {
        IFontProvider* fontProvider = getFontProvider();
        Vec2 textSize = fontProvider->measureText(info.text.c_str(), info.fontSize);
        FontMetrics metrics = fontProvider->getFontMetrics(info.westernFont, info.fontSize);
        
        Vec2 textPos(
            info.bounds.x + (info.bounds.width - textSize.x) * 0.5f,
            info.bounds.y + (info.bounds.height - metrics.lineHeight) * 0.5f + metrics.ascender
        );
        
        Vec4 textColor = info.isEnabled ? Vec4::FromRGBA(255, 255, 255, 255) : m_textDisabledColor;
        cmdList.drawText(info.text.c_str(), textPos, info.westernFont,
                        info.chineseFont, info.fontSize, textColor);
    }
}

void ProtoolsClassicStyle::drawCancelButton(const ButtonDrawInfo& info, RenderList& cmdList) {
    drawButton(info, cmdList);
}

void ProtoolsClassicStyle::drawFrame(const FrameDrawInfo& info, RenderList& cmdList) {
    cmdList.fillRect(info.bounds, info.backgroundColor, info.cornerRadius);
    
    if (info.borderWidth > 0.0f) {
        cmdList.drawRect(info.bounds, info.borderColor, info.borderWidth, info.cornerRadius);
    }
}

void ProtoolsClassicStyle::drawGroupBox(const GroupBoxDrawInfo& info, RenderList& cmdList) {
    const float TITLE_HEIGHT = m_groupBoxTitleBarHeight;
    static constexpr float TITLE_PADDING_LEFT = 8.0f;
    static constexpr float CORNER_RADIUS = 2.0f;
    
    IFontProvider* fontProvider = getFontProvider();
    
    Vec4 blackColor = Vec4::FromRGBA(0, 0, 0, 76);
    
    Rect titleRect(
        info.bounds.x,
        info.bounds.y,
        info.bounds.width,
        TITLE_HEIGHT
    );
    CornerRadius titleCornerRadius(0.0f, 0.0f, CORNER_RADIUS, CORNER_RADIUS);
    cmdList.fillRect(titleRect, blackColor, titleCornerRadius);
    
    Rect contentRect(
        info.bounds.x,
        info.bounds.y + TITLE_HEIGHT,
        info.bounds.width,
        info.bounds.height - TITLE_HEIGHT
    );
    CornerRadius contentCornerRadius(CORNER_RADIUS, CORNER_RADIUS, 0.0f, 0.0f);
    
    if (info.borderWidth > 0.0f) {
        cmdList.drawRect(contentRect, blackColor, info.borderWidth, contentCornerRadius);
    }
    
    if (!info.title.empty()) {
        FontMetrics metrics = fontProvider->getFontMetrics(info.titleFont, info.titleFontSize);
        
        float textX = info.bounds.x + TITLE_PADDING_LEFT;
        float textY = info.bounds.y + (TITLE_HEIGHT - metrics.lineHeight) * 0.5f + metrics.ascender;
        
        Vec2 titleTextPos(textX, textY);
        Vec4 titleTextColor = Vec4::FromRGBA(255, 255, 255, 255);
        FontHandle chineseFont = fontProvider->getDefaultCJKFont();
        
        cmdList.drawText(info.title.c_str(), titleTextPos, info.titleFont,
                        chineseFont, info.titleFontSize, titleTextColor);
    }
}

void ProtoolsClassicStyle::drawScrollbarTrack(const ScrollbarTrackDrawInfo& info, RenderList& cmdList) {
    NineSliceMargins margins(2.0f, 2.0f, 2.0f, 2.0f);
    cmdList.drawImage("components/scrollbar/scrollbar_track.png",
                      info.bounds,
                      ScaleMode::NineSlice,
                      margins);
}

void ProtoolsClassicStyle::drawScrollbarThumb(const ScrollbarThumbDrawInfo& info, RenderList& cmdList) {
    const char* thumbImage = (info.isDragging || info.isHovered)
                           ? "components/scrollbar/scrollbar_thumb_pressed.png"
                           : "components/scrollbar/scrollbar_thumb_normal.png";
    
    NineSliceMargins margins(2.0f, 2.0f, 2.0f, 2.0f);
    cmdList.drawImage(thumbImage, info.bounds, ScaleMode::NineSlice, margins);
}

void ProtoolsClassicStyle::drawScrollbarButton(const ScrollbarButtonDrawInfo& info, RenderList& cmdList) {
    const char* buttonImage = (info.buttonState == ScrollbarButtonState::Pressed ||
                               info.buttonState == ScrollbarButtonState::Hovered)
                            ? "components/scrollbar/scrollbar_thumb_pressed.png"
                            : "components/scrollbar/scrollbar_thumb_normal.png";
    
    NineSliceMargins buttonMargins(2.0f, 2.0f, 2.0f, 2.0f);
    cmdList.drawImage(buttonImage, info.bounds, ScaleMode::NineSlice, buttonMargins);
    
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
    
    Vec4 triangleColor;
    switch (info.buttonState) {
        case ScrollbarButtonState::Pressed:
            triangleColor = m_scrollbarTrianglePressed;
            break;
        case ScrollbarButtonState::Hovered:
            triangleColor = m_scrollbarTriangleHovered;
            break;
        default:
            triangleColor = m_scrollbarTriangleNormal;
            break;
    }
    
    cmdList.fillTriangle(p1, p2, p3, triangleColor);
}

void ProtoolsClassicStyle::drawTextInput(const TextInputDrawInfo& info, RenderList& cmdList) {
    Vec4 bgColor = !info.isEnabled ? Vec4::FromRGBA(225, 225, 225, 255) :
                   Vec4::FromRGBA(225, 225, 225, 255);
    
    Vec4 borderColor = !info.isEnabled ? Vec4::FromRGBA(200, 200, 200, 255) :
                       info.isHovered ? Vec4::FromRGBA(140, 140, 140, 255) :
                       Vec4::FromRGBA(180, 180, 180, 255);
    
    cmdList.fillRect(info.bounds, bgColor);
    cmdList.drawRect(info.bounds, borderColor, 1.0f);
    
    cmdList.pushClipRect(info.bounds);
    
    if (info.hasSelection) {
        Rect selectionRect(
            info.selectionStartX,
            info.bounds.y + 3.0f,
            info.selectionWidth,
            info.bounds.height - 6.0f
        );
        cmdList.fillRect(selectionRect, Vec4::FromRGBA(0, 122, 255, 100));
    }
    
    if (!info.text.empty()) {
        IFontProvider* fontProvider = getFontProvider();
        FontHandle westernFont = getDefaultLabelFont();
        FontHandle chineseFont = fontProvider->getDefaultCJKFont();
        
        FontMetrics metrics = fontProvider->getFontMetrics(westernFont, info.fontSize);
        float textY = info.textY + metrics.ascender;
        
        cmdList.drawText(info.text.c_str(), Vec2(info.textX, textY),
                        westernFont, chineseFont, info.fontSize, m_textColor);
    } else if (!info.placeholder.empty() && !info.hasFocus) {
        IFontProvider* fontProvider = getFontProvider();
        FontHandle westernFont = getDefaultLabelFont();
        FontHandle chineseFont = fontProvider->getDefaultCJKFont();
        
        FontMetrics metrics = fontProvider->getFontMetrics(westernFont, info.fontSize);
        float textY = info.textY + metrics.ascender;
        
        Vec4 placeholderColor = Vec4::FromRGBA(160, 160, 160, 255);
        cmdList.drawText(info.placeholder.c_str(), Vec2(info.textX, textY),
                        westernFont, chineseFont, info.fontSize, placeholderColor);
    }
    
    if (info.showCursor) {
        float cursorY1 = info.bounds.y + (info.bounds.height - info.cursorHeight) * 0.5f;
        float cursorY2 = cursorY1 + info.cursorHeight;
        cmdList.drawLine(Vec2(info.cursorX, cursorY1), Vec2(info.cursorX, cursorY2),
                        m_textColor, 1.5f);
    }
    
    cmdList.popClipRect();
}

void ProtoolsClassicStyle::drawSpinBox(const SpinBoxDrawInfo& info, RenderList& cmdList) {
    cmdList.fillRect(info.bounds, Vec4::FromRGBA(76, 76, 76, 255), CornerRadius(2.0f));

    if (info.displayText.empty()) return;
    
    IFontProvider* fontProvider = getFontProvider();
    FontHandle boldFont = fontProvider->getDefaultBoldFont();
    FontMetrics metrics = fontProvider->getFontMetrics(boldFont, info.fontSize);
    
    float contentHeight = info.bounds.height - info.paddingTop - info.paddingBottom;
    float textY = info.bounds.y + info.paddingTop + (contentHeight - metrics.lineHeight) * 0.5f + metrics.ascender;
    float textX = info.bounds.x + info.paddingLeft;
    
    Vec4 textColor = m_spinBoxTextNormal;
    
    if (info.isEditing) {
        Vec2 textSize = fontProvider->measureText(info.displayText.c_str(), info.fontSize);
        
        Rect textBgRect(
            textX - 1.0f,
            info.bounds.y + info.paddingTop + (contentHeight - metrics.lineHeight) * 0.5f - 1.0f,
            textSize.x + 2.0f,
            metrics.lineHeight + 2.0f
        );
        
        cmdList.fillRect(textBgRect, m_spinBoxEditingBg);
        textColor = Vec4::FromRGBA(50, 50, 50, 255);
    }
    
    cmdList.drawText(info.displayText.c_str(), Vec2(textX, textY),
                    boldFont, info.chineseFont, info.fontSize, textColor);
}

void ProtoolsClassicStyle::drawComboBox(const ComboBoxDrawInfo& info, RenderList& cmdList) {
    static constexpr float TEXT_PADDING_LEFT  = 4.0f;
    static constexpr float NINE_SLICE_MARGIN  = 4.0f;
    static constexpr float ARROW_BASE_SIZE    = 7.0f;
    static constexpr float ARROW_HIGHT_SIZE   = 4.0f;
    static constexpr float ARROW_MARGIN_RIGHT = 3.0f;
    static constexpr float ARROW_MARGIN_TOP   = 4.0f;
    
    Vec4 textColor;
    const char* backgroundResource = "components/dropdown/dropdown_background_grey.png";
    textColor = Vec4::FromRGBA(0, 0, 0, 255);

    NineSliceMargins margins(NINE_SLICE_MARGIN, NINE_SLICE_MARGIN, NINE_SLICE_MARGIN, NINE_SLICE_MARGIN);
    cmdList.drawImage(backgroundResource, info.bounds, ScaleMode::NineSlice, margins);

    std::string displayText = info.isEmpty ? info.placeholder : info.text;
    if (!displayText.empty()) {
        IFontProvider* fontProvider = getFontProvider();
        FontMetrics metrics = fontProvider->getFontMetrics(info.westernFont, info.fontSize);
        
        float textX = info.bounds.x + TEXT_PADDING_LEFT;
        float textY = info.bounds.y + (info.bounds.height - metrics.lineHeight) * 0.5f + metrics.ascender;
        
        Vec2 textPosition(textX, textY);
        cmdList.drawText(displayText.c_str(), textPosition,
                        info.westernFont, info.chineseFont, info.fontSize, textColor);
    }
    
    float arrowX = info.bounds.x + info.bounds.width - ARROW_MARGIN_RIGHT - ARROW_BASE_SIZE;
    float arrowY = info.bounds.y + ARROW_MARGIN_TOP;
    Rect arrowRect(arrowX, arrowY, ARROW_BASE_SIZE, ARROW_HIGHT_SIZE);
    
    cmdList.drawImage("components/dropdown/dropdown_triangle.png", arrowRect, ScaleMode::Original);
}

void ProtoolsClassicStyle::drawFocusIndicator(const FocusIndicatorDrawInfo& info, RenderList& cmdList) {
    Vec4 focusColor = Vec4::FromRGBA(255, 200, 0, 255);
    cmdList.drawRect(info.bounds, focusColor, UIStyle::FOCUS_INDICATOR_BORDER_WIDTH, info.cornerRadius);
}

Vec4 ProtoolsClassicStyle::getDefaultScrollAreaBackground() const {
    return m_scrollAreaBg;
}

void ProtoolsClassicStyle::drawCheckBox(const CheckBoxDrawInfo& info, RenderList& cmdList) {
    std::string resourcePath = "components/checkbox/classical/";
    
    if (!info.isEnabled) {
        switch (info.state) {
            case CheckBoxState::Checked:
                resourcePath += "checkbox_checked_disabled@2x.png";
                break;
            case CheckBoxState::Indeterminate:
                resourcePath += "checkbox_indeterminate_disabled@2x.png";
                break;
            default:
                resourcePath += "checkbox_unchecked-disabled@2x.png";
                break;
        }
    } else {
        switch (info.state) {
            case CheckBoxState::Checked:
                resourcePath += "checkbox_checked@2x.png";
                break;
            case CheckBoxState::Indeterminate:
                resourcePath += "checkbox_indeterminate@2x.png";
                break;
            default:
                resourcePath += "checkbox_unchecked@2x.png";
                break;
        }
    }
    
    cmdList.drawImage(resourcePath.c_str(), info.bounds, ScaleMode::Original);
}

void ProtoolsClassicStyle::drawRadioButton(const RadioButtonDrawInfo& info, RenderList& cmdList) {
    std::string resourcePath = "components/radio/classical/";
    
    if (!info.isEnabled) {
        if (info.isChecked) {
            resourcePath += "radio_checked_disabled@2x.png";
        } else {
            resourcePath += "radio_unchecked_disabled@2x.png";
        }
    } else {
        if (info.isChecked) {
            resourcePath += "radio_checked@2x.png";
        } else {
            resourcePath += "radio_unchecked@2x.png";
        }
    }
    
    cmdList.drawImage(resourcePath.c_str(), info.bounds, ScaleMode::Original);
}

FontHandle ProtoolsClassicStyle::getDefaultButtonFont() const {
    IFontProvider* provider = getFontProvider();
    return provider->getDefaultFont();
}

FontHandle ProtoolsClassicStyle::getDefaultLabelFont() const {
    IFontProvider* provider = getFontProvider();
    return provider->getDefaultFont();
}

FontHandle ProtoolsClassicStyle::getDefaultTitleFont() const {
    IFontProvider* provider = getFontProvider();
    return provider->getDefaultBoldFont();
}

}
