#include "SoloMuteSection.h"
#include "theme/MixerTheme.h"
#include <YuchenUI/YuchenUI.h>

SoloMuteSection::SoloMuteSection(const YuchenUI::Rect& bounds)
    : ChannelSection(bounds)
    , m_passiveMuted(false)
    , m_recordFlashState(false)
{
    m_listenButton.isPressed = false;
    m_listenButton.isHovered = false;
    m_listenButton.normalBg = "mixer_solo_mute_button/sm_btn_bg_gray.png";
    m_listenButton.activeBg = "mixer_solo_mute_button/sm_btn_bg_lime.png";
    m_listenButton.passiveBg = nullptr;
    m_listenButton.icon = "mixer_solo_mute_button/sm_listen_icon.png";
    
    m_recordButton.isPressed = false;
    m_recordButton.isHovered = false;
    m_recordButton.normalBg = "mixer_solo_mute_button/sm_btn_bg_maroon.png";
    m_recordButton.activeBg = "mixer_solo_mute_button/sm_btn_bg_crimson.png";
    m_recordButton.passiveBg = nullptr;
    m_recordButton.icon = "mixer_solo_mute_button/sm_record_icon.png";
    
    m_soloButton.isPressed = false;
    m_soloButton.isHovered = false;
    m_soloButton.normalBg = "mixer_solo_mute_button/sm_btn_bg_gray.png";
    m_soloButton.activeBg = "mixer_solo_mute_button/sm_btn_bg_gold.png";
    m_soloButton.passiveBg = nullptr;
    m_soloButton.icon = "mixer_solo_mute_button/sm_solo_icon.png";
    
    m_muteButton.isPressed = false;
    m_muteButton.isHovered = false;
    m_muteButton.normalBg = "mixer_solo_mute_button/sm_btn_bg_gray.png";
    m_muteButton.activeBg = "mixer_solo_mute_button/sm_btn_bg_orange.png";
    m_muteButton.passiveBg = "mixer_solo_mute_button/sm_btn_bg_sienna.png";
    m_muteButton.icon = "mixer_solo_mute_button/sm_mute_icon.png";
}

SoloMuteSection::~SoloMuteSection()
{
}

void SoloMuteSection::setOwnerContext(YuchenUI::UIContext* context)
{
    ChannelSection::setOwnerContext(context);
    
    if (context)
    {
        createButtons();
    }
}

void SoloMuteSection::createButtons()
{
    float totalWidth = BUTTON_WIDTH * 2 + H_SPACING;
    float totalHeight = BUTTON_HEIGHT * 2 + V_SPACING;
    
    float leftMargin = (m_bounds.width - totalWidth) / 2.0f;
    float topMargin = (PREFERRED_HEIGHT - totalHeight) / 2.0f;
    
    m_listenButton.bounds = YuchenUI::Rect(
        leftMargin,
        topMargin,
        BUTTON_WIDTH,
        BUTTON_HEIGHT
    );
    
    m_recordButton.bounds = YuchenUI::Rect(
        leftMargin + BUTTON_WIDTH + H_SPACING,
        topMargin,
        BUTTON_WIDTH,
        BUTTON_HEIGHT
    );
    
    m_soloButton.bounds = YuchenUI::Rect(
        leftMargin,
        topMargin + BUTTON_HEIGHT + V_SPACING,
        BUTTON_WIDTH,
        BUTTON_HEIGHT
    );
    
    m_muteButton.bounds = YuchenUI::Rect(
        leftMargin + BUTTON_WIDTH + H_SPACING,
        topMargin + BUTTON_HEIGHT + V_SPACING,
        BUTTON_WIDTH,
        BUTTON_HEIGHT
    );
}

void SoloMuteSection::addDrawCommands(YuchenUI::RenderList& commandList,
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
    
    drawButton(commandList, m_listenButton, absPos, false);
    
    bool showRecordActive = m_recordButton.isPressed && m_recordFlashState;
    ButtonState recordButtonToDraw = m_recordButton;
    if (m_recordButton.isPressed && !m_recordFlashState)
    {
        recordButtonToDraw.isPressed = false;
    }
    drawButton(commandList, recordButtonToDraw, absPos, false);
    
    drawButton(commandList, m_soloButton, absPos, false);
    drawButton(commandList, m_muteButton, absPos, m_passiveMuted);
    
    renderChildren(commandList, absPos);
}

void SoloMuteSection::drawButton(YuchenUI::RenderList& commandList,
                                 const ButtonState& button,
                                 const YuchenUI::Vec2& absPos,
                                 bool usePassiveBg) const
{
    YuchenUI::Rect btnRect(
        absPos.x + button.bounds.x,
        absPos.y + button.bounds.y,
        button.bounds.width,
        button.bounds.height
    );
    
    const char* bgPath = button.getCurrentBg(usePassiveBg);
    YuchenUI::NineSliceMargins margins(5.0f, 5.0f, 5.0f, 5.0f);
    commandList.drawImage(bgPath, btnRect, YuchenUI::ScaleMode::NineSlice, margins);
    
    YuchenUI::Rect iconRect(
        btnRect.x + (btnRect.width - 20.0f) / 2.0f,
        btnRect.y + (btnRect.height - 20.0f) / 2.0f,
        20.0f,
        20.0f
    );
    commandList.drawImage(button.icon, iconRect, YuchenUI::ScaleMode::Original);
}

bool SoloMuteSection::handleMouseMove(const YuchenUI::Vec2& position, const YuchenUI::Vec2& offset)
{
    if (!m_isVisible || !m_isEnabled) return false;
    
    YuchenUI::Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    YuchenUI::Vec2 localPos(position.x - absPos.x, position.y - absPos.y);
    
    m_listenButton.isHovered = m_listenButton.hitTest(localPos);
    m_recordButton.isHovered = m_recordButton.hitTest(localPos);
    m_soloButton.isHovered = m_soloButton.hitTest(localPos);
    m_muteButton.isHovered = m_muteButton.hitTest(localPos);
    
    return m_listenButton.isHovered || m_recordButton.isHovered ||
           m_soloButton.isHovered || m_muteButton.isHovered;
}

bool SoloMuteSection::handleMouseClick(const YuchenUI::Vec2& position, bool pressed, const YuchenUI::Vec2& offset)
{
    if (!m_isVisible || !m_isEnabled) return false;
    
    if (!pressed) return false;
    
    YuchenUI::Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    YuchenUI::Vec2 localPos(position.x - absPos.x, position.y - absPos.y);
    
    if (m_listenButton.hitTest(localPos))
    {
        handleListenClick();
        return true;
    }
    
    if (m_recordButton.hitTest(localPos))
    {
        handleRecordClick();
        return true;
    }
    
    if (m_soloButton.hitTest(localPos))
    {
        handleSoloClick();
        return true;
    }
    
    if (m_muteButton.hitTest(localPos))
    {
        handleMuteClick();
        return true;
    }
    
    return false;
}

void SoloMuteSection::handleListenClick()
{
    m_listenButton.isPressed = !m_listenButton.isPressed;
    
    if (m_onListenChanged)
    {
        m_onListenChanged(m_listenButton.isPressed);
    }
}

void SoloMuteSection::handleRecordClick()
{
    m_recordButton.isPressed = !m_recordButton.isPressed;
    
    if (m_onRecordChanged)
    {
        m_onRecordChanged(m_recordButton.isPressed);
    }
}

void SoloMuteSection::handleSoloClick()
{
    m_soloButton.isPressed = !m_soloButton.isPressed;
    
    if (m_onSoloChanged)
    {
        m_onSoloChanged(m_soloButton.isPressed);
    }
}

void SoloMuteSection::handleMuteClick()
{
    m_muteButton.isPressed = !m_muteButton.isPressed;
    
    if (m_onMuteChanged)
    {
        m_onMuteChanged(m_muteButton.isPressed);
    }
}

void SoloMuteSection::setOnListenChanged(std::function<void(bool)> callback)
{
    m_onListenChanged = callback;
}

void SoloMuteSection::setOnRecordChanged(std::function<void(bool)> callback)
{
    m_onRecordChanged = callback;
}

void SoloMuteSection::setOnSoloChanged(std::function<void(bool)> callback)
{
    m_onSoloChanged = callback;
}

void SoloMuteSection::setOnMuteChanged(std::function<void(bool)> callback)
{
    m_onMuteChanged = callback;
}

void SoloMuteSection::setPassiveMuted(bool muted)
{
    m_passiveMuted = muted;
}

void SoloMuteSection::updateRecordFlash(bool flashOn)
{
    m_recordFlashState = flashOn;
}

bool SoloMuteSection::ButtonState::hitTest(const YuchenUI::Vec2& pos) const
{
    return pos.x >= bounds.x && pos.x <= bounds.x + bounds.width &&
           pos.y >= bounds.y && pos.y <= bounds.y + bounds.height;
}

const char* SoloMuteSection::ButtonState::getCurrentBg(bool passive) const
{
    if (passive && passiveBg != nullptr)
    {
        return passiveBg;
    }
    
    return isPressed ? activeBg : normalBg;
}
