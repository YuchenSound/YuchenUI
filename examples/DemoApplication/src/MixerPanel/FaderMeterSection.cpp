#include "MixerPanel/FaderMeterSection.h"
#include <YuchenUI/YuchenUI.h>
#include <iostream>

FaderMeterSection::FaderMeterSection(const YuchenUI::Rect& bounds)
    : ChannelSection(bounds)
    , m_fader(nullptr)
    , m_levelMeter(nullptr)
{
}

FaderMeterSection::~FaderMeterSection()
{
}

void FaderMeterSection::setOwnerContext(YuchenUI::UIContext* context)
{
    ChannelSection::setOwnerContext(context);
    
    // 使用 setOwnerContext 而不是 setOwnerContent
    if (context && !m_fader && !m_levelMeter)
    {
        createComponents();
    }
}

void FaderMeterSection::createComponents()
{
    if (!m_ownerContext) return;
    
    clearChildren();
    
    YuchenUI::Rect faderRect(0, 0, FADER_WIDTH, m_bounds.height);
    m_fader = addChild(new YuchenUI::Fader(m_ownerContext, faderRect));
    m_fader->setValueDb(0.0f);
    m_fader->setColorTheme(YuchenUI::FaderColorTheme::Normal);
    m_fader->setShowScale(true);
    
    YuchenUI::Rect meterRect(FADER_WIDTH + SPACING, 0, METER_WIDTH, m_bounds.height);
    m_levelMeter = addChild(new YuchenUI::LevelMeter(
        m_ownerContext,
        meterRect,
        2,
        YuchenUI::ScaleType::SAMPLE_PEAK
    ));
    m_levelMeter->setDecayRate(40.0f);
    m_levelMeter->setPeakHoldTime(3000.0f);
    m_levelMeter->setShowControlVoltage(false);
}

void FaderMeterSection::updateLevel(const std::vector<float>& levels)
{
    if (m_levelMeter)
    {
        m_levelMeter->updateLevels(levels);
    }
}

void FaderMeterSection::setFaderValue(float dbValue)
{
    if (m_fader)
    {
        m_fader->setValueDb(dbValue);
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
        m_fader->setOnValueChanged(callback);
    }
}
