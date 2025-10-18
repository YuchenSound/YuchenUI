#include "MixerDarkTheme.h"

YuchenUI::Vec4 MixerDarkTheme::getFaderMeterSectionBackground() const
{
    return YuchenUI::Vec4::FromRGBA(0, 0, 0, 51);
}

YuchenUI::Vec4 MixerDarkTheme::getChannelStripBackground() const
{
    return YuchenUI::Vec4::FromRGBA(77, 77, 77, 255);
}

YuchenUI::Vec4 MixerDarkTheme::getChannelStripBorder() const
{
    return YuchenUI::Vec4::FromRGBA(0, 0, 0, 255);
}

YuchenUI::Vec4 MixerDarkTheme::getNameSectionBackground() const
{
    return YuchenUI::Vec4::FromRGBA(46, 46, 46, 255);
}
