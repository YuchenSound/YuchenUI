#include "FaderMeterSection.h"
#include "theme/MixerTheme.h"
#include <YuchenUI/YuchenUI.h>
#include <iostream>

FaderMeterSection::FaderMeterSection(const YuchenUI::Rect& bounds)
    : ChannelSection(bounds)
    , m_fader(nullptr)
    , m_levelMeter(nullptr)
    , m_onFaderValueChanged(nullptr)
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

void FaderMeterSection::createComponents()
{
    if (!m_ownerContext) return;
    
    clearChildren();
    
    YuchenUI::Rect faderRect(3, 0, FADER_WIDTH, m_bounds.height);
    m_fader = addChild(new YuchenUI::Fader(m_ownerContext, faderRect));
    m_fader->setValueDb(0.0f);
    m_fader->setColorTheme(YuchenUI::FaderColorTheme::Normal);
    m_fader->setShowScale(true);
    
    YuchenUI::Rect meterRect(3 + FADER_WIDTH + 10, 3, METER_WIDTH, m_bounds.height - 3);
    m_levelMeter = addChild(new YuchenUI::LevelMeter(m_ownerContext, meterRect, 2, YuchenUI::ScaleType::SAMPLE_PEAK));
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
    m_onFaderValueChanged = callback;
    if (m_fader)
    {
        m_fader->setOnValueChanged([this](float dbValue) {
            if (m_onFaderValueChanged)
            {
                m_onFaderValueChanged(dbValue);
            }
        });
    }
}

void FaderMeterSection::setFaderColorTheme(YuchenUI::FaderColorTheme theme)
{
    if (m_fader)
    {
        m_fader->setColorTheme(theme);
    }
}
