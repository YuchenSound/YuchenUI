#pragma once

#include <YuchenUI/YuchenUI-Desktop.h>
#include <vector>
#include <memory>

class ChannelStrip;
class MixerTheme;

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
    
    void updateMixerTheme();
    void applyMixerThemeToChildren();
    
    void handleSoloChanged(int channelNumber, bool active);
    
    YuchenUI::ScrollArea* m_scrollArea;
    std::vector<ChannelStrip*> m_channelStrips;
    
    std::unique_ptr<MixerTheme> m_mixerTheme;
    YuchenUI::StyleType m_lastStyleType;
    
    static constexpr int CHANNEL_COUNT = 36;
    
    float m_time;
    float m_globalRecordTime;
    std::vector<float> m_phases;
    
    bool m_anySoloActive;
};
