#include "content/adventure_registry.hpp"
#include "content/chapter_registry.hpp"
#include "content/character_presentation_registry.hpp"
#include "content/cutscene_registry.hpp"
#include "content/dialogue_registry.hpp"
#include "content/exploration_registry.hpp"
#include "content/map_registry.hpp"
#include "content/training_registry.hpp"
#include "content/world_presentation_registry.hpp"
#include "core/ability_hotbar.hpp"
#include "core/combat.hpp"
#include "core/field_movement.hpp"
#include "core/input.hpp"
#include "core/runtime.hpp"
#include "core/save.hpp"
#include "core/quest.hpp"
#include "core/story_progression.hpp"
#include "core/world.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>

namespace {

bool hasScene(const px::ChapterDefinition& chapter, const std::string& id) {
    return std::any_of(chapter.openingFlow.begin(), chapter.openingFlow.end(),
        [&](const px::SceneStep& scene){ return scene.id == id; });
}

bool hasPrimitive(const px::WorldPresentationDefinition& stage, const std::string& id) {
    return std::any_of(stage.primitives.begin(), stage.primitives.end(),
        [&](const px::WorldPrimitiveDefinition& primitive){ return primitive.id == id; });
}

void tap(px::RuntimeSession& runtime, px::InputState& input, px::Action action, float dt = 0.1f) {
    input.beginFrame(); input.set(action, true); runtime.tick(input, dt);
    input.beginFrame(); input.set(action, false); runtime.tick(input, dt);
}

void holdFrames(px::RuntimeSession& runtime, px::InputState& input, px::Action action,
                int frames, float dt = 0.1f) {
    for (int i = 0; i < frames; ++i) {
        input.beginFrame(); input.set(action, true); runtime.tick(input, dt);
    }
    input.beginFrame(); input.set(action, false); runtime.tick(input, dt);
}

void idleFrames(px::RuntimeSession& runtime, px::InputState& input, int frames, float dt = 0.1f) {
    for (int i = 0; i < frames; ++i) { input.beginFrame(); runtime.tick(input, dt); }
}

void moveToward(px::RuntimeSession& runtime, px::InputState& input, px::Vec2 target,
                float stopRadius = 45.0f, int maxFrames = 360) {
    for (int i = 0; i < maxFrames && px::distance(runtime.view().playerPosition, target) > stopRadius; ++i) {
        input.beginFrame();
        input.set(px::Action::MoveLeft, false); input.set(px::Action::MoveRight, false);
        input.set(px::Action::MoveUp, false); input.set(px::Action::MoveDown, false);
        const auto p = runtime.view().playerPosition;
        if (target.x - p.x > 8.0f) input.set(px::Action::MoveRight, true);
        else if (target.x - p.x < -8.0f) input.set(px::Action::MoveLeft, true);
        if (target.z - p.z > 8.0f) input.set(px::Action::MoveDown, true);
        else if (target.z - p.z < -8.0f) input.set(px::Action::MoveUp, true);
        runtime.tick(input, 0.1f);
    }
    input.beginFrame();
    input.set(px::Action::MoveLeft, false); input.set(px::Action::MoveRight, false);
    input.set(px::Action::MoveUp, false); input.set(px::Action::MoveDown, false);
    runtime.tick(input, 0.01f);
}

void finishDialogue(px::RuntimeSession& runtime) {
    int guard = 0;
    while (runtime.view().dialogueVisible && guard++ < 64) runtime.confirm();
    assert(guard < 64);
}

void waitForTelegraph(px::RuntimeSession& runtime, px::InputState& input) {
    for (int i = 0; i < 50 && !runtime.view().opponentAttackTelegraphed; ++i) {
        input.beginFrame(); runtime.tick(input, 0.05f);
    }
    assert(runtime.view().opponentAttackTelegraphed);
}

void resolveTrainingAttack(px::RuntimeSession& runtime, px::InputState& input, px::Action action,
                           int recoveryFrames) {
    tap(runtime, input, action, 0.1f);
    idleFrames(runtime, input, recoveryFrames, 0.1f);
}

void finishOpeningTraining(px::RuntimeSession& runtime, px::InputState& input) {
    finishDialogue(runtime);
    assert(runtime.view().sceneId == "sage_object_swap_field_trial");
    for (int i = 0; i < 3; ++i) tap(runtime, input, px::Action::Ability2);
    finishDialogue(runtime);
    finishDialogue(runtime);
    assert(runtime.view().sceneId == "sage_tutorial_spar" && runtime.view().trainingManualVisible);
    assert(runtime.map().id == "sage_training_arena");
    assert(runtime.view().trainingManualPageCount >= 8);
    assert(runtime.view().trainingManualPage.id == "welcome");
    assert(runtime.view().trainingManualPage.title == "HOW THE REFRESHER WORKS");
    assert(runtime.view().trainingManualPage.entries.size() == 4);
    assert(runtime.view().trainingManualOptions.size() == 3);
    assert(runtime.view().trainingManualOptions[0] == "SAGE'S FULL CHALLENGE");
    assert(runtime.view().trainingManualOptions[2] == "SKIP • START THE ROAD");
    assert(!runtime.view().hotbarVisible && !runtime.view().showEnergy && !runtime.view().showGuard);
    tap(runtime, input, px::Action::MoveRight);
    assert(runtime.view().trainingManualPage.id == "movement");
    assert(runtime.view().trainingManualPage.entries.size() == 3);
    assert(runtime.view().trainingManualPage.entries[0].label == "Move");
    tap(runtime, input, px::Action::MoveLeft);
    tap(runtime, input, px::Action::MoveDown);
    assert(runtime.view().trainingManualSelection == 1);
    tap(runtime, input, px::Action::MoveUp);
    assert(runtime.view().trainingManualSelection == 0);
    runtime.confirm();

    // Arena actors must face one another rather than the center/depth of the
    // ring. Moving away keeps Rrvvfo locked on the opponent and reverses the
    // run cycle into a readable Sonic Battle-style backpedal.
    assert(std::fabs(runtime.view().playerYawDegrees + 90.0f) < 0.01f);
    assert(std::fabs(runtime.view().opponentYawDegrees - 90.0f) < 0.01f);
    const auto beforeBackpedal = runtime.view().playerPosition;
    input.beginFrame(); input.set(px::Action::MoveLeft, true); runtime.tick(input, 0.1f);
    assert(runtime.view().playerPosition.x < beforeBackpedal.x);
    assert(std::fabs(runtime.view().playerYawDegrees + 90.0f) < 0.01f);
    assert(std::fabs(runtime.view().opponentYawDegrees - 90.0f) < 0.01f);
    assert(runtime.view().playerAnimation == "combat_retreat" && runtime.view().playerAnimationSpeed > 0.0f);
    input.beginFrame(); input.set(px::Action::MoveLeft, false); runtime.tick(input, 0.01f);

    // A directionless dash cannot satisfy the movement lesson with a fake,
    // zero-distance burst. A previously established facing still works.
    px::FieldMovementState directionlessDash;
    px::InputState dashInput;
    dashInput.beginFrame(); dashInput.set(px::Action::Dash, true);
    const auto directionlessPosition = px::FieldMovementSystem::tick(
        runtime.map(), {-1240.0f, 120.0f}, directionlessDash, dashInput, .05f);
    assert(!directionlessDash.dashStartedThisFrame);
    assert(directionlessPosition.x == -1240.0f && directionlessPosition.z == 120.0f);

    holdFrames(runtime, input, px::Action::MoveLeft, 3, 0.1f);
    tap(runtime, input, px::Action::Jump, 0.02f);
    tap(runtime, input, px::Action::Dash, 0.1f);
    assert(runtime.view().trainingStepIndex == 1);

    holdFrames(runtime, input, px::Action::MoveLeft, 3, 0.1f);
    assert(px::distance(runtime.view().playerPosition, runtime.view().opponentPosition) > 185.0f);
    resolveTrainingAttack(runtime, input, px::Action::Light, 2);
    resolveTrainingAttack(runtime, input, px::Action::Heavy, 6);
    resolveTrainingAttack(runtime, input, px::Action::Launcher, 5);
    assert(runtime.view().trainingProgress == 3);
    resolveTrainingAttack(runtime, input, px::Action::Grab, 3);
    assert(runtime.view().trainingProgress == 3);
    moveToward(runtime, input, runtime.view().opponentPosition, 62.0f);
    resolveTrainingAttack(runtime, input, px::Action::Grab, 3);
    assert(runtime.view().trainingStepIndex == 2);
    assert(runtime.view().showGuard && runtime.view().showOpponentHealth && !runtime.view().showEnergy);
    const auto tutorialCheckpoint = runtime.saveSnapshot();
    assert(std::find(tutorialCheckpoint.story.flags.begin(), tutorialCheckpoint.story.flags.end(),
                     "ch1_tutorial_checkpoint=2") != tutorialCheckpoint.story.flags.end());
    runtime.resetCurrentScene();
    assert(runtime.view().trainingManualVisible && runtime.view().trainingManualOptions.size() == 4);
    assert(runtime.view().trainingManualSelection == 1 && runtime.view().trainingCheckpointLabel == "PERFECT BLOCK");
    runtime.confirm();
    assert(runtime.view().trainingStepIndex == 2);

    assert(std::fabs(runtime.view().opponentAttackSecondsRemaining - 1.35f) < 0.01f);
    idleFrames(runtime, input, 20, 0.05f);
    assert(!runtime.view().opponentAttackTelegraphed);
    idleFrames(runtime, input, 8, 0.05f);
    assert(runtime.view().trainingStepIndex == 2);
    assert(runtime.view().opponentAttackAttempt == 1);
    assert(std::fabs(runtime.view().opponentAttackSecondsRemaining - 1.45f) < 0.06f);
    assert(!runtime.view().opponentAttackTelegraphed);
    // A failed parry now produces real Legacy-style hit-stop, so wall-clock
    // frames are intentionally not identical to combat-simulation frames.
    waitForTelegraph(runtime, input);
    assert(runtime.view().opponentAttackSecondsRemaining <= 0.32f);
    tap(runtime, input, px::Action::Block, 0.05f);
    idleFrames(runtime, input, 7, 0.05f);
    assert(runtime.view().trainingStepIndex == 3);
    holdFrames(runtime, input, px::Action::Charge, 18, 0.1f);
    assert(runtime.view().trainingStepIndex == 4);
    tap(runtime, input, px::Action::Ability1);
    tap(runtime, input, px::Action::Ability2);
    assert(runtime.view().trainingStepIndex == 5);
    assert(runtime.view().hotbarVisible && runtime.view().hotbar.size() == 3);
    holdFrames(runtime, input, px::Action::Charge, 6, 0.1f);
    tap(runtime, input, px::Action::Ability3);
    assert(runtime.view().lensActive);
    idleFrames(runtime, input, 8, 0.1f);
    assert(runtime.view().trainingStepIndex == 5);
    assert(runtime.view().opponentAttackAttempt == 1);
    assert(std::fabs(runtime.view().opponentAttackSecondsRemaining - 1.10f) < 0.11f);
    waitForTelegraph(runtime, input);
    input.beginFrame(); input.set(px::Action::MoveLeft, true); input.set(px::Action::Dash, true); runtime.tick(input, 0.05f);
    input.beginFrame(); input.set(px::Action::MoveLeft, false); input.set(px::Action::Dash, false); runtime.tick(input, 0.01f);
    idleFrames(runtime, input, 32, 0.05f);
    assert(runtime.view().trainingStepIndex == 6);

    resolveTrainingAttack(runtime, input, px::Action::Light, 2);
    resolveTrainingAttack(runtime, input, px::Action::Light, 2);
    resolveTrainingAttack(runtime, input, px::Action::Heavy, 6);
    assert(runtime.view().sparCleanHits == 3 && runtime.view().sceneComplete);
    runtime.confirm();
    assert(runtime.view().sceneId == "ch1_post_spar_banter");
}

void reachRouteChoice(px::RuntimeSession& runtime, px::InputState& input) {
    finishOpeningTraining(runtime, input);
    finishDialogue(runtime);
    finishDialogue(runtime);
    assert(runtime.view().sceneId == "tournament_road_departure");
    moveToward(runtime, input, {-785.0f, 0.0f}, 50.0f);
    assert(runtime.view().sceneId == "river_object_swap_problem");
    moveToward(runtime, input, {-70.0f, 0.0f}, 52.0f);
    assert(runtime.view().playerPosition.x <= -25.0f);
    tap(runtime, input, px::Action::Ability2);
    assert(runtime.view().sceneId == "legacy_route_choice");
    assert(runtime.view().choiceVisible && runtime.view().choiceTitle == "CHOOSE A ROUTE");
    assert(runtime.view().choiceOptions == std::vector<std::string>({"MAIN ROAD", "FOREST SHORTCUT", "CLIFF ROUTE"}));
    assert(std::fabs(runtime.view().playerPosition.x - 230.0f) < 0.01f);
}

void finishRelay(px::RuntimeSession& runtime, px::InputState& input,
                 const px::ExplorationRegistry& exploration) {
    assert(runtime.view().sceneId == "swap_relay_trial");
    for (const auto target : exploration.get("swap_relay_trial").relayMarkers) {
        moveToward(runtime, input, target, 70.0f);
        tap(runtime, input, px::Action::Ability2);
    }
    idleFrames(runtime, input, 7, 0.1f);
    assert(runtime.view().sceneId == "transport_wheel_recovery");
}

} // namespace

int main() {
    px::ChapterRegistry chapters;
    px::MapRegistry maps;
    px::CutsceneRegistry cutscenes;
    px::DialogueRegistry dialogue;
    px::ExplorationRegistry exploration;
    px::TrainingRegistry training;
    px::AdventureRegistry adventures;
    px::WorldPresentationRegistry worlds;
    px::CharacterPresentationRegistry characters;

    const auto& sageRefresher = training.get("sage_tutorial_spar");
    assert(std::fabs(sageRefresher.parryFirstAttackSeconds - 1.35f) < 0.001f);
    assert(std::fabs(sageRefresher.parryRetryAttackSeconds - 1.45f) < 0.001f);
    assert(std::fabs(sageRefresher.parryWarningSeconds - 0.32f) < 0.001f);
    assert(std::fabs(sageRefresher.lensFirstAttackSeconds - 0.90f) < 0.001f);
    assert(std::fabs(sageRefresher.lensRetryAttackSeconds - 1.10f) < 0.001f);
    assert(std::fabs(sageRefresher.lensWarningSeconds - 0.32f) < 0.001f);
    assert(std::fabs(sageRefresher.lensEvadeConfirmationSeconds - 1.05f) < 0.001f);

    const auto& ch1 = chapters.get("rrvvfo_ch1");
    assert(ch1.primaryMap == "training_region" && ch1.openingFlow.size() == 22);
    for (const auto* scene : {"sage_object_swap_field_trial", "river_object_swap_problem", "legacy_route_choice",
                              "selected_route_adventure", "swap_relay_trial", "runaway_tournament_cart",
                              "roadside_encounter", "lens_roadblock_reveal", "tournament_outskirts_arrival"})
        assert(hasScene(ch1, scene));
    assert(!hasScene(ch1, "sage_energy_signature_training"));
    assert(!hasScene(ch1, "bark_wade_reunion"));
    assert(ch1.nextChapterId == "rrvvfo_ch2" && !chapters.has("rrvvfo_ch2"));
    const std::vector<std::string> exactOpeningOrder{
        "ch1_object_swap_setup", "sage_object_swap_field_trial", "ch1_object_swap_result",
        "ch1_opening_sage_setup", "sage_tutorial_spar", "ch1_post_spar_banter",
        "tournament_road_departure_dialogue", "tournament_road_departure", "river_object_swap_problem",
        "legacy_route_choice", "selected_route_adventure", "swap_relay_trial",
        "transport_wheel_recovery", "runaway_tournament_cart", "roadside_encounter",
        "reach_tournament_checkpoint", "tournament_checkpoint_dialogue", "reach_lens_roadblock",
        "lens_manual_reaction", "lens_roadblock_reveal", "reach_tournament_outskirts",
        "tournament_outskirts_arrival"
    };
    for (std::size_t i = 0; i < exactOpeningOrder.size(); ++i) assert(ch1.openingFlow[i].id == exactOpeningOrder[i]);
    assert(std::none_of(ch1.openingFlow.begin(), ch1.openingFlow.end(), [](const px::SceneStep& scene){
        return scene.id.find("mission") != std::string::npos;
    }));

    const auto& swapDialogue = dialogue.get("ch1_object_swap_setup");
    assert(swapDialogue.size() == 4);
    assert(swapDialogue[0].text == "Before you start throwing attacks around, prove you can move without a path.");
    assert(swapDialogue[1].text == "There is literally a path right there.");
    assert(swapDialogue[2].text == "Then ignore it. See the glowing anchor? Swap to it. Three anchors. No walking between them.");
    assert(swapDialogue[3].text == "If an object can get somewhere, you can turn that into a route. Figure out the rest.");
    assert(dialogue.get("road_npc_dojo_student").size() == 2);
    assert(dialogue.get("road_npc_lost_competitor").size() == 2);
    assert(dialogue.get("road_npc_lost_competitor_pass_delivered").size() == 4);
    assert(dialogue.get("sign_quest_intro").size() == 5);
    assert(dialogue.get("sign_quest_complete").size() == 5);
    assert(dialogue.get("tournament_outskirts_arrival").size() == 5);
    const auto& legacyFieldCutscene = cutscenes.get("ch1_opening_sage_setup");
    assert(legacyFieldCutscene.staging[0].position.x == -1450.0f);
    assert(legacyFieldCutscene.staging[0].position.z == 42.0f);
    assert(legacyFieldCutscene.staging[1].position.x == -1030.0f);
    assert(legacyFieldCutscene.staging[1].position.z == -42.0f);

    const auto& field = worlds.get("training-field");
    const auto& roadPresentation = worlds.get("training-road");
    assert(field.camera.yawDegrees == 38.0f && field.camera.fovDegrees == 43.0f);
    assert(field.camera.baseDistance == 900.0f && field.camera.height == 410.0f);
    for (const auto* primitive : {"field_grass_surface", "field_center_ring_0",
                                  "field_center_cross_x", "field_fence_north_rail",
                                  "field_fence_south_rail", "field_tree_west_canopy",
                                  "field_tree_east_canopy"})
        assert(hasPrimitive(field, primitive));
    assert(!hasPrimitive(field, "field_center_mark"));
    assert(roadPresentation.camera.yawDegrees == 38.0f && roadPresentation.camera.fovDegrees == 45.0f);
    assert(roadPresentation.camera.baseDistance == 980.0f && roadPresentation.camera.height == 430.0f);
    for (const auto* primitive : {"sage_bell", "focus_pillar_center", "broken_bridge_rail_-45",
                                  "broken_bridge_rail_195", "river", "tournament_gate_red",
                                  "outskirts_shop_fire", "outskirts_shop_blue", "outskirts_shop_violet"})
        assert(hasPrimitive(roadPresentation, primitive));
    assert(characters.get("rrvvfo").sourceModelPath == "assets/characters/rrvvfo/rrvvfo-dev.glb");
    assert(characters.get("rrvvfo").desktopCookedAsset == "assets/characters/rrvvfo/rrvvfo-dev.pxskel");
    assert(characters.get("rrvvfo").cookedAssetReady);
    assert(characters.get("sage").fallback == px::CharacterFallbackKind::ProceduralMentor);

    const auto& adventure = adventures.get("rrvvfo_ch1_road");
    assert(adventure.npcs.size() == 7);
    assert(adventure.farBankRock.x == 230.0f && adventure.gateRelayMarkers.size() == 3);
    assert(adventure.gateReleaseDelaySeconds == 0.52f);
    assert(adventure.runawayCart.sequence.size() == 4 && adventure.runawayCart.durationSeconds == 6.2f);
    assert(adventure.runawayCart.safeRetries == 1 && adventure.runawayCart.failForward);
    assert(adventure.roadsideFight.koTarget == 1 && adventure.roadsideFight.startingEnergy == 45.0f);
    assert(adventure.sidePuzzle.id == "sign_that_points_back");
    assert(adventure.sidePuzzle.npcId == "sign_painter" && adventure.sidePuzzle.radius == 105.0f);
    assert(adventure.ambientLife.birdCount == 5 && adventure.ambientLife.birdSpeed == 95.0f);
    assert(adventure.ambientLife.deliveryCartSpeed == 42.0f);
    assert(exploration.get("selected_route_adventure").routeChallenges.size() == 3);
    assert(exploration.get("selected_route_adventure").mainWorkMarkers.size() == 4);
    assert(exploration.get("selected_route_adventure").forestBellMarkers.size() == 4);
    assert(exploration.get("selected_route_adventure").jumpMarkers.size() == 5);
    assert(exploration.get("selected_route_adventure").routeHintFirstSeconds == 18.0f);
    assert(exploration.get("selected_route_adventure").routeHintSecondSeconds == 36.0f);
    assert(exploration.get("transport_wheel_recovery").rule == px::ExplorationRuleKind::SwapRelay);
    assert(exploration.get("transport_wheel_recovery").relayMarkers.size() == 2);
    {
        px::SaveData checkpoint;
        checkpoint.story.chapterId = "rrvvfo_ch1";
        checkpoint.story.sceneIndex = 10;
        checkpoint.story.checkpointId = ch1.openingFlow[10].checkpointId;
        checkpoint.story.flags = {"ch1_route_progress=2"};
        checkpoint.world.mapId = ch1.primaryMap;
        checkpoint.world.routeChoice = "forest";
        checkpoint.world.position = {250.0f, -350.0f};
        px::RuntimeSession restarted(chapters, maps, cutscenes, dialogue, exploration, training, adventures);
        restarted.loadSnapshot(checkpoint);
        const auto restartPoint = restarted.view().playerPosition;
        px::InputState checkpointInput;
        moveToward(restarted, checkpointInput, {410.0f, -350.0f}, 30.0f, 80);
        assert(px::distance(restarted.view().playerPosition, restartPoint) > 20.0f);
        restarted.resetCurrentScene();
        assert(px::distance(restarted.view().playerPosition, restartPoint) < 0.01f);
        assert(restarted.view().objective.find("BELLS 2 / 4") != std::string::npos);
    }

    const auto hotbar = px::AbilityHotbarCatalog::rrvvfoChapter1();
    assert(hotbar.size() == 3 && hotbar[0].id == "fireBlast" && hotbar[0].energyCost == 22.0f);
    assert(hotbar[1].id == "objectSwap" && hotbar[1].energyCost == 20.0f);
    const auto& storyFireBlast = px::CombatSystem::attackFor("rrvvfo", px::AttackKind::Projectile);
    assert(storyFireBlast.damage == 15.0f && storyFireBlast.guardDamage == 9.0f);
    assert(std::none_of(hotbar.begin(), hotbar.end(), [](const px::AbilitySlotDefinition& ability){
        return ability.id == "shotsOfAgony" || ability.id == "solarWeave" || ability.label == "???";
    }));

    // Omega save migration: a stable checkpoint ID wins over a stale numeric index.
    {
        px::SaveData legacy;
        legacy.schemaVersion = 4;
        legacy.story.chapterId = "rrvvfo_ch1";
        legacy.story.sceneIndex = 0;
        legacy.story.checkpointId = ch1.openingFlow[10].checkpointId;
        legacy.world.mapId = ch1.primaryMap;
        px::RuntimeSession migrated(chapters, maps, cutscenes, dialogue, exploration, training, adventures);
        migrated.loadSnapshot(px::SaveCodec::deserialize(px::SaveCodec::serialize(legacy)));
        assert(migrated.view().sceneId == ch1.openingFlow[10].id);
    }

    // Shared Chapter 2–4 foundation: Legacy progression math, protected card visibility and data-only quests.
    px::TournamentCardState card{"rrvvfo"};
    assert(!card.acquired && px::StoryProgressionSystem::levelForXp(99) == 1);
    const auto xpResult = px::StoryProgressionSystem::grantXp(card, 100);
    assert(xpResult.levelsGained == 1 && card.level == 2 && card.pendingBonusChoices == 1);
    const auto automaticStats = px::StoryProgressionSystem::statsFor(card);
    assert(automaticStats.hp == 104 && automaticStats.power == 11 && automaticStats.focus == 11);
    px::StoryProgressionSystem::applyBonusWheel(card, px::StoryStat::Defense, 3);
    assert(px::StoryProgressionSystem::statsFor(card).defense == 14 && card.pendingBonusChoices == 0);
    px::QuestDefinition potion{"old_man_potion", px::QuestKind::Optional, "Old Man Potion Quest",
                               {"village_defended"}, {"potion_route_complete"}};
    px::QuestState potionState{px::QuestStatus::Active, {}};
    assert(px::QuestSystem::prerequisitesMet(potion, {"village_defended"}));
    px::QuestSystem::addFlag(potionState, "potion_route_complete");
    assert(px::QuestSystem::completionMet(potion, potionState));

    // Legacy-derived shared combat data and timing behavior.
    const std::string rrvvfoId = "rrvvfo";
    const auto& l1 = px::CombatSystem::attackFor(rrvvfoId, px::AttackKind::Light1);
    const auto& heavy = px::CombatSystem::attackFor(rrvvfoId, px::AttackKind::Heavy);
    assert(l1.duration == .27f && l1.activeStart == .06f && l1.damage == 5.1f);
    assert(heavy.duration == .62f && heavy.activeStart == .19f && heavy.damage == 13.5f);
    px::FighterState attacker{"rrvvfo"}, defender{"road_fighter"};
    assert(px::CombatSystem::startAttack(attacker, px::AttackKind::Heavy));
    auto hit = px::CombatSystem::advanceAttack(attacker, defender, .10f, true);
    assert(!hit.connected && defender.hp == 100.0f);
    hit = px::CombatSystem::advanceAttack(attacker, defender, .10f, true, {true, true});
    assert(hit.connected && hit.wallSplat && defender.hp < 100.0f);
    assert(px::CombatSystem::flowCancel(attacker));
    assert(px::CombatSystem::startDash(attacker));
    px::FighterState airborne{"rrvvfo"}; airborne.airborne = true;
    assert(px::CombatSystem::startAirDash(airborne));
    px::FighterState guardAttacker{"rrvvfo"}, guardDefender{"sage"};
    px::CombatSystem::startBlock(guardDefender);
    const auto perfect = px::CombatSystem::heavy(guardAttacker, guardDefender);
    assert(perfect.blocked && perfect.perfectBlocked && perfect.damage == 0.0f);
    px::FighterState counter{"rrvvfo"}, counterTarget{"sage"};
    assert(px::CombatSystem::startCounter(counter));
    assert(px::CombatSystem::light(counterTarget, counter).countered);
    px::FighterState breaker{"rrvvfo"}; breaker.energy = 100.0f; breaker.stunTimer = .5f;
    assert(px::CombatSystem::comboBreaker(breaker) && breaker.stunTimer == 0.0f);
    px::FighterState pursuit{"rrvvfo"}, launched{"road_fighter"};
    const auto launchHit = px::CombatSystem::launcher(pursuit, launched);
    assert(launchHit.launched && launchHit.pursuitOpened && px::CombatSystem::startPursuit(pursuit));
    assert(px::CombatSystem::resolveProjectileClash(1.0f, 1.0f) == px::ClashResult::Draw);
    const auto ai = px::CombatSystem::chooseAiAction(px::AiArchetype::Balanced, defender, attacker, 250.0f, 0);
    assert(ai.approach);

    const auto& roadMap = maps.get("training_region");
    assert(maps.get("sage_training_arena").blockers.empty());
    assert(maps.get("roadside_arena").blockers.empty());
    const auto blockedRiver = px::WorldCollision::move(roadMap, {-40.0f, 0.0f}, {80.0f, 0.0f});
    assert(blockedRiver.x <= -25.0f || blockedRiver.x >= 175.0f);

    // 0.4G gameplay-remaster QoL: a jump pressed just before landing is buffered
    // for a tenth of a second rather than being eaten between frames.
    px::FieldMovementState bufferedJump;
    bufferedJump.height = 4.0f;
    bufferedJump.verticalVelocity = -200.0f;
    px::InputState movementInput;
    movementInput.beginFrame(); movementInput.set(px::Action::Jump, true);
    px::FieldMovementSystem::tick(roadMap, {-1200.0f, 0.0f}, bufferedJump, movementInput, 0.01f);
    assert(bufferedJump.jumpBufferTime > 0.0f && !bufferedJump.jumpStartedThisFrame);
    movementInput.beginFrame(); movementInput.set(px::Action::Jump, false);
    px::FieldMovementSystem::tick(roadMap, {-1200.0f, 0.0f}, bufferedJump, movementInput, 0.02f);
    assert(bufferedJump.jumpStartedThisFrame && bufferedJump.verticalVelocity > 0.0f);

    // Dashes retain their committed direction but accept a restrained correction.
    px::FieldMovementState steeringDash;
    px::InputState steeringInput;
    steeringInput.beginFrame();
    steeringInput.set(px::Action::MoveRight, true);
    steeringInput.set(px::Action::Dash, true);
    px::FieldMovementSystem::tick(maps.get("roadside_arena"), {800.0f, 0.0f}, steeringDash, steeringInput, 0.01f);
    steeringInput.beginFrame();
    steeringInput.set(px::Action::MoveRight, false);
    steeringInput.set(px::Action::Dash, false);
    steeringInput.set(px::Action::MoveDown, true);
    px::FieldMovementSystem::tick(maps.get("roadside_arena"), {807.0f, 0.0f}, steeringDash, steeringInput, 0.05f);
    assert(steeringDash.dashDirection.z > 0.0f);
    assert(steeringDash.dashDirection.x > steeringDash.dashDirection.z);

    // Seen directed scenes persist through the shared save and are skippable only
    // after they have genuinely been completed once. First-time Story pacing is untouched.
    {
        px::RuntimeSession seenRuntime(chapters, maps, cutscenes, dialogue, exploration, training, adventures);
        px::InputState seenInput;
        seenRuntime.startChapter("rrvvfo_ch1");
        assert(seenRuntime.view().sceneId == "ch1_object_swap_setup");
        finishDialogue(seenRuntime);
        auto seenSave = seenRuntime.saveSnapshot("legacy");
        assert(std::find(seenSave.story.flags.begin(), seenSave.story.flags.end(),
                         "seen_ch1_scene=ch1_object_swap_setup") != seenSave.story.flags.end());
        seenSave.story.sceneIndex = 0;
        px::RuntimeSession replayRuntime(chapters, maps, cutscenes, dialogue, exploration, training, adventures);
        replayRuntime.loadSnapshot(px::SaveCodec::deserialize(px::SaveCodec::serialize(seenSave)));
        assert(replayRuntime.view().sceneId == "ch1_object_swap_setup");
        tap(replayRuntime, seenInput, px::Action::Cancel, 0.02f);
        assert(replayRuntime.view().sceneId == "sage_object_swap_field_trial");
    }

    // Holding Confirm accelerates dialogue only after a deliberate delay. It
    // never confirms route choices or other gameplay interactions.
    {
        px::RuntimeSession holdRuntime(chapters, maps, cutscenes, dialogue, exploration, training, adventures);
        px::InputState holdInput;
        holdRuntime.startChapter("rrvvfo_ch1");
        holdInput.beginFrame(); holdInput.set(px::Action::Confirm, true);
        holdRuntime.tick(holdInput, 0.02f);
        assert(holdRuntime.view().dialogueIndex == 1);
        for (int i = 0; i < 6; ++i) {
            holdInput.beginFrame(); holdInput.set(px::Action::Confirm, true);
            holdRuntime.tick(holdInput, 0.20f);
        }
        assert(holdRuntime.view().dialogueIndex == 2);
        holdInput.beginFrame(); holdInput.set(px::Action::Confirm, false);
        holdRuntime.tick(holdInput, 0.02f);
    }

    // The Sage's lesson is useful, but never mandatory. Skipping from the
    // opening choice advances to the same Legacy road flow and persists safely.
    {
        px::RuntimeSession skipRuntime(chapters, maps, cutscenes, dialogue, exploration, training, adventures);
        px::InputState skipInput;
        skipRuntime.startChapter("rrvvfo_ch1");
        finishDialogue(skipRuntime);
        for (int i = 0; i < 3; ++i) tap(skipRuntime, skipInput, px::Action::Ability2);
        finishDialogue(skipRuntime);
        finishDialogue(skipRuntime);
        assert(skipRuntime.view().sceneId == "sage_tutorial_spar");
        assert(skipRuntime.view().trainingManualOptions.size() == 3);
        tap(skipRuntime, skipInput, px::Action::MoveDown);
        tap(skipRuntime, skipInput, px::Action::MoveDown);
        tap(skipRuntime, skipInput, px::Action::Confirm);
        assert(skipRuntime.view().sceneId == "ch1_post_spar_banter");
        const auto skipped = skipRuntime.saveSnapshot();
        assert(std::find(skipped.story.flags.begin(), skipped.story.flags.end(), "ch1_tutorial_skipped") != skipped.story.flags.end());
    }

    // Shared runtime animation/VFX signals are gameplay-owned, so Mac/Linux/3DS
    // consume the same state instead of guessing from keyboard/controller input.
    {
        px::RuntimeSession presentationRuntime(chapters, maps, cutscenes, dialogue, exploration, training, adventures);
        px::InputState presentationInput;
        presentationRuntime.startChapter("rrvvfo_ch1");
        finishDialogue(presentationRuntime);
        holdFrames(presentationRuntime, presentationInput, px::Action::MoveLeft, 1, 0.1f);
        assert(std::fabs(presentationRuntime.view().playerYawDegrees - 90.0f) < 0.01f);
        holdFrames(presentationRuntime, presentationInput, px::Action::MoveDown, 1, 0.1f);
        assert(std::fabs(presentationRuntime.view().playerYawDegrees) < 0.01f);
        tap(presentationRuntime, presentationInput, px::Action::Ability2, 0.02f);
        assert(presentationRuntime.view().playerAnimation == "object_swap");
        assert(std::any_of(presentationRuntime.view().worldMarkers.begin(), presentationRuntime.view().worldMarkers.end(),
            [](const px::RuntimeMarkerView& marker){ return marker.kind == "object-swap-fx"; }));
    }

    // Fight and Training are real menu destinations, not presentation shells.
    // Both enter the existing shared Story combat and never need a platform-only mode.
    {
        px::RuntimeSession fightRuntime(chapters, maps, cutscenes, dialogue, exploration, training, adventures);
        px::InputState fightInput;
        fightRuntime.startCpuFight();
        assert(fightRuntime.standaloneMode());
        assert(fightRuntime.view().mode == px::GameMode::ArenaCombat);
        assert(fightRuntime.view().roadsideFightActive && fightRuntime.view().opponentVisible);
        assert(fightRuntime.map().id == "roadside_arena");
        assert(fightRuntime.view().objective == "FIGHT • RRVVFO VS CPU");
        const auto fightStart = fightRuntime.view().playerPosition;
        holdFrames(fightRuntime, fightInput, px::Action::MoveRight, 2, 0.05f);
        assert(fightRuntime.view().playerPosition.x > fightStart.x);
        tap(fightRuntime, fightInput, px::Action::Jump, 0.03f);
        assert(fightRuntime.view().playerHeight > 0.0f);
        tap(fightRuntime, fightInput, px::Action::Light, 0.03f);
        assert(fightRuntime.view().player.activeAttack != px::AttackKind::None);
        tap(fightRuntime, fightInput, px::Action::Pause);
        assert(fightRuntime.view().pauseOptions ==
               std::vector<std::string>({"RESUME", "RESTART FIGHT", "RETURN TO TITLE"}));
        tap(fightRuntime, fightInput, px::Action::MoveDown);
        tap(fightRuntime, fightInput, px::Action::MoveDown);
        tap(fightRuntime, fightInput, px::Action::Confirm);
        assert(fightRuntime.consumeReturnToTitleRequest());

        px::RuntimeSession directTraining(chapters, maps, cutscenes, dialogue, exploration, training, adventures);
        px::InputState trainingInput;
        directTraining.startStandaloneTraining();
        assert(directTraining.standaloneMode());
        assert(directTraining.view().sceneId == "sage_tutorial_spar");
        assert(directTraining.view().trainingManualVisible);
        assert(directTraining.view().trainingManualOptions.back() == "EXIT • RETURN TO MENU");
        tap(directTraining, trainingInput, px::Action::MoveDown);
        tap(directTraining, trainingInput, px::Action::MoveDown);
        tap(directTraining, trainingInput, px::Action::Confirm);
        assert(directTraining.consumeReturnToTitleRequest());
    }

    // Main route, side interaction, optional fight, QTE and canonical Chapter 1 ending.
    px::RuntimeSession runtime(chapters, maps, cutscenes, dialogue, exploration, training, adventures);
    px::InputState input;
    runtime.startChapter("rrvvfo_ch1");
    assert(runtime.view().opponentPosition.x == -1080.0f && runtime.view().opponentPosition.z == 95.0f);
    finishDialogue(runtime);
    tap(runtime, input, px::Action::Ability2);
    runtime.resetCurrentScene();
    assert(runtime.view().relayProgress == 0);
    runtime.startChapter("rrvvfo_ch1");
    reachRouteChoice(runtime, input);
    tap(runtime, input, px::Action::Confirm);
    assert(runtime.view().sceneId == "selected_route_adventure" && runtime.view().routeChoice == "main");
    tap(runtime, input, px::Action::Pause);
    assert(runtime.view().pauseVisible && runtime.view().pauseOptions.size() == 7);
    assert(runtime.view().manualSaveAllowed && runtime.view().pauseOptions[1] == "SAVE GAME");
    tap(runtime, input, px::Action::MoveDown);
    tap(runtime, input, px::Action::Confirm);
    assert(runtime.consumeManualSaveRequest());
    runtime.notifyManualSaveResult(true);
    assert(runtime.view().saveStatus.find("SAVED") != std::string::npos);
    runtime.notifyManualSaveUnavailable();
    assert(runtime.view().saveStatus.find("SAFE AREA") != std::string::npos);
    tap(runtime, input, px::Action::Pause);
    assert(!runtime.view().pauseVisible && runtime.view().sceneId == "selected_route_adventure");
    {
        auto checkpoint = runtime.saveSnapshot("legacy");
        checkpoint.world.hp = 73.0f; checkpoint.world.energy = 44.0f; checkpoint.world.guard = 62.0f;
        px::RuntimeSession restored(chapters, maps, cutscenes, dialogue, exploration, training, adventures);
        restored.loadSnapshot(px::SaveCodec::deserialize(px::SaveCodec::serialize(checkpoint)));
        assert(restored.view().sceneId == "selected_route_adventure" && restored.view().routeChoice == "main");
        assert(restored.view().player.hp == 73.0f && restored.view().player.energy == 44.0f && restored.view().player.guard == 62.0f);
        const auto savedRock = std::find_if(checkpoint.world.objects.begin(), checkpoint.world.objects.end(),
            [](const px::WorldObjectState& object){ return object.id == "far_bank_rock"; });
        const auto restoredRock = std::find_if(restored.view().worldMarkers.begin(), restored.view().worldMarkers.end(),
            [](const px::RuntimeMarkerView& marker){ return marker.id == "far_bank_rock"; });
        assert(savedRock != checkpoint.world.objects.end() && restoredRock != restored.view().worldMarkers.end());
        assert(px::distance(savedRock->position, restoredRock->position) < 0.01f);
    }
    moveToward(runtime, input, {250.0f, 0.0f}, 5.0f);
    finishDialogue(runtime);
    tap(runtime, input, px::Action::Ability1);
    finishDialogue(runtime);
    assert(runtime.view().sceneId == "swap_relay_trial");
    finishRelay(runtime, input, exploration);
    tap(runtime, input, px::Action::Ability2);
    finishDialogue(runtime);
    assert(runtime.view().sceneId == "runaway_tournament_cart");
    finishDialogue(runtime);
    assert(runtime.view().qteVisible);
    tap(runtime, input, px::Action::MoveRight);
    tap(runtime, input, px::Action::Jump);
    tap(runtime, input, px::Action::MoveLeft);
    tap(runtime, input, px::Action::MoveRight);
    finishDialogue(runtime);
    assert(runtime.view().sceneId == "roadside_encounter");

    const auto competitor = std::find_if(runtime.view().ambientActors.begin(), runtime.view().ambientActors.end(),
        [](const px::RuntimeActorView& actor){ return actor.id == "lost_competitor"; });
    assert(competitor != runtime.view().ambientActors.end());
    moveToward(runtime, input, competitor->position, 70.0f);
    tap(runtime, input, px::Action::Interact);
    finishDialogue(runtime);
    assert(runtime.view().choiceVisible);
    tap(runtime, input, px::Action::Confirm);
    finishDialogue(runtime);
    moveToward(runtime, input, {780.0f, 0.0f}, 30.0f);
    assert(runtime.view().choiceVisible);
    const auto roadsideReturn = runtime.view().playerPosition;
    tap(runtime, input, px::Action::Confirm);
    finishDialogue(runtime);
    assert(runtime.view().roadsideFightActive);
    assert(runtime.map().id == "roadside_arena");
    bool passWon = false;
    for (int attempt = 0; attempt < 4 && !passWon; ++attempt) {
        for (int i = 0; i < 500 && runtime.view().roadsideFightActive; ++i) {
            input.beginFrame();
            if (runtime.view().player.activeAttack == px::AttackKind::None) input.set(px::Action::Heavy, true);
            runtime.tick(input, 0.1f);
            input.beginFrame(); input.set(px::Action::Heavy, false); runtime.tick(input, 0.01f);
        }
        const auto attemptSave = runtime.saveSnapshot();
        passWon = std::find(attemptSave.story.flags.begin(), attemptSave.story.flags.end(),
                            "ch1_spectator_pass_won") != attemptSave.story.flags.end();
        if (!passWon && runtime.view().choiceVisible) tap(runtime, input, px::Action::Confirm);
    }
    assert(passWon);
    assert(px::distance(runtime.view().playerPosition, roadsideReturn) < 0.01f);
    assert(runtime.view().sceneId == "roadside_encounter");
    assert(runtime.view().objective == "OPTIONAL • RETURN THE SPECTATOR PASS");
    const auto returnCompetitor = std::find_if(runtime.view().ambientActors.begin(), runtime.view().ambientActors.end(),
        [](const px::RuntimeActorView& actor){ return actor.id == "lost_competitor"; });
    assert(returnCompetitor != runtime.view().ambientActors.end());
    moveToward(runtime, input, returnCompetitor->position, 70.0f);
    tap(runtime, input, px::Action::Interact);
    assert(runtime.view().dialogueVisible && runtime.view().dialogueSpeaker == "LOST COMPETITOR");
    finishDialogue(runtime);
    assert(runtime.view().sceneId == "reach_tournament_checkpoint");
    assert(runtime.map().id == "training_region");
    moveToward(runtime, input, {915.0f, 0.0f}, 35.0f);
    finishDialogue(runtime);
    moveToward(runtime, input, {995.0f, 0.0f}, 35.0f);
    finishDialogue(runtime);
    assert(runtime.view().sceneId == "lens_roadblock_reveal");
    const float hpBeforeLens = runtime.view().player.hp;
    tap(runtime, input, px::Action::Ability3);
    assert(runtime.view().sceneId == "reach_tournament_outskirts");
    assert(runtime.view().player.hp == std::max(1.0f, hpBeforeLens - 1.0f));

    const auto painter = std::find_if(runtime.view().ambientActors.begin(), runtime.view().ambientActors.end(),
        [](const px::RuntimeActorView& actor){ return actor.id == "sign_painter"; });
    assert(painter != runtime.view().ambientActors.end());
    moveToward(runtime, input, painter->position, 70.0f);
    tap(runtime, input, px::Action::Interact);
    finishDialogue(runtime);
    assert(runtime.view().objectiveDetail.find("THE SIGN THAT POINTS BACK") != std::string::npos);
    moveToward(runtime, input, adventure.sidePuzzle.lensTarget, 45.0f);
    tap(runtime, input, px::Action::Ability3);
    finishDialogue(runtime);
    assert(runtime.view().objectiveDetail.find("OBJECT SWAP") != std::string::npos);
    moveToward(runtime, input, adventure.sidePuzzle.swapTarget, 45.0f);
    tap(runtime, input, px::Action::Ability2);
    finishDialogue(runtime);
    assert(std::any_of(runtime.view().worldMarkers.begin(), runtime.view().worldMarkers.end(),
        [](const px::RuntimeMarkerView& marker){ return marker.kind == "corrected-sign" && marker.complete; }));

    moveToward(runtime, input, {1280.0f, -10.0f}, 28.0f);
    finishDialogue(runtime);
    assert(runtime.view().chapterComplete);
    assert(runtime.view().nextChapterId == "rrvvfo_ch2");
    const auto save = px::SaveCodec::deserialize(px::SaveCodec::serialize(runtime.saveSnapshot("legacy")));
    assert(save.story.chapterId == "rrvvfo_ch1" && save.world.routeChoice == "main");
    assert(std::find(save.story.flags.begin(), save.story.flags.end(), "ch1_complete_at_outskirts") != save.story.flags.end());
    assert(std::find(save.story.flags.begin(), save.story.flags.end(), "ch1_transport_rescued") != save.story.flags.end());
    assert(std::find(save.story.flags.begin(), save.story.flags.end(), "ch1_precision_swap_mastered") != save.story.flags.end());
    assert(std::find(save.story.flags.begin(), save.story.flags.end(), "ch1_cart_perfect_intercept") != save.story.flags.end());
    assert(std::find(save.story.flags.begin(), save.story.flags.end(), "ch1_spectator_pass_delivered") != save.story.flags.end());
    assert(std::find(save.story.flags.begin(), save.story.flags.end(), "ch1_sign_that_points_back_complete") != save.story.flags.end());
    assert(std::find(save.story.flags.begin(), save.story.flags.end(), "ch1_wayfinder_badge") != save.story.flags.end());

    // The Legacy route panel selects one of three routes; each route remains physical gameplay afterward.
    for (const std::string route : {"forest", "cliff"}) {
        px::RuntimeSession alternate(chapters, maps, cutscenes, dialogue, exploration, training, adventures);
        px::InputState alternateInput;
        alternate.startChapter("rrvvfo_ch1");
        reachRouteChoice(alternate, alternateInput);
        tap(alternate, alternateInput, px::Action::MoveRight);
        if (route == "cliff") tap(alternate, alternateInput, px::Action::MoveRight);
        tap(alternate, alternateInput, px::Action::Confirm);
        assert(alternate.view().routeChoice == route && alternate.view().sceneId == "selected_route_adventure");
        if (route == "cliff") {
            const auto beforeClear = alternate.saveSnapshot();
            assert(std::find(beforeClear.story.flags.begin(), beforeClear.story.flags.end(), "ch1_road_dare_badge") == beforeClear.story.flags.end());
            for (const auto marker : exploration.get("selected_route_adventure").jumpMarkers) {
                moveToward(alternate, alternateInput, marker, 48.0f);
                tap(alternate, alternateInput, px::Action::Jump, 0.05f);
            }
        }
        const px::Vec2 finish = route == "forest" ? px::Vec2{430.0f, -390.0f} : px::Vec2{430.0f, 390.0f};
        moveToward(alternate, alternateInput, finish, 55.0f);
        assert(alternate.view().sceneId == "swap_relay_trial");
        if (route == "cliff") {
            assert(alternate.view().gameplayNotice == "SCENIC DISCOVERY • CLIFFSIDE VIEW");
            const auto cliffSave = alternate.saveSnapshot();
            assert(std::find(cliffSave.story.flags.begin(), cliffSave.story.flags.end(), "ch1_road_dare_badge") != cliffSave.story.flags.end());
            assert(std::find(cliffSave.story.flags.begin(), cliffSave.story.flags.end(), "ch1_title_road_runner") != cliffSave.story.flags.end());
        }
    }

    std::cout << "PASS: Chapter 1 Legacy story/tutorial/UI structure, route panel plus all three routes, "
                 "NPC/QTE/optional encounter flow, save/reload, canonical Chapter 2 boundary, shared combat, "
                 "progression/quest foundations, and portable presentation data\n";
    return 0;
}
