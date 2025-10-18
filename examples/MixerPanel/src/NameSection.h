#pragma once

#include "ChannelSection.h"
#include <string>

namespace YuchenUI {
    class TextLabel;
    class IFontProvider;
    class RenderList;
}

class NameSection : public ChannelSection {
public:
    explicit NameSection(const YuchenUI::Rect& bounds, const std::string& name);
    virtual ~NameSection();
    
    void setName(const std::string& name);
    const std::string& getName() const;
    
    void setOwnerContext(YuchenUI::UIContext* context) override;
    
    void addDrawCommands(YuchenUI::RenderList& commandList,
                        const YuchenUI::Vec2& offset = YuchenUI::Vec2()) const override;
    
    static constexpr float PREFERRED_HEIGHT = 15.0f;

private:
    void createLabel();
    
    YuchenUI::TextLabel* m_label;
    std::string m_name;
};
