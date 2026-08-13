#pragma once
#include "content/combat_manual_registry.hpp"
#include "content/menu_registry.hpp"
#include "content/story_recap_registry.hpp"
#include "content/story_route_registry.hpp"
#include "core/input.hpp"
#include "core/save.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace px {

enum class MenuScreen : std::uint8_t {
    Title,
    ModeSelect,
    BattleSelect,
    ExtrasSelect,
    Options,
    AdventureRecords,
    CombatManual,
    CharacterProfiles,
    Credits,
    StoryCharacterSelect,
    StorySoFar,
    StoryComingLater,
    UnlockCelebration
};

enum class MenuEvent : std::uint8_t {
    None, Move, Confirm, Back, Error, TitleConfirm, CarouselTransition, RouteUnlock
};

enum class MenuOutcome : std::uint8_t {
    None, BeginStory, ContinueStory, ReplayChapter, LaunchMode
};

struct MenuSnapshot {
    MenuScreen screen{MenuScreen::Title};
    MenuEvent event{MenuEvent::None};
    MenuOutcome outcome{MenuOutcome::None};
    std::size_t modeIndex{0};
    MenuModeDefinition selectedMode;
    bool storySoFarSelected{false};
    std::vector<StoryRouteDefinition> visibleRoutes;
    std::size_t routeIndex{0};
    StoryRouteDefinition selectedRoute;
    std::size_t routeActionIndex{0};
    std::vector<std::string> routeActions;
    std::size_t recapSectionIndex{0};
    std::size_t recapFrameIndex{0};
    StoryRecapSection recapSection;
    StoryRecapFrame recapFrame;
    std::string unlockRouteId;
    std::string primaryPrompt;
    float transitionProgress{0.0f};
    int transitionDirection{0};

    std::string submenuTitle;
    std::vector<std::string> submenuOptions;
    std::size_t submenuSelection{0};
    std::string submenuDetail;
    std::vector<std::string> informationLines;
    CombatManualPage manualPage;
    std::size_t manualPageIndex{0};
    std::size_t manualPageCount{0};

    bool continueAvailable{false};
    std::string continueCharacter;
    std::string continueArea;
    std::string continueObjective;
    int continueProgressPercent{0};
    float continuePlaytimeSeconds{0.0f};
};

class MenuState {
public:
    MenuState(const MenuRegistry& menus,
              const StoryRouteRegistry& routes,
              const StoryRecapRegistry& recap,
              SaveData& save);

    void handle(Action action);
    void confirm();
    void back();
    void skipStorySoFar();
    void openTitle();
    void openModeSelect();
    void openStoryCharacterSelect();
    void openStorySoFar(std::size_t section = 0, std::size_t frame = 0);
    void selectMode(const std::string& stableId);
    bool showQueuedStoryUnlock();
    void clearOutcome();
    void tick(float deltaSeconds);
    void setReducedMotion(bool enabled) { reducedMotion_ = enabled; }

    MenuSnapshot snapshot() const;
    MenuScreen screen() const { return screen_; }
    MenuOutcome outcome() const { return outcome_; }
    MenuEvent lastEvent() const { return lastEvent_; }

private:
    void moveHorizontal(int direction);
    void moveVertical(int direction);
    void moveList(std::size_t& index, std::size_t count, int direction);
    void toggleOption(int direction);
    std::vector<std::string> routeActions() const;
    std::vector<std::string> visibleRouteIds() const;
    bool rrvvfoStoryStarted() const;
    bool wholeCharacterStoryComplete() const;
    void normalizeRouteIndex();
    void advanceRecap(int direction);
    MenuModeDefinition resolvedMode() const;

    const MenuRegistry& menus_;
    const StoryRouteRegistry& routes_;
    const StoryRecapRegistry& recap_;
    CombatManualRegistry manual_;
    SaveData& save_;
    MenuScreen screen_{MenuScreen::Title};
    MenuScreen returnScreen_{MenuScreen::ModeSelect};
    std::size_t modeIndex_{0};
    std::size_t battleSelection_{0};
    std::size_t extrasSelection_{0};
    std::size_t optionsSelection_{0};
    std::size_t manualPageIndex_{0};
    bool storySoFarSelected_{false};
    std::size_t routeIndex_{0};
    std::size_t routeActionIndex_{0};
    std::size_t recapSectionIndex_{0};
    std::size_t recapFrameIndex_{0};
    std::string comingLaterRouteId_;
    std::string unlockRouteId_;
    MenuOutcome outcome_{MenuOutcome::None};
    MenuEvent lastEvent_{MenuEvent::None};
    float transitionSeconds_{0.0f};
    int transitionDirection_{0};
    bool reducedMotion_{false};
};

} // namespace px
