#pragma once

#include "ChannelSection.h"
#include <functional>

namespace YuchenUI {
    class UIContext;
}

class SoloMuteSection : public ChannelSection {
public:
    explicit SoloMuteSection(const YuchenUI::Rect& bounds);
    virtual ~SoloMuteSection();
    
    void setOnListenChanged(std::function<void(bool)> callback);
    void setOnRecordChanged(std::function<void(bool)> callback);
    void setOnSoloChanged(std::function<void(bool)> callback);
    void setOnMuteChanged(std::function<void(bool)> callback);
    
    void setPassiveMuted(bool muted);
    void updateRecordFlash(bool flashOn);
    
    void setOwnerContext(YuchenUI::UIContext* context) override;
    
    static constexpr float PREFERRED_HEIGHT = 53.0f;

private:
    struct ButtonState {
        YuchenUI::Rect bounds;
        bool isPressed;
        bool isHovered;
        const char* normalBg;
        const char* activeBg;
        const char* passiveBg;
        const char* icon;
        
        bool hitTest(const YuchenUI::Vec2& pos) const;
        const char* getCurrentBg(bool passive = false) const;
    };
    
    void createButtons();
    void addDrawCommands(YuchenUI::RenderList& commandList,
                        const YuchenUI::Vec2& offset = YuchenUI::Vec2()) const override;
    
    bool handleMouseMove(const YuchenUI::Vec2& position, const YuchenUI::Vec2& offset = YuchenUI::Vec2()) override;
    bool handleMouseClick(const YuchenUI::Vec2& position, bool pressed, const YuchenUI::Vec2& offset = YuchenUI::Vec2()) override;
    
    void handleListenClick();
    void handleRecordClick();
    void handleSoloClick();
    void handleMuteClick();
    
    void drawButton(YuchenUI::RenderList& commandList,
                   const ButtonState& button,
                   const YuchenUI::Vec2& absPos,
                   bool usePassiveBg = false) const;
    
    ButtonState m_listenButton;
    ButtonState m_recordButton;
    ButtonState m_soloButton;
    ButtonState m_muteButton;
    
    std::function<void(bool)> m_onListenChanged;
    std::function<void(bool)> m_onRecordChanged;
    std::function<void(bool)> m_onSoloChanged;
    std::function<void(bool)> m_onMuteChanged;
    
    bool m_passiveMuted;
    bool m_recordFlashState;
    
    static constexpr float BUTTON_WIDTH = 28.0f;
    static constexpr float BUTTON_HEIGHT = 20.0f;
    static constexpr float H_SPACING = 4.0f;
    static constexpr float V_SPACING = 5.0f;
};
