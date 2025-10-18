#include "ChannelStrip.h"
#include "FaderMeterSection.h"
#include "MeterNumberSection.h"
#include "SoloMuteSection.h"
#include "NameSection.h"
#include "theme/MixerTheme.h"
#include <YuchenUI/rendering/RenderList.h>
#include <sstream>
#include <iostream>

ChannelStrip::ChannelStrip(const YuchenUI::Rect& bounds, int channelNumber, TrackType trackType)
    : Widget()
    , m_channelNumber(channelNumber)
    , m_trackType(trackType)
    , m_faderMeterSection(nullptr)
    , m_meterNumberSection(nullptr)
    , m_soloMuteSection(nullptr)
    , m_nameSection(nullptr)
    , m_mixerTheme(nullptr)
{
    setBounds(bounds);
}

ChannelStrip::~ChannelStrip()
{
}

float ChannelStrip::getStripHeight()
{
    return FaderMeterSection::PREFERRED_HEIGHT +
           MeterNumberSection::PREFERRED_HEIGHT +
           SoloMuteSection::PREFERRED_HEIGHT +
           NameSection::PREFERRED_HEIGHT;
}

void ChannelStrip::setOwnerContext(YuchenUI::UIContext* context)
{
    Widget::setOwnerContext(context);
    
    if (context && !m_faderMeterSection && !m_meterNumberSection && !m_soloMuteSection && !m_nameSection)
    {
        createSections();
    }
}

void ChannelStrip::setMixerTheme(MixerTheme* theme)
{
    m_mixerTheme = theme;
    
    if (m_faderMeterSection)
    {
        m_faderMeterSection->setMixerTheme(theme);
    }
    if (m_meterNumberSection)
    {
        m_meterNumberSection->setMixerTheme(theme);
    }
    if (m_soloMuteSection)
    {
        m_soloMuteSection->setMixerTheme(theme);
    }
    if (m_nameSection)
    {
        m_nameSection->setMixerTheme(theme);
    }
}

void ChannelStrip::createSections()
{
    if (!m_ownerContext) return;
    
    clearChildren();
    
    float currentY = 0.0f;
    
    YuchenUI::Rect faderMeterBounds(
        BORDER_SIZE,
        currentY,
        CONTENT_WIDTH,
        FaderMeterSection::PREFERRED_HEIGHT
    );
    m_faderMeterSection = addChild(new FaderMeterSection(faderMeterBounds));
    m_faderMeterSection->setMixerTheme(m_mixerTheme);
    
    m_faderMeterSection->setOnFaderValueChanged([this](float dbValue) {
        if (m_meterNumberSection)
        {
            m_meterNumberSection->setFaderValue(dbValue);
        }
        std::cout << "Channel " << m_channelNumber
                  << " fader changed: " << dbValue << " dB" << std::endl;
    });
    
    currentY += FaderMeterSection::PREFERRED_HEIGHT;
    
    YuchenUI::Rect meterNumberBounds(
        BORDER_SIZE,
        currentY,
        CONTENT_WIDTH,
        MeterNumberSection::PREFERRED_HEIGHT
    );
    m_meterNumberSection = addChild(new MeterNumberSection(meterNumberBounds));
    m_meterNumberSection->setMixerTheme(m_mixerTheme);
    m_meterNumberSection->setTrackType(m_trackType);
    
    currentY += MeterNumberSection::PREFERRED_HEIGHT;
    
    YuchenUI::Rect soloMuteBounds(
        BORDER_SIZE,
        currentY,
        CONTENT_WIDTH,
        SoloMuteSection::PREFERRED_HEIGHT
    );
    m_soloMuteSection = addChild(new SoloMuteSection(soloMuteBounds));
    m_soloMuteSection->setMixerTheme(m_mixerTheme);
    
    m_soloMuteSection->setOnListenChanged([this](bool active) {
        if (m_onListenChanged)
        {
            m_onListenChanged(m_channelNumber, active);
        }
    });
    
    m_soloMuteSection->setOnRecordChanged([this](bool active) {
        if (m_faderMeterSection)
        {
            m_faderMeterSection->setFaderColorTheme(
                active ? YuchenUI::FaderColorTheme::Red : YuchenUI::FaderColorTheme::Normal
            );
        }
        
        if (m_onRecordChanged)
        {
            m_onRecordChanged(m_channelNumber, active);
        }
    });
    
    m_soloMuteSection->setOnSoloChanged([this](bool active) {
        if (m_onSoloChanged)
        {
            m_onSoloChanged(m_channelNumber, active);
        }
    });
    
    m_soloMuteSection->setOnMuteChanged([this](bool active) {
        if (m_onMuteChanged)
        {
            m_onMuteChanged(m_channelNumber, active);
        }
    });
    
    currentY += SoloMuteSection::PREFERRED_HEIGHT;
    
    std::ostringstream oss;
    oss << "Ch " << m_channelNumber;
    YuchenUI::Rect nameBounds(
        BORDER_SIZE,
        currentY,
        CONTENT_WIDTH,
        NameSection::PREFERRED_HEIGHT
    );
    m_nameSection = addChild(new NameSection(nameBounds, oss.str()));
    m_nameSection->setMixerTheme(m_mixerTheme);
}

void ChannelStrip::addDrawCommands(YuchenUI::RenderList& commandList,
                                   const YuchenUI::Vec2& offset) const
{
    if (!m_isVisible) return;
    
    YuchenUI::Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    
    if (m_mixerTheme)
    {
        YuchenUI::Vec4 bgColor = m_mixerTheme->getChannelStripBackground();
        commandList.fillRect(
            YuchenUI::Rect(absPos.x, absPos.y, m_bounds.width, m_bounds.height),
            bgColor
        );
        
        YuchenUI::Vec4 borderColor = m_mixerTheme->getChannelStripBorder();
        
        commandList.drawLine(
            YuchenUI::Vec2(absPos.x + 0.5f, absPos.y),
            YuchenUI::Vec2(absPos.x + 0.5f, absPos.y + m_bounds.height),
            borderColor,
            1.0f
        );
        
        commandList.drawLine(
            YuchenUI::Vec2(absPos.x + m_bounds.width - 0.5f, absPos.y),
            YuchenUI::Vec2(absPos.x + m_bounds.width - 0.5f, absPos.y + m_bounds.height),
            borderColor,
            1.0f
        );
    }
    
    renderChildren(commandList, absPos);
}

bool ChannelStrip::handleMouseMove(const YuchenUI::Vec2& position, const YuchenUI::Vec2& offset)
{
    return dispatchMouseEvent(position, false, offset, true);
}

bool ChannelStrip::handleMouseClick(const YuchenUI::Vec2& position, bool pressed, const YuchenUI::Vec2& offset)
{
    return dispatchMouseEvent(position, pressed, offset, false);
}

void ChannelStrip::updateLevel(const std::vector<float>& levels)
{
    if (m_faderMeterSection)
    {
        m_faderMeterSection->updateLevel(levels);
    }
    
    if (m_meterNumberSection)
    {
        m_meterNumberSection->setMeterValue(-144.0f);
    }
}

void ChannelStrip::setChannelName(const std::string& name)
{
    if (m_nameSection)
    {
        m_nameSection->setName(name);
    }
}

void ChannelStrip::setFaderValue(float dbValue)
{
    if (m_faderMeterSection)
    {
        m_faderMeterSection->setFaderValue(dbValue);
    }
    if (m_meterNumberSection)
    {
        m_meterNumberSection->setFaderValue(dbValue);
    }
}

float ChannelStrip::getFaderValue() const
{
    if (m_faderMeterSection)
    {
        return m_faderMeterSection->getFaderValue();
    }
    return 0.0f;
}

void ChannelStrip::setOnListenChanged(std::function<void(int, bool)> callback)
{
    m_onListenChanged = callback;
}

void ChannelStrip::setOnRecordChanged(std::function<void(int, bool)> callback)
{
    m_onRecordChanged = callback;
}

void ChannelStrip::setOnSoloChanged(std::function<void(int, bool)> callback)
{
    m_onSoloChanged = callback;
}

void ChannelStrip::setOnMuteChanged(std::function<void(int, bool)> callback)
{
    m_onMuteChanged = callback;
}

void ChannelStrip::setPassiveMuted(bool muted)
{
    if (m_soloMuteSection)
    {
        m_soloMuteSection->setPassiveMuted(muted);
    }
}

void ChannelStrip::updateRecordFlash(bool flashOn)
{
    if (m_soloMuteSection)
    {
        m_soloMuteSection->updateRecordFlash(flashOn);
    }
}
