#include "MixerWindow.h"
#include "ChannelStrip.h"
#include <cmath>

MixerWindowContent::MixerWindowContent()
    : m_scrollArea(nullptr)
    , m_time(0.0f)
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
    
    createUI();
}

void MixerWindowContent::createUI()
{
    YuchenUI::IFontProvider* fontProvider = m_context->getFontProvider();
    
    YuchenUI::Rect titleBounds(10, 10, m_contentArea.width - 20, 30);
    m_titleLabel = std::make_unique<YuchenUI::TextLabel>(titleBounds);
    m_titleLabel->setText("Mixer");
    m_titleLabel->setFont(fontProvider->getDefaultBoldFont());
    m_titleLabel->setFontSize(18.0f);
    m_titleLabel->setAlignment(YuchenUI::TextAlignment::Center,
                              YuchenUI::VerticalAlignment::Middle);
    addComponent(m_titleLabel.get());
    
    YuchenUI::Rect scrollBounds(10, 50, m_contentArea.width - 20, m_contentArea.height - 60);
    std::cout << "[MixerWindow] Creating ScrollArea at bounds: ("
              << scrollBounds.x << ", " << scrollBounds.y << ", "
              << scrollBounds.width << ", " << scrollBounds.height << ")" << std::endl;

    
    
    m_scrollArea = new YuchenUI::ScrollArea(scrollBounds);
    m_scrollArea->setOwnerContext(m_context);
    addComponent(m_scrollArea);
    
    float totalWidth = (ChannelStrip::STRIP_WIDTH + CHANNEL_SPACING) * CHANNEL_COUNT;
    m_scrollArea->setContentSize(YuchenUI::Vec2(totalWidth, ChannelStrip::STRIP_HEIGHT));
    m_scrollArea->setShowVerticalScrollbar(false);
    m_scrollArea->setShowHorizontalScrollbar(true);
    
    createChannelStrips();
}

void MixerWindowContent::createChannelStrips()
{
    if (!m_scrollArea) return;
    
    m_channelStrips.clear();
    
    for (int i = 0; i < CHANNEL_COUNT; ++i)
    {
        float xPos = i * (ChannelStrip::STRIP_WIDTH + CHANNEL_SPACING);
        YuchenUI::Rect stripBounds(xPos, 0,
                                   ChannelStrip::STRIP_WIDTH,
                                   ChannelStrip::STRIP_HEIGHT);
        
        ChannelStrip* strip = m_scrollArea->addChild(new ChannelStrip(stripBounds, i + 1));
        m_channelStrips.push_back(strip);
    }
}

void MixerWindowContent::onDestroy()
{
    m_channelStrips.clear();
    IUIContent::onDestroy();
}

void MixerWindowContent::onUpdate(float deltaTime)
{
    m_time += deltaTime;
    updateTestSignals();
}

void MixerWindowContent::updateTestSignals()
{
    if (!m_scrollArea) return;
    
    for (size_t i = 0; i < m_channelStrips.size(); ++i)
    {
        std::vector<float> levels(2);
        generateTestLevel(static_cast<int>(i), levels);
        m_channelStrips[i]->updateLevel(levels);
    }
}

void MixerWindowContent::generateTestLevel(int channelIndex, std::vector<float>& levels)
{
    float frequency1 = 0.5f + channelIndex * 0.15f;
    float frequency2 = 1.2f + channelIndex * 0.2f;
    
    float signal1 = std::sin(m_phases[channelIndex] * frequency1) * 0.4f;
    float signal2 = std::sin(m_phases[channelIndex] * frequency2) * 0.3f;
    float noise = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.15f;
    
    float amplitudeL = signal1 + signal2 + noise;
    float amplitudeR = signal1 * 0.8f + signal2 * 1.2f + noise;
    
    amplitudeL = std::clamp(amplitudeL, -1.0f, 1.0f);
    amplitudeR = std::clamp(amplitudeR, -1.0f, 1.0f);
    
    if (std::abs(amplitudeL) < 0.00001f)
        levels[0] = -144.0f;
    else
        levels[0] = std::clamp(20.0f * std::log10(std::abs(amplitudeL)), -144.0f, 0.0f);
    
    if (std::abs(amplitudeR) < 0.00001f)
        levels[1] = -144.0f;
    else
        levels[1] = std::clamp(20.0f * std::log10(std::abs(amplitudeR)), -144.0f, 0.0f);
    
    m_phases[channelIndex] += 0.1f;
    if (m_phases[channelIndex] > 2.0f * 3.14159f)
        m_phases[channelIndex] -= 2.0f * 3.14159f;
}

void MixerWindowContent::render(YuchenUI::RenderList& commandList)
{
    if (m_titleLabel)
    {
        m_titleLabel->addDrawCommands(commandList);
    }
    
    if (m_scrollArea)
    {
        m_scrollArea->addDrawCommands(commandList);
    }
}
