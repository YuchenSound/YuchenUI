#include "MeterNumberSection.h"
#include "theme/MixerTheme.h"
#include <YuchenUI/YuchenUI.h>

MeterNumberSection::MeterNumberSection(const YuchenUI::Rect& bounds)
    : ChannelSection(bounds)
    , m_numberBackground(nullptr)
    , m_faderNumberDisplay(nullptr)
    , m_meterNumberDisplay(nullptr)
    , m_volumeIcon(nullptr)
    , m_trackTypeIcon(nullptr)
    , m_trackType(TrackType::Audio)
{
}

MeterNumberSection::~MeterNumberSection()
{
}

void MeterNumberSection::setOwnerContext(YuchenUI::UIContext* context)
{
    ChannelSection::setOwnerContext(context);
    
    if (context && !m_numberBackground)
    {
        createComponents();
    }
}

void MeterNumberSection::addDrawCommands(YuchenUI::RenderList& commandList,
                                         const YuchenUI::Vec2& offset) const
{
    if (!m_isVisible) return;
    
    YuchenUI::Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    
    if (m_mixerTheme)
    {
        YuchenUI::Vec4 bgColor = m_mixerTheme->getFaderMeterSectionBackground();
        YuchenUI::Rect iconAreaRect(
            absPos.x + 2.0f,
            absPos.y,
            m_bounds.width - 4.0f,
            ICON_AREA_HEIGHT + NUMBER_DISPLAY_HEIGHT
        );
        commandList.fillRect(iconAreaRect, bgColor);
    }
    
    renderChildren(commandList, absPos);
}

void MeterNumberSection::createComponents()
{
    if (!m_ownerContext) return;
    
    clearChildren();
    
    float numberBgX = 4.0f;
    float numberBgY = 0.0f;
    float numberBgWidth = m_bounds.width - 8.0f;
    YuchenUI::Rect numberBgRect(numberBgX, numberBgY, numberBgWidth, NUMBER_DISPLAY_HEIGHT);
    m_numberBackground = addChild(new YuchenUI::NumberBackground(numberBgRect));
    
    const float margin = 3.0f;
    float spinBoxWidth = (numberBgWidth - margin * 3.0f) / 2.0f;
    
    YuchenUI::IFontProvider* fontProvider = m_ownerContext->getFontProvider();
    
    YuchenUI::Rect faderDisplayRect(margin, 0, spinBoxWidth, NUMBER_DISPLAY_HEIGHT);
    m_faderNumberDisplay = m_numberBackground->addChild(new YuchenUI::SpinBox(faderDisplayRect));
    m_faderNumberDisplay->setHasBackground(false);
    m_faderNumberDisplay->setReadOnly(true);
    m_faderNumberDisplay->setFont(fontProvider->getDefaultBoldFont());
    m_faderNumberDisplay->setValue(0.0);
    m_faderNumberDisplay->setPrecision(1);
    m_faderNumberDisplay->setFontSize(10.0f);
    
    float meterDisplayX = margin + spinBoxWidth + margin;
    YuchenUI::Rect meterDisplayRect(meterDisplayX, 0, spinBoxWidth, NUMBER_DISPLAY_HEIGHT);
    m_meterNumberDisplay = m_numberBackground->addChild(new YuchenUI::SpinBox(meterDisplayRect));
    m_meterNumberDisplay->setHasBackground(false);
    m_meterNumberDisplay->setReadOnly(true);
    m_meterNumberDisplay->setFont(fontProvider->getDefaultBoldFont());
    m_meterNumberDisplay->setValue(-144.0);
    m_meterNumberDisplay->setPrecision(1);
    m_meterNumberDisplay->setFontSize(10.0f);
    
    float iconAreaY = NUMBER_DISPLAY_HEIGHT;
    
    YuchenUI::Rect volumeIconRect(4.0f, iconAreaY + 3.0f, 12.0f, 16.0f);
    m_volumeIcon = addChild(new YuchenUI::Image(volumeIconRect));
    m_volumeIcon->setResource("components/icon/vol_null_off.png");
    m_volumeIcon->setScaleMode(YuchenUI::ScaleMode::Original);
    
    float trackTypeIconX = m_bounds.width - 4.0f - 16.0f;
    YuchenUI::Rect trackTypeIconRect(trackTypeIconX, iconAreaY + 3.0f, 16.0f, 15.0f);
    m_trackTypeIcon = addChild(new YuchenUI::Image(trackTypeIconRect));
    m_trackTypeIcon->setResource(getTrackTypeIconPath());
    m_trackTypeIcon->setScaleMode(YuchenUI::ScaleMode::Original);
}

void MeterNumberSection::setFaderValue(float dbValue)
{
    if (m_faderNumberDisplay)
    {
        m_faderNumberDisplay->setValue(dbValue);
    }
}

void MeterNumberSection::setMeterValue(float dbValue)
{
    if (m_meterNumberDisplay)
    {
        m_meterNumberDisplay->setValue(dbValue);
    }
}

void MeterNumberSection::setTrackType(TrackType type)
{
    m_trackType = type;
    if (m_trackTypeIcon)
    {
        m_trackTypeIcon->setResource(getTrackTypeIconPath());
    }
}

const char* MeterNumberSection::getTrackTypeIconPath() const
{
    switch (m_trackType)
    {
        case TrackType::Audio:
            return "components/icon/track_type_audio.png";
        case TrackType::Aux:
            return "components/icon/track_type_aux.png";
        case TrackType::Master:
            return "components/icon/track_type_master.png";
        default:
            return "components/icon/track_type_audio.png";
    }
}
