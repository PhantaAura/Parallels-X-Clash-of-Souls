#include "core/menu_state.hpp"
#include "core/story_unlocks.hpp"
#include <algorithm>

namespace px {
namespace {

const std::vector<std::string>& battleOptions() {
    static const std::vector<std::string> values{"CPU FIGHT", "LOCAL VS • FUTURE", "ONLINE • FUTURE"};
    return values;
}

const std::vector<std::string>& extrasOptions() {
    static const std::vector<std::string> values{
        "ADVENTURE RECORDS", "COMBAT MANUAL", "STORY SO FAR", "CHARACTER PROFILES / GALLERY", "CREDITS"
    };
    return values;
}

std::string onOff(bool value) { return value ? "ON" : "OFF"; }

std::string percent(float value) {
    return std::to_string(static_cast<int>(value * 100.0f + .5f)) + "%";
}

} // namespace

MenuState::MenuState(const MenuRegistry& menus,
                     const StoryRouteRegistry& routes,
                     const StoryRecapRegistry& recap,
                     SaveData& save)
    : menus_(menus), routes_(routes), recap_(recap), save_(save) {
    StoryUnlockSystem::reconstructFromProgress(save_);
    recapSectionIndex_ = std::min(save_.frontend.storySoFarSection,
        recap_.sections().empty() ? std::size_t{0} : recap_.sections().size() - 1);
    battleSelection_ = std::min(save_.frontend.lastBattleSelection, battleOptions().size() - 1);
    extrasSelection_ = std::min(save_.frontend.lastExtrasSelection, extrasOptions().size() - 1);
    optionsSelection_ = std::min<std::size_t>(save_.frontend.lastOptionsSelection, 15);
    // Continue is deliberately dominant when a valid Story exists. A fresh
    // file lands on Story instead of advertising an unusable command.
    selectMode(rrvvfoStoryStarted() ? "continue" :
               (save_.frontend.lastMenuMode.empty() || save_.frontend.lastMenuMode == "continue"
                    ? "story" : save_.frontend.lastMenuMode));
}

void MenuState::clearOutcome() { outcome_ = MenuOutcome::None; lastEvent_ = MenuEvent::None; }

void MenuState::tick(float deltaSeconds) {
    if (reducedMotion_) { transitionSeconds_ = 0.0f; return; }
    transitionSeconds_ = std::max(0.0f, transitionSeconds_ - std::max(0.0f, deltaSeconds));
}

void MenuState::handle(Action action) {
    lastEvent_ = MenuEvent::None;
    if (screen_ == MenuScreen::Title) {
        const bool startInput = action != Action::MoveUp && action != Action::MoveDown &&
            action != Action::MoveLeft && action != Action::MoveRight && action != Action::Cancel;
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

void MenuState::openTitle() { screen_ = MenuScreen::Title; outcome_ = MenuOutcome::None; lastEvent_ = MenuEvent::None; }
void MenuState::openModeSelect() { screen_ = MenuScreen::ModeSelect; outcome_ = MenuOutcome::None; storySoFarSelected_ = false; }

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
    modeIndex_ = it == modes.end() ? std::min<std::size_t>(1, modes.size() - 1) :
        static_cast<std::size_t>(std::distance(modes.begin(), it));
    storySoFarSelected_ = false;
}

void MenuState::confirm() {
    outcome_ = MenuOutcome::None;
    switch (screen_) {
        case MenuScreen::Title:
            screen_ = MenuScreen::ModeSelect;
            transitionSeconds_ = reducedMotion_ ? 0.0f : .18f;
            transitionDirection_ = 1;
            lastEvent_ = MenuEvent::TitleConfirm;
            break;
        case MenuScreen::ModeSelect: {
            const auto mode = resolvedMode();
            save_.frontend.lastMenuMode = mode.stableId;
            if (mode.id == MenuModeId::Continue) {
                if (!rrvvfoStoryStarted()) { lastEvent_ = MenuEvent::Error; break; }
                outcome_ = MenuOutcome::ContinueStory;
            } else if (mode.id == MenuModeId::Story) openStoryCharacterSelect();
            else if (mode.id == MenuModeId::ArenaBattle) screen_ = MenuScreen::BattleSelect;
            else if (mode.id == MenuModeId::Extras) screen_ = MenuScreen::ExtrasSelect;
            else if (mode.id == MenuModeId::Options) screen_ = MenuScreen::Options;
            else if (mode.id == MenuModeId::Training) outcome_ = MenuOutcome::LaunchMode;
            else lastEvent_ = MenuEvent::Error;
            if (lastEvent_ != MenuEvent::Error) lastEvent_ = MenuEvent::Confirm;
            break;
        }
        case MenuScreen::BattleSelect:
            if (battleSelection_ == 0) outcome_ = MenuOutcome::LaunchMode;
            else lastEvent_ = MenuEvent::Error;
            if (battleSelection_ == 0) lastEvent_ = MenuEvent::Confirm;
            break;
        case MenuScreen::ExtrasSelect:
            if (extrasSelection_ == 0) screen_ = MenuScreen::AdventureRecords;
            else if (extrasSelection_ == 1) screen_ = MenuScreen::CombatManual;
            else if (extrasSelection_ == 2) { returnScreen_ = MenuScreen::ExtrasSelect; openStorySoFar(save_.frontend.storySoFarSection, 0); }
            else if (extrasSelection_ == 3) screen_ = MenuScreen::CharacterProfiles;
            else screen_ = MenuScreen::Credits;
            lastEvent_ = MenuEvent::Confirm;
            break;
        case MenuScreen::Options: toggleOption(1); lastEvent_ = MenuEvent::Confirm; break;
        case MenuScreen::CombatManual:
            manualPageIndex_ = (manualPageIndex_ + 1) % manual_.pages().size();
            lastEvent_ = MenuEvent::Move;
            break;
        case MenuScreen::AdventureRecords:
        case MenuScreen::CharacterProfiles:
        case MenuScreen::Credits:
            back();
            break;
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
            if (routeActionIndex_ == 1 && wholeCharacterStoryComplete()) outcome_ = MenuOutcome::ReplayChapter;
            else if (routeActionIndex_ == 0) outcome_ = rrvvfoStoryStarted() ? MenuOutcome::ContinueStory : MenuOutcome::BeginStory;
            else { lastEvent_ = MenuEvent::Error; return; }
            lastEvent_ = MenuEvent::Confirm;
            break;
        }
        case MenuScreen::StorySoFar: advanceRecap(1); lastEvent_ = MenuEvent::Confirm; break;
        case MenuScreen::StoryComingLater: openStoryCharacterSelect(); lastEvent_ = MenuEvent::Back; break;
        case MenuScreen::UnlockCelebration:
            StoryUnlockSystem::acknowledgeQueuedUnlock(save_, unlockRouteId_);
            unlockRouteId_.clear(); screen_ = returnScreen_; lastEvent_ = MenuEvent::Confirm; break;
    }
}

void MenuState::back() {
    outcome_ = MenuOutcome::None;
    switch (screen_) {
        case MenuScreen::Title: break;
        case MenuScreen::ModeSelect: screen_ = MenuScreen::Title; break;
        case MenuScreen::BattleSelect:
        case MenuScreen::ExtrasSelect:
        case MenuScreen::Options: screen_ = MenuScreen::ModeSelect; break;
        case MenuScreen::AdventureRecords:
        case MenuScreen::CombatManual:
        case MenuScreen::CharacterProfiles:
        case MenuScreen::Credits: screen_ = MenuScreen::ExtrasSelect; break;
        case MenuScreen::StoryCharacterSelect: screen_ = MenuScreen::ModeSelect; break;
        case MenuScreen::StorySoFar: screen_ = returnScreen_; break;
        case MenuScreen::StoryComingLater: screen_ = MenuScreen::StoryCharacterSelect; break;
        case MenuScreen::UnlockCelebration:
            StoryUnlockSystem::acknowledgeQueuedUnlock(save_, unlockRouteId_);
            unlockRouteId_.clear(); screen_ = returnScreen_; break;
    }
    lastEvent_ = MenuEvent::Back;
}

void MenuState::skipStorySoFar() { if (screen_ == MenuScreen::StorySoFar) { screen_ = returnScreen_; lastEvent_ = MenuEvent::Back; } }

bool MenuState::showQueuedStoryUnlock() {
    const auto next = StoryUnlockSystem::nextQueuedUnlock(save_);
    if (next.empty()) return false;
    returnScreen_ = screen_ == MenuScreen::Title ? MenuScreen::ModeSelect : screen_;
    unlockRouteId_ = next; screen_ = MenuScreen::UnlockCelebration; lastEvent_ = MenuEvent::RouteUnlock;
    return true;
}

void MenuState::moveList(std::size_t& index, std::size_t count, int direction) {
    if (!count) return;
    index = (index + count + (direction < 0 ? count - 1 : 1)) % count;
    lastEvent_ = MenuEvent::Move;
}

void MenuState::moveHorizontal(int direction) {
    outcome_ = MenuOutcome::None;
    if (screen_ == MenuScreen::ModeSelect) {
        moveList(modeIndex_, menus_.modes().size(), direction);
        transitionSeconds_ = reducedMotion_ ? 0.0f : .16f;
        transitionDirection_ = direction < 0 ? -1 : 1;
        save_.frontend.lastMenuMode = menus_.modes()[modeIndex_].stableId;
        lastEvent_ = MenuEvent::CarouselTransition;
    } else if (screen_ == MenuScreen::StoryCharacterSelect) {
        moveList(routeIndex_, visibleRouteIds().size(), direction);
        routeActionIndex_ = 0;
        if (!visibleRouteIds().empty()) save_.frontend.selectedStoryRoute = visibleRouteIds()[routeIndex_];
        transitionSeconds_ = reducedMotion_ ? 0.0f : .16f;
        transitionDirection_ = direction < 0 ? -1 : 1;
        lastEvent_ = MenuEvent::CarouselTransition;
    } else if (screen_ == MenuScreen::StorySoFar) advanceRecap(direction);
    else if (screen_ == MenuScreen::CombatManual) moveList(manualPageIndex_, manual_.pages().size(), direction);
    else if (screen_ == MenuScreen::Options) toggleOption(direction);
}

void MenuState::moveVertical(int direction) {
    outcome_ = MenuOutcome::None;
    if (screen_ == MenuScreen::BattleSelect) {
        moveList(battleSelection_, battleOptions().size(), direction);
        save_.frontend.lastBattleSelection = battleSelection_;
    } else if (screen_ == MenuScreen::ExtrasSelect) {
        moveList(extrasSelection_, extrasOptions().size(), direction);
        save_.frontend.lastExtrasSelection = extrasSelection_;
    } else if (screen_ == MenuScreen::Options) {
        moveList(optionsSelection_, 16, direction);
        save_.frontend.lastOptionsSelection = optionsSelection_;
    } else if (screen_ == MenuScreen::StoryCharacterSelect) moveList(routeActionIndex_, routeActions().size(), direction);
    else if (screen_ == MenuScreen::StorySoFar && !recap_.sections().empty()) {
        moveList(recapSectionIndex_, recap_.sections().size(), direction);
        recapFrameIndex_ = 0; save_.frontend.storySoFarSection = recapSectionIndex_;
    }
}

void MenuState::toggleOption(int direction) {
    const bool forward = direction >= 0;
    switch (optionsSelection_) {
        case 0: save_.qol.holdToAdvanceDialogue = !save_.qol.holdToAdvanceDialogue; break;
        case 1: save_.qol.dialogueAutoAdvance = !save_.qol.dialogueAutoAdvance; break;
        case 2: save_.qol.dialogueSpeed = save_.qol.dialogueSpeed == "slow" ? "normal" : save_.qol.dialogueSpeed == "normal" ? "fast" : "slow"; break;
        case 3: save_.qol.firstTimeHints = !save_.qol.firstTimeHints; break;
        case 4: save_.qol.reducedMotion = !save_.qol.reducedMotion; reducedMotion_ = save_.qol.reducedMotion; break;
        case 5: save_.qol.reducedCameraShake = !save_.qol.reducedCameraShake; break;
        case 6: save_.qol.reducedFlashes = !save_.qol.reducedFlashes; break;
        case 7: save_.qol.highContrastHud = !save_.qol.highContrastHud; break;
        case 8: save_.qol.cameraSensitivity = save_.qol.cameraSensitivity < .9f ? 1.0f : save_.qol.cameraSensitivity < 1.1f ? 1.25f : .75f; break;
        case 9: save_.qol.invertCameraX = !save_.qol.invertCameraX; break;
        case 10: save_.qol.invertCameraY = !save_.qol.invertCameraY; break;
        case 11: save_.qol.gentleCameraRecenter = !save_.qol.gentleCameraRecenter; break;
        case 12: save_.qol.hudScale = save_.qol.hudScale < .95f ? 1.0f : save_.qol.hudScale < 1.05f ? 1.10f : .90f; break;
        case 13: save_.qol.dialogueScale = save_.qol.dialogueScale < 1.08f ? 1.15f : save_.qol.dialogueScale < 1.22f ? 1.30f : 1.0f; break;
        case 14: save_.qol.combatMessages = save_.qol.combatMessages == "full" ? "minimal" : save_.qol.combatMessages == "minimal" ? "off" : "full"; break;
        case 15: save_.qol.objectiveDisplay = save_.qol.objectiveDisplay == "full" ? "minimal" : save_.qol.objectiveDisplay == "minimal" ? "off" : "full"; break;
        default: break;
    }
    (void)forward;
    lastEvent_ = MenuEvent::Move;
}

void MenuState::advanceRecap(int direction) {
    if (recap_.sections().empty()) return;
    const auto& section = recap_.sections()[recapSectionIndex_];
    if (direction < 0) {
        if (recapFrameIndex_ > 0) --recapFrameIndex_;
        else if (recapSectionIndex_ > 0) { --recapSectionIndex_; recapFrameIndex_ = recap_.sections()[recapSectionIndex_].frames.size() - 1; }
    } else if (recapFrameIndex_ + 1 < section.frames.size()) ++recapFrameIndex_;
    else if (recapSectionIndex_ + 1 < recap_.sections().size()) { ++recapSectionIndex_; recapFrameIndex_ = 0; }
    else screen_ = returnScreen_;
    save_.frontend.storySoFarSection = recapSectionIndex_;
}

std::vector<std::string> MenuState::visibleRouteIds() const { return StoryUnlockSystem::visibleRoutes(save_); }

void MenuState::normalizeRouteIndex() {
    const auto visible = visibleRouteIds();
    const auto selected = std::find(visible.begin(), visible.end(), save_.frontend.selectedStoryRoute);
    routeIndex_ = selected == visible.end() ? 0 : static_cast<std::size_t>(std::distance(visible.begin(), selected));
    if (!visible.empty()) save_.frontend.selectedStoryRoute = visible[routeIndex_];
}

bool MenuState::rrvvfoStoryStarted() const { return !save_.story.chapterId.empty(); }

bool MenuState::wholeCharacterStoryComplete() const {
    const auto visible = visibleRouteIds();
    if (visible.empty()) return false;
    const std::string flag = visible[std::min(routeIndex_, visible.size() - 1)] + "_story_complete";
    return std::find(save_.story.flags.begin(), save_.story.flags.end(), flag) != save_.story.flags.end();
}

std::vector<std::string> MenuState::routeActions() const {
    const auto visible = visibleRouteIds();
    if (visible.empty()) return {};
    const auto& route = routes_.get(visible[std::min(routeIndex_, visible.size() - 1)]);
    if (!route.implemented) return {"STORY COMING LATER"};
    std::vector<std::string> actions{rrvvfoStoryStarted() ? "CONTINUE STORY" : "BEGIN STORY"};
    if (wholeCharacterStoryComplete()) actions.push_back("REPLAY STORY");
    return actions;
}

MenuModeDefinition MenuState::resolvedMode() const {
    auto mode = menus_.modes()[modeIndex_];
    if (mode.id == MenuModeId::Continue) {
        mode.implemented = rrvvfoStoryStarted();
        mode.status = mode.implemented ? "READY" : "NO STORY SAVE";
        if (mode.implemented && !save_.frontend.currentArea.empty()) mode.description =
            "Rrvvfo • " + save_.frontend.currentArea + " • " + save_.frontend.currentObjective;
    }
    return mode;
}

MenuSnapshot MenuState::snapshot() const {
    MenuSnapshot result;
    result.screen = screen_; result.event = lastEvent_; result.outcome = outcome_;
    result.modeIndex = modeIndex_; result.selectedMode = resolvedMode();
    result.routeIndex = routeIndex_; result.routeActionIndex = routeActionIndex_; result.routeActions = routeActions();
    for (const auto& id : visibleRouteIds()) result.visibleRoutes.push_back(routes_.get(id));
    if (!result.visibleRoutes.empty()) result.selectedRoute = result.visibleRoutes[std::min(routeIndex_, result.visibleRoutes.size() - 1)];
    result.recapSectionIndex = recapSectionIndex_; result.recapFrameIndex = recapFrameIndex_;
    if (!recap_.sections().empty()) {
        result.recapSection = recap_.sections()[recapSectionIndex_];
        result.recapFrame = result.recapSection.frames[recapFrameIndex_];
    }
    result.unlockRouteId = unlockRouteId_;
    result.transitionProgress = std::min(1.0f, transitionSeconds_ / .18f);
    result.transitionDirection = transitionDirection_;
    result.continueAvailable = rrvvfoStoryStarted();
    result.continueCharacter = "Rrvvfo";
    result.continueArea = save_.frontend.currentArea.empty() ? "Latest checkpoint" : save_.frontend.currentArea;
    result.continueObjective = save_.frontend.currentObjective.empty() ? "Continue Story" : save_.frontend.currentObjective;
    result.continueProgressPercent = save_.frontend.storyProgressPercent;
    result.continuePlaytimeSeconds = save_.frontend.playtimeSeconds;

    if (screen_ == MenuScreen::Title) result.primaryPrompt = "PRESS ANY BUTTON";
    else if (screen_ == MenuScreen::ModeSelect) result.primaryPrompt = result.selectedMode.implemented ? "CONFIRM" : "NO STORY SAVE";
    else if (screen_ == MenuScreen::BattleSelect) {
        result.submenuTitle = "BATTLE"; result.submenuOptions = battleOptions(); result.submenuSelection = battleSelection_;
        result.submenuDetail = battleSelection_ == 0 ? "Playable Rrvvfo vs CPU fight using shared Story combat." : "Reserved inside Battle; not selectable in this build.";
        result.primaryPrompt = battleSelection_ == 0 ? "START FIGHT" : "FUTURE MODE";
    } else if (screen_ == MenuScreen::ExtrasSelect) {
        result.submenuTitle = "EXTRAS"; result.submenuOptions = extrasOptions(); result.submenuSelection = extrasSelection_;
        result.submenuDetail = "Records and reference material never modify Story progression."; result.primaryPrompt = "OPEN";
    } else if (screen_ == MenuScreen::Options) {
        result.submenuTitle = "OPTIONS"; result.submenuSelection = optionsSelection_;
        result.submenuOptions = {
            "HOLD TO ADVANCE • " + onOff(save_.qol.holdToAdvanceDialogue),
            "AUTO-ADVANCE • " + onOff(save_.qol.dialogueAutoAdvance),
            "DIALOGUE SPEED • " + save_.qol.dialogueSpeed,
            "CONTEXTUAL HINTS • " + onOff(save_.qol.firstTimeHints),
            "REDUCED MOTION • " + onOff(save_.qol.reducedMotion),
            "REDUCED CAMERA SHAKE • " + onOff(save_.qol.reducedCameraShake),
            "REDUCED FLASHES • " + onOff(save_.qol.reducedFlashes),
            "HIGH CONTRAST • " + onOff(save_.qol.highContrastHud),
            "CAMERA SENSITIVITY • " + percent(save_.qol.cameraSensitivity),
            "INVERT CAMERA X • " + onOff(save_.qol.invertCameraX),
            "INVERT CAMERA Y • " + onOff(save_.qol.invertCameraY),
            "GENTLE RECENTER • " + onOff(save_.qol.gentleCameraRecenter),
            "HUD SCALE • " + percent(save_.qol.hudScale),
            "DIALOGUE SCALE • " + percent(save_.qol.dialogueScale),
            "COMBAT MESSAGES • " + save_.qol.combatMessages,
            "OBJECTIVE DISPLAY • " + save_.qol.objectiveDisplay
        };
        result.submenuDetail = "CONFIRM / LEFT / RIGHT changes a setting. All platforms use the same values.";
        result.primaryPrompt = "CHANGE";
    } else if (screen_ == MenuScreen::AdventureRecords) {
        result.submenuTitle = "ADVENTURE RECORDS";
        result.informationLines = {
            "WINS • " + std::to_string(save_.records.wins), "LOSSES • " + std::to_string(save_.records.losses),
            "BEST COMBO • " + std::to_string(save_.records.bestCombo),
            "PERFECT BLOCKS • " + std::to_string(save_.records.perfectBlocks),
            "GUARD BREAKS • " + std::to_string(save_.records.guardBreaks),
            "PURSUIT FINISHERS • " + std::to_string(save_.records.pursuitFinishers),
            "BEST RANK • " + std::string(save_.records.hasRank ? RpgProgressSystem::rankLabel(save_.records.bestRank) : "—"),
            "ENERGY BEAMS • " + std::to_string(save_.records.energyBeamUses),
            "OBJECT SWAPS • " + std::to_string(save_.records.objectSwaps)
        };
        result.primaryPrompt = "BACK";
    } else if (screen_ == MenuScreen::CombatManual) {
        result.submenuTitle = "COMBAT MANUAL"; result.manualPageIndex = manualPageIndex_;
        result.manualPageCount = manual_.pages().size(); result.manualPage = manual_.pages()[manualPageIndex_];
        result.primaryPrompt = "PREVIOUS • NEXT • BACK";
    } else if (screen_ == MenuScreen::CharacterProfiles) {
        result.submenuTitle = "CHARACTER PROFILES / GALLERY";
        result.informationLines = {"RRVVFO • FIRE NINJA HERO • PROFILE AVAILABLE", "SAGE • MENTOR • PROFILE AVAILABLE"};
        for (const auto& profile : save_.rpg.profileUnlocks) result.informationLines.push_back(profile + " • UNLOCKED");
        if (save_.rpg.profileUnlocks.empty()) result.informationLines.push_back("Complete side stories to add gallery moments and profiles.");
        result.primaryPrompt = "BACK";
    } else if (screen_ == MenuScreen::Credits) {
        result.submenuTitle = "CREDITS";
        result.informationLines = {"PARALLELS X: CLASH OF SOULS 3.0R", "Created by PhantaAura / emeraldhunter",
            "Legacy browser game: story, personality, world and gameplay authority", "Native shared runtime: Mac • Linux • Old 3DS",
            "Special thanks to every player testing the road."};
        result.primaryPrompt = "BACK";
    } else if (screen_ == MenuScreen::StoryCharacterSelect && !result.routeActions.empty())
        result.primaryPrompt = result.routeActions[std::min(routeActionIndex_, result.routeActions.size() - 1)];
    else if (screen_ == MenuScreen::StorySoFar) result.primaryPrompt = "PREVIOUS • NEXT • SKIP • EXIT";
    else if (screen_ == MenuScreen::StoryComingLater) result.primaryPrompt = "STORY COMING LATER";
    return result;
}

} // namespace px
