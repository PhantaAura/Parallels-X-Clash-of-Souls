#include "core/menu_state.hpp"
#include "core/story_unlocks.hpp"
#include <algorithm>

namespace px {

MenuState::MenuState(const MenuRegistry& menus,
                     const StoryRouteRegistry& routes,
                     const StoryRecapRegistry& recap,
                     SaveData& save)
    : menus_(menus), routes_(routes), recap_(recap), save_(save) {
    StoryUnlockSystem::reconstructFromProgress(save_);
    recapSectionIndex_ = std::min(save_.frontend.storySoFarSection,
                                  recap_.sections().empty() ? std::size_t{0} : recap_.sections().size() - 1);
}

void MenuState::clearOutcome() {
    outcome_ = MenuOutcome::None;
    lastEvent_ = MenuEvent::None;
}

void MenuState::tick(float deltaSeconds) {
    if (reducedMotion_) {
        transitionSeconds_ = 0.0f;
        return;
    }
    transitionSeconds_ = std::max(0.0f, transitionSeconds_ - std::max(0.0f, deltaSeconds));
}

void MenuState::handle(Action action) {
    lastEvent_ = MenuEvent::None;
    if (screen_ == MenuScreen::Title) {
        const bool startInput = action != Action::MoveUp && action != Action::MoveDown &&
                                action != Action::MoveLeft && action != Action::MoveRight &&
                                action != Action::Cancel;
        if (startInput) confirm();
        return;
    }
    if (action == Action::MoveLeft) moveHorizontal(-1);
    else if (action == Action::MoveRight) moveHorizontal(1);
    else if (action == Action::MoveUp) moveVertical(-1);
    else if (action == Action::MoveDown) moveVertical(1);
    else if (action == Action::Confirm || action == Action::Interact) confirm();
    else if (action == Action::Cancel || action == Action::Pause) back();
}

void MenuState::openTitle() {
    screen_ = MenuScreen::Title;
    outcome_ = MenuOutcome::None;
    lastEvent_ = MenuEvent::None;
}

void MenuState::openModeSelect() {
    screen_ = MenuScreen::ModeSelect;
    outcome_ = MenuOutcome::None;
    storySoFarSelected_ = false;
}

void MenuState::openStoryCharacterSelect() {
    screen_ = MenuScreen::StoryCharacterSelect;
    outcome_ = MenuOutcome::None;
    routeActionIndex_ = 0;
    normalizeRouteIndex();
}

void MenuState::openStorySoFar(std::size_t section, std::size_t frame) {
    if (recap_.sections().empty()) return;
    screen_ = MenuScreen::StorySoFar;
    outcome_ = MenuOutcome::None;
    recapSectionIndex_ = std::min(section, recap_.sections().size() - 1);
    recapFrameIndex_ = std::min(frame, recap_.sections()[recapSectionIndex_].frames.size() - 1);
    save_.frontend.storySoFarSection = recapSectionIndex_;
}

void MenuState::selectMode(const std::string& stableId) {
    const auto& modes = menus_.modes();
    const auto it = std::find_if(modes.begin(), modes.end(), [&](const MenuModeDefinition& mode) {
        return mode.stableId == stableId;
    });
    if (it == modes.end()) return;
    modeIndex_ = static_cast<std::size_t>(std::distance(modes.begin(), it));
    storySoFarSelected_ = false;
}

void MenuState::confirm() {
    outcome_ = MenuOutcome::None;
    switch (screen_) {
        case MenuScreen::Title:
            screen_ = MenuScreen::ModeSelect;
            transitionSeconds_ = reducedMotion_ ? 0.0f : 0.18f;
            transitionDirection_ = 1;
            lastEvent_ = MenuEvent::TitleConfirm;
            break;
        case MenuScreen::ModeSelect: {
            const auto& mode = menus_.modes()[modeIndex_];
            if (mode.id == MenuModeId::Story) {
                if (storySoFarSelected_) {
                    openStorySoFar(save_.frontend.storySoFarSection, 0);
                } else {
                    openStoryCharacterSelect();
                }
                lastEvent_ = MenuEvent::Confirm;
            } else if (!mode.implemented) {
                lastEvent_ = MenuEvent::Error;
            } else {
                outcome_ = MenuOutcome::LaunchMode;
                lastEvent_ = MenuEvent::Confirm;
            }
            break;
        }
        case MenuScreen::StoryCharacterSelect: {
            const auto visible = visibleRouteIds();
            if (visible.empty()) return;
            const auto& route = routes_.get(visible[routeIndex_]);
            save_.frontend.selectedStoryRoute = route.id;
            if (!route.implemented) {
                comingLaterRouteId_ = route.id;
                screen_ = MenuScreen::StoryComingLater;
                lastEvent_ = MenuEvent::Confirm;
                return;
            }
            const auto actions = routeActions();
            if (routeActionIndex_ >= actions.size()) routeActionIndex_ = 0;
            if (routeActionIndex_ == 1 && wholeChapterReplayAvailable()) outcome_ = MenuOutcome::ReplayChapter;
            else if (routeActionIndex_ == 0)
                outcome_ = rrvvfoStoryStarted() ? MenuOutcome::ContinueStory : MenuOutcome::BeginStory;
            else {
                lastEvent_ = MenuEvent::Error;
                return;
            }
            lastEvent_ = MenuEvent::Confirm;
            break;
        }
        case MenuScreen::StorySoFar:
            advanceRecap(1);
            lastEvent_ = MenuEvent::Confirm;
            break;
        case MenuScreen::StoryComingLater:
            openStoryCharacterSelect();
            lastEvent_ = MenuEvent::Back;
            break;
        case MenuScreen::UnlockCelebration:
            StoryUnlockSystem::acknowledgeQueuedUnlock(save_, unlockRouteId_);
            unlockRouteId_.clear();
            screen_ = returnScreen_;
            lastEvent_ = MenuEvent::Confirm;
            break;
    }
}

void MenuState::back() {
    outcome_ = MenuOutcome::None;
    switch (screen_) {
        case MenuScreen::Title: break;
        case MenuScreen::ModeSelect:
            screen_ = MenuScreen::Title;
            break;
        case MenuScreen::StoryCharacterSelect:
            screen_ = MenuScreen::ModeSelect;
            storySoFarSelected_ = false;
            break;
        case MenuScreen::StorySoFar:
            screen_ = MenuScreen::ModeSelect;
            storySoFarSelected_ = true;
            break;
        case MenuScreen::StoryComingLater:
            screen_ = MenuScreen::StoryCharacterSelect;
            break;
        case MenuScreen::UnlockCelebration:
            StoryUnlockSystem::acknowledgeQueuedUnlock(save_, unlockRouteId_);
            unlockRouteId_.clear();
            screen_ = returnScreen_;
            break;
    }
    lastEvent_ = MenuEvent::Back;
}

void MenuState::skipStorySoFar() {
    if (screen_ != MenuScreen::StorySoFar) return;
    screen_ = MenuScreen::ModeSelect;
    storySoFarSelected_ = true;
    lastEvent_ = MenuEvent::Back;
}

bool MenuState::showQueuedStoryUnlock() {
    const auto next = StoryUnlockSystem::nextQueuedUnlock(save_);
    if (next.empty()) return false;
    returnScreen_ = screen_ == MenuScreen::Title ? MenuScreen::ModeSelect : screen_;
    unlockRouteId_ = next;
    screen_ = MenuScreen::UnlockCelebration;
    lastEvent_ = MenuEvent::RouteUnlock;
    return true;
}

void MenuState::moveHorizontal(int direction) {
    outcome_ = MenuOutcome::None;
    if (screen_ == MenuScreen::ModeSelect) {
        const auto count = menus_.modes().size();
        modeIndex_ = (modeIndex_ + count + (direction < 0 ? count - 1 : 1)) % count;
        transitionSeconds_ = reducedMotion_ ? 0.0f : 0.16f;
        transitionDirection_ = direction < 0 ? -1 : 1;
        storySoFarSelected_ = false;
        lastEvent_ = MenuEvent::CarouselTransition;
    } else if (screen_ == MenuScreen::StoryCharacterSelect) {
        const auto count = visibleRouteIds().size();
        if (!count) return;
        routeIndex_ = (routeIndex_ + count + (direction < 0 ? count - 1 : 1)) % count;
        transitionSeconds_ = reducedMotion_ ? 0.0f : 0.16f;
        transitionDirection_ = direction < 0 ? -1 : 1;
        routeActionIndex_ = 0;
        save_.frontend.selectedStoryRoute = visibleRouteIds()[routeIndex_];
        lastEvent_ = MenuEvent::CarouselTransition;
    } else if (screen_ == MenuScreen::StorySoFar) {
        advanceRecap(direction);
        lastEvent_ = MenuEvent::Move;
    }
}

void MenuState::moveVertical(int direction) {
    outcome_ = MenuOutcome::None;
    if (screen_ == MenuScreen::ModeSelect && menus_.modes()[modeIndex_].id == MenuModeId::Story) {
        storySoFarSelected_ = !storySoFarSelected_;
        lastEvent_ = MenuEvent::Move;
    } else if (screen_ == MenuScreen::StoryCharacterSelect) {
        const auto actions = routeActions();
        if (actions.empty()) return;
        routeActionIndex_ = (routeActionIndex_ + actions.size() +
                             (direction < 0 ? actions.size() - 1 : 1)) % actions.size();
        lastEvent_ = MenuEvent::Move;
    } else if (screen_ == MenuScreen::StorySoFar && !recap_.sections().empty()) {
        const auto count = recap_.sections().size();
        recapSectionIndex_ = (recapSectionIndex_ + count +
                              (direction < 0 ? count - 1 : 1)) % count;
        recapFrameIndex_ = 0;
        save_.frontend.storySoFarSection = recapSectionIndex_;
        lastEvent_ = MenuEvent::Move;
    }
}

void MenuState::advanceRecap(int direction) {
    if (recap_.sections().empty()) return;
    const auto& section = recap_.sections()[recapSectionIndex_];
    if (direction < 0) {
        if (recapFrameIndex_ > 0) {
            --recapFrameIndex_;
        } else if (recapSectionIndex_ > 0) {
            --recapSectionIndex_;
            recapFrameIndex_ = recap_.sections()[recapSectionIndex_].frames.size() - 1;
        }
    } else {
        if (recapFrameIndex_ + 1 < section.frames.size()) {
            ++recapFrameIndex_;
        } else if (recapSectionIndex_ + 1 < recap_.sections().size()) {
            ++recapSectionIndex_;
            recapFrameIndex_ = 0;
        } else {
            screen_ = MenuScreen::ModeSelect;
            storySoFarSelected_ = true;
        }
    }
    save_.frontend.storySoFarSection = recapSectionIndex_;
}

std::vector<std::string> MenuState::visibleRouteIds() const {
    return StoryUnlockSystem::visibleRoutes(save_);
}

void MenuState::normalizeRouteIndex() {
    const auto visible = visibleRouteIds();
    const auto selected = std::find(visible.begin(), visible.end(), save_.frontend.selectedStoryRoute);
    routeIndex_ = selected == visible.end() ? 0 : static_cast<std::size_t>(std::distance(visible.begin(), selected));
    if (!visible.empty()) save_.frontend.selectedStoryRoute = visible[routeIndex_];
}

bool MenuState::rrvvfoStoryStarted() const {
    return !save_.story.chapterId.empty();
}

bool MenuState::wholeChapterReplayAvailable() const {
    return rrvvfoStoryStarted();
}

std::vector<std::string> MenuState::routeActions() const {
    const auto visible = visibleRouteIds();
    if (visible.empty()) return {};
    const auto& route = routes_.get(visible[std::min(routeIndex_, visible.size() - 1)]);
    if (!route.implemented) return {"STORY COMING LATER"};
    std::vector<std::string> actions{rrvvfoStoryStarted() ? "CONTINUE STORY" : "BEGIN STORY"};
    if (wholeChapterReplayAvailable()) actions.push_back("REPLAY CHAPTER");
    return actions;
}

MenuSnapshot MenuState::snapshot() const {
    MenuSnapshot result;
    result.screen = screen_;
    result.event = lastEvent_;
    result.outcome = outcome_;
    result.modeIndex = modeIndex_;
    result.selectedMode = menus_.modes()[modeIndex_];
    result.storySoFarSelected = storySoFarSelected_;
    result.routeIndex = routeIndex_;
    result.routeActionIndex = routeActionIndex_;
    result.routeActions = routeActions();
    for (const auto& id : visibleRouteIds()) result.visibleRoutes.push_back(routes_.get(id));
    if (!result.visibleRoutes.empty())
        result.selectedRoute = result.visibleRoutes[std::min(routeIndex_, result.visibleRoutes.size() - 1)];
    result.recapSectionIndex = recapSectionIndex_;
    result.recapFrameIndex = recapFrameIndex_;
    if (!recap_.sections().empty()) {
        result.recapSection = recap_.sections()[recapSectionIndex_];
        result.recapFrame = result.recapSection.frames[recapFrameIndex_];
    }
    result.unlockRouteId = unlockRouteId_;
    result.transitionProgress = std::min(1.0f, transitionSeconds_ / 0.18f);
    result.transitionDirection = transitionDirection_;
    if (screen_ == MenuScreen::Title) result.primaryPrompt = "PRESS ANY BUTTON";
    else if (screen_ == MenuScreen::ModeSelect)
        result.primaryPrompt = result.selectedMode.implemented ? "CONFIRM" : "COMING LATER";
    else if (screen_ == MenuScreen::StoryCharacterSelect && !result.routeActions.empty())
        result.primaryPrompt = result.routeActions[result.routeActionIndex];
    else if (screen_ == MenuScreen::StorySoFar) result.primaryPrompt = "PREVIOUS  •  NEXT  •  SKIP  •  EXIT";
    else if (screen_ == MenuScreen::StoryComingLater) result.primaryPrompt = "STORY COMING LATER";
    return result;
}

} // namespace px
