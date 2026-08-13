#include "content/combat_manual_registry.hpp"
#include "content/chapter_registry.hpp"
#include "content/faction_registry.hpp"
#include "content/menu_registry.hpp"
#include "content/story_recap_registry.hpp"
#include "content/story_route_registry.hpp"
#include "core/menu_state.hpp"
#include "core/save.hpp"
#include "core/story_unlocks.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool contains(const std::vector<std::string>& values, const std::string& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

px::MenuState makeMenu(const px::MenuRegistry& menus,
                       const px::StoryRouteRegistry& routes,
                       const px::StoryRecapRegistry& recap,
                       px::SaveData& save) {
    return px::MenuState(menus, routes, recap, save);
}

} // namespace

int main() {
    px::MenuRegistry menus;
    px::ChapterRegistry chapters;
    px::StoryRouteRegistry routes;
    px::StoryRecapRegistry recap;
    px::CombatManualRegistry manual;
    px::FactionRegistry factions;

    assert(chapters.ids().size() == 2);
    assert(chapters.has("rrvvfo_ch1"));
    assert(chapters.has("rrvvfo_ch2"));
    assert(!chapters.has("bark_ch1"));
    assert(!chapters.has("wade_ch1"));
    assert(!chapters.has("virek_ch1"));

    const std::vector<std::string> exactModeOrder{
        "continue", "story", "battle", "training", "extras", "options"
    };
    assert(menus.modes().size() == exactModeOrder.size());
    for (std::size_t i = 0; i < exactModeOrder.size(); ++i)
        assert(menus.modes()[i].stableId == exactModeOrder[i]);
    assert(std::none_of(menus.modes().begin(), menus.modes().end(), [](const auto& mode) {
        return mode.stableId == "story_so_far";
    }));
    assert(menus.get("battle").implemented && menus.get("battle").label == "BATTLE");
    assert(menus.get("options").implemented && menus.get("extras").implemented);
    assert(menus.get("training").implemented && menus.get("training").status == "PLAYABLE");

    px::SaveData fresh;
    auto menu = makeMenu(menus, routes, recap, fresh);
    assert(menu.screen() == px::MenuScreen::Title);
    menu.handle(px::Action::MoveLeft);
    assert(menu.screen() == px::MenuScreen::Title);
    menu.handle(px::Action::Cancel);
    assert(menu.screen() == px::MenuScreen::Title);
    menu.handle(px::Action::Confirm);
    assert(menu.screen() == px::MenuScreen::ModeSelect);
    assert(menu.lastEvent() == px::MenuEvent::TitleConfirm);
    assert(menu.snapshot().transitionProgress > 0.0f);
    menu.tick(1.0f);
    assert(menu.snapshot().transitionProgress == 0.0f);
    menu.handle(px::Action::MoveLeft);
    assert(menu.snapshot().selectedMode.stableId == "continue");
    assert(menu.lastEvent() == px::MenuEvent::CarouselTransition);
    assert(menu.snapshot().transitionDirection == -1);
    menu.handle(px::Action::MoveRight);
    assert(menu.snapshot().selectedMode.stableId == "story");

    px::SaveData reducedMotionSave;
    auto reducedMotionMenu = makeMenu(menus, routes, recap, reducedMotionSave);
    reducedMotionMenu.setReducedMotion(true);
    reducedMotionMenu.handle(px::Action::Confirm);
    reducedMotionMenu.handle(px::Action::MoveRight);
    assert(reducedMotionMenu.snapshot().selectedMode.stableId == "battle");
    assert(reducedMotionMenu.snapshot().transitionProgress == 0.0f);

    menu.selectMode("extras");
    menu.handle(px::Action::Confirm);
    assert(menu.screen() == px::MenuScreen::ExtrasSelect);
    menu.handle(px::Action::MoveDown);
    menu.handle(px::Action::MoveDown);
    menu.handle(px::Action::Confirm);
    assert(menu.screen() == px::MenuScreen::StorySoFar);
    const auto storyBeforeRecap = fresh.story;
    menu.handle(px::Action::MoveRight);
    assert(menu.snapshot().recapFrameIndex == 1);
    menu.handle(px::Action::MoveLeft);
    assert(menu.snapshot().recapFrameIndex == 0);
    menu.handle(px::Action::MoveDown);
    assert(menu.snapshot().recapSectionIndex == 1);
    menu.skipStorySoFar();
    assert(menu.screen() == px::MenuScreen::ExtrasSelect);
    assert(fresh.story.chapterId == storyBeforeRecap.chapterId);
    assert(fresh.story.sceneIndex == storyBeforeRecap.sceneIndex);
    assert(fresh.story.flags == storyBeforeRecap.flags);

    menu.back();
    menu.selectMode("options");
    menu.confirm();
    assert(menu.screen() == px::MenuScreen::Options);
    const bool oldAuto = fresh.qol.dialogueAutoAdvance;
    menu.handle(px::Action::MoveDown);
    menu.confirm();
    assert(fresh.qol.dialogueAutoAdvance != oldAuto);

    px::SaveData directModeSave;
    auto directModeMenu = makeMenu(menus, routes, recap, directModeSave);
    directModeMenu.openModeSelect();
    directModeMenu.selectMode("battle");
    assert(directModeMenu.snapshot().primaryPrompt == "CONFIRM");
    directModeMenu.confirm();
    assert(directModeMenu.screen() == px::MenuScreen::BattleSelect);
    directModeMenu.confirm();
    assert(directModeMenu.outcome() == px::MenuOutcome::LaunchMode);
    directModeMenu.clearOutcome();
    directModeMenu.openModeSelect();
    directModeMenu.selectMode("training");
    directModeMenu.confirm();
    assert(directModeMenu.outcome() == px::MenuOutcome::LaunchMode);

    menu.openModeSelect();
    menu.selectMode("story");
    menu.confirm();
    auto character = menu.snapshot();
    assert(menu.screen() == px::MenuScreen::StoryCharacterSelect);
    assert(character.visibleRoutes.size() == 1);
    assert(character.selectedRoute.id == "rrvvfo");
    assert(character.primaryPrompt == "BEGIN STORY");
    menu.confirm();
    assert(menu.outcome() == px::MenuOutcome::BeginStory);

    px::SaveData discovered;
    assert(px::StoryUnlockSystem::discover(discovered, px::StoryDiscoveryEvent::BarkReunion));
    assert(px::StoryUnlockSystem::discover(discovered, px::StoryDiscoveryEvent::WadeReunion));
    discovered.story.flags.push_back("rrvvfo_chapter_9_complete");
    px::StoryUnlockSystem::reconstructFromProgress(discovered);
    assert(!px::StoryUnlockSystem::isDiscovered(discovered, "virek"));
    assert(px::StoryUnlockSystem::discover(discovered, px::StoryDiscoveryEvent::VirekEmeraldMissingNoticed));
    assert(px::StoryUnlockSystem::visibleRoutes(discovered) ==
           std::vector<std::string>({"rrvvfo", "bark", "wade", "virek"}));

    const auto encoded = px::SaveCodec::serialize(discovered);
    const auto decoded = px::SaveCodec::deserialize(encoded);
    assert(contains(decoded.frontend.discoveredStoryRoutes, "bark"));
    assert(contains(decoded.frontend.discoveredStoryRoutes, "wade"));
    assert(contains(decoded.frontend.discoveredStoryRoutes, "virek"));

    px::SaveData qolSave;
    qolSave.frontend.objectiveHistory = {"REACH THE RIVER", "CHOOSE A ROUTE"};
    qolSave.qol.highContrastHud = true;
    qolSave.qol.largerText = true;
    qolSave.qol.reducedFlashes = true;
    qolSave.qol.combatMessages = "minimal";
    const auto qolDecoded = px::SaveCodec::deserialize(px::SaveCodec::serialize(qolSave));
    assert(qolDecoded.schemaVersion == px::SaveData::kSchemaVersion);
    assert(qolDecoded.frontend.objectiveHistory.size() == 2);
    assert(qolDecoded.qol.highContrastHud && qolDecoded.qol.largerText && qolDecoded.qol.reducedFlashes);
    assert(qolDecoded.qol.combatMessages == "minimal");

    auto discoveredMenu = makeMenu(menus, routes, recap, discovered);
    discoveredMenu.openModeSelect();
    discoveredMenu.selectMode("story");
    discoveredMenu.confirm();
    discoveredMenu.handle(px::Action::MoveRight);
    assert(discoveredMenu.snapshot().selectedRoute.id == "bark");
    discoveredMenu.confirm();
    assert(discoveredMenu.screen() == px::MenuScreen::StoryComingLater);
    assert(discoveredMenu.outcome() == px::MenuOutcome::None);
    discoveredMenu.confirm();
    assert(discoveredMenu.screen() == px::MenuScreen::StoryCharacterSelect);

    px::SaveData continuation;
    continuation.story.chapterId = "rrvvfo_ch1";
    auto continueMenu = makeMenu(menus, routes, recap, continuation);
    continueMenu.openStoryCharacterSelect();
    assert(continueMenu.snapshot().primaryPrompt == "CONTINUE STORY");
    continueMenu.confirm();
    assert(continueMenu.outcome() == px::MenuOutcome::ContinueStory);
    continueMenu.clearOutcome();
    continueMenu.openStoryCharacterSelect();
    continueMenu.handle(px::Action::MoveDown);
    assert(continueMenu.snapshot().primaryPrompt == "CONTINUE STORY");
    assert(continueMenu.snapshot().routeActions.size() == 1);
    continuation.story.flags.push_back("rrvvfo_story_complete");
    auto completedMenu = makeMenu(menus, routes, recap, continuation);
    completedMenu.openStoryCharacterSelect();
    completedMenu.handle(px::Action::MoveDown);
    assert(completedMenu.snapshot().primaryPrompt == "REPLAY STORY");
    completedMenu.confirm();
    assert(completedMenu.outcome() == px::MenuOutcome::ReplayChapter);

    px::SaveData reconstruction;
    reconstruction.story.flags = {
        "ch2_rrvvfo_bark_reunion",
        "ch2_rrvvfo_wade_reunion",
        "rrvvfo_noticed_virek_emerald_missing"
    };
    px::StoryUnlockSystem::reconstructFromProgress(reconstruction);
    assert(px::StoryUnlockSystem::visibleRoutes(reconstruction).size() == 4);
    auto notificationMenu = makeMenu(menus, routes, recap, reconstruction);
    notificationMenu.openModeSelect();
    assert(notificationMenu.showQueuedStoryUnlock());
    assert(notificationMenu.screen() == px::MenuScreen::UnlockCelebration);
    assert(notificationMenu.snapshot().unlockRouteId == "bark");
    notificationMenu.confirm();
    assert(notificationMenu.screen() == px::MenuScreen::ModeSelect);
    assert(px::StoryUnlockSystem::nextQueuedUnlock(reconstruction) == "wade");

    const std::string schema2 =
        "schema=2\nroute=project_hollow_branch\nchapter=rrvvfo_ch1\nscene=0\n"
        "checkpoint=opening\npendingChapter=\nmap=project_hollow_facility\nx=0\nz=0\n"
        "routeChoice=\nhp=100\nenergy=100\nguard=100\ninput=modern\n"
        "flag=project_hollow_name_revealed\n";
    const auto migrated = px::SaveCodec::deserialize(schema2);
    assert(migrated.schemaVersion == px::SaveData::kSchemaVersion);
    assert(migrated.story.routeId == "organization_red_branch");
    assert(migrated.world.mapId == "organization_red_facility");
    assert(contains(migrated.story.flags, "organization_red_name_revealed"));
    assert(contains(migrated.frontend.discoveredStoryRoutes, "rrvvfo"));
    assert(factions.canonicalId("project_hollow") == "organization_red");
    assert(factions.get("organization_red").displayName == "ORGANIZATION OF THE RED");

    assert(recap.sections().size() == 10);
    assert(recap.totalFrameCount() >= 30);
    assert(recap.suggestedRuntimeSeconds() >= 300.0f);
    assert(recap.suggestedRuntimeSeconds() <= 420.0f);
    const std::vector<std::string> recapIds{
        "the_brothers", "the_asrylyte", "two_sides", "rrvvfos_burden", "fake_death",
        "shadow_energy", "virek", "oddballs", "fall_of_perfection", "after_battle"
    };
    for (std::size_t i = 0; i < recapIds.size(); ++i)
        assert(recap.sections()[i].id == recapIds[i] && !recap.sections()[i].frames.empty());
    std::string recapText;
    for (const auto& section : recap.sections()) for (const auto& frame : section.frames)
        recapText += section.title + frame.caption + frame.body;
    for (const auto* forbidden : {"Shots of Agony", "biological father", "biological uncle", "royal blood", "Clone Organization"})
        assert(recapText.find(forbidden) == std::string::npos);

    assert(manual.pages().size() >= 8);
    assert(manual.get("movement").entries.size() == 3);
    assert(manual.get("basic_combat").entries.size() == 5);
    assert(manual.get("kinetic_combat").entries.size() >= 5);
    assert(manual.get("resource_control").entries.size() >= 4);
    assert(manual.get("welcome").title == "HOW THE REFRESHER WORKS");
    assert(std::none_of(manual.get("rrvvfo_techniques").entries.begin(),
                        manual.get("rrvvfo_techniques").entries.end(), [](const auto& entry) {
        return entry.label.find("Shots") != std::string::npos || entry.label.find("???") != std::string::npos;
    }));

    std::cout << "PASS: title/menu carousel state, Story So Far, route discovery and migration, "
                 "unfinished-route honesty, recap registry, manual hierarchy, and Organization of the Red IDs\n";
    return 0;
}
