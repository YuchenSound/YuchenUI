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

ProtoolsDarkStyle::ProtoolsDarkStyle()
    : m_mainWindowBg(Vec4::FromRGBA(75, 75, 75, 255))
    , m_dialogBg(Vec4::FromRGBA(30, 30, 30, 255))
    , m_toolWindowBg(Vec4::FromRGBA(48, 48, 48, 255))
    , m_uiTextColor(Vec4::FromRGBA(255, 255, 255, 196))
    , m_textDisabledColor(Vec4::FromRGBA(120, 120, 120, 255))
    , m_buttonNormal(Vec4::FromRGBA(100, 100, 100, 255))
    , m_buttonHover(Vec4::FromRGBA(120, 120, 120, 255))
    , m_buttonPressed(Vec4::FromRGBA(80, 80, 80, 255))
    , m_buttonDisabled(Vec4::FromRGBA(60, 60, 60, 255))
    , m_confirmNormal(Vec4::FromRGBA(46, 152, 209, 128))
    , m_confirmHover(Vec4::FromRGBA(46, 152, 209, 192))
    , m_confirmPressed(Vec4::FromRGBA(46, 152, 209, 255))
    , m_borderNormal(Vec4::FromRGBA(128, 128, 128, 255))
    , m_borderHover(Vec4::FromRGBA(150, 150, 150, 255))
    , m_borderPressed(Vec4::FromRGBA(100, 100, 100, 255))
    , m_frameBg(Vec4::FromRGBA(255, 255, 255, 51))
    , m_frameBorder(Vec4::FromRGBA(128, 128, 128, 255))
    , m_groupBoxBg(Vec4::FromRGBA(30, 30, 30, 255))
    , m_groupBoxBorder(Vec4::FromRGBA(128, 128, 128, 255))
    , m_scrollAreaBg(Vec4::FromRGBA(47, 47, 47, 255))
    , m_scrollbarBackground(Vec4::FromRGBA(31, 31, 31, 255))
    , m_scrollbarThumb(Vec4::FromRGBA(56, 56, 56, 255))
    , m_scrollbarThumbHovered(Vec4::FromRGBA(69, 69, 69, 255))
    , m_scrollbarButtonNormal(Vec4::FromRGBA(56, 56, 56, 255))
    , m_scrollbarButtonHovered(Vec4::FromRGBA(69, 69, 69, 255))
    , m_scrollbarButtonPressed(Vec4::FromRGBA(99, 99, 99, 255))
    , m_scrollbarButtonBorder(Vec4::FromRGBA(79, 79, 79, 255))
    , m_scrollbarTriangleNormal(Vec4::FromRGBA(150, 150, 150, 255))
    , m_scrollbarTriangleHovered(Vec4::FromRGBA(181, 181, 181, 255))
    , m_scrollbarTrianglePressed(Vec4::FromRGBA(255, 255, 255, 255))
    , m_spinBoxTextNormal(Vec4::FromRGBA(56, 209, 119, 255))
    , m_spinBoxTextEditing(Vec4::FromRGBA(0, 0, 0, 255))
    , m_spinBoxEditingBg(Vec4::FromRGBA(56, 209, 119, 255))
    , m_spinBoxCursor(Vec4::FromRGBA(0, 0, 0, 255))
    , m_groupBoxTitleBarHeight(20.0f)
{
}

FontHandle ProtoolsDarkStyle::getDefaultButtonFont() const {
    return FontManager::getInstance().getArialBold();
}

FontHandle ProtoolsDarkStyle::getDefaultLabelFont() const {
    return FontManager::getInstance().getArialRegular();
}

FontHandle ProtoolsDarkStyle::getDefaultTitleFont() const {
    return FontManager::getInstance().getArialBold();
}

Vec4 ProtoolsDarkStyle::getDefaultFrameBackground() const {
    return m_frameBg;
}

Vec4 ProtoolsDarkStyle::getDefaultFrameBorder() const {
    return m_frameBorder;
}

Vec4 ProtoolsDarkStyle::getDefaultGroupBoxBackground() const {
    return m_groupBoxBg;
}

Vec4 ProtoolsDarkStyle::getDefaultGroupBoxBorder() const {
    return m_groupBoxBorder;
}

float ProtoolsDarkStyle::getGroupBoxTitleBarHeight() const {
    return m_groupBoxTitleBarHeight;
}

Vec4 ProtoolsDarkStyle::getWindowBackground(WindowType type) const {
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

Vec4 ProtoolsDarkStyle::getDefaultTextColor() const {
    return m_uiTextColor;
}

void ProtoolsDarkStyle::drawButton(const ButtonDrawInfo& info, RenderList& cmdList) {
    drawButtonInternal(info, cmdList,
                      m_buttonNormal, m_buttonHover, m_buttonPressed, m_buttonDisabled,
                      m_borderNormal, m_borderHover, m_borderPressed);
}

void ProtoolsDarkStyle::drawConfirmButton(const ButtonDrawInfo& info, RenderList& cmdList) {
    Vec4 confirmBorder = Vec4::FromRGBA(46, 152, 209, 255);
    drawButtonInternal(info, cmdList,
                      m_confirmNormal, m_confirmHover, m_confirmPressed, m_buttonDisabled,
                      confirmBorder, confirmBorder, confirmBorder);
}

void ProtoolsDarkStyle::drawCancelButton(const ButtonDrawInfo& info, RenderList& cmdList) {
    Vec4 cancelNormal = Vec4::FromRGBA(255, 255, 255, 30);
    Vec4 cancelHover = Vec4::FromRGBA(255, 255, 255, 60);
    Vec4 cancelPressed = Vec4::FromRGBA(255, 255, 255, 128);
    Vec4 cancelBorder = Vec4::FromRGBA(255, 255, 255, 128);
    
    drawButtonInternal(info, cmdList,
                      cancelNormal, cancelHover, cancelPressed, m_buttonDisabled,
                      cancelBorder, cancelBorder, cancelBorder);
}

void ProtoolsDarkStyle::drawButtonInternal(const ButtonDrawInfo& info, RenderList& cmdList,
                                          const Vec4& normalColor, const Vec4& hoverColor,
                                          const Vec4& pressedColor, const Vec4& disabledColor,
                                          const Vec4& borderNormal, const Vec4& borderHover,
                                          const Vec4& borderPressed) {
    Vec4 bgColor = !info.isEnabled ? disabledColor :
                   info.isPressed ? pressedColor :
                   info.isHovered ? hoverColor : normalColor;
    
    Vec4 borderColor = !info.isEnabled ? Vec4::FromRGBA(80, 80, 80, 255) :
                       info.isPressed ? borderPressed :
                       info.isHovered ? borderHover : borderNormal;
    
    cmdList.fillRect(info.bounds, bgColor, CornerRadius(2.0f));
    cmdList.drawRect(info.bounds, borderColor, 1.0f, CornerRadius(2.0f));
    
    if (!info.text.empty()) {
        FontManager& fontManager = FontManager::getInstance();
        Vec2 textSize = fontManager.measureText(info.text.c_str(), info.fontSize);
        FontMetrics metrics = fontManager.getFontMetrics(info.westernFont, info.fontSize);
        
        Vec2 textPos(
            info.bounds.x + (info.bounds.width - textSize.x) * 0.5f,
            info.bounds.y + (info.bounds.height - metrics.lineHeight) * 0.5f + metrics.ascender
        );
        
        Vec4 textColor = info.isEnabled ? info.textColor : m_textDisabledColor;
        cmdList.drawText(info.text.c_str(), textPos, info.westernFont,
                        info.chineseFont, info.fontSize, textColor);
    }
}

void ProtoolsDarkStyle::drawFrame(const FrameDrawInfo& info, RenderList& cmdList) {
    cmdList.fillRect(info.bounds, info.backgroundColor, info.cornerRadius);
    
    if (info.borderWidth > 0.0f) {
        cmdList.drawRect(info.bounds, info.borderColor, info.borderWidth, info.cornerRadius);
    }
}

void ProtoolsDarkStyle::drawGroupBox(const GroupBoxDrawInfo& info, RenderList& cmdList) {
    const float TITLE_HEIGHT = m_groupBoxTitleBarHeight;
    static constexpr float TITLE_PADDING_LEFT = 8.0f;
    static constexpr float CORNER_RADIUS = 2.0f;
    
    FontManager& fontManager = FontManager::getInstance();
    
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
        FontMetrics metrics = fontManager.getFontMetrics(info.titleFont, info.titleFontSize);
        
        float textX = info.bounds.x + TITLE_PADDING_LEFT;
        float textY = info.bounds.y + (TITLE_HEIGHT - metrics.lineHeight) * 0.5f + metrics.ascender;
        
        Vec2 titleTextPos(textX, textY);
        Vec4 titleTextColor = m_uiTextColor;
        FontHandle chineseFont = fontManager.getPingFangFont();
        
        cmdList.drawText(info.title.c_str(), titleTextPos, info.titleFont,
                        chineseFont, info.titleFontSize, titleTextColor);
    }
}

void ProtoolsDarkStyle::drawScrollbarTrack(const ScrollbarTrackDrawInfo& info, RenderList& cmdList) {
    cmdList.fillRect(info.bounds, m_scrollbarBackground);
}

void ProtoolsDarkStyle::drawScrollbarThumb(const ScrollbarThumbDrawInfo& info, RenderList& cmdList) {
    cmdList.fillRect(info.bounds, Vec4::FromRGBA(0, 0, 0, 0));
    
    Rect innerRect;
    if (info.orientation == ScrollbarOrientation::Vertical) {
        float margin = (info.bounds.width - 6.0f) / 2.0f;
        innerRect = Rect(
            info.bounds.x + margin,
            info.bounds.y + margin,
            6.0f,
            info.bounds.height - margin * 2.0f
        );
    } else {
        float margin = (info.bounds.height - 6.0f) / 2.0f;
        innerRect = Rect(
            info.bounds.x + margin,
            info.bounds.y + margin,
            info.bounds.width - margin * 2.0f,
            6.0f
        );
    }
    
    Vec4 thumbColor = (info.isDragging || info.isHovered) ? m_scrollbarThumbHovered : m_scrollbarThumb;
    cmdList.fillRect(innerRect, thumbColor);
}

void ProtoolsDarkStyle::drawScrollbarButton(const ScrollbarButtonDrawInfo& info, RenderList& cmdList) {
    Vec4 buttonColor;
    Vec4 triangleColor;
    
    switch (info.buttonState) {
        case ScrollbarButtonState::Pressed:
            buttonColor = m_scrollbarButtonPressed;
            triangleColor = m_scrollbarTrianglePressed;
            break;
        case ScrollbarButtonState::Hovered:
            buttonColor = m_scrollbarButtonHovered;
            triangleColor = m_scrollbarTriangleHovered;
            break;
        default:
            buttonColor = m_scrollbarButtonNormal;
            triangleColor = m_scrollbarTriangleNormal;
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

void ProtoolsDarkStyle::drawTextInput(const TextInputDrawInfo& info, RenderList& cmdList) {
    Vec4 bgColor = !info.isEnabled ? Vec4::FromRGBA(40, 40, 40, 255) :
                   Vec4::FromRGBA(42, 42, 42, 255);
    
    Vec4 borderColor = !info.isEnabled ? Vec4::FromRGBA(60, 60, 60, 255) :
                       info.isHovered ? Vec4::FromRGBA(100, 100, 100, 255) :
                       Vec4::FromRGBA(80, 80, 80, 255);
    
    cmdList.fillRect(info.bounds, bgColor, CornerRadius(2.0f));
    cmdList.drawRect(info.bounds, borderColor, 1.0f, CornerRadius(2.0f));
    
    cmdList.pushClipRect(info.bounds);
    
    if (info.hasSelection) {
        Rect selectionRect(
            info.selectionStartX,
            info.bounds.y + 3.0f,
            info.selectionWidth,
            info.bounds.height - 6.0f
        );
        cmdList.fillRect(selectionRect, Vec4::FromRGBA(46, 152, 209, 128));
    }
    
    if (!info.text.empty()) {
        FontManager& fontManager = FontManager::getInstance();
        FontHandle westernFont = getDefaultLabelFont();
        FontHandle chineseFont = fontManager.getPingFangFont();
        
        FontMetrics metrics = fontManager.getFontMetrics(westernFont, info.fontSize);
        float textY = info.textY + metrics.ascender;
        
        cmdList.drawText(info.text.c_str(), Vec2(info.textX, textY),
                        westernFont, chineseFont, info.fontSize, m_uiTextColor);
    } else if (!info.placeholder.empty() && !info.hasFocus) {
        FontManager& fontManager = FontManager::getInstance();
        FontHandle westernFont = getDefaultLabelFont();
        FontHandle chineseFont = fontManager.getPingFangFont();
        
        FontMetrics metrics = fontManager.getFontMetrics(westernFont, info.fontSize);
        float textY = info.textY + metrics.ascender;
        
        Vec4 placeholderColor = Vec4::FromRGBA(120, 120, 120, 255);
        cmdList.drawText(info.placeholder.c_str(), Vec2(info.textX, textY),
                        westernFont, chineseFont, info.fontSize, placeholderColor);
    }
    
    if (info.showCursor) {
        float cursorY1 = info.bounds.y + (info.bounds.height - info.cursorHeight) * 0.5f;
        float cursorY2 = cursorY1 + info.cursorHeight;
        cmdList.drawLine(Vec2(info.cursorX, cursorY1), Vec2(info.cursorX, cursorY2),
                        m_uiTextColor, 1.0f);
    }
    
    cmdList.popClipRect();
}

void ProtoolsDarkStyle::drawSpinBox(const SpinBoxDrawInfo& info, RenderList& cmdList) {
    cmdList.fillRect(info.bounds, Vec4::FromRGBA(0, 0, 0, 255), CornerRadius(2.0f));

    if (info.displayText.empty()) return;
    
    FontManager& fontManager = FontManager::getInstance();
    FontHandle arialBold = fontManager.getArialBold();
    FontMetrics metrics = fontManager.getFontMetrics(arialBold, info.fontSize);
    
    float contentHeight = info.bounds.height - info.paddingTop - info.paddingBottom;
    float textY = info.bounds.y + info.paddingTop + (contentHeight - metrics.lineHeight) * 0.5f + metrics.ascender;
    float textX = info.bounds.x + info.paddingLeft;
    
    Vec4 textColor = m_spinBoxTextNormal;
    
    if (info.isEditing) {
        Vec2 textSize = fontManager.measureText(info.displayText.c_str(), info.fontSize);
        
        Rect textBgRect(
            textX - 1.0f,
            info.bounds.y + info.paddingTop + (contentHeight - metrics.lineHeight) * 0.5f - 1.0f,
            textSize.x + 2.0f,
            metrics.lineHeight + 2.0f
        );
        
        cmdList.fillRect(textBgRect, m_spinBoxEditingBg);
        textColor = Vec4::FromRGBA(0, 0, 0, 255);
    }
    
    cmdList.drawText(info.displayText.c_str(), Vec2(textX, textY),
                    arialBold, info.chineseFont, info.fontSize, textColor);
}

void ProtoolsDarkStyle::drawComboBox(const ComboBoxDrawInfo& info, RenderList& cmdList) {
    static constexpr float TEXT_PADDING_LEFT  = 4.0f;
    static constexpr float NINE_SLICE_MARGIN  = 4.0f;
    static constexpr float ARROW_BASE_SIZE    = 7.0f;
    static constexpr float ARROW_HIGHT_SIZE   = 4.0f;
    static constexpr float ARROW_MARGIN_RIGHT = 3.0f;
    static constexpr float ARROW_MARGIN_TOP   = 4.0f;
    
    const char* backgroundResource = "components/dropdown/dropdown_background_black.png";

    NineSliceMargins margins(NINE_SLICE_MARGIN, NINE_SLICE_MARGIN, NINE_SLICE_MARGIN, NINE_SLICE_MARGIN);
    cmdList.drawImage(backgroundResource, info.bounds, ScaleMode::NineSlice, margins);

    std::string displayText = info.isEmpty ? info.placeholder : info.text;
    if (!displayText.empty()) {
        FontManager& fontManager = FontManager::getInstance();
        FontMetrics metrics = fontManager.getFontMetrics(info.westernFont, info.fontSize);
        
        float textX = info.bounds.x + TEXT_PADDING_LEFT;
        float textY = info.bounds.y + (info.bounds.height - metrics.lineHeight) * 0.5f + metrics.ascender;
        
        Vec2 textPosition(textX, textY);
        cmdList.drawText(displayText.c_str(), textPosition,
                        info.westernFont, info.chineseFont, info.fontSize, m_uiTextColor);
    }
    
    float arrowX = info.bounds.x + info.bounds.width - ARROW_MARGIN_RIGHT - ARROW_BASE_SIZE;
    float arrowY = info.bounds.y + ARROW_MARGIN_TOP;
    Rect arrowRect(arrowX, arrowY, ARROW_BASE_SIZE, ARROW_HIGHT_SIZE);
    
    cmdList.drawImage("components/dropdown/dropdown_triangle.png", arrowRect, ScaleMode::Original);
}

Vec4 ProtoolsDarkStyle::getDefaultScrollAreaBackground() const {
    return m_scrollAreaBg;
}

void ProtoolsDarkStyle::drawFocusIndicator(const FocusIndicatorDrawInfo& info, RenderList& cmdList) {
    Vec4 focusColor = Vec4::FromRGBA(255, 200, 0, 255);
    cmdList.drawRect(info.bounds, focusColor, UIStyle::FOCUS_INDICATOR_BORDER_WIDTH, info.cornerRadius);
}


void ProtoolsDarkStyle::drawCheckBox(const CheckBoxDrawInfo& info, RenderList& cmdList) {
    std::string resourcePath = "components/checkbox/dark/";
    
    if (!info.isEnabled) {
        switch (info.state) {
            case CheckBoxState::Checked:
                resourcePath += "checkbox_checked_disabled@2x.png";
                break;
            case CheckBoxState::Indeterminate:
                resourcePath += "checkbox_indeterminate_disabled@2x.png";
                break;
            default:
                resourcePath += "checkbox_unchecked_disabled@2x.png";
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

void ProtoolsDarkStyle::drawRadioButton(const RadioButtonDrawInfo& info, RenderList& cmdList) {
    std::string resourcePath = "components/radio/dark/";
    
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

}
