#include "MixerPanel/ChannelStrip.h"
#include "MixerPanel/FaderMeterSection.h"
#include "MixerPanel/NameSection.h"
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

void ChannelStrip::setOwnerContext(YuchenUI::UIContext* context)
{
    Widget::setOwnerContext(context);
    
    // 使用 setOwnerContext 而不是 setOwnerContent，因为 addChild() 会调用这个
    if (context && !m_faderMeterSection && !m_nameSection)
    {
        createSections();
    }
}

void ChannelStrip::createSections()
{
    if (!m_ownerContext) return;
    
    clearChildren();
    
    YuchenUI::Rect faderMeterBounds(0, 0, FADER_METER_WIDTH, m_bounds.height);
    m_faderMeterSection = addChild(new FaderMeterSection(faderMeterBounds));
    
    m_faderMeterSection->setOnFaderValueChanged([this](float dbValue) {
        std::cout << "Channel " << m_channelNumber
                  << " fader changed: " << dbValue << " dB" << std::endl;
    });
    
    std::ostringstream oss;
    oss << "Ch " << m_channelNumber;
    YuchenUI::Rect nameBounds(FADER_METER_WIDTH, 0, NAME_WIDTH, m_bounds.height);
    m_nameSection = addChild(new NameSection(nameBounds, oss.str()));
}

void ChannelStrip::addDrawCommands(YuchenUI::RenderList& commandList,
                                   const YuchenUI::Vec2& offset) const
{
    if (!m_isVisible) return;
    
    YuchenUI::Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    
    renderChildren(commandList, absPos);
}

bool ChannelStrip::handleMouseMove(const YuchenUI::Vec2& position, const YuchenUI::Vec2& offset)
{
    return dispatchMouseEvent(position, false, offset, true);
}

bool ChannelStrip::handleMouseClick(const YuchenUI::Vec2& position, bool pressed, const YuchenUI::Vec2& offset)
{
    // 限制每个实例最多打印3次
    static int printCount = 0;
    const int MAX_PRINTS = 3;
    
    if (pressed && printCount < MAX_PRINTS)
    {
        YuchenUI::Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
        YuchenUI::Rect absRect(absPos.x, absPos.y, m_bounds.width, m_bounds.height);
        
        // 只打印真正处理事件的 Strip（鼠标在范围内）
        if (absRect.contains(position))
        {
            std::cout << "\n[ChannelStrip " << m_channelNumber << "] Click Debug:" << std::endl;
            std::cout << "  position: (" << position.x << ", " << position.y << ")" << std::endl;
            std::cout << "  received offset: (" << offset.x << ", " << offset.y << ")" << std::endl;
            std::cout << "  m_bounds: (" << m_bounds.x << ", " << m_bounds.y << ", "
                      << m_bounds.width << ", " << m_bounds.height << ")" << std::endl;
            std::cout << "  calculated absPos: (" << absPos.x << ", " << absPos.y << ")" << std::endl;
            std::cout << "  absRect: (" << absRect.x << ", " << absRect.y << ", "
                      << absRect.width << ", " << absRect.height << ")" << std::endl;
            std::cout << "  contains position: true" << std::endl;
            printCount++;
        }
    }
    
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
