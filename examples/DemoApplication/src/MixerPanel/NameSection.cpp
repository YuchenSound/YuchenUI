#include "MixerPanel/NameSection.h"
#include <YuchenUI/widgets/TextLabel.h>
#include <YuchenUI/text/IFontProvider.h>
#include <YuchenUI/core/UIContext.h>

NameSection::NameSection(const YuchenUI::Rect& bounds, const std::string& name)
    : ChannelSection(bounds)
    , m_label(nullptr)
    , m_name(name)
{
}

NameSection::~NameSection()
{
}

void NameSection::setOwnerContext(YuchenUI::UIContext* context)
{
    ChannelSection::setOwnerContext(context);
    
    // 使用 setOwnerContext 而不是 setOwnerContent
    if (context && !m_label)
    {
        createLabel();
    }
}

void NameSection::createLabel()
{
    if (!m_ownerContext) return;
    
    clearChildren();
    
    YuchenUI::Rect labelBounds(0, 0, m_bounds.width, m_bounds.height);
    m_label = addChild(new YuchenUI::TextLabel(labelBounds));
    m_label->setText(m_name);
    m_label->setFontSize(11.0f);
    m_label->setAlignment(YuchenUI::TextAlignment::Center,
                         YuchenUI::VerticalAlignment::Bottom);
    
    YuchenUI::IFontProvider* fontProvider = m_ownerContext->getFontProvider();
    if (fontProvider)
    {
        m_label->setFont(fontProvider->getDefaultFont());
    }
}

void NameSection::setName(const std::string& name)
{
    m_name = name;
    if (m_label)
    {
        m_label->setText(m_name);
    }
}

const std::string& NameSection::getName() const
{
    return m_name;
}
