#include "MixerClassicTheme.h"

YuchenUI::Vec4 MixerClassicTheme::getFaderMeterSectionBackground() const
{
    return YuchenUI::Vec4::FromRGBA(255, 255, 255, 51);
}

YuchenUI::Vec4 MixerClassicTheme::getChannelStripBackground() const
{
    return YuchenUI::Vec4::FromRGBA(77, 77, 77, 255);
}

YuchenUI::Vec4 MixerClassicTheme::getChannelStripBorder() const
{
    return YuchenUI::Vec4::FromRGBA(49, 49, 49, 255);
}

YuchenUI::Vec4 MixerClassicTheme::getNameSectionBackground() const
{
    return YuchenUI::Vec4::FromRGBA(154, 154, 154, 255);
}
