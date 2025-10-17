#include "MixerPanel/ChannelStrip.h"
#include "MixerPanel/FaderMeterSection.h"
#include "MixerPanel/NameSection.h"
#include <YuchenUI/rendering/RenderList.h>
#include <sstream>
#include <iostream>

ChannelStrip::ChannelStrip(const YuchenUI::Rect& bounds, int channelNumber)
    : Widget()
    , m_channelNumber(channelNumber)
    , m_faderMeterSection(nullptr)
    , m_nameSection(nullptr)
{
    setBounds(bounds);
}

ChannelStrip::~ChannelStrip()
{
}

float ChannelStrip::getStripHeight()
{
    return FaderMeterSection::PREFERRED_HEIGHT + NameSection::PREFERRED_HEIGHT;
}

void ChannelStrip::setOwnerContext(YuchenUI::UIContext* context)
{
    Widget::setOwnerContext(context);
    
    if (context && !m_faderMeterSection && !m_nameSection)
    {
        createSections();
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
    
    m_faderMeterSection->setOnFaderValueChanged([this](float dbValue) {
        std::cout << "Channel " << m_channelNumber
                  << " fader changed: " << dbValue << " dB" << std::endl;
    });
    
    currentY += FaderMeterSection::PREFERRED_HEIGHT;
    
    std::ostringstream oss;
    oss << "Ch " << m_channelNumber;
    YuchenUI::Rect nameBounds(
        BORDER_SIZE,
        currentY,
        CONTENT_WIDTH,
        NameSection::PREFERRED_HEIGHT
    );
    m_nameSection = addChild(new NameSection(nameBounds, oss.str()));
}

void ChannelStrip::addDrawCommands(YuchenUI::RenderList& commandList,
                                   const YuchenUI::Vec2& offset) const
{
    if (!m_isVisible) return;
    
    YuchenUI::Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    
    commandList.fillRect(
        YuchenUI::Rect(absPos.x, absPos.y, m_bounds.width, m_bounds.height),
        YuchenUI::Vec4::FromRGBA(77, 77, 77, 255)
    );
    
    YuchenUI::Vec4 borderColor = YuchenUI::Vec4::FromRGBA(49, 49, 49, 255);
    
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
}

float ChannelStrip::getFaderValue() const
{
    if (m_faderMeterSection)
    {
        return m_faderMeterSection->getFaderValue();
    }
    return 0.0f;
}
