#include "theme/MixerTheme.h"
#include "theme/MixerDarkTheme.h"
#include "theme/MixerClassicTheme.h"

std::unique_ptr<MixerTheme> MixerTheme::create(YuchenUI::StyleType type)
{
    switch (type)
    {
        case YuchenUI::StyleType::ProtoolsDark:
            return std::make_unique<MixerDarkTheme>();
            
        case YuchenUI::StyleType::ProtoolsClassic:
            return std::make_unique<MixerClassicTheme>();
            
        default:
            return std::make_unique<MixerDarkTheme>();
    }
}
