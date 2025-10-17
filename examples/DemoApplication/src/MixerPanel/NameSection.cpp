#include "MixerPanel/NameSection.h"
#include <YuchenUI/widgets/TextLabel.h>
#include <YuchenUI/text/IFontProvider.h>
#include <YuchenUI/core/UIContext.h>
#include <YuchenUI/rendering/RenderList.h>

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
    
    if (context && !m_label)
    {
        createLabel();
    }
}

void NameSection::addDrawCommands(YuchenUI::RenderList& commandList,
                                   const YuchenUI::Vec2& offset) const
{
    if (!m_isVisible) return;
    
    YuchenUI::Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    
    commandList.fillRect(
        YuchenUI::Rect(absPos.x + 2.0f, absPos.y, m_bounds.width - 4.0f, m_bounds.height),
        YuchenUI::Vec4::FromRGBA(154, 154, 154, 255)
    );
    
    renderChildren(commandList, absPos);
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
                         YuchenUI::VerticalAlignment::Middle);
    
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
