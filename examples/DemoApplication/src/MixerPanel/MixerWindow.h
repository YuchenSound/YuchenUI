#pragma once

#include <YuchenUI/YuchenUI-Desktop.h>
#include <vector>

class ChannelStrip;

class MixerWindowContent : public YuchenUI::IUIContent {
public:
    MixerWindowContent();
    virtual ~MixerWindowContent();
    
    void onCreate(YuchenUI::UIContext* context, const YuchenUI::Rect& contentArea) override;
    void onDestroy() override;
    void onResize(const YuchenUI::Rect& newArea) override;
    void onUpdate(float deltaTime) override;
    void render(YuchenUI::RenderList& commandList) override;

private:
    void createUI();
    void createChannelStrips();
    void updateTestSignals();
    void generateTestLevel(int channelIndex, std::vector<float>& levels);
    void updateScrollAreaBounds();
    
    YuchenUI::ScrollArea* m_scrollArea;
    std::vector<ChannelStrip*> m_channelStrips;
    
    static constexpr int CHANNEL_COUNT = 36;
    
    float m_time;
    std::vector<float> m_phases;
};
