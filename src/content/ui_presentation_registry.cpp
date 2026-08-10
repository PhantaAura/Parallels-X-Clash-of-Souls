#include "content/ui_presentation_registry.hpp"

namespace px {

UiPresentationRegistry::UiPresentationRegistry() {
    theme_ = {
        {7, 24, 52, 255},
        {8, 8, 8, 255},
        {202, 25, 34, 255},
        {235, 42, 36, 255},
        {255, 139, 31, 255},
        {255, 211, 70, 255},
        {248, 246, 238, 255},
        {194, 205, 216, 255},
        {13, 91, 180, 244},
        {244, 234, 207, 255},
    };
    layout_ = {
        1280, 720,
        {36, 30, 1208, 660},
        {58, 38, 1164, 74},
        {88, 150, 1104, 392},
        {690, 122, 470, 430},
        {432, 535, 416, 56},
        {70, 646, 1140, 40},
        {430, 132, 420, 430},
        {72, 132, 410, 180},
        {54, 120, 760, 480},
        {838, 120, 388, 480},
        {44, 100, 264, 560},
        {326, 100, 910, 560},
        {28, 28, 455, 72},
        {28, 28, 500, 104},
        {752, 28, 500, 104},
        {826, 572, 426, 116},
        {46, 514, 1188, 170},
    };
}

UiColor UiPresentationRegistry::accent(const UiTheme& theme, const std::string& accentId) {
    if (accentId == "ember") return theme.ember;
    if (accentId == "warm_gold") return theme.warmGold;
    if (accentId == "off_white") return theme.offWhite;
    if (accentId == "earth") return {160, 105, 55, 255};
    if (accentId == "lightning") return {70, 138, 232, 255};
    if (accentId == "emerald") return {45, 190, 112, 255};
    if (accentId == "muted_red") return {112, 43, 49, 255};
    return theme.crimson;
}

} // namespace px
