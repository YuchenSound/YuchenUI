#include "MixerWindow.h"
#include "ChannelStrip.h"
#include "theme/MixerTheme.h"
#include <cmath>
#include <iostream>

MixerWindowContent::MixerWindowContent()
    : m_scrollArea(nullptr)
    , m_mixerTheme(nullptr)
    , m_lastStyleType(YuchenUI::StyleType::ProtoolsDark)
    , m_time(0.0f)
    , m_globalRecordTime(0.0f)
    , m_anySoloActive(false)
{
    m_phases.resize(CHANNEL_COUNT, 0.0f);
    m_channelStrips.reserve(CHANNEL_COUNT);
}

MixerWindowContent::~MixerWindowContent()
{
}

void MixerWindowContent::onCreate(YuchenUI::UIContext* context,
                                  const YuchenUI::Rect& contentArea)
{
    m_context = context;
    m_contentArea = contentArea;
    
    updateMixerTheme();
    
    createUI();
}

void MixerWindowContent::updateMixerTheme()
{
    if (!m_context) return;
    
    YuchenUI::UIStyle* currentStyle = m_context->getCurrentStyle();
    if (!currentStyle) return;
    
    YuchenUI::StyleType currentType = currentStyle->getType();
    
    if (!m_mixerTheme || m_lastStyleType != currentType)
    {
        m_mixerTheme = MixerTheme::create(currentType);
        m_lastStyleType = currentType;
        
        applyMixerThemeToChildren();
    }
}

void MixerWindowContent::applyMixerThemeToChildren()
{
    if (!m_mixerTheme) return;
    
    for (ChannelStrip* strip : m_channelStrips)
    {
        if (strip)
        {
            strip->setMixerTheme(m_mixerTheme.get());
        }
    }
}

void MixerWindowContent::createUI()
{
    YuchenUI::Rect scrollBounds(0, 0, m_contentArea.width, m_contentArea.height);
    
    m_scrollArea = new YuchenUI::ScrollArea(scrollBounds);
    m_scrollArea->setOwnerContext(m_context);
    addComponent(m_scrollArea);
    
    float stripHeight = ChannelStrip::getStripHeight();
    float totalWidth = ChannelStrip::STRIP_WIDTH * CHANNEL_COUNT;
    
    m_scrollArea->setContentSize(YuchenUI::Vec2(totalWidth, stripHeight));
    m_scrollArea->setShowVerticalScrollbar(false);
    m_scrollArea->setShowHorizontalScrollbar(true);
    
    createChannelStrips();
}

void MixerWindowContent::createChannelStrips()
{
    if (!m_scrollArea) return;
    
    m_channelStrips.clear();
    
    float stripHeight = ChannelStrip::getStripHeight();
    
    for (int i = 0; i < CHANNEL_COUNT; ++i)
    {
        float xPos = i * ChannelStrip::STRIP_WIDTH;
        YuchenUI::Rect stripBounds(xPos, 0, ChannelStrip::STRIP_WIDTH, stripHeight);
        
        TrackType trackType = TrackType::Audio;
        if (i == 0)
        {
            trackType = TrackType::Master;
        }
        else if (i % 5 == 1)
        {
            trackType = TrackType::Aux;
        }
        
        ChannelStrip* strip = m_scrollArea->addChild(new ChannelStrip(stripBounds, i + 1, trackType));
        
        if (m_mixerTheme)
        {
            strip->setMixerTheme(m_mixerTheme.get());
        }
        
        strip->setOnListenChanged([this](int channelNum, bool active) {
            std::cout << "Channel " << channelNum << " listen: " << (active ? "ON" : "OFF") << std::endl;
        });
        
        strip->setOnRecordChanged([this](int channelNum, bool active) {
            std::cout << "Channel " << channelNum << " record: " << (active ? "ON" : "OFF") << std::endl;
        });
        
        strip->setOnSoloChanged([this](int channelNum, bool active) {
            handleSoloChanged(channelNum, active);
        });
        
        strip->setOnMuteChanged([this](int channelNum, bool active) {
            std::cout << "Channel " << channelNum << " mute: " << (active ? "ON" : "OFF") << std::endl;
        });
        
        m_channelStrips.push_back(strip);
    }
}

void MixerWindowContent::handleSoloChanged(int channelNumber, bool active)
{
    std::cout << "Channel " << channelNumber << " solo: " << (active ? "ON" : "OFF") << std::endl;
    
    bool anySoloActive = false;
    for (auto* strip : m_channelStrips)
    {
        if (strip->getChannelNumber() != channelNumber)
        {
            anySoloActive = anySoloActive || active;
        }
    }
    
    for (auto* strip : m_channelStrips)
    {
        if (strip->getChannelNumber() == channelNumber)
        {
            continue;
        }
        
        strip->setPassiveMuted(active);
    }
    
    m_anySoloActive = anySoloActive;
}

void MixerWindowContent::updateScrollAreaBounds()
{
    if (!m_scrollArea) return;
    
    YuchenUI::Rect newBounds(0, 0, m_contentArea.width, m_contentArea.height);
    m_scrollArea->setBounds(newBounds);
}

void MixerWindowContent::onResize(const YuchenUI::Rect& newArea)
{
    m_contentArea = newArea;
    updateScrollAreaBounds();
}

void MixerWindowContent::onDestroy()
{
    m_channelStrips.clear();
    m_mixerTheme.reset();
    IUIContent::onDestroy();
}

void MixerWindowContent::onUpdate(float deltaTime)
{
    m_time += deltaTime;
    m_globalRecordTime += deltaTime;
    
    updateMixerTheme();
    
    float cycle = std::fmod(m_globalRecordTime, 1.0f);
    bool flashOn = (cycle < 0.5f);
    
    for (auto* strip : m_channelStrips)
    {
        strip->updateRecordFlash(flashOn);
    }
    
    updateTestSignals();
}

void MixerWindowContent::updateTestSignals()
{
    if (!m_scrollArea) return;

}

void MixerWindowContent::generateTestLevel(int channelIndex, std::vector<float>& levels)
{
    levels[0] = -144.0f;
    levels[1] = -144.0f;
}

void MixerWindowContent::render(YuchenUI::RenderList& commandList)
{
    if (m_scrollArea)
    {
        m_scrollArea->addDrawCommands(commandList);
    }
}
