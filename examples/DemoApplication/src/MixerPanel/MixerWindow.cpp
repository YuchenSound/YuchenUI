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
        
        ChannelStrip* strip = m_scrollArea->addChild(new ChannelStrip(stripBounds, i + 1));
        m_channelStrips.push_back(strip);
    }
}

void MixerWindowContent::updateScrollAreaBounds()
{
    if (!m_scrollArea) return;
    
    // ScrollArea 填充整个内容区域
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
    // 使用 deltaTime 计算相位，使动画速度恒定
    float frequency1 = 0.5f + channelIndex * 0.15f;  // Hz
    float frequency2 = 1.2f + channelIndex * 0.2f;   // Hz
    
    // 相位 = 时间 × 频率 × 2π
    float phase1 = m_time * frequency1 * 2.0f * 3.14159f;
    float phase2 = m_time * frequency2 * 2.0f * 3.14159f;
    
    float signal1 = std::sin(phase1) * 0.4f;
    float signal2 = std::sin(phase2) * 0.3f;
    float noise = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.15f;
    
    float amplitudeL = signal1 + signal2 + noise;
    float amplitudeR = signal1 * 0.8f + signal2 * 1.2f + noise;
    
    amplitudeL = std::clamp(amplitudeL, -1.0f, 1.0f);
    amplitudeR = std::clamp(amplitudeR, -1.0f, 1.0f);
    
    // 转换为 dB
    if (std::abs(amplitudeL) < 0.00001f)
        levels[0] = -144.0f;
    else
        levels[0] = std::clamp(20.0f * std::log10(std::abs(amplitudeL)), -144.0f, 0.0f);
    
    if (std::abs(amplitudeR) < 0.00001f)
        levels[1] = -144.0f;
    else
        levels[1] = std::clamp(20.0f * std::log10(std::abs(amplitudeR)), -144.0f, 0.0f);
}

void MixerWindowContent::render(YuchenUI::RenderList& commandList)
{
    if (m_scrollArea)
    {
        m_scrollArea->addDrawCommands(commandList);
    }
}
