#pragma once
#include <cstdint>
#include <string>

namespace px {

struct UiColor {
    std::uint8_t r{0};
    std::uint8_t g{0};
    std::uint8_t b{0};
    std::uint8_t a{255};
};

struct UiRect {
    int x{0};
    int y{0};
    int w{0};
    int h{0};
};

struct UiTheme {
    UiColor charcoal;
    UiColor nearBlack;
    UiColor crimson;
    UiColor fireRed;
    UiColor ember;
    UiColor warmGold;
    UiColor offWhite;
    UiColor mutedText;
    UiColor panel;
    UiColor manualPaper;
};

struct UiLayout {
    int designWidth{1280};
    int designHeight{720};
    UiRect safeArea;
    UiRect topIdentity;
    UiRect carouselFocus;
    UiRect carouselArt;
    UiRect storyContextAction;
    UiRect footerPrompt;
    UiRect routeArt;
    UiRect routeTitle;
    UiRect recapVisual;
    UiRect recapCopy;
    UiRect manualNavigation;
    UiRect manualPage;
    UiRect objective;
    UiRect playerHud;
    UiRect opponentHud;
    UiRect hotbar;
    UiRect dialogue;
};

class UiPresentationRegistry {
public:
    UiPresentationRegistry();
    const UiTheme& theme() const { return theme_; }
    const UiLayout& layout() const { return layout_; }
    static UiColor accent(const UiTheme& theme, const std::string& accentId);

private:
    UiTheme theme_;
    UiLayout layout_;
};

} // namespace px
