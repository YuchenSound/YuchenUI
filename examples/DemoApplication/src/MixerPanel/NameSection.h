#pragma once

#include "ChannelSection.h"
#include <string>

namespace YuchenUI {
    class TextLabel;
    class IFontProvider;
}

class NameSection : public ChannelSection {
public:
    explicit NameSection(const YuchenUI::Rect& bounds, const std::string& name);
    virtual ~NameSection();
    
    void setName(const std::string& name);
    const std::string& getName() const;
    
    void setOwnerContext(YuchenUI::UIContext* context) override;  // 改为 setOwnerContext

private:
    void createLabel();
    
    YuchenUI::TextLabel* m_label;
    std::string m_name;
};
