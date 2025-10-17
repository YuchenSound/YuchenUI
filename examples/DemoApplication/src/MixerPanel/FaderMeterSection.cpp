#include "MixerPanel/FaderMeterSection.h"
#include <YuchenUI/YuchenUI.h>
#include <iostream>

FaderMeterSection::FaderMeterSection(const YuchenUI::Rect& bounds)
    : ChannelSection(bounds)
    , m_fader(nullptr)
    , m_levelMeter(nullptr)
    , m_faderMeterNumberBackground(nullptr)
    , m_faderNumberDisplay(nullptr)
    , m_meterNumberDisplay(nullptr)
{
}

FaderMeterSection::~FaderMeterSection()
{
}

void FaderMeterSection::setOwnerContext(YuchenUI::UIContext* context)
{
    ChannelSection::setOwnerContext(context);
    
    if (context && !m_fader && !m_levelMeter)
    {
        createComponents();
    }
}

void FaderMeterSection::addDrawCommands(YuchenUI::RenderList& commandList,
                                        const YuchenUI::Vec2& offset) const
{
    if (!m_isVisible) return;
    
    YuchenUI::Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    
    commandList.fillRect(
        YuchenUI::Rect(absPos.x + 2.0f, absPos.y, m_bounds.width - 4.0f, m_bounds.height),
        YuchenUI::Vec4::FromRGBA(255, 255, 255, 51)
    );
    
    renderChildren(commandList, absPos);
}

void FaderMeterSection::createComponents()
{
    if (!m_ownerContext) return;
    
    clearChildren();
    
    float mainContentHeight = m_bounds.height - NUMBER_BACKGROUND_HEIGHT;
    
    // 创建推子
    YuchenUI::Rect faderRect(3, 0, FADER_WIDTH, mainContentHeight);
    m_fader = addChild(new YuchenUI::Fader(m_ownerContext, faderRect));
    m_fader->setValueDb(0.0f);
    m_fader->setColorTheme(YuchenUI::FaderColorTheme::Normal);
    m_fader->setShowScale(true);
    
    // 创建电平表
    YuchenUI::Rect meterRect(3 + FADER_WIDTH + 10, 3, METER_WIDTH, mainContentHeight - 3);
    m_levelMeter = addChild(new YuchenUI::LevelMeter(m_ownerContext, meterRect, 2, YuchenUI::ScaleType::SAMPLE_PEAK));
    m_levelMeter->setDecayRate(40.0f);
    m_levelMeter->setPeakHoldTime(3000.0f);
    m_levelMeter->setShowControlVoltage(false);
    
    // 创建共享的数字背景框（横跨整个底部）
    float numberBgX = 2.0f;
    float numberBgY = m_bounds.height - NUMBER_BACKGROUND_HEIGHT;
    float numberBgWidth = m_bounds.width - 4.0f;
    YuchenUI::Rect numberBgRect(numberBgX, numberBgY, numberBgWidth, NUMBER_BACKGROUND_HEIGHT);
    m_faderMeterNumberBackground = addChild(new YuchenUI::NumberBackground(numberBgRect));
    
    // 计算 SpinBox 的均匀布局
    // 布局：|<4px>|spin1|<2px>|spin2|<4px>|
    const float leftMargin = 4.0f;
    const float rightMargin = 4.0f;
    const float middleGap = 2.0f;
    
    float availableWidth = numberBgWidth - leftMargin - rightMargin;
    float spinBoxWidth = (availableWidth - middleGap) / 2.0f;
    
    YuchenUI::IFontProvider* fontProvider = m_ownerContext->getFontProvider();
    YuchenUI::Rect faderDisplayRect(leftMargin, 0, spinBoxWidth, NUMBER_BACKGROUND_HEIGHT);
    m_faderNumberDisplay = m_faderMeterNumberBackground->addChild(new YuchenUI::SpinBox(faderDisplayRect));
    m_faderNumberDisplay->setHasBackground(false);
    m_faderNumberDisplay->setReadOnly(true);
    m_faderNumberDisplay->setFont(fontProvider->getDefaultBoldFont());
    m_faderNumberDisplay->setValue(0.0);
    m_faderNumberDisplay->setPrecision(1);
    m_faderNumberDisplay->setFontSize(10.0f);
    
    float meterDisplayX = leftMargin + spinBoxWidth + middleGap;
    YuchenUI::Rect meterDisplayRect(meterDisplayX, 0, spinBoxWidth, NUMBER_BACKGROUND_HEIGHT);
    m_meterNumberDisplay = m_faderMeterNumberBackground->addChild(new YuchenUI::SpinBox(meterDisplayRect));
    m_meterNumberDisplay->setHasBackground(false);
    m_meterNumberDisplay->setReadOnly(true);
    m_meterNumberDisplay->setFont(fontProvider->getDefaultBoldFont());
    m_meterNumberDisplay->setValue(0.0);
    m_meterNumberDisplay->setPrecision(1);
    m_meterNumberDisplay->setFontSize(10.0f);
}

void FaderMeterSection::updateLevel(const std::vector<float>& levels)
{
    if (m_levelMeter)
    {
        m_levelMeter->updateLevels(levels);
        updateMeterDisplay();
    }
}

void FaderMeterSection::setFaderValue(float dbValue)
{
    if (m_fader)
    {
        m_fader->setValueDb(dbValue);
        updateFaderDisplay();
    }
}

float FaderMeterSection::getFaderValue() const
{
    if (m_fader)
    {
        return m_fader->getValueDb();
    }
    return 0.0f;
}

void FaderMeterSection::setOnFaderValueChanged(std::function<void(float)> callback)
{
    if (m_fader)
    {
        m_fader->setOnValueChanged([this, callback](float dbValue) {
            updateFaderDisplay();
            if (callback)
            {
                callback(dbValue);
            }
        });
    }
}

void FaderMeterSection::updateFaderDisplay()
{
    if (m_faderNumberDisplay && m_fader)
    {
        float dbValue = m_fader->getValueDb();
        m_faderNumberDisplay->setValue(dbValue);
    }
}

void FaderMeterSection::updateMeterDisplay()
{
    if (m_meterNumberDisplay && m_levelMeter)
    {
        if (m_levelMeter->getChannelCount() > 0)
        {
            float maxPeak = -144.0f;
            for (size_t i = 0; i < m_levelMeter->getChannelCount(); ++i)
            {
                float channelPeak = -144.0f;
                maxPeak = std::max(maxPeak, channelPeak);
            }
            m_meterNumberDisplay->setValue(maxPeak);
        }
    }
}
