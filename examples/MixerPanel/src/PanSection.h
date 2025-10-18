// PanSection.h
#pragma once

#include "ChannelSection.h"

namespace YuchenUI {
    class Knob;
    class NumberBackground;
    class SpinBox;
    class UIContext;
}

class PanSection : public ChannelSection {
public:
    explicit PanSection(const YuchenUI::Rect& bounds);
    virtual ~PanSection();
    
    void setLeftPanValue(int value);
    void setRightPanValue(int value);
    
    int getLeftPanValue() const;
    int getRightPanValue() const;
    
    void setOnLeftPanChanged(std::function<void(int)> callback);
    void setOnRightPanChanged(std::function<void(int)> callback);
    
    void setOwnerContext(YuchenUI::UIContext* context) override;
    
    static constexpr float PREFERRED_HEIGHT = 63.0f;

private:
    void createComponents();
    void addDrawCommands(YuchenUI::RenderList& commandList,
                        const YuchenUI::Vec2& offset = YuchenUI::Vec2()) const override;
    
    int frameToValue(int frame) const;
    int valueToFrame(int value) const;
    
    void updateLeftDisplay(int value);
    void updateRightDisplay(int value);

    YuchenUI::Knob* m_leftKnob;
    YuchenUI::Knob* m_rightKnob;
    YuchenUI::NumberBackground* m_numberBackground;
    YuchenUI::SpinBox* m_leftDisplay;
    YuchenUI::SpinBox* m_rightDisplay;
    
    int m_leftPanValue;
    int m_rightPanValue;
    
    std::function<void(int)> m_onLeftPanChanged;
    std::function<void(int)> m_onRightPanChanged;
    
    static constexpr float KNOB_TOP_MARGIN = 7.0f;
    static constexpr float KNOB_WIDTH = 34.0f;
    static constexpr float KNOB_HEIGHT = 36.0f;
    static constexpr float KNOB_SPACING = 1.0f;
    static constexpr float MIDDLE_SPACING = 3.0f;
    static constexpr float NUMBER_DISPLAY_HEIGHT = 17.0f;
    static constexpr int FRAME_COUNT = 29;
    static constexpr int CENTER_FRAME = 15;
};
