#include "PanSection.h"
#include "theme/MixerTheme.h"
#include <YuchenUI/YuchenUI.h>
#include <cmath>

PanSection::PanSection(const YuchenUI::Rect& bounds)
    : ChannelSection(bounds)
    , m_leftKnob(nullptr)
    , m_rightKnob(nullptr)
    , m_numberBackground(nullptr)
    , m_leftDisplay(nullptr)
    , m_rightDisplay(nullptr)
    , m_leftPanValue(-100)
    , m_rightPanValue(100)
    , m_onLeftPanChanged(nullptr)
    , m_onRightPanChanged(nullptr)
{
}

PanSection::~PanSection()
{
}

void PanSection::setOwnerContext(YuchenUI::UIContext* context)
{
    ChannelSection::setOwnerContext(context);
    
    if (context && !m_leftKnob && !m_rightKnob)
    {
        createComponents();
    }
}

void PanSection::addDrawCommands(YuchenUI::RenderList& commandList,
                                 const YuchenUI::Vec2& offset) const
{
    if (!m_isVisible) return;
    
    YuchenUI::Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    
    if (m_mixerTheme)
    {
        YuchenUI::Vec4 bgColor = m_mixerTheme->getFaderMeterSectionBackground();
        commandList.fillRect(
            YuchenUI::Rect(absPos.x + 2.0f, absPos.y, m_bounds.width - 4.0f, m_bounds.height),
            bgColor
        );
    }
    
    renderChildren(commandList, absPos);
}

void PanSection::createComponents()
{
    if (!m_ownerContext) return;
    
    clearChildren();
    
    float totalKnobWidth = KNOB_WIDTH * 2 + KNOB_SPACING;
    float knobLeftMargin = (m_bounds.width - totalKnobWidth) / 2.0f;
    
    YuchenUI::Rect leftKnobRect(
        knobLeftMargin,
        KNOB_TOP_MARGIN,
        KNOB_WIDTH,
        KNOB_HEIGHT
    );
    m_leftKnob = addChild(new YuchenUI::Knob(leftKnobRect));
    m_leftKnob->setKnobType(YuchenUI::KnobType::Centered);
    m_leftKnob->setValueRange(0.0f, 28.0f);
    m_leftKnob->setValue(static_cast<float>(valueToFrame(m_leftPanValue)));
    m_leftKnob->setOnValueChanged([this](float frameValue) {
        int frame = static_cast<int>(std::round(frameValue));
        int value = frameToValue(frame);
        m_leftPanValue = value;
        updateLeftDisplay(value);
        if (m_onLeftPanChanged)
        {
            m_onLeftPanChanged(value);
        }
    });
    
    YuchenUI::Rect rightKnobRect(
        knobLeftMargin + KNOB_WIDTH + KNOB_SPACING,
        KNOB_TOP_MARGIN,
        KNOB_WIDTH,
        KNOB_HEIGHT
    );
    m_rightKnob = addChild(new YuchenUI::Knob(rightKnobRect));
    m_rightKnob->setKnobType(YuchenUI::KnobType::Centered);
    m_rightKnob->setValueRange(0.0f, 28.0f);
    m_rightKnob->setValue(static_cast<float>(valueToFrame(m_rightPanValue)));
    m_rightKnob->setOnValueChanged([this](float frameValue) {
        int frame = static_cast<int>(std::round(frameValue));
        int value = frameToValue(frame);
        m_rightPanValue = value;
        updateRightDisplay(value);
        if (m_onRightPanChanged)
        {
            m_onRightPanChanged(value);
        }
    });
    
    float numberBgY = KNOB_TOP_MARGIN + KNOB_HEIGHT + MIDDLE_SPACING;
    float numberBgX = 4.0f;
    float numberBgWidth = m_bounds.width - 8.0f;
    YuchenUI::Rect numberBgRect(numberBgX, numberBgY, numberBgWidth, NUMBER_DISPLAY_HEIGHT);
    m_numberBackground = addChild(new YuchenUI::NumberBackground(numberBgRect));
    
    const float margin = 3.0f;
    float spinBoxWidth = (numberBgWidth - margin * 3.0f) / 2.0f;
    
    YuchenUI::IFontProvider* fontProvider = m_ownerContext->getFontProvider();
    
    YuchenUI::Rect leftDisplayRect(margin, 0, spinBoxWidth, NUMBER_DISPLAY_HEIGHT);
    m_leftDisplay = m_numberBackground->addChild(new YuchenUI::SpinBox(leftDisplayRect));
    m_leftDisplay->setHasBackground(false);
    m_leftDisplay->setReadOnly(true);
    m_leftDisplay->setFont(fontProvider->getDefaultBoldFont());
    m_leftDisplay->setValue(static_cast<double>(std::abs(m_leftPanValue)));
    m_leftDisplay->setPrecision(0);
    m_leftDisplay->setFontSize(10.0f);
    
    float rightDisplayX = margin + spinBoxWidth + margin;
    YuchenUI::Rect rightDisplayRect(rightDisplayX, 0, spinBoxWidth, NUMBER_DISPLAY_HEIGHT);
    m_rightDisplay = m_numberBackground->addChild(new YuchenUI::SpinBox(rightDisplayRect));
    m_rightDisplay->setHasBackground(false);
    m_rightDisplay->setReadOnly(true);
    m_rightDisplay->setFont(fontProvider->getDefaultBoldFont());
    m_rightDisplay->setValue(static_cast<double>(std::abs(m_rightPanValue)));
    m_rightDisplay->setPrecision(0);
    m_rightDisplay->setFontSize(10.0f);
}

int PanSection::valueToFrame(int value) const
{
    if (value == 0)
    {
        return CENTER_FRAME;
    }
    else if (value < 0)
    {
        return static_cast<int>(std::round((value + 100) * 14.0 / 100.0));
    }
    else
    {
        return 16 + static_cast<int>(std::round((value - 1) * 12.0 / 99.0));
    }
}

int PanSection::frameToValue(int frame) const
{
    if (frame == CENTER_FRAME)
    {
        return 0;
    }
    else if (frame < CENTER_FRAME)
    {
        return static_cast<int>(std::round(frame * 100.0 / 14.0)) - 100;
    }
    else
    {
        return static_cast<int>(std::round((frame - 16) * 99.0 / 12.0)) + 1;
    }
}

void PanSection::updateLeftDisplay(int value)
{
    if (m_leftDisplay)
    {
        m_leftDisplay->setValue(static_cast<double>(std::abs(value)));
    }
}

void PanSection::updateRightDisplay(int value)
{
    if (m_rightDisplay)
    {
        m_rightDisplay->setValue(static_cast<double>(std::abs(value)));
    }
}

void PanSection::setLeftPanValue(int value)
{
    if (value < -100) value = -100;
    if (value > 100) value = 100;
    
    m_leftPanValue = value;
    
    if (m_leftKnob)
    {
        m_leftKnob->setValue(static_cast<float>(valueToFrame(value)));
    }
    
    updateLeftDisplay(value);
}

void PanSection::setRightPanValue(int value)
{
    if (value < -100) value = -100;
    if (value > 100) value = 100;
    
    m_rightPanValue = value;
    
    if (m_rightKnob)
    {
        m_rightKnob->setValue(static_cast<float>(valueToFrame(value)));
    }
    
    updateRightDisplay(value);
}

int PanSection::getLeftPanValue() const
{
    return m_leftPanValue;
}

int PanSection::getRightPanValue() const
{
    return m_rightPanValue;
}

void PanSection::setOnLeftPanChanged(std::function<void(int)> callback)
{
    m_onLeftPanChanged = callback;
}

void PanSection::setOnRightPanChanged(std::function<void(int)> callback)
{
    m_onRightPanChanged = callback;
}
