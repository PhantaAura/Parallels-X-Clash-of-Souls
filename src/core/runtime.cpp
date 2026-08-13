#include "core/runtime.hpp"
#include "core/field_movement.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace px {

namespace {

constexpr float kRrvvfoInputBufferSeconds = 0.135f; // Legacy 2.9A.40.7.1.1 fighter feel profile.
constexpr float kFeedbackStepSeconds = 1.0f / 60.0f;
constexpr float kDashBufferSeconds = 0.135f;
constexpr float kPi = 3.14159265358979323846f;
constexpr float kExplorationTurnDegreesPerSecond = 1320.0f; // U5: snappier exploration turnaround without changing combat facing.
constexpr float kCombatTurnDegreesPerSecond = 1440.0f;

float wrapDegrees(float degrees) {
    degrees = std::fmod(degrees + 180.0f, 360.0f);
    if (degrees < 0.0f) degrees += 360.0f;
    return degrees - 180.0f;
}

float yawForDirection(Vec2 direction) {
    // Rrvvfo's imported model faces local +Z. The renderer maps that to
    // (-sin(yaw), cos(yaw)) in world X/Z.
    return std::atan2(-direction.x, direction.z) * 180.0f / kPi;
}

float turnTowardDegrees(float current, float target, float maximumStep) {
    const float difference = wrapDegrees(target - current);
    return wrapDegrees(current + std::clamp(difference, -maximumStep, maximumStep));
}

Vec2 forwardForYaw(float yawDegrees) {
    const float radians = yawDegrees * kPi / 180.0f;
    return {-std::sin(radians), std::cos(radians)};
}

bool containsFlag(const std::vector<std::string>& flags, const std::string& value) {
    return std::find(flags.begin(), flags.end(), value) != flags.end();
}

void addUnique(std::vector<std::string>& values, const std::string& value) {
    if (std::find(values.begin(), values.end(), value) == values.end()) values.push_back(value);
}

bool withinAbilityTarget(Vec2 player, Vec2 target, float authoredRadius) {
    // Small input-device forgiveness around an authored Legacy target. It never
    // changes the target or solves the route; it only accepts the aim the player
    // is visibly trying to make.
    const float forgiveness = std::clamp(authoredRadius * .12f, 18.0f, 50.0f);
    return distance(player, target) <= authoredRadius + forgiveness;
}

std::size_t savedTutorialStep(const std::vector<std::string>& flags) {
    constexpr const char* prefix = "ch1_tutorial_checkpoint=";
    for (const auto& flag : flags) {
        if (flag.rfind(prefix, 0) != 0) continue;
        const auto value = static_cast<std::size_t>(std::stoul(flag.substr(std::char_traits<char>::length(prefix))));
        return std::min<std::size_t>(value, 6);
    }
    return 0;
}

std::size_t savedUnsignedFlag(const std::vector<std::string>& flags, const char* prefix, std::size_t maximum = 64) {
    const auto prefixLength = std::char_traits<char>::length(prefix);
    for (const auto& flag : flags) {
        if (flag.rfind(prefix, 0) != 0) continue;
        try { return std::min<std::size_t>(static_cast<std::size_t>(std::stoul(flag.substr(prefixLength))), maximum); }
        catch (...) { return 0; }
    }
    return 0;
}

void updateTournamentProgressView(RuntimeView& view, std::size_t phaseIndex) {
    view.tournamentPhase.clear();
    view.nextTournamentMatch.clear();
    view.bracketSummary.clear();
    if (view.chapterId != "rrvvfo_ch2") return;
    if (phaseIndex <= 2) {
        view.tournamentPhase = "ARRIVAL & REGISTRATION DELAY";
        view.nextTournamentMatch = "Recover the Lost Bracket";
        view.bracketSummary = "Registration paused • bracket incomplete";
    } else if (phaseIndex <= 8) {
        view.tournamentPhase = "PRACTICE & REGISTRATION";
        view.nextTournamentMatch = phaseIndex < 4 ? "Practice Ring Brawl" : "Opening Ceremony";
        view.bracketSummary = "Rrvvfo • Wade • Bark registered";
    } else if (phaseIndex <= 11) {
        view.tournamentPhase = "PRELIMINARIES";
        view.nextTournamentMatch = phaseIndex < 11 ? "Hailey vs Plouke" : "Rrvvfo vs Hamual";
        view.bracketSummary = "Plouke advances • first round active";
    } else if (phaseIndex <= 14) {
        view.tournamentPhase = "EARLY BRACKET";
        view.nextTournamentMatch = phaseIndex < 13 ? "Observe Plouke's stillness" : "Rrvvfo vs Daniel";
        view.bracketSummary = "Rrvvfo advances • Hamual eliminated";
    } else if (phaseIndex <= 17) {
        view.tournamentPhase = "MID BRACKET";
        view.nextTournamentMatch = phaseIndex < 16 ? "Bark vs Pouki" : "Rrvvfo vs Wade";
        view.bracketSummary = "Daniel eliminated • Pouki advances";
    } else if (phaseIndex <= 19) {
        view.tournamentPhase = "SEMIFINAL INTERMISSION";
        view.nextTournamentMatch = "Rrvvfo vs Plouke";
        view.bracketSummary = "Rrvvfo advances • final clues available";
    } else if (phaseIndex <= 21) {
        view.tournamentPhase = "TOURNAMENT FINAL";
        view.nextTournamentMatch = phaseIndex == 20 ? "Enter the final ring" : "Final result";
        view.bracketSummary = "Rrvvfo vs Plouke • championship match";
    } else {
        view.tournamentPhase = "AFTERMATH & CLEANUP";
        view.nextTournamentMatch = "Tournament complete";
        view.bracketSummary = "Plouke wins • investigation continues";
    }
}

} // namespace

RuntimeSession::RuntimeSession(const ChapterRegistry& chapters,
                               const MapRegistry& maps,
                               const CutsceneRegistry& cutscenes,
                               const DialogueRegistry& dialogue,
                               const ExplorationRegistry& exploration,
                               const TrainingRegistry& training,
                               const AdventureRegistry& adventures)
    : chapters_(chapters), maps_(maps), cutscenes_(cutscenes), dialogue_(dialogue), exploration_(exploration),
      training_(training), adventures_(adventures), game_(chapters) {}

void RuntimeSession::startChapter(const std::string& chapterId) {
    standaloneFightMode_ = false;
    standaloneTrainingMode_ = false;
    replayMode_ = false;
    recentDialogue_.clear();
    lastRecordedDialogueKey_.clear();
    seenSceneSkipHoldTime_ = 0.0f;
    adventureRecords_ = {};
    rpgProgress_ = {};
    battleRankTime_ = 0.0f;
    pendingRankDialogueId_.clear();
    pendingRankDialogueContinuation_ = pendingRankChoiceKind_ = 0;
    storyPlaytimeSeconds_ = 0.0f;
    userCameraYawOffset_ = userCameraHeightOffset_ = 0.0f;
    game_.startChapter(chapterId);
    player_ = FighterState{game_.chapter().playableCharacter};
    opponent_ = FighterState{"sage"};
    playerPosition_ = map().playerStart;
    routeChoice_.clear();
    clearCombatInputBuffer();
    gameplayNotice_.clear();
    gameplayNoticeTime_ = 0.0f;
    hitFreezeTime_ = 0.0f;
    cameraImpulse_ = 0.0f;
    impactFlash_ = 0.0f;
    combatFeedbackTime_ = 0.0f;
    combatFeedback_.clear();
    perfectBlockFlash_ = pursuitFinishFlash_ = guardBreakFlash_ = finalHitFlash_ = false;
    bufferedDash_ = false;
    bufferedDashTime_ = 0.0f;
    playerActionAnimation_.clear();
    playerActionAnimationTime_ = 0.0f;
    landingAnimationTime_ = 0.0f;
    combatReadyAnimationTime_ = 0.0f;
    explorationRunStartAnimationTime_ = 0.0f;
    explorationRunStopAnimationTime_ = 0.0f;
    explorationWasMoving_ = false;
    hardLanding_ = false;
    playerCharging_ = false;
    pureEnergyBeamActive_ = false;
    fireAwakeningUnlocked_ = false;
    fireAwakeningTime_ = 0.0f;
    flowCancelLearned_ = false;
    cliffRouteHintLevel_ = 0;
    routeHintStage_ = 0;
    routeProgress_ = 0;
    routeChallengeTime_ = 0.0f;
    mainRouteFireCleared_ = false;
    lensRouteChosen_ = false;
    southernDetourChosen_ = false;
    southernDetourComplete_ = false;
    terrainCollapseSeen_ = false;
    detourDashDone_ = false;
    detourSwapDone_ = false;
    disabledBlockers_.clear();
    setTerrainCollapseActive(false);
    setDetourBlockersActive(false);
    if (terrainCollapseSeen_) setTerrainCollapseActive(true);
    relayMarkers_.clear();
    relayIndex_ = 0;
    transientDialogueId_.clear();
    choiceKind_ = 0;
    choiceIndex_ = 0;
    lostCompetitorHelped_ = false;
    lostCompetitorDeclined_ = false;
    roadsideEncounterResolved_ = false;
    spectatorPassWon_ = false;
    spectatorPassDelivered_ = false;
    tutorialSkipped_ = false;
    signPuzzleStage_ = 0;
    transportRescued_ = false;
    runawayCartSaved_ = false;
    precisionSwapMastered_ = false;
    cliffRewardEarned_ = false;
    wadeRaceBestSeconds_ = 0.0f;
    wadeRaceTargetBeaten_ = false;
    crackedRingIntroSeen_ = false;
    wadeLostFanStarted_ = wadeLostFanFound_ = wadeLostFanComplete_ = false;
    fakeChampionContacted_ = fakeChampionRevealed_ = fakeChampionComplete_ = false;
    runawayDummyComplete_ = false;
    festivalFoodComplete_ = festivalPhotoComplete_ = false;
    missingPrizeStarted_ = missingPrizeFound_ = missingPrizeComplete_ = false;
    oneMatchComplete_ = false;
    controlledFlameStarted_ = controlledFlameComplete_ = false;
    altRoverQuestStage_ = 0;
    barkPracticeWins_ = 0;
    activeOptionalEncounterId_.clear();
    roadsideFightActive_ = false;
    roadsideFightIntroShown_ = false;
    roadsideReturnPosition_ = {};
    seenCutscenes_.clear();
    roadsideAiDecisionIndex_ = 0;
    opponentHeight_ = 0.0f;
    opponentVerticalVelocity_ = 0.0f;
    combatLensTimer_ = 0.0f;
    qteActive_ = false;
    qteAttempts_ = 0;
    adventureTime_ = 0.0f;
    cliffJumpComplete_.clear();
    farBankRockPosition_ = {230.0f, 0.0f};
freeSwapMapId_.clear();
freeSwapObjects_.clear();
objectSwapCooldownTime_ = 0.0f;
objectSwapPhaseTime_ = 0.0f;
lensBlindnessDelay_ = 0.0f;
lensBlindnessTime_ = 0.0f;
    relayReleaseTimer_ = 0.0f;
    mainRouteDialogueShown_ = false;
    mainRouteReady_ = false;
    chapterComplete_ = false;
    objectiveHistory_.clear();
    // QoL belongs to the player, not the chapter. Preserve it when Story/Fight/Training restarts.
    pauseSelection_ = 0;
    pausePage_ = 0;
    manualSaveRequested_ = false;
    returnToTitleRequested_ = false;
    saveStatus_.clear();
    saveStatusTime_ = 0.0f;
    dialogueHoldTime_ = 0.0f;
    trainingCheckpointStep_ = 0;
    trainingManualSelection_ = 0;
    trainingManualPageIndex_ = 0;
    enterCurrentScene();
}

void RuntimeSession::startReplayChapter(const SaveData& source) {
    const auto sourceQol = source.qol;
    const auto sourceCard = source.tournamentCard;
    const auto sourceRecords = source.records;
    const auto sourceRpg = source.rpg;
    std::vector<std::string> sourceSeen;
    constexpr const char* seenPrefix = "seen_ch1_scene=";
    constexpr const char* genericSeenPrefix = "seen_scene=";
    for (const auto& flag : source.story.flags) {
        if (flag.rfind(seenPrefix, 0) == 0)
            sourceSeen.push_back(flag.substr(std::char_traits<char>::length(seenPrefix)));
        else if (flag.rfind(genericSeenPrefix, 0) == 0)
            sourceSeen.push_back(flag.substr(std::char_traits<char>::length(genericSeenPrefix)));
    }
    startChapter("rrvvfo_ch1");
    replayMode_ = true;
    qolSettings_ = sourceQol;
    tournamentCard_ = sourceCard;
    adventureRecords_ = sourceRecords;
    rpgProgress_ = sourceRpg;
    seenCutscenes_ = std::move(sourceSeen);
    showGameplayNotice("STORY REPLAY • MAIN SAVE PROTECTED", 1.65f);
    syncView();
}

void RuntimeSession::startCpuFight() {
    startChapter("rrvvfo_ch1");
    const auto& flow = chapters_.get("rrvvfo_ch1").openingFlow;
    const auto scene = std::find_if(flow.begin(), flow.end(), [](const SceneStep& step) {
        return step.id == "roadside_encounter";
    });
    if (scene == flow.end()) throw std::runtime_error("Chapter 1 roadside fight is unavailable");
    auto direct = game_.saveData();
    direct.story.sceneIndex = static_cast<std::size_t>(std::distance(flow.begin(), scene));
    direct.story.checkpointId = scene->checkpointId;
    game_.loadSave(direct);
    standaloneFightMode_ = true;
    enterCurrentScene();
    startRoadsideFight();
    showGameplayNotice("FIGHT • RRVVFO VS CPU", 1.65f);
    syncView();
}

void RuntimeSession::startStandaloneTraining() {
    startChapter("rrvvfo_ch1");
    const auto& flow = chapters_.get("rrvvfo_ch1").openingFlow;
    const auto scene = std::find_if(flow.begin(), flow.end(), [](const SceneStep& step) {
        return step.id == "sage_tutorial_spar";
    });
    if (scene == flow.end()) throw std::runtime_error("Chapter 1 Sage training is unavailable");
    auto direct = game_.saveData();
    direct.story.sceneIndex = static_cast<std::size_t>(std::distance(flow.begin(), scene));
    direct.story.checkpointId = scene->checkpointId;
    game_.loadSave(direct);
    standaloneTrainingMode_ = true;
    enterCurrentScene();
    syncView();
}

void RuntimeSession::loadSnapshot(const SaveData& data) {
    standaloneFightMode_ = false;
    standaloneTrainingMode_ = false;
    replayMode_ = false;
    recentDialogue_.clear();
    lastRecordedDialogueKey_.clear();
    seenSceneSkipHoldTime_ = 0.0f;
    game_.loadSave(data);
    tournamentCard_ = data.tournamentCard;
    adventureRecords_ = data.records;
    rpgProgress_ = data.rpg;
    storyPlaytimeSeconds_ = data.frontend.playtimeSeconds;
    battleRankTime_ = 0.0f;
    pendingRankDialogueId_.clear();
    pendingRankDialogueContinuation_ = pendingRankChoiceKind_ = 0;
    userCameraYawOffset_ = userCameraHeightOffset_ = 0.0f;
    player_ = FighterState{game_.chapter().playableCharacter};
    opponent_ = FighterState{"sage"};
    clearCombatInputBuffer();
    gameplayNotice_.clear();
    gameplayNoticeTime_ = 0.0f;
    hitFreezeTime_ = 0.0f;
    cameraImpulse_ = 0.0f;
    impactFlash_ = 0.0f;
    combatFeedbackTime_ = 0.0f;
    combatFeedback_.clear();
    perfectBlockFlash_ = pursuitFinishFlash_ = guardBreakFlash_ = finalHitFlash_ = false;
    bufferedDash_ = false;
    bufferedDashTime_ = 0.0f;
    playerActionAnimation_.clear();
    playerActionAnimationTime_ = 0.0f;
    landingAnimationTime_ = 0.0f;
    combatReadyAnimationTime_ = 0.0f;
    explorationRunStartAnimationTime_ = 0.0f;
    explorationRunStopAnimationTime_ = 0.0f;
    explorationWasMoving_ = false;
    hardLanding_ = false;
    playerCharging_ = false;
    cliffRouteHintLevel_ = 0;
    routeChoice_ = data.world.routeChoice;
    seenCutscenes_.clear();
    constexpr const char* seenPrefix = "seen_ch1_scene=";
    for (const auto& flag : data.story.flags)
        if (flag.rfind(seenPrefix, 0) == 0) seenCutscenes_.push_back(flag.substr(std::char_traits<char>::length(seenPrefix)));
    constexpr const char* genericSeenPrefix = "seen_scene=";
    for (const auto& flag : data.story.flags) {
        if (flag.rfind(genericSeenPrefix, 0) != 0) continue;
        const auto id = flag.substr(std::char_traits<char>::length(genericSeenPrefix));
        if (std::find(seenCutscenes_.begin(), seenCutscenes_.end(), id) == seenCutscenes_.end()) seenCutscenes_.push_back(id);
    }
    trainingCheckpointStep_ = savedTutorialStep(data.story.flags);
    lostCompetitorHelped_ = containsFlag(data.story.flags, "ch1_lost_competitor_helped");
    lostCompetitorDeclined_ = containsFlag(data.story.flags, "ch1_lost_competitor_declined");
    roadsideEncounterResolved_ = containsFlag(data.story.flags, "ch1_roadside_encounter_resolved");
    spectatorPassWon_ = containsFlag(data.story.flags, "ch1_spectator_pass_won");
    const bool spectatorPassPending = containsFlag(data.story.flags, "ch1_spectator_pass_pending");
    spectatorPassDelivered_ = containsFlag(data.story.flags, "ch1_spectator_pass_delivered") ||
                              (!spectatorPassPending && roadsideEncounterResolved_ && spectatorPassWon_);
    tutorialSkipped_ = containsFlag(data.story.flags, "ch1_tutorial_skipped");
    signPuzzleStage_ = containsFlag(data.story.flags, "ch1_sign_that_points_back_complete") ? 3 :
                       containsFlag(data.story.flags, "ch1_sign_that_points_back_revealed") ? 2 :
                       containsFlag(data.story.flags, "ch1_sign_that_points_back_started") ? 1 : 0;
    transportRescued_ = containsFlag(data.story.flags, "ch1_transport_rescued");
    runawayCartSaved_ = containsFlag(data.story.flags, "ch1_cart_perfect_intercept") ||
                        containsFlag(data.story.flags, "ch1_cart_supplies_saved");
    qteAttempts_ = containsFlag(data.story.flags, "ch1_cart_supplies_saved") ? 2 :
                   containsFlag(data.story.flags, "ch1_cart_perfect_intercept") ? 1 : 0;
    precisionSwapMastered_ = containsFlag(data.story.flags, "ch1_precision_swap_mastered");
    cliffRewardEarned_ = containsFlag(data.story.flags, "ch1_road_dare_badge");
    wadeRaceBestSeconds_ = static_cast<float>(savedUnsignedFlag(data.story.flags, "ch2_wade_race_best_ms=", 3600000)) / 1000.0f;
    wadeRaceTargetBeaten_ = containsFlag(data.story.flags, "ch2_wade_race_target_beaten");
    crackedRingIntroSeen_ = containsFlag(data.story.flags, "ch2_cracked_ring_intro_seen");
    wadeLostFanStarted_ = containsFlag(data.story.flags, "ch2_wade_lost_fan_started");
    wadeLostFanFound_ = containsFlag(data.story.flags, "ch2_wade_lost_fan_found");
    wadeLostFanComplete_ = containsFlag(data.story.flags, "ch2_wade_lost_fan_complete");
    fakeChampionContacted_ = containsFlag(data.story.flags, "ch2_fake_champion_contacted");
    fakeChampionRevealed_ = containsFlag(data.story.flags, "ch2_fake_champion_revealed");
    fakeChampionComplete_ = containsFlag(data.story.flags, "ch2_fake_champion_complete");
    runawayDummyComplete_ = containsFlag(data.story.flags, "ch2_runaway_dummy_complete");
    festivalFoodComplete_ = containsFlag(data.story.flags, "ch2_festival_food_complete");
    festivalPhotoComplete_ = containsFlag(data.story.flags, "ch2_festival_photo_complete");
    missingPrizeStarted_ = containsFlag(data.story.flags, "ch2_missing_prize_started");
    missingPrizeFound_ = containsFlag(data.story.flags, "ch2_missing_prize_found");
    missingPrizeComplete_ = containsFlag(data.story.flags, "ch2_missing_prize_complete");
    oneMatchComplete_ = containsFlag(data.story.flags, "ch2_one_match_anyway_complete");
    controlledFlameStarted_ = containsFlag(data.story.flags, "ch2_controlled_flame_started");
    controlledFlameComplete_ = containsFlag(data.story.flags, "ch2_controlled_flame_complete");
    altRoverQuestStage_ = static_cast<int>(savedUnsignedFlag(data.story.flags, "ch2_alt_rover_stage=", 6));
    barkPracticeWins_ = static_cast<int>(savedUnsignedFlag(data.story.flags, "ch2_bark_practice_wins=", 999));
    fireAwakeningUnlocked_ = containsFlag(data.story.flags, "rrvvfo_fire_awakening_unlocked") ||
        data.story.pendingChapterId == "rrvvfo_ch3";
    fireAwakeningTime_ = 0.0f;
    pureEnergyBeamActive_ = false;
    activeOptionalEncounterId_.clear();
    flowCancelLearned_ = containsFlag(data.story.flags, "ch1_flow_cancel_learned");
    mainRouteFireCleared_ = containsFlag(data.story.flags, "ch1_main_fire_cleared");
    lensRouteChosen_ = containsFlag(data.story.flags, "ch1_lens_route_chosen");
    southernDetourChosen_ = containsFlag(data.story.flags, "ch1_southern_detour_chosen");
    southernDetourComplete_ = containsFlag(data.story.flags, "ch1_southern_detour_complete");
    terrainCollapseSeen_ = containsFlag(data.story.flags, "ch1_terrain_collapse_seen");
    detourDashDone_ = containsFlag(data.story.flags, "ch1_detour_dash_done");
    detourSwapDone_ = containsFlag(data.story.flags, "ch1_detour_swap_done");
    const bool migratedPastNewDetour = !terrainCollapseSeen_ && game_.story().sceneIndex > 15;
    if (migratedPastNewDetour) terrainCollapseSeen_ = true;
    chapterComplete_ = (game_.story().chapterId == "rrvvfo_ch1" && containsFlag(data.story.flags, "ch1_complete_at_outskirts")) ||
                       (game_.story().chapterId == "rrvvfo_ch2" && data.story.pendingChapterId == "rrvvfo_ch3");
    objectiveHistory_ = data.frontend.objectiveHistory;
    qolSettings_ = data.qol;
    pauseSelection_ = 0;
    pausePage_ = 0;
    manualSaveRequested_ = false;
    returnToTitleRequested_ = false;
    saveStatus_.clear();
    saveStatusTime_ = 0.0f;
    dialogueHoldTime_ = 0.0f;
    disabledBlockers_.clear();
    setTerrainCollapseActive(false);
    setDetourBlockersActive(false);
    if (terrainCollapseSeen_) setTerrainCollapseActive(true);
    if (!routeChoice_.empty()) {
        if (routeChoice_ == "main") { disableBlocker("fallen_tree_north"); disableBlocker("fallen_tree_south"); }
        if (routeChoice_ == "forest") disableBlocker("fallen_tree_north");
        if (routeChoice_ == "cliff") disableBlocker("fallen_tree_south");
    }
    if (precisionSwapMastered_) {
        disableBlocker("swap_gate"); disableBlocker("swap_gate_forest"); disableBlocker("swap_gate_cliff");
    }
    if (game_.story().sceneIndex > 10) {
        disableBlocker("fallen_tree_center"); disableBlocker("fallen_tree_north"); disableBlocker("fallen_tree_south");
    }
    if (game_.story().sceneIndex > 11) {
        disableBlocker("swap_gate"); disableBlocker("swap_gate_forest"); disableBlocker("swap_gate_cliff");
    }
    if (game_.story().sceneIndex > 20) {
        disableBlocker("lens_roadblock"); disableBlocker("lens_roadblock_north"); disableBlocker("lens_roadblock_south");
    }
    farBankRockPosition_ = {230.0f, 0.0f};
    const auto rock = std::find_if(data.world.objects.begin(), data.world.objects.end(), [](const WorldObjectState& object){
        return object.id == "far_bank_rock";
    });
    if (rock != data.world.objects.end()) farBankRockPosition_ = rock->position;
    enterCurrentScene();
refreshFreeSwapObjects();
for (auto& object : freeSwapObjects_) {
    const auto savedObject = std::find_if(data.world.objects.begin(), data.world.objects.end(), [&](const WorldObjectState& saved){ return saved.id == object.id; });
    if (savedObject != data.world.objects.end()) object = *savedObject;
}
    const std::string loadedSceneProgressPrefix = "scene_progress=" + game_.story().chapterId + ":" + game_.scene().id + ":";
    sceneSequenceProgress_ = savedUnsignedFlag(data.story.flags, loadedSceneProgressPrefix.c_str(), 128);
    routeProgress_ = savedUnsignedFlag(data.story.flags, "ch1_route_progress=", 8);
    const auto cliffMask = savedUnsignedFlag(data.story.flags, "ch1_cliff_mask=", 31);
    if (!cliffJumpComplete_.empty()) {
        for (std::size_t i = 0; i < cliffJumpComplete_.size(); ++i) cliffJumpComplete_[i] = (cliffMask & (1u << i)) != 0;
    }
    if (game_.scene().id == "selected_route_adventure" && routeChoice_ == "main" && (mainRouteFireCleared_ || routeProgress_ > 0)) {
        mainRouteDialogueShown_ = true; mainRouteReady_ = true;
        if (mainRouteFireCleared_) disableBlocker("fallen_tree_center");
    }
    if (game_.scene().id == "collapsed_tournament_road_detour") {
        for (std::size_t i=0; i<routeProgress_ && i<3; ++i) disableBlocker("detour_jump_gate_" + std::to_string(i+1));
        if (detourDashDone_) disableBlocker("detour_dash_gate");
        if (detourSwapDone_) disableBlocker("detour_swap_gate");
    }
    if (exploration_.has(game_.scene().id) && exploration_.get(game_.scene().id).rule == ExplorationRuleKind::SwapRelay) {
        relayIndex_ = static_cast<int>(savedUnsignedFlag(data.story.flags, "ch1_relay_index=", relayMarkers_.size()));
        for (std::size_t i = 0; i < relayMarkers_.size(); ++i) {
            const auto id = "relay_marker_" + std::to_string(i);
            const auto savedMarker = std::find_if(data.world.objects.begin(), data.world.objects.end(), [&](const WorldObjectState& object){ return object.id == id; });
            if (savedMarker != data.world.objects.end()) relayMarkers_[i] = savedMarker->position;
        }
        if (game_.scene().id == "swap_relay_trial" && relayIndex_ >= static_cast<int>(relayMarkers_.size())) relayReleaseTimer_ = 0.01f;
    }
    playerPosition_ = data.world.position;
    if (migratedPastNewDetour) {
        const auto& migratedSceneId = game_.scene().id;
        if (migratedSceneId == "reach_tournament_checkpoint" && playerPosition_.x < 1340.0f) playerPosition_ = {1340.0f, -70.0f};
        else if (migratedSceneId == "reach_lens_roadblock" && playerPosition_.x < 1530.0f) playerPosition_ = {1530.0f, 0.0f};
        else if (migratedSceneId == "lens_roadblock_reveal" && playerPosition_.x < 1600.0f) playerPosition_ = {1600.0f, 0.0f};
        else if (migratedSceneId == "reach_tournament_outskirts" && playerPosition_.x < 1780.0f) playerPosition_ = {1780.0f, 0.0f};
    }
    player_.hp = std::clamp(data.world.hp, 1.0f, player_.maxHp);
    player_.energy = std::clamp(data.world.energy, 0.0f, 100.0f);
    player_.guard = std::clamp(data.world.guard, 0.0f, 100.0f);
    if (training_.has(game_.scene().id)) {
        beginTrainingAt(trainingCheckpointStep_);
        trainingManualVisible_ = true;
        trainingManualSelection_ = trainingCheckpointStep_ == 0 ? 0 : 1;
    }
    syncView();
    // A loaded manual save is itself a valid restart point for this session.
    sceneCheckpointSnapshot_ = saveSnapshot(data.inputPreset);
    sceneCheckpointValid_ = true;
}

const MapDefinition& RuntimeSession::map() const {
    if (!activeOptionalEncounterId_.empty()) return maps_.get(arenaEncounters_.get(activeOptionalEncounterId_).mapId);
    if (roadsideFightActive_) return maps_.get("roadside_arena");
    if (game_.scene().kind == SceneKind::Arena) {
        if (arenaEncounters_.has(game_.scene().id)) return maps_.get(arenaEncounters_.get(game_.scene().id).mapId);
        return maps_.get("sage_training_arena");
    }
    return maps_.get(game_.chapter().primaryMap);
}

const TrainingStepDefinition* RuntimeSession::currentTrainingStep() const {
    const auto& scene = game_.scene();
    if (!training_.has(scene.id)) return nullptr;
    const auto& steps = training_.get(scene.id).steps;
    return trainingStepIndex_ < steps.size() ? &steps[trainingStepIndex_] : nullptr;
}

void RuntimeSession::enterCurrentScene() {
    dialogueIndex_ = 0;
    sceneComplete_ = false;
    sceneSequenceProgress_ = 0;
    sceneSequenceSeconds_ = 0.0f;
    sceneSequenceStarted_ = false;
    attackCooldown_ = 0.0f;
    sparCleanHits_ = 0;
    blockHeld_ = false;
    movementState_ = {};
    trainingManualVisible_ = false;
    trainingManualSelection_ = 0;
    trainingManualPageIndex_ = 0;
    trainingStepIndex_ = 0;
    trainingProgress_ = 0;
    trainingSignals_ = 0;
    opponentAttackTimer_ = 0.0f;
    opponentAttackAttempts_ = 0;
    opponentAttackTelegraphed_ = false;
    trainingTimedDefenseArmed_ = false;
    lensActive_ = false;
    trainingEvadeAttempt_ = false;
    lensTrialActive_ = false;
    lensTrialTimer_ = 0.0f;
    lensTrialHp_ = 100.0f;
    trainingMovementDistance_ = 0.0f;
    cutsceneOpponentVisible_ = false;
    cutsceneActors_.clear();
cutsceneBeatTime_ = 0.0f;
cutsceneBeatIndex_ = static_cast<std::size_t>(-1);
cinematicCameraActive_ = false;
cinematicCameraBlend_ = 0.0f;
cinematicCameraReturnTime_ = 0.0f;
cinematicCameraFocus_ = {};
cinematicCameraYawDegrees_ = 38.0f;
cinematicCameraDistance_ = 900.0f;
cinematicCameraHeight_ = 410.0f;
cinematicCameraFovDegrees_ = 43.0f;
cinematicExpression_.clear();
cinematicWorldEvent_.clear();
    transientDialogueId_.clear();
    transientDialogueIndex_ = 0;
    transientDialogueContinuation_ = 0;
    choiceKind_ = 0;
    qteActive_ = false;
    qteId_.clear();
    qteIndex_ = 0;
    qteRemaining_ = 0.0f;
    roadsideFightActive_ = false;
    combatLensTimer_ = 0.0f;
    hitFreezeTime_ = 0.0f;
    cameraImpulse_ = 0.0f;
    impactFlash_ = 0.0f;
    combatFeedbackTime_ = 0.0f;
    combatFeedback_.clear();
    perfectBlockFlash_ = pursuitFinishFlash_ = guardBreakFlash_ = finalHitFlash_ = false;
    bufferedDash_ = false;
    bufferedDashTime_ = 0.0f;
    playerActionAnimation_.clear();
    playerActionAnimationTime_ = 0.0f;
    landingAnimationTime_ = 0.0f;
    combatReadyAnimationTime_ = 0.0f;
    explorationRunStartAnimationTime_ = 0.0f;
    explorationRunStopAnimationTime_ = 0.0f;
    explorationWasMoving_ = false;
    hardLanding_ = false;
    playerCharging_ = false;

    const auto& scene = game_.scene();
    if (scene.kind == SceneKind::Cutscene && cutscenes_.has(scene.id)) {
        for (const auto& actor : cutscenes_.get(scene.id).staging) {
            if (actor.actorId == game_.chapter().playableCharacter || actor.actorId == "rrvvfo") {
                playerPosition_ = actor.position;
                playerYawDegrees_ = actor.yawDegrees;
            } else if (actor.actorId == "sage") {
                opponentPosition_ = actor.position;
                opponentYawDegrees_ = actor.yawDegrees;
                cutsceneOpponentVisible_ = actor.visible;
            } else if (actor.visible) cutsceneActors_.push_back({actor.actorId, actor.actorId, actor.position, actor.yawDegrees, false, "idle", true});
        }
    }
    if (scene.kind == SceneKind::Arena) {
    if (arenaEncounters_.has(scene.id)) {
        startStoryArena();
    } else {
        player_ = FighterState{game_.chapter().playableCharacter};
        opponent_ = FighterState{"sage"};
        trainingManualVisible_ = training_.has(scene.id);
        if (trainingManualVisible_) {
            const auto& definition = training_.get(scene.id);
            playerPosition_ = definition.playerStart;
            opponentPosition_ = definition.opponentStart;
            updateCombatFacing(0.0f, true);
            player_.energy = 45.0f;
            beginTrainingAt(trainingCheckpointStep_);
        }
    }
}
if (scene.kind == SceneKind::Exploration && exploration_.has(scene.id)) {
        const auto& definition = exploration_.get(scene.id);
        if (definition.hasPlayerStart) playerPosition_ = definition.playerStart;
        if (definition.companionVisible) opponentPosition_ = definition.companionPosition;
        if (definition.rule == ExplorationRuleKind::SwapRelay) {
            relayMarkers_ = definition.relayMarkers;
            relayIndex_ = 0;
        }
        if (definition.rule == ExplorationRuleKind::ChooseRoute) {
            choiceKind_ = 0;
            choiceIndex_ = 0;
            routeChoiceIntroTime_ = 0.85f;
        }
        if (scene.id == "selected_route_adventure") {
            mainRouteDialogueShown_ = false;
            mainRouteReady_ = routeChoice_ != "main";
            routeProgress_ = 0; routeChallengeTime_ = 0.0f; routeHintStage_ = 0;
            cliffJumpComplete_.assign(definition.jumpMarkers.size(), false);
        }
        if (scene.id == "collapsed_tournament_road_detour") {
            routeProgress_ = 0;
            cliffJumpComplete_.assign(definition.jumpMarkers.size(), false);
            setTerrainCollapseActive(true);
            setDetourBlockersActive(true);
            if (!terrainCollapseSeen_ && !definition.openingDialogueId.empty()) beginTransientDialogue(definition.openingDialogueId, 30);
        }
        if (scene.id == "lens_roadblock_reveal" && !lensRouteChosen_ && !southernDetourChosen_ && !southernDetourComplete_) {
            choiceKind_ = 5; choiceIndex_ = 0;
        }
        if (definition.rule == ExplorationRuleKind::QteSequence && !definition.openingDialogueId.empty()) {
            beginTransientDialogue(definition.openingDialogueId, 5);
        }
        if (scene.id == "ch2_cracked_ring" && !crackedRingIntroSeen_) {
            crackedRingIntroSeen_ = true;
            beginTransientDialogue("ch2_cracked_ring_intro");
        }
        if (game_.story().chapterId == "rrvvfo_ch2" && scene.id.rfind("ch2_intermission_", 0) == 0) {
            RuntimeView progress;
            progress.chapterId = "rrvvfo_ch2";
            updateTournamentProgressView(progress, game_.story().sceneIndex);
            showGameplayNotice("ANNOUNCER • " + progress.nextTournamentMatch + " • EXPLORE UNTIL READY", 2.25f);
        }
    }
    refreshFreeSwapObjects();
    previousPlayerPosition_ = playerPosition_;
    syncView();
    sceneCheckpointSnapshot_ = saveSnapshot();
    sceneCheckpointValid_ = true;
}

void RuntimeSession::resetCurrentScene() {
    if (training_.has(game_.scene().id)) {
        enterCurrentScene();
        beginTrainingAt(trainingCheckpointStep_);
        trainingManualVisible_ = true;
        trainingManualSelection_ = trainingCheckpointStep_ == 0 ? 0 : 1;
        syncView();
        return;
    }
    if (sceneCheckpointValid_) {
        const auto checkpoint = sceneCheckpointSnapshot_;
        loadSnapshot(checkpoint);
        showGameplayNotice("CHECKPOINT RESTORED", 1.25f);
        return;
    }
    enterCurrentScene();
}

void RuntimeSession::tick(InputState& input, float dt) {
    dt = std::clamp(std::max(0.0f, dt), 0.0f, 0.1f);
    if (!standaloneMode() && game_.mode() != GameMode::Title && game_.mode() != GameMode::MainMenu &&
        game_.mode() != GameMode::Pause)
        storyPlaytimeSeconds_ += dt;
    battleRankTime_ = std::max(0.0f, battleRankTime_ - dt);
    if (!pendingRankDialogueId_.empty() || pendingRankChoiceKind_ != 0) {
        if (battleRankTime_ <= 0.0f) {
            if (pendingRankChoiceKind_ != 0) {
                choiceKind_ = pendingRankChoiceKind_;
                choiceIndex_ = 0;
            } else {
                const std::string dialogueId = pendingRankDialogueId_;
                const int continuation = pendingRankDialogueContinuation_;
                pendingRankDialogueId_.clear();
                pendingRankDialogueContinuation_ = 0;
                beginTransientDialogue(dialogueId, continuation);
            }
            pendingRankChoiceKind_ = 0;
        }
        syncView();
        return;
    }
    const float cameraX = input.cameraX() * (qolSettings_.invertCameraX ? -1.0f : 1.0f);
    const float cameraY = input.cameraY() * (qolSettings_.invertCameraY ? -1.0f : 1.0f);
    if (std::abs(cameraX) > .01f || std::abs(cameraY) > .01f) {
        userCameraYawOffset_ = std::clamp(userCameraYawOffset_ + cameraX * 95.0f * qolSettings_.cameraSensitivity * dt, -38.0f, 38.0f);
        userCameraHeightOffset_ = std::clamp(userCameraHeightOffset_ + cameraY * 110.0f * qolSettings_.cameraSensitivity * dt, -90.0f, 90.0f);
    } else if (qolSettings_.gentleCameraRecenter && !cinematicCameraActive_) {
        const float blend = std::clamp(dt * 1.25f, 0.0f, 1.0f);
        userCameraYawOffset_ *= 1.0f - blend;
        userCameraHeightOffset_ *= 1.0f - blend;
    }
    playerCharging_ = false;
    pureEnergyBeamActive_ = player_.activeAttack == AttackKind::Beam;
    if (fireAwakeningTime_ > 0.0f) {
        fireAwakeningTime_ = std::max(0.0f, fireAwakeningTime_ - dt);
        player_.energy = std::max(0.0f, player_.energy - dt * 7.5f);
        if (player_.energy <= 0.0f) {
            fireAwakeningTime_ = 0.0f;
            showGameplayNotice("FIRE AWAKENING • ENERGY SPENT", .90f);
        }
    }
    if (cinematicCameraActive_ && game_.mode() == GameMode::Cutscene) {
        cinematicCameraBlend_ = std::min(1.0f, cinematicCameraBlend_ + dt * 7.5f);
        cinematicCameraReturnTime_ = 0.0f;
    } else if (cinematicCameraReturnTime_ > 0.0f) {
        cinematicCameraReturnTime_ = std::max(0.0f, cinematicCameraReturnTime_ - dt);
        cinematicCameraBlend_ = std::max(0.0f, cinematicCameraBlend_ - dt / .45f);
    } else {
        cinematicCameraBlend_ = 0.0f;
    }
    objectSwapCooldownTime_ = std::max(0.0f, objectSwapCooldownTime_ - dt);
    objectSwapPhaseTime_ = std::max(0.0f, objectSwapPhaseTime_ - dt);
    tournamentCardRevealTime_ = std::max(0.0f, tournamentCardRevealTime_ - dt);
    ploukeAwakeningVisualTime_ = std::max(0.0f, ploukeAwakeningVisualTime_ - dt);
    if (lensBlindnessDelay_ > 0.0f) {
        lensBlindnessDelay_ = std::max(0.0f, lensBlindnessDelay_ - dt);
        if (lensBlindnessDelay_ <= 0.0f) lensBlindnessTime_ = lensBlindnessDuration_;
    } else {
        lensBlindnessTime_ = std::max(0.0f, lensBlindnessTime_ - dt);
    }

    // Presentation timers continue during hit-freeze; combat/world simulation does not.
    gameplayNoticeTime_ = std::max(0.0f, gameplayNoticeTime_ - dt);
    if (gameplayNoticeTime_ <= 0.0f) gameplayNotice_.clear();
    saveStatusTime_ = std::max(0.0f, saveStatusTime_ - dt);
    if (saveStatusTime_ <= 0.0f) saveStatus_.clear();
    combatFeedbackTime_ = std::max(0.0f, combatFeedbackTime_ - dt);
    if (combatFeedbackTime_ <= 0.0f) {
        combatFeedback_.clear();
        perfectBlockFlash_ = pursuitFinishFlash_ = guardBreakFlash_ = finalHitFlash_ = false;
    }
    cameraImpulse_ = std::max(0.0f, cameraImpulse_ - dt * 18.0f);
    impactFlash_ = std::max(0.0f, impactFlash_ - dt * 7.5f);
    playerActionAnimationTime_ = std::max(0.0f, playerActionAnimationTime_ - dt);
    if (playerActionAnimationTime_ <= 0.0f) playerActionAnimation_.clear();
    landingAnimationTime_ = std::max(0.0f, landingAnimationTime_ - dt);
    if (landingAnimationTime_ <= 0.0f) hardLanding_ = false;
    combatReadyAnimationTime_ = std::max(0.0f, combatReadyAnimationTime_ - dt);
    explorationRunStartAnimationTime_ = std::max(0.0f, explorationRunStartAnimationTime_ - dt);
    explorationRunStopAnimationTime_ = std::max(0.0f, explorationRunStopAnimationTime_ - dt);
    bufferedCombatTime_ = std::max(0.0f, bufferedCombatTime_ - dt);
    if (bufferedCombatTime_ <= 0.0f) hasBufferedCombatAction_ = false;
    bufferedDashTime_ = std::max(0.0f, bufferedDashTime_ - dt);
    if (bufferedDashTime_ <= 0.0f) bufferedDash_ = false;

    if (game_.mode() == GameMode::Pause) {
        tickPause(input);
        syncView();
        return;
    }
    if (input.pressed(Action::Pause) && !trainingManualVisible_ && transientDialogueId_.empty() &&
        choiceKind_ == 0 && !qteActive_ && game_.mode() != GameMode::Cutscene) {
        objectiveBeforePause_ = view_.objective;
        pauseSelection_ = 0;
        pausePage_ = 0;
        game_.pause();
        syncView();
        return;
    }

    if (!transientDialogueId_.empty()) {
        tickTransientDialogue(input, dt);
        syncView();
        return;
    }

    if (choiceKind_ != 0) {
        tickChoice(input);
        syncView();
        return;
    }

    // Legacy hit-stop freezes the actual combat state, not merely the renderer.
    // Inputs pressed during those few frames are retained and consumed as soon as
    // the fighter is legal to act again.
    if (hitFreezeTime_ > 0.0f && (game_.mode() == GameMode::ArenaCombat || roadsideFightActive_)) {
        queueCombatInputsDuringFreeze(input);
        hitFreezeTime_ = std::max(0.0f, hitFreezeTime_ - dt);
        syncView();
        return;
    }

    attackCooldown_ = std::max(0.0f, attackCooldown_ - dt);
    CombatSystem::tick(player_, dt);
    CombatSystem::tick(opponent_, dt);
    adventureTime_ += dt;

    switch (game_.mode()) {
        case GameMode::Cutscene: tickCutscene(input, dt); break;
        case GameMode::ArenaCombat: tickArena(input, dt); break;
        case GameMode::Exploration: tickExploration(input, dt); break;
        default: break;
    }
    syncView();
}

void RuntimeSession::tickCutscene(InputState& input, float dt) {
    const auto& id = game_.scene().id;
    const bool seen = std::find(seenCutscenes_.begin(), seenCutscenes_.end(), id) != seenCutscenes_.end();
    if (seen && input.down(Action::Cancel)) {
        seenSceneSkipHoldTime_ += dt;
        if (seenSceneSkipHoldTime_ >= .65f) {
            seenSceneSkipHoldTime_ = 0.0f;
            completeScene();
            return;
        }
    } else {
        seenSceneSkipHoldTime_ = 0.0f;
    }
    applyCutsceneActions(dt);
    if (dialogueAdvanceRequested(input, dt) && cutsceneBeatReadyToAdvance()) advanceDialogue();
}


Vec2 RuntimeSession::cutsceneActorPosition(const std::string& actorId) const {
    if (actorId.empty() || actorId == "rrvvfo" || actorId == game_.chapter().playableCharacter) return playerPosition_;
    if (actorId == "sage") return opponentPosition_;
    for (const auto& actor : cutsceneActors_) if (actor.id == actorId) return actor.position;
    return playerPosition_;
}

void RuntimeSession::applyCutsceneActions(float dt) {
    if (!cutscenes_.has(game_.scene().id)) return;
    const auto& definition = cutscenes_.get(game_.scene().id);
    if (cutsceneBeatIndex_ != dialogueIndex_) {
        cutsceneBeatIndex_ = dialogueIndex_;
        cutsceneBeatTime_ = 0.0f;
        cinematicWorldEvent_.clear();
        cinematicExpression_.clear();
        cinematicCameraActive_ = false;
        if (cinematicCameraBlend_ > 0.0f) cinematicCameraReturnTime_ = .45f;
    }
    cutsceneBeatTime_ += dt;
    const auto moveActor = [&](const CutsceneAction& action, Vec2& actorPosition, float& yaw) {
        const Vec2 delta{action.position.x - actorPosition.x, action.position.z - actorPosition.z};
        const float remaining = distance(actorPosition, action.position);
        if (remaining <= 1.0f) { actorPosition = action.position; return; }
        const float step = std::min(remaining, std::max(1.0f, action.speed) * dt);
        const Vec2 direction{delta.x / remaining, delta.z / remaining};
        actorPosition = {actorPosition.x + direction.x * step, actorPosition.z + direction.z * step};
        yaw = turnTowardDegrees(yaw, yawForDirection(direction), kExplorationTurnDegreesPerSecond * dt);
    };

    for (const auto& action : definition.actions) {
        if (action.dialogueIndex != dialogueIndex_) continue;
        if (cutsceneBeatTime_ < action.startSeconds) continue;
        const bool actionFirstFrame = cutsceneBeatTime_ - dt <= action.startSeconds + .0001f;
        switch (action.kind) {
            case CutsceneActionKind::MoveTo:
                if (action.actorId == "rrvvfo" || action.actorId == game_.chapter().playableCharacter)
                    moveActor(action, playerPosition_, playerYawDegrees_);
                else if (action.actorId == "sage")
                    moveActor(action, opponentPosition_, opponentYawDegrees_);
                else for (auto& actor : cutsceneActors_) if (actor.id == action.actorId) {
                    moveActor(action, actor.position, actor.yawDegrees); actor.animation = "walk"; break;
                }
                break;
            case CutsceneActionKind::FaceActor:
            case CutsceneActionKind::LookAt: {
                const Vec2 target = cutsceneActorPosition(action.targetActorId);
                if (action.actorId == "rrvvfo" || action.actorId == game_.chapter().playableCharacter)
                    playerYawDegrees_ = turnTowardDegrees(playerYawDegrees_, yawForDirection({target.x-playerPosition_.x,target.z-playerPosition_.z}), kExplorationTurnDegreesPerSecond*dt);
                else if (action.actorId == "sage")
                    opponentYawDegrees_ = turnTowardDegrees(opponentYawDegrees_, yawForDirection({target.x-opponentPosition_.x,target.z-opponentPosition_.z}), kExplorationTurnDegreesPerSecond*dt);
                else for (auto& actor : cutsceneActors_) if (actor.id == action.actorId) {
                    actor.yawDegrees = turnTowardDegrees(actor.yawDegrees, yawForDirection({target.x-actor.position.x,target.z-actor.position.z}), kExplorationTurnDegreesPerSecond*dt); break;
                }
                break;
            }
            case CutsceneActionKind::PlayAnimation:
                if (actionFirstFrame && (action.actorId == "rrvvfo" || action.actorId == game_.chapter().playableCharacter) && !action.cue.empty())
                    triggerAbilityAnimation(action.cue, std::max(.10f, action.durationSeconds));
                else if (actionFirstFrame) for (auto& actor : cutsceneActors_) if (actor.id == action.actorId) { actor.animation = action.cue; break; }
                break;
            case CutsceneActionKind::CameraTrack:
            case CutsceneActionKind::CameraFocus: {
                cinematicCameraActive_ = true;
                const Vec2 target = !action.targetActorId.empty() ? cutsceneActorPosition(action.targetActorId) :
                                    (!action.actorId.empty() ? cutsceneActorPosition(action.actorId) : action.position);
                cinematicCameraFocus_ = target;
                cinematicCameraYawDegrees_ = action.cameraYawDegrees;
                cinematicCameraDistance_ = action.cameraDistance;
                cinematicCameraHeight_ = action.cameraHeight;
                cinematicCameraFovDegrees_ = action.cameraFovDegrees;
                break;
            }
            case CutsceneActionKind::TriggerWorldEvent:
                if (actionFirstFrame) {
                    cinematicWorldEvent_ = action.cue;
                    // The Sage's side entrance gets a tiny authored landing bump.
                    // Reduced Motion keeps the reframe but removes that bump.
                    if (action.cue == "sage_side_entrance" && !qolSettings_.reducedMotion)
                        cameraImpulse_ = std::max(cameraImpulse_, 2.2f);
                }
                break;
            case CutsceneActionKind::Expression:
                cinematicExpression_ = action.cue;
                break;
            case CutsceneActionKind::Dialogue:
            case CutsceneActionKind::Wait:
                break;
        }
    }
}

bool RuntimeSession::cutsceneBeatReadyToAdvance() const {
    if (!cutscenes_.has(game_.scene().id)) return true;
    const auto& definition = cutscenes_.get(game_.scene().id);
    float minimumTime = 0.0f;
    for (const auto& action : definition.actions) {
        if (action.dialogueIndex != dialogueIndex_) continue;
        if (action.kind == CutsceneActionKind::Wait) minimumTime = std::max(minimumTime, action.startSeconds + action.durationSeconds);
        if (action.kind == CutsceneActionKind::MoveTo) {
            const Vec2 actor = cutsceneActorPosition(action.actorId);
            if (distance(actor, action.position) > 14.0f) return false;
        }
    }
    return cutsceneBeatTime_ >= minimumTime;
}
bool RuntimeSession::dialogueAdvanceRequested(const InputState& input, float dt) {
    const bool pressed = input.pressed(Action::Confirm) || input.pressed(Action::Interact);
    const bool held = input.down(Action::Confirm) || input.down(Action::Interact);
    if (pressed) {
        dialogueHoldTime_ = 0.0f;
        dialogueAutoAdvanceTime_ = 0.0f;
        return true;
    }
    if (!held || !qolSettings_.holdToAdvanceDialogue) {
        dialogueHoldTime_ = 0.0f;
        const bool protectedInstruction = training_.has(game_.scene().id) || trainingManualVisible_ || choiceKind_ != 0;
        if (qolSettings_.dialogueAutoAdvance && !protectedInstruction) {
            dialogueAutoAdvanceTime_ += dt;
            const float wait = qolSettings_.dialogueSpeed == "slow" ? 2.8f :
                               qolSettings_.dialogueSpeed == "fast" ? 1.35f : 2.0f;
            if (dialogueAutoAdvanceTime_ >= wait) {
                dialogueAutoAdvanceTime_ = 0.0f;
                return true;
            }
        } else dialogueAutoAdvanceTime_ = 0.0f;
        return false;
    }
    dialogueHoldTime_ += dt;
    if (dialogueHoldTime_ < 0.55f) return false;
    // After the deliberate initial hold, advance at a readable pace without
    // ever confirming choices or gameplay interactions.
    dialogueHoldTime_ = 0.38f;
    return true;
}

std::string RuntimeSession::trainingCheckpointLabel(std::size_t stepIndex) {
    if (stepIndex >= 6) return "FINAL SPAR";
    if (stepIndex >= 5) return "LENS PREPARATION";
    if (stepIndex >= 4) return "CORE ABILITIES";
    if (stepIndex >= 2) return "PERFECT BLOCK";
    return "MOVEMENT";
}

void RuntimeSession::beginTrainingAt(std::size_t stepIndex) {
    const auto& definition = training_.get(game_.scene().id);
    trainingStepIndex_ = std::min(stepIndex, definition.steps.size());
    trainingProgress_ = 0;
    trainingSignals_ = 0;
    trainingMovementDistance_ = 0.0f;
    opponentAttackTimer_ = 0.0f;
    opponentAttackAttempts_ = 0;
    opponentAttackTelegraphed_ = false;
    trainingTimedDefenseArmed_ = false;
    attackCooldown_ = 0.0f;
    movementState_.dashTime = 0.0f;
    movementState_.dashing = false;
    player_.activeAttack = AttackKind::None;
    player_.attackElapsed = 0.0f;
    player_.attackConnected = false;
    trainingEvadeAttempt_ = false;
    lensTrialActive_ = false;
    lensTrialTimer_ = 0.0f;
    lensTrialHp_ = player_.hp;

    if (trainingStepIndex_ >= definition.steps.size()) {
        sceneComplete_ = true;
        showGameplayNotice("SAGE • BETTER. DON'T MAKE ME REGRET IT.", 1.65f);
        return;
    }
    const auto task = definition.steps[trainingStepIndex_].task;
    if (task == TrainingTaskKind::BasicAttacks || task == TrainingTaskKind::PerfectBlock ||
        task == TrainingTaskKind::LensRead || task == TrainingTaskKind::CleanHits) {
        const auto& definition = training_.get(game_.scene().id);
        playerPosition_ = definition.playerStart;
        opponentPosition_ = definition.opponentStart;
    }
    if (task == TrainingTaskKind::ChargeEnergy) player_.energy = definition.chargeStartingEnergy;
    if (task == TrainingTaskKind::CoreAbilities) player_.energy = 100.0f;
    if (task == TrainingTaskKind::LensRead) player_.energy = definition.lensStartingEnergy;
    showGameplayNotice(definition.steps[trainingStepIndex_].startBark, 1.65f);
}

void RuntimeSession::activateTrainingManualSelection() {
    const bool hasResume = trainingCheckpointStep_ > 0;
    const std::size_t skipIndex = hasResume ? 3 : 2;
    if (trainingManualSelection_ == skipIndex) {
        if (standaloneTrainingMode_) {
            trainingManualVisible_ = false;
            returnToTitleRequested_ = true;
            showGameplayNotice("TRAINING CLOSED", 1.0f);
            return;
        }
        tutorialSkipped_ = true;
        trainingManualVisible_ = false;
        showGameplayNotice("TRAINING SKIPPED • THE ROAD IS OPEN", 1.65f);
        completeScene();
        return;
    }

    std::size_t start = 0;
    if (hasResume && trainingManualSelection_ == 1) start = trainingCheckpointStep_;
    else if ((!hasResume && trainingManualSelection_ == 1) || (hasResume && trainingManualSelection_ == 2)) start = 4;
    beginTrainingAt(start);
    trainingManualVisible_ = false;
    combatReadyAnimationTime_ = 0.30f;
}

void RuntimeSession::updateTrainingCheckpoint() {
    const std::size_t candidate = trainingStepIndex_ >= 6 ? 6 : trainingStepIndex_ >= 5 ? 5 :
                                  trainingStepIndex_ >= 4 ? 4 : trainingStepIndex_ >= 2 ? 2 : 0;
    trainingCheckpointStep_ = std::max(trainingCheckpointStep_, candidate);
}

void RuntimeSession::advanceTrainingStep() {
    beginTrainingAt(trainingStepIndex_ + 1);
    updateTrainingCheckpoint();
}

void RuntimeSession::updateTrainingProgress() {
    if (const auto* step = currentTrainingStep()) {
        if (trainingProgress_ >= step->requiredCount) advanceTrainingStep();
    }
}

void RuntimeSession::tickTrainingOpponent(float dt, bool blockPressedThisFrame) {
    const auto* step = currentTrainingStep();
    if (!step || (step->task != TrainingTaskKind::PerfectBlock && step->task != TrainingTaskKind::LensRead)) return;
    if (step->task == TrainingTaskKind::LensRead && !lensActive_) return;

    const auto& definition = training_.get(game_.scene().id);
    const bool lensLesson = step->task == TrainingTaskKind::LensRead;
    if (lensLesson && lensTrialActive_) {
        lensTrialTimer_ = std::max(0.0f, lensTrialTimer_ - dt);
        if (lensTrialTimer_ > 0.0f) return;
        lensTrialActive_ = false;
        if (player_.hp >= lensTrialHp_) {
            trainingSignals_ |= 4u;
            trainingProgress_ = 3;
            updateTrainingProgress();
        }
        return;
    }

    const float resolveAt = lensLesson
        ? (opponentAttackAttempts_ == 0 ? definition.lensFirstAttackSeconds : definition.lensRetryAttackSeconds)
        : (opponentAttackAttempts_ == 0 ? definition.parryFirstAttackSeconds : definition.parryRetryAttackSeconds);
    const float warning = lensLesson ? definition.lensWarningSeconds : definition.parryWarningSeconds;
    opponentAttackTimer_ += dt;
    opponentAttackTelegraphed_ = opponentAttackTimer_ >= resolveAt - warning;
    if (opponentAttackTelegraphed_ && blockPressedThisFrame) trainingTimedDefenseArmed_ = true;
    if (opponentAttackTimer_ < resolveAt) return;
    ++opponentAttackAttempts_;

    if (lensLesson && trainingEvadeAttempt_) {
        lensTrialActive_ = true;
        lensTrialTimer_ = definition.lensEvadeConfirmationSeconds;
        lensTrialHp_ = player_.hp;
        opponentAttackTimer_ = 0.0f;
        opponentAttackTelegraphed_ = false;
        trainingTimedDefenseArmed_ = false;
        trainingEvadeAttempt_ = false;
        return;
    }

    if (trainingTimedDefenseArmed_) CombatSystem::startBlock(player_);
    const auto result = CombatSystem::light(opponent_, player_);
    emitCombatFeedback(result, AttackKind::Light1, false);
    if (trainingTimedDefenseArmed_) CombatSystem::stopBlock(player_);
    if (step->task == TrainingTaskKind::PerfectBlock && result.perfectBlocked) {
        trainingProgress_ = 1;
        updateTrainingProgress();
    } else if (step->task == TrainingTaskKind::LensRead && result.perfectBlocked) {
        trainingSignals_ |= 4u;
        trainingProgress_ = ((trainingSignals_ & 1u) ? 1 : 0) +
                            ((trainingSignals_ & 2u) ? 1 : 0) +
                            ((trainingSignals_ & 4u) ? 1 : 0);
        updateTrainingProgress();
    }
    opponentAttackTimer_ = 0.0f;
    opponentAttackTelegraphed_ = false;
    trainingTimedDefenseArmed_ = false;
    trainingEvadeAttempt_ = false;
}

void RuntimeSession::tickArena(InputState& input, float dt) {
    if (arenaEncounters_.has(game_.scene().id)) { tickStoryArena(input, dt); return; }
    if (trainingManualVisible_) {
        const bool hasResume = trainingCheckpointStep_ > 0;
        const std::size_t optionCount = hasResume ? 4 : 3;
        const std::size_t pageCount = manual_.pages().size();
        if (input.pressed(Action::MoveLeft)) {
            trainingManualPageIndex_ = trainingManualPageIndex_ == 0 ? pageCount - 1 : trainingManualPageIndex_ - 1;
        }
        if (input.pressed(Action::MoveRight)) {
            trainingManualPageIndex_ = (trainingManualPageIndex_ + 1) % pageCount;
        }
        if (input.pressed(Action::MoveUp)) {
            trainingManualSelection_ = trainingManualSelection_ == 0 ? optionCount - 1 : trainingManualSelection_ - 1;
        }
        if (input.pressed(Action::MoveDown)) {
            trainingManualSelection_ = (trainingManualSelection_ + 1) % optionCount;
        }
        if (input.pressed(Action::Confirm) || input.pressed(Action::Interact)) {
            activateTrainingManualSelection();
        }
        return;
    }

    const auto* stepBeforeMove = currentTrainingStep();
    const bool dashRequested = input.pressed(Action::Dash) || (bufferedDash_ && bufferedDashTime_ > 0.0f);
    if (dashRequested) {
        bool consumed = false;
        if (player_.pursuitWindow > 0.0f && CombatSystem::startPursuit(player_)) {
            playerPosition_ = {opponentPosition_.x - 78.0f, opponentPosition_.z};
            movementState_.dashCooldown = 0.0f;
            consumed = true;
        } else if (tryFlowCancel()) {
            consumed = true;
        }
        if (consumed) { bufferedDash_ = false; bufferedDashTime_ = 0.0f; }
    }
    FieldMovementConfig movement;
    movement.walkSpeed = 250.0f;
    const float speedReward = 1.0f + static_cast<float>(std::max(0, tournamentCard_.bonuses.speed)) * .01f;
    movement.walkSpeed *= speedReward * RpgProgressSystem::necklaceMovementMultiplier(rpgProgress_);
    movement.dashSpeed *= speedReward * RpgProgressSystem::necklaceDashMultiplier(rpgProgress_);
    previousPlayerPosition_ = playerPosition_;
    InputState lockedMovementInput; lockedMovementInput.beginFrame();
    const InputState& movementInput = (player_.stunTimer > 0.0f || player_.knockdownTimer > 0.0f) ? lockedMovementInput : input;
    const float preLandingVelocity = movementState_.verticalVelocity;
    playerPosition_ = FieldMovementSystem::tick(map(), playerPosition_, movementState_, movementInput, dt, movement, disabledBlockers_);
    updateCombatFacing(dt);
    if (movementState_.landedThisFrame) {
        hardLanding_ = preLandingVelocity < -520.0f;
        landingAnimationTime_ = hardLanding_ ? 0.16f : 0.08f;
    }
    player_.airborne = movementState_.height > 0.0f;

    const bool blockNow = input.down(Action::Block);
    const bool blockPressedThisFrame = blockNow && !blockHeld_;
    if (blockNow && !blockHeld_) CombatSystem::startBlock(player_);
    if (!blockNow && blockHeld_) CombatSystem::stopBlock(player_);
    blockHeld_ = blockNow;
    if (input.pressed(Action::Counter) && CombatSystem::startCounter(player_)) triggerAbilityAnimation("counter", 0.216f);
    if (input.pressed(Action::Breaker) && CombatSystem::comboBreaker(player_)) triggerAbilityAnimation("breaker", 0.150f);

    const AttackKind resolvingAttack = player_.activeAttack;
    const auto resolvedHit = advancePlayerAttack(dt);
    emitCombatFeedback(resolvedHit, resolvingAttack, true);
    if (resolvedHit.connected && !resolvedHit.blocked && player_.flowCancelWindow > 0.0f &&
        !flowCancelLearned_ && qolSettings_.firstTimeHints)
        showGameplayNotice("FLOW CANCEL READY • DASH", 0.80f);
    tryBufferedArenaAttack();
    if (stepBeforeMove && stepBeforeMove->task == TrainingTaskKind::BasicAttacks &&
        resolvingAttack == AttackKind::Grab && resolvedHit.connected) trainingSignals_ |= 8u;
    if (stepBeforeMove && stepBeforeMove->task == TrainingTaskKind::CleanHits && resolvedHit.connected &&
        !resolvedHit.blocked && !resolvedHit.perfectBlocked && !resolvedHit.countered) {
        ++sparCleanHits_;
        trainingProgress_ = sparCleanHits_;
        updateTrainingProgress();
    }

    if (stepBeforeMove && stepBeforeMove->task == TrainingTaskKind::Movement) {
        const float movedThisFrame = distance(previousPlayerPosition_, playerPosition_);
        const float maximumLegitimateMove = movement.dashSpeed * dt + 2.0f;
        if (movedThisFrame <= maximumLegitimateMove) trainingMovementDistance_ += movedThisFrame;
        const auto& definition = training_.get(game_.scene().id);
        if (trainingMovementDistance_ >= definition.meaningfulMovementDistance) trainingSignals_ |= 1u;
        if (movementState_.jumpStartedThisFrame) trainingSignals_ |= 2u;
        if (movementState_.dashStartedThisFrame) trainingSignals_ |= 4u;
        trainingProgress_ = ((trainingSignals_ & 1u) ? 1 : 0) + ((trainingSignals_ & 2u) ? 1 : 0) + ((trainingSignals_ & 4u) ? 1 : 0);
        updateTrainingProgress();
    }

    const auto* step = currentTrainingStep();
    if (!step) {
        if (sceneComplete_ && (input.pressed(Action::Confirm) || input.pressed(Action::Interact))) completeScene();
        return;
    }

    if (step->task == TrainingTaskKind::BasicAttacks) {
        constexpr Action actions[] = {Action::Light, Action::Heavy, Action::Launcher, Action::Grab};
        for (unsigned i = 0; i < 4; ++i) {
            if (!input.pressed(actions[i])) continue;
            const auto attempt = requestArenaAttack(actions[i]);
            const bool completed = actions[i] == Action::Grab ? false : attempt.started;
            if (completed) trainingSignals_ |= (1u << i);
        }
        trainingProgress_ = 0;
        for (unsigned i = 0; i < 4; ++i) if (trainingSignals_ & (1u << i)) ++trainingProgress_;
        updateTrainingProgress();
    } else if (step->task == TrainingTaskKind::ChargeEnergy) {
        const auto& definition = training_.get(game_.scene().id);
        const bool moving = distance(previousPlayerPosition_, playerPosition_) > 0.2f;
        playerCharging_ = input.down(Action::Charge) && !moving;
        if (input.down(Action::Charge)) CombatSystem::charge(player_, dt, !moving);
        trainingProgress_ = static_cast<int>(player_.energy);
        if (player_.energy >= definition.chargeTargetEnergy) trainingProgress_ = static_cast<int>(definition.chargeTargetEnergy);
        updateTrainingProgress();
    } else if (step->task == TrainingTaskKind::CoreAbilities) {
        if (const auto* ability = pressedAbility(input)) {
            if (ability->id == "fireBlast" && !(trainingSignals_ & 1u) && player_.energy >= ability->energyCost) {
                player_.energy -= ability->energyCost; trainingSignals_ |= 1u;
                triggerAbilityAnimation("fire_blast", 0.42f);
            } else if (ability->id == "objectSwap" && !(trainingSignals_ & 2u) && player_.energy >= ability->energyCost) {
                player_.energy -= ability->energyCost; trainingSignals_ |= 2u;
                triggerAbilityAnimation("object_swap", 0.32f);
                std::swap(playerPosition_, opponentPosition_);
                updateCombatFacing(0.0f, true);
            }
        }
        trainingProgress_ = ((trainingSignals_ & 1u) ? 1 : 0) + ((trainingSignals_ & 2u) ? 1 : 0);
        updateTrainingProgress();
    } else if (step->task == TrainingTaskKind::LensRead) {
        const auto& definition = training_.get(game_.scene().id);
        const bool moving = distance(previousPlayerPosition_, playerPosition_) > 0.2f;
        playerCharging_ = !lensActive_ && input.down(Action::Charge) && !moving;
        if (playerCharging_) CombatSystem::charge(player_, dt, true);
        if (player_.energy >= definition.lensRequiredEnergy) trainingSignals_ |= 1u;
        if (!lensActive_) {
            const auto* ability = pressedAbility(input);
            if (ability && ability->id == "lensOfTruth" && (trainingSignals_ & 1u) &&
                player_.energy >= ability->energyCost && player_.hp > ability->hpCost) {
                player_.energy -= ability->energyCost;
                player_.hp -= ability->hpCost;
                lensActive_ = true;
                triggerAbilityAnimation("lens_activate", 0.30f);
                scheduleLensBlindness(1.10f);
                trainingSignals_ |= 2u;
            }
        } else if (opponentAttackTelegraphed_ && movementState_.dashStartedThisFrame) {
            trainingEvadeAttempt_ = true;
        }
        trainingProgress_ = ((trainingSignals_ & 1u) ? 1 : 0) +
                            ((trainingSignals_ & 2u) ? 1 : 0) +
                            ((trainingSignals_ & 4u) ? 1 : 0);
        if (trainingSignals_ & 4u) updateTrainingProgress();
    } else if (step->task == TrainingTaskKind::CleanHits) {
        constexpr Action actions[] = {Action::Light, Action::Heavy, Action::Launcher, Action::Grab};
        for (auto action : actions) {
            if (!input.pressed(action)) continue;
            requestArenaAttack(action);
        }
    }

    tickTrainingOpponent(dt, blockPressedThisFrame);
    if (sceneComplete_ && (input.pressed(Action::Confirm) || input.pressed(Action::Interact))) completeScene();
}


void RuntimeSession::startStoryArena() {
    const auto& encounter = activeArenaEncounter();
    storyArenaActive_ = true;
    storyPlayerStocksLost_ = 0;
    storyOpponentStocksLost_ = 0;
    storyArenaAiTimer_ = .45f;
    storyArenaAiDecisionIndex_ = 0;
    storyArenaRewardPending_ = false;
    pendingArenaSceneComplete_ = false;
    player_ = FighterState{game_.chapter().playableCharacter};
    opponent_ = FighterState{encounter.opponentId};
    activeMealBoost_ = encounter.official && rpgProgress_.nextOfficialMealBoost;
    if (activeMealBoost_) {
        rpgProgress_.nextOfficialMealBoost = false;
        manualSaveRequested_ = true;
        showGameplayNotice("CONTROLLED FLAME MEAL • NEXT-MATCH BOOST ACTIVE", 1.35f);
    }
    configurePlayerProgression();
    resetBattlePerformance();
    player_.energy = encounter.startingEnergy;
    opponent_.energy = encounter.startingEnergy;
    playerPosition_ = encounter.playerStart;
    opponentPosition_ = encounter.opponentStart;
    movementState_ = {};
    opponentHeight_ = 0.0f;
    opponentVerticalVelocity_ = 0.0f;
    combatReadyAnimationTime_ = .30f;
    updateCombatFacing(0.0f, true);
    if (activeOptionalEncounterId_.empty()) {
        const std::string introId = encounter.sceneId == "ch2_vs_hamual" ? "ch2_hamual_intro" :
                                    encounter.sceneId == "ch2_vs_daniel" ? "ch2_daniel_intro" :
                                    encounter.sceneId == "ch2_vs_wade" ? "ch2_wade_intro" : std::string{};
        if (!introId.empty()) beginTransientDialogue(introId);
    }
    if (encounter.resolution == StoryArenaResolution::PloukeStoryFinal) {
        ploukeFinalSeconds_ = 0.0f;
        ploukeAwakeningAttempted_ = false;
        ploukeAwakeningFailureComplete_ = false;
        ploukeAwakeningVisualTime_ = 0.0f;
        ploukeFinalClashActive_ = false;
        ploukeFinalClashTime_ = 0.0f;
        ploukeFinalClashIndex_ = 0;
    }
}

void RuntimeSession::resetStoryArenaStock() {
    const auto& encounter = activeArenaEncounter();
    player_ = FighterState{game_.chapter().playableCharacter};
    opponent_ = FighterState{encounter.opponentId};
    configurePlayerProgression();
    player_.energy = encounter.startingEnergy;
    opponent_.energy = encounter.startingEnergy;
    playerPosition_ = encounter.playerStart;
    opponentPosition_ = encounter.opponentStart;
    movementState_ = {};
    opponentHeight_ = 0.0f;
    opponentVerticalVelocity_ = 0.0f;
    attackCooldown_ = 0.0f;
    clearCombatInputBuffer();
    combatReadyAnimationTime_ = .22f;
    updateCombatFacing(0.0f, true);
}

void RuntimeSession::configurePlayerProgression() {
    const float hpBonus = static_cast<float>(std::max(0, tournamentCard_.bonuses.hp));
    player_.maxHp = 100.0f + hpBonus + (activeMealBoost_ ? 10.0f : 0.0f);
    player_.hp = player_.maxHp;
    player_.powerMultiplier = (1.0f + static_cast<float>(std::max(0, tournamentCard_.bonuses.power)) * .02f) *
                              RpgProgressSystem::necklacePowerMultiplier(rpgProgress_);
    player_.defenseMultiplier = 1.0f + static_cast<float>(std::max(0, tournamentCard_.bonuses.defense)) * .02f +
                                (activeMealBoost_ ? .05f : 0.0f);
    player_.chargeRate = 42.0f * (1.0f + static_cast<float>(std::max(0, tournamentCard_.bonuses.focus)) * .01f) *
                         RpgProgressSystem::necklaceChargeMultiplier(rpgProgress_);
}

void RuntimeSession::resetBattlePerformance() {
    battlePerformance_ = {};
}

void RuntimeSession::recordBattleAction(AttackKind kind) {
    const auto value = static_cast<unsigned>(kind);
    if (value > 0 && value < 31) battlePerformance_.actionVarietyMask |= (1u << value);
}

void RuntimeSession::addNecklaceMastery(float amount, const char* reason) {
    if (!RpgProgressSystem::addNecklaceMastery(rpgProgress_, amount)) return;
    if (!rpgProgress_.weightedNecklaceMasteryRewardGranted) {
        rpgProgress_.weightedNecklaceMasteryRewardGranted = true;
        ++tournamentCard_.bonuses.power;
        ++tournamentCard_.bonuses.speed;
        showGameplayNotice(std::string{"NECKLACE MASTERED • POWER +1 • SPEED +1 • "} + reason, 2.0f);
        manualSaveRequested_ = true;
    }
}

void RuntimeSession::finalizeBattleRank(bool playerWon) {
    battlePerformance_.playerWon = playerWon;
    battlePerformance_.stocksLost = storyPlayerStocksLost_;
    const auto result = RpgProgressSystem::evaluate(battlePerformance_);
    RpgProgressSystem::commitBattle(adventureRecords_, battlePerformance_, result);
    battleRankLabel_ = RpgProgressSystem::rankLabel(result.rank);
    battleRankScore_ = result.score;
    battleRankTime_ = 1.65f;
    addNecklaceMastery(playerWon ? 6.0f : 2.5f, "BATTLE TRAINING");
    manualSaveRequested_ = true;
    showGameplayNotice("BATTLE RANK • " + battleRankLabel_ + " • " + std::to_string(battleRankScore_), 1.65f);
}

void RuntimeSession::tickStoryArena(InputState& input, float dt) {
    const auto& encounter = activeArenaEncounter();
    if (!storyArenaActive_) { startStoryArena(); return; }
    if (!encounter.official && input.pressed(Action::Cancel)) {
        showGameplayNotice("OPTIONAL FIGHT • LEFT SAFELY", 1.15f);
        resolveOptionalTournamentFight(false);
        return;
    }
    if (encounter.resolution == StoryArenaResolution::PloukeStoryFinal) {
        ploukeFinalSeconds_ += dt;
        // The existing Energy resource—not a hidden finale variable—visibly
        // drains under Plouke's pressure and Rrvvfo's aggressive usage.
        player_.energy = std::max(0.0f, player_.energy - dt * .72f);
        if (!ploukeAwakeningAttempted_ && (player_.energy <= 9.0f || ploukeFinalSeconds_ >= 24.0f)) {
            ploukeAwakeningAttempted_ = true;
            player_.energy = 0.0f;
            ploukeAwakeningVisualTime_ = 4.8f;
            triggerAbilityAnimation("fire_awakening_fail", .70f);
            cameraImpulse_ = std::max(cameraImpulse_, 3.0f);
            beginTransientDialogue("ch2_plouke_awakening_failure", 71);
            return;
        }
        if (ploukeFinalClashActive_) {
            tickPloukeFinalClash(input, dt);
            return;
        }
    }

    FieldMovementConfig movement;
    movement.walkSpeed = 250.0f;
    movement.dashSpeed = 720.0f;
    movement.dashSeconds = .22f;
    movement.dashCooldownSeconds = .30f;
    const float speedReward = 1.0f + static_cast<float>(std::max(0, tournamentCard_.bonuses.speed)) * .01f;
    movement.walkSpeed *= speedReward * RpgProgressSystem::necklaceMovementMultiplier(rpgProgress_);
    movement.dashSpeed *= speedReward * RpgProgressSystem::necklaceDashMultiplier(rpgProgress_);
    const bool dashRequested = input.pressed(Action::Dash) || (bufferedDash_ && bufferedDashTime_ > 0.0f);
    if (dashRequested) {
        bool consumed=false;
        if (player_.pursuitWindow>0.0f && CombatSystem::startPursuit(player_)) {
            playerPosition_={opponentPosition_.x-78.0f,opponentPosition_.z}; movementState_.dashCooldown=0.0f; consumed=true;
        } else if (tryFlowCancel()) consumed=true;
        else if (movementState_.height>0.0f) { CombatSystem::startAirDash(player_); consumed=true; }
        else { CombatSystem::startDash(player_); consumed=true; }
        if(consumed){bufferedDash_=false;bufferedDashTime_=0.0f;}
    }
    previousPlayerPosition_=playerPosition_;
    InputState locked; locked.beginFrame();
    const InputState& moveInput=(player_.stunTimer>0||player_.knockdownTimer>0)?locked:input;
    playerPosition_=FieldMovementSystem::tick(map(),playerPosition_,movementState_,moveInput,dt,movement,disabledBlockers_);
    player_.airborne=movementState_.height>0.0f;
    updateCombatFacing(dt);
    playerCharging_ = input.down(Action::Charge) && distance(previousPlayerPosition_, playerPosition_) < .2f &&
                      player_.activeAttack == AttackKind::None;
    if (playerCharging_) CombatSystem::charge(player_, dt, true);

    const bool blockNow=input.down(Action::Block);
    if(blockNow&&!blockHeld_)CombatSystem::startBlock(player_);
    if(!blockNow&&blockHeld_)CombatSystem::stopBlock(player_);
    blockHeld_=blockNow;
    if(input.pressed(Action::Counter)&&CombatSystem::startCounter(player_))triggerAbilityAnimation("counter",.216f);
    if(input.pressed(Action::Breaker)&&CombatSystem::comboBreaker(player_))triggerAbilityAnimation("breaker",.150f);
    for(const auto action:{Action::Light,Action::Heavy,Action::Launcher,Action::Grab}) if(input.pressed(action)) requestArenaAttack(action);
    handleRoadsideAbilities(input);

    Vec2 toward{playerPosition_.x-opponentPosition_.x,playerPosition_.z-opponentPosition_.z};
    float separation=std::max(.001f,std::sqrt(toward.x*toward.x+toward.z*toward.z));
    storyArenaAiTimer_-=dt;
    if(storyArenaAiTimer_<=0.0f){
        storyArenaAiTimer_=encounter.ai==AiArchetype::Adaptive?.34f:.44f;
        const auto decision=CombatSystem::chooseAiAction(encounter.ai,opponent_,player_,separation,storyArenaAiDecisionIndex_++);
        if(decision.block)CombatSystem::startBlock(opponent_);else CombatSystem::stopBlock(opponent_);
        if(opponent_.stunTimer<=0&&decision.approach&&separation>105){
            opponentPosition_.x+=toward.x/separation*encounter.opponentMoveSpeed*.28f;
            opponentPosition_.z+=toward.z/separation*encounter.opponentMoveSpeed*.28f;
        }
        if(opponent_.stunTimer<=0&&decision.retreat){opponentPosition_.x-=toward.x/separation*72.0f;opponentPosition_.z-=toward.z/separation*72.0f;}
        if(opponent_.stunTimer<=0&&decision.counter)CombatSystem::startCounter(opponent_);
        if(opponent_.stunTimer<=0&&decision.attack!=AttackKind::None)CombatSystem::startAttack(opponent_,decision.attack);
    }

    const AttackKind playerAttack=player_.activeAttack;
    const AttackKind opponentAttack=opponent_.activeAttack;
    const bool playerInRange=playerAttack==AttackKind::Projectile||playerAttack==AttackKind::Beam||
        separation<=CombatSystem::attackFor(player_.id,playerAttack).range+72.0f;
    const bool opponentInRange=separation<=CombatSystem::attackFor(opponent_.id,opponentAttack).range+72.0f;
    const auto playerHit=CombatSystem::advanceAttack(player_,opponent_,dt,playerInRange,{false,false});
    emitCombatFeedback(playerHit,playerAttack,true);
    if(playerHit.connected&&!playerHit.blocked&&!playerHit.countered){
        const float push=std::min(150.0f,playerHit.knockback*.88f);
        Vec2 away{opponentPosition_.x-playerPosition_.x,opponentPosition_.z-playerPosition_.z};
        const float len=std::max(.001f,std::sqrt(away.x*away.x+away.z*away.z));
        opponentPosition_.x+=away.x/len*push; opponentPosition_.z+=away.z/len*push;
    }
    tryBufferedArenaAttack();
    const auto opponentHit=CombatSystem::advanceAttack(opponent_,player_,dt,opponentInRange,{false,false});
    emitCombatFeedback(opponentHit,opponentAttack,false);
    if(opponentHit.connected&&!opponentHit.blocked&&!opponentHit.countered){
        const float push=std::min(145.0f,opponentHit.knockback*.84f);
        Vec2 away{playerPosition_.x-opponentPosition_.x,playerPosition_.z-opponentPosition_.z};
        const float len=std::max(.001f,std::sqrt(away.x*away.x+away.z*away.z));
        playerPosition_.x+=away.x/len*push; playerPosition_.z+=away.z/len*push;
    }
    updateCombatFacing(dt);

    const auto outside=[&](Vec2 p){return p.x<encounter.ringBounds.minX||p.x>encounter.ringBounds.maxX||p.z<encounter.ringBounds.minZ||p.z>encounter.ringBounds.maxZ;};
    const bool playerLostStock=player_.hp<=0.0f||outside(playerPosition_);
    const bool opponentLostStock=opponent_.hp<=0.0f||outside(opponentPosition_);
    if(playerLostStock||opponentLostStock){
        if(playerLostStock)++storyPlayerStocksLost_;
        if(opponentLostStock)++storyOpponentStocksLost_;
        if(encounter.resolution==StoryArenaResolution::PloukeStoryFinal &&
           (storyPlayerStocksLost_>=encounter.stockTarget||storyOpponentStocksLost_>=encounter.stockTarget)){
            if (!ploukeAwakeningAttempted_) {
                storyPlayerStocksLost_ = std::min(storyPlayerStocksLost_, encounter.stockTarget - 1);
                storyOpponentStocksLost_ = std::min(storyOpponentStocksLost_, encounter.stockTarget - 1);
                ploukeAwakeningAttempted_ = true;
                player_.energy = 0.0f;
                ploukeAwakeningVisualTime_ = 4.8f;
                resetStoryArenaStock();
                player_.energy = 0.0f;
                triggerAbilityAnimation("fire_awakening_fail", .70f);
                beginTransientDialogue("ch2_plouke_awakening_failure", 71);
                return;
            }
            ploukeHighPerformance_=storyOpponentStocksLost_>=2;
            finishStoryArena(false); return;
        }
        if(storyOpponentStocksLost_>=encounter.stockTarget){finishStoryArena(true);return;}
        if(storyPlayerStocksLost_>=encounter.stockTarget){
            if (!encounter.official) { finishStoryArena(false); return; }
            finalizeBattleRank(false);
            storyArenaActive_ = false;
            pendingRankChoiceKind_ = 31;
            showGameplayNotice("OFFICIAL MATCH • RETRY REQUIRED",1.25f);
            return;
        }
        resetStoryArenaStock();
    }
}

void RuntimeSession::tickPloukeFinalClash(InputState& input, float dt) {
    static constexpr Action sequence[] = {Action::Charge, Action::Ability2, Action::Light, Action::Heavy};
    ploukeFinalClashTime_ = std::max(0.0f, ploukeFinalClashTime_ - dt);
    if (ploukeFinalClashTime_ <= 0.0f) {
        finishPloukeFinalClash(false);
        return;
    }
    for (const auto action : sequence) {
        if (!input.pressed(action)) continue;
        if (ploukeFinalClashIndex_ >= 4 || action != sequence[ploukeFinalClashIndex_]) {
            ploukeFinalClashTime_ = std::max(0.25f, ploukeFinalClashTime_ - .65f);
            cameraImpulse_ = std::max(cameraImpulse_, 1.8f);
            showGameplayNotice("CLASH SLIPPING • FOLLOW THE ENERGY RHYTHM", .70f);
            return;
        }
        ++ploukeFinalClashIndex_;
        cameraImpulse_ = std::max(cameraImpulse_, 1.4f + static_cast<float>(ploukeFinalClashIndex_) * .45f);
        triggerAbilityAnimation(action == Action::Ability2 ? "energy_beam" : "clash_push", .28f);
        if (ploukeFinalClashIndex_ >= 4) finishPloukeFinalClash(true);
        return;
    }
}

void RuntimeSession::finishPloukeFinalClash(bool strongFinish) {
    ploukeFinalClashActive_ = false;
    ploukeFinalClashTime_ = 0.0f;
    ploukeHighPerformance_ = strongFinish;
    if (strongFinish) {
        // Rrvvfo wins the power exchange, but his committed finish carries him
        // over the edge. The tournament result remains the authored ring-out.
        storyOpponentStocksLost_ = std::max(storyOpponentStocksLost_, 2);
        playerPosition_.x = activeArenaEncounter().ringBounds.maxX + 36.0f;
        triggerAbilityAnimation("combat_ringout", .42f);
    } else {
        player_.hp = 0.0f;
        triggerAbilityAnimation("combat_knockdown", .42f);
    }
    finishStoryArena(false);
}

void RuntimeSession::finishStoryArena(bool playerWon) {
    const auto& encounter=activeArenaEncounter();
    finalizeBattleRank(playerWon);
    storyArenaActive_=false;
    if (!activeOptionalEncounterId_.empty()) {
        optionalArenaWon_ = playerWon;
        if (!playerWon) {
            pendingRankChoiceKind_ = 30;
            showGameplayNotice("OPTIONAL FIGHT • RETRY OR LEAVE", 1.25f);
            return;
        }
        pendingOptionalDialogueId_ = encounter.postDialogueId;
        if (playerWon && !encounter.postDialogueId.empty()) {
            pendingRankDialogueId_ = encounter.postDialogueId;
            pendingRankDialogueContinuation_ = 80;
        }
        else resolveOptionalTournamentFight(true);
        return;
    }
    pendingArenaSceneComplete_=true;
    storyArenaRewardPending_=playerWon||encounter.resolution==StoryArenaResolution::PloukeStoryFinal;
    triggerAbilityAnimation("combat_relax",.28f);
    if(encounter.resolution==StoryArenaResolution::PloukeStoryFinal){
        pendingRankDialogueId_ = ploukeHighPerformance_?"ch2_plouke_final_ringout":"ch2_plouke_final_exhausted";
        pendingRankDialogueContinuation_ = 70;
    }else if(!encounter.postDialogueId.empty()) {
        pendingRankDialogueId_ = encounter.postDialogueId;
        pendingRankDialogueContinuation_ = 70;
    }
    else resolveArenaRewardAndAdvance();
}

void RuntimeSession::resolveArenaRewardAndAdvance() {
    if(!pendingArenaSceneComplete_)return;
    pendingArenaSceneComplete_=false;
    if(storyArenaRewardPending_&&arenaEncounters_.has(game_.scene().id)){
        const auto& encounter=arenaEncounters_.get(game_.scene().id);
        storyArenaRewardPending_=false;
        const auto result=StoryProgressionSystem::grantXp(tournamentCard_,encounter.xpReward);
        if(result.levelsGained>0){
            tournamentBonusRoll_=1+((tournamentCard_.level+encounter.recommendedLevel)%3);
            choiceKind_=20;choiceIndex_=0;
            tournamentCardRevealTime_=4.5f;
            showGameplayNotice("LEVEL UP • BONUS +"+std::to_string(tournamentBonusRoll_),1.25f);
            return;
        }
    }
    completeScene();
}

const ArenaEncounterDefinition& RuntimeSession::activeArenaEncounter() const {
    return arenaEncounters_.get(activeOptionalEncounterId_.empty() ? game_.scene().id : activeOptionalEncounterId_);
}

void RuntimeSession::startOptionalTournamentFight(const std::string& encounterId) {
    if (!arenaEncounters_.has(encounterId)) return;
    activeOptionalEncounterId_ = encounterId;
    optionalArenaReturnPosition_ = playerPosition_;
    pendingOptionalDialogueId_.clear();
    optionalArenaWon_ = false;
    startStoryArena();
    showGameplayNotice("OPTIONAL FIGHT • B / CANCEL TO RUN", 1.45f);
}

void RuntimeSession::resolveOptionalTournamentFight(bool won) {
    const std::string completed = activeOptionalEncounterId_;
    activeOptionalEncounterId_.clear();
    storyArenaActive_ = false;
    pendingArenaSceneComplete_ = false;
    storyArenaRewardPending_ = false;
    playerPosition_ = optionalArenaReturnPosition_;
    player_ = FighterState{"rrvvfo"};
    player_.energy = 55.0f;
    opponent_ = FighterState{"sage"};
    movementState_ = {};
    if (won) {
        if (completed == "ch2_fake_champion_optional") fakeChampionComplete_ = true;
        else if (completed == "ch2_runaway_dummy_optional") runawayDummyComplete_ = true;
        else if (completed == "ch2_bark_practice_optional") ++barkPracticeWins_;
        else if (completed == "ch2_one_match_anyway_optional") {
            choiceKind_ = 22;
            choiceIndex_ = 0;
        }
        if (completed == "ch2_fake_champion_optional") {
            ++tournamentCard_.bonuses.defense;
            addUnique(rpgProgress_.titles, "Truth Breaker");
        } else if (completed == "ch2_runaway_dummy_optional") {
            ++tournamentCard_.bonuses.speed;
        }
        manualSaveRequested_ = true;
        if (completed != "ch2_one_match_anyway_optional")
            showGameplayNotice(completed == "ch2_bark_practice_optional" ? "FREE PRACTICE COMPLETE" : "OPTIONAL ACTIVITY COMPLETE", 1.45f);
    }
}
void RuntimeSession::showGameplayNotice(const std::string& text, float seconds) {
    gameplayNotice_ = text;
    gameplayNoticeTime_ = std::max(0.0f, seconds);
}

bool RuntimeSession::tryFlowCancel() {
    if (!CombatSystem::flowCancel(player_)) return false;
    movementState_.dashCooldown = 0.0f;
    flowCancelLearned_ = true;
    triggerAbilityAnimation("flow_cancel", 0.18f);
    showGameplayNotice("FLOW CANCEL • KEEP MOVING", 1.05f);
    hitFreezeTime_ = std::max(hitFreezeTime_, 3.0f * kFeedbackStepSeconds);
    cameraImpulse_ = std::max(cameraImpulse_, 3.2f);
    impactFlash_ = std::max(impactFlash_, 0.11f);
    combatFeedback_ = "FLOW CANCEL";
    combatFeedbackTime_ = 0.55f;
    return true;
}

void RuntimeSession::clearCombatInputBuffer() {
    hasBufferedCombatAction_ = false;
    bufferedCombatTime_ = 0.0f;
    bufferedDash_ = false;
    bufferedDashTime_ = 0.0f;
}

void RuntimeSession::queueCombatInputsDuringFreeze(const InputState& input) {
    constexpr Action attacks[] = {Action::Light, Action::Heavy, Action::Launcher, Action::Grab};
    for (const auto action : attacks) {
        if (!input.pressed(action)) continue;
        hasBufferedCombatAction_ = true;
        bufferedCombatAction_ = action;
        bufferedCombatTime_ = kRrvvfoInputBufferSeconds;
    }
    if (input.pressed(Action::Dash)) {
        bufferedDash_ = true;
        bufferedDashTime_ = kDashBufferSeconds;
    }
}

void RuntimeSession::emitCombatFeedback(const HitResult& result, AttackKind kind, bool playerAttacker) {
    if (!result.connected) return;
    if (storyArenaActive_) {
        if (playerAttacker) battlePerformance_.bestCombo = std::max(battlePerformance_.bestCombo, player_.comboHits);
        else battlePerformance_.damageTaken += result.damage;
        if (result.perfectBlocked && !playerAttacker) ++battlePerformance_.perfectBlocks;
        if (result.guardBroken && playerAttacker) ++battlePerformance_.guardBreaks;
        if (kind == AttackKind::PursuitHeavy && playerAttacker && !result.blocked && !result.countered)
            ++battlePerformance_.pursuitFinishers;
    }
    int frames = std::max(1, result.hitstopFrames);
    float impulse = result.blocked ? 1.7f : 2.35f;
    float flash = result.blocked ? 0.055f : 0.075f;
    std::string label;

    if (result.perfectBlocked) {
        frames = std::max(frames, 7);
        impulse = 4.8f;
        flash = 0.16f;
        if (!playerAttacker) {
            addNecklaceMastery(.75f, "PERFECT BLOCK");
            label = "PERFECT BLOCK";
            perfectBlockFlash_ = true;
            triggerAbilityAnimation("perfect_block", 0.14f);
        }
    }
    if (result.guardBroken) {
        frames = std::max(frames, 8);
        impulse = 5.2f;
        flash = 0.18f;
        label = "GUARD BREAK";
        guardBreakFlash_ = true;
    }
    if (kind == AttackKind::PursuitHeavy && !result.blocked && !result.countered) {
        if (playerAttacker) addNecklaceMastery(.75f, "PURSUIT");
        frames = std::max(frames, 8);
        impulse = 5.8f;
        flash = 0.20f;
        label = "PURSUIT FINISH";
        pursuitFinishFlash_ = true;
    }
    if (result.wallSplat) {
        frames = std::max(frames, 8); impulse = std::max(impulse, 5.4f); flash = std::max(flash, .18f); label = "WALL SPLAT";
    } else if (result.groundBounce) {
        frames = std::max(frames, 8); impulse = std::max(impulse, 5.0f); flash = std::max(flash, .17f); label = "GROUND BOUNCE";
    }
    const bool finalHit = playerAttacker ? opponent_.hp <= 0.0f : player_.hp <= 0.0f;
    if (finalHit) {
        frames = std::max(frames, 10);
        impulse = 6.5f;
        flash = 0.26f;
        label = "FINAL HIT";
        finalHitFlash_ = true;
    }

    hitFreezeTime_ = std::max(hitFreezeTime_, static_cast<float>(frames) * kFeedbackStepSeconds);
    cameraImpulse_ = std::max(cameraImpulse_, impulse);
    impactFlash_ = std::max(impactFlash_, flash);
    if (!label.empty()) {
        combatFeedback_ = std::move(label);
        combatFeedbackTime_ = 0.65f;
    }
}

void RuntimeSession::emitCombatClashFeedback() {
    hitFreezeTime_ = std::max(hitFreezeTime_, 5.0f * kFeedbackStepSeconds);
    cameraImpulse_ = std::max(cameraImpulse_, 4.6f);
    impactFlash_ = std::max(impactFlash_, 0.16f);
    combatFeedback_ = "CLASH";
    combatFeedbackTime_ = 0.55f;
}

void RuntimeSession::triggerAbilityAnimation(const std::string& clip, float seconds) {
    playerActionAnimation_ = clip;
    playerActionAnimationTime_ = std::max(0.0f, seconds);
}

std::string RuntimeSession::resolvePlayerAnimation() const {
    if (!playerActionAnimation_.empty() && playerActionAnimationTime_ > 0.0f) return playerActionAnimation_;
    if (player_.knockdownTimer > 0.0f || player_.stunTimer > 0.0f) return "hurt";
    switch (player_.activeAttack) {
        case AttackKind::Light1: return "light_1";
        case AttackKind::Light2: return "light_2";
        case AttackKind::Light3: return "light_3";
        case AttackKind::Heavy: return "heavy";
        case AttackKind::Launcher: return "launcher";
        case AttackKind::AirLight: return "air_light";
        case AttackKind::AirHeavy: return "air_heavy";
        case AttackKind::PursuitLight: return "pursuit_light";
        case AttackKind::PursuitHeavy: return "pursuit_heavy";
        case AttackKind::Grab: return "grab";
        case AttackKind::Projectile: return "fire_blast";
        case AttackKind::Beam: return "heavy";
        case AttackKind::None: break;
    }
    const bool combat = game_.mode() == GameMode::ArenaCombat || roadsideFightActive_;
    if (combatReadyAnimationTime_ > 0.0f && combat) return "combat_ready";
    if (playerCharging_) return "charge";
    if (player_.blocking) return "block";
    if (movementState_.dashing || player_.dashTime > 0.0f || player_.pursuitTime > 0.0f) return "dash";
    if (movementState_.height > 0.0f) return movementState_.verticalVelocity < -35.0f ? "fall" : "jump_start";
    if (landingAnimationTime_ > 0.0f) {
        if (hardLanding_) return combat ? "combat_hard_land" : "hard_land";
        return combat ? "combat_land" : "land";
    }
    const bool moving = distance(previousPlayerPosition_, playerPosition_) > 0.35f;
    if (!combat) {
        if (moving && explorationRunStartAnimationTime_ > 0.0f) return "run_start";
        if (!moving && explorationRunStopAnimationTime_ > 0.0f) return "run_stop";
        return moving ? "run" : "idle";
    }
    if (moving) return playerBackpedaling() ? "combat_retreat" : "combat_advance";
    return "fighting_stance";
}

void RuntimeSession::updateExplorationFacing(float dt) {
    if (!movementState_.hasFacing) return;
    const Vec2 direction = movementState_.dashing ? movementState_.dashDirection : movementState_.facing;
    if (std::abs(direction.x) + std::abs(direction.z) <= 0.0001f) return;
    playerYawDegrees_ = turnTowardDegrees(
        playerYawDegrees_, yawForDirection(direction), kExplorationTurnDegreesPerSecond * std::max(0.0f, dt));
}

void RuntimeSession::updateCombatFacing(float dt, bool snap) {
    const Vec2 towardOpponent{opponentPosition_.x - playerPosition_.x, opponentPosition_.z - playerPosition_.z};
    const float length = std::sqrt(towardOpponent.x * towardOpponent.x + towardOpponent.z * towardOpponent.z);
    if (length <= 0.001f) return;
    const Vec2 playerDirection{towardOpponent.x / length, towardOpponent.z / length};
    const Vec2 opponentDirection{-playerDirection.x, -playerDirection.z};
    const float playerTarget = yawForDirection(playerDirection);
    const float opponentTarget = yawForDirection(opponentDirection);
    if (snap) {
        playerYawDegrees_ = playerTarget;
        opponentYawDegrees_ = opponentTarget;
        return;
    }
    const float step = kCombatTurnDegreesPerSecond * std::max(0.0f, dt);
    playerYawDegrees_ = turnTowardDegrees(playerYawDegrees_, playerTarget, step);
    opponentYawDegrees_ = turnTowardDegrees(opponentYawDegrees_, opponentTarget, step);
}

bool RuntimeSession::playerBackpedaling() const {
    if (game_.mode() != GameMode::ArenaCombat && !roadsideFightActive_) return false;
    const Vec2 movement{playerPosition_.x - previousPlayerPosition_.x, playerPosition_.z - previousPlayerPosition_.z};
    const float length = std::sqrt(movement.x * movement.x + movement.z * movement.z);
    if (length <= 0.35f) return false;
    const auto forward = forwardForYaw(playerYawDegrees_);
    return (movement.x * forward.x + movement.z * forward.z) / length < -0.35f;
}

RuntimeSession::ArenaAttackAttempt RuntimeSession::requestArenaAttack(Action action, bool allowBuffer) {
    const auto attempt = performArenaAttack(action);
    if (attempt.started || !allowBuffer) return attempt;
    // Rrvvfo's final Legacy feel profile used a 135 ms action buffer.
    // Keeping it fighter-side preserves his flexible/improvised pressure rhythm.
    if (player_.activeAttack != AttackKind::None && player_.stunTimer <= 0.0f && !player_.guardBroken) {
        hasBufferedCombatAction_ = true;
        bufferedCombatAction_ = action;
        bufferedCombatTime_ = kRrvvfoInputBufferSeconds;
    }
    return attempt;
}

bool RuntimeSession::tryBufferedArenaAttack() {
    if (!hasBufferedCombatAction_ || bufferedCombatTime_ <= 0.0f || player_.activeAttack != AttackKind::None) return false;
    const Action action = bufferedCombatAction_;
    const auto attempt = performArenaAttack(action);
    if (!attempt.started) return false;
    clearCombatInputBuffer();
    return true;
}

RuntimeSession::ArenaAttackAttempt RuntimeSession::performArenaAttack(Action action) {
    ArenaAttackAttempt attempt;
    AttackKind kind = AttackKind::None;
    if (action == Action::Light) {
        if (player_.pursuitTime > 0.0f || player_.pursuitFollowupWindow > 0.0f) kind = AttackKind::PursuitLight;
        else if (movementState_.height > 0.0f) kind = AttackKind::AirLight;
        else kind = player_.lightStep == 0 ? AttackKind::Light1 : player_.lightStep == 1 ? AttackKind::Light2 : AttackKind::Light3;
    } else if (action == Action::Heavy) {
        if (player_.pursuitTime > 0.0f || player_.pursuitFollowupWindow > 0.0f || player_.pursuitFinishWindow > 0.0f) kind = AttackKind::PursuitHeavy;
        else if (movementState_.height > 0.0f) kind = AttackKind::AirHeavy;
        else kind = AttackKind::Heavy;
    } else if (action == Action::Launcher) {
        kind = AttackKind::Launcher;
    } else if (action == Action::Grab) kind = AttackKind::Grab;
    attempt.started = CombatSystem::startAttack(player_, kind);
    if (attempt.started && storyArenaActive_) recordBattleAction(kind);
    if (attempt.started && kind >= AttackKind::Light1 && kind <= AttackKind::Light3)
        player_.lightStep = (player_.lightStep + 1) % 3;
    return attempt;
}

HitResult RuntimeSession::advancePlayerAttack(float dt) {
    // Legacy ranges are measured between combat volumes, not actor origins.
    const bool inRange = distance(playerPosition_, opponentPosition_) <=
        CombatSystem::attackFor(player_.id, player_.activeAttack).range + 72.0f;
    const HitContext context{playerPosition_.x < -1370.0f || playerPosition_.x > 1370.0f, true};
    return CombatSystem::advanceAttack(player_, opponent_, dt, inRange, context);
}

const AbilitySlotDefinition* RuntimeSession::pressedAbility(const InputState& input) const {
    const auto& layout = view_.hotbar;
    constexpr Action actions[] = {Action::Ability1, Action::Ability2, Action::Ability3, Action::Ability4, Action::Ability5};
    for (auto action : actions) {
        if (!input.pressed(action)) continue;
        const auto* ability = AbilityHotbarCatalog::abilityForAction(layout, action);
        if (ability && ability->state == AbilityState::Ready) return ability;
    }
    return nullptr;
}

bool RuntimeSession::blockerDisabled(const std::string& id) const {
    return std::find(disabledBlockers_.begin(), disabledBlockers_.end(), id) != disabledBlockers_.end();
}

void RuntimeSession::disableBlocker(const std::string& id) {
    if (id.empty() || blockerDisabled(id)) return;
    disabledBlockers_.push_back(id);
}

void RuntimeSession::enableBlocker(const std::string& id) {
    disabledBlockers_.erase(std::remove(disabledBlockers_.begin(), disabledBlockers_.end(), id), disabledBlockers_.end());
}

void RuntimeSession::setTerrainCollapseActive(bool active) {
    if (active) enableBlocker("terrain_collapse"); else disableBlocker("terrain_collapse");
}

void RuntimeSession::setDetourBlockersActive(bool active) {
    constexpr const char* ids[] = {"detour_jump_gate_1","detour_jump_gate_2","detour_jump_gate_3","detour_dash_gate","detour_swap_gate"};
    for (const auto* id : ids) { if (active) enableBlocker(id); else disableBlocker(id); }
}

const RouteChallenge* RuntimeSession::currentRouteChallenge(const ExplorationDefinition& definition) const {
    const auto it = std::find_if(definition.routeChallenges.begin(), definition.routeChallenges.end(), [&](const RouteChallenge& route){
        return route.routeId == routeChoice_;
    });
    return it == definition.routeChallenges.end() ? nullptr : &*it;
}

const AdventureDefinition* RuntimeSession::currentAdventure() const {
    const auto& scene = game_.scene();
    if (!exploration_.has(scene.id)) return nullptr;
    const auto& definition = exploration_.get(scene.id);
    return !definition.adventureId.empty() && adventures_.has(definition.adventureId)
        ? &adventures_.get(definition.adventureId) : nullptr;
}

Vec2 RuntimeSession::adventureNpcPosition(const AdventureNpcDefinition& npc) const {
    Vec2 base = npc.basePosition;
    if (npc.id == "lost_competitor" && spectatorPassDelivered_ && game_.scene().id == "reach_tournament_outskirts")
        base = {1885.0f, 170.0f};
    return {
        base.x + std::sin(adventureTime_ * 0.55f + npc.motionPhase) * 45.0f,
        base.z + std::cos(adventureTime_ * 0.48f + npc.motionPhase) * 24.0f
    };
}

void RuntimeSession::beginTransientDialogue(const std::string& id, int continuation) {
    if (id.empty() || !dialogue_.has(id)) return;
    transientDialogueId_ = id;
    transientDialogueIndex_ = 0;
    transientDialogueContinuation_ = continuation;
}

void RuntimeSession::tickTransientDialogue(InputState& input, float dt) {
    if (!dialogueAdvanceRequested(input, dt)) return;
    const auto& lines = dialogue_.get(transientDialogueId_);
    if (transientDialogueIndex_ + 1 < lines.size()) {
        ++transientDialogueIndex_;
        dialogueAutoAdvanceTime_ = 0.0f;
        return;
    }
    finishTransientDialogue();
}

bool RuntimeSession::canManualSave() const {
    const GameMode activeMode = game_.mode() == GameMode::Pause ? game_.modeBeforePause() : game_.mode();
    return activeMode == GameMode::Exploration && !roadsideFightActive_ && transientDialogueId_.empty() &&
           choiceKind_ == 0 && !qteActive_ && !chapterComplete_;
}

bool RuntimeSession::consumeManualSaveRequest() {
    const bool requested = manualSaveRequested_;
    manualSaveRequested_ = false;
    return requested;
}

bool RuntimeSession::consumeReturnToTitleRequest() {
    const bool requested = returnToTitleRequested_;
    returnToTitleRequested_ = false;
    return requested;
}

void RuntimeSession::notifyManualSaveResult(bool success) {
    saveStatus_ = success ? "SAVED • " + game_.story().checkpointId : "SAVE FAILED • CHECK STORAGE";
    saveStatusTime_ = 2.0f;
    syncView();
}

void RuntimeSession::notifyManualSaveUnavailable() {
    saveStatus_ = "SAVE UNAVAILABLE • REACH A SAFE AREA";
    saveStatusTime_ = 2.0f;
    syncView();
}

void RuntimeSession::tickPause(InputState& input) {
    if (input.pressed(Action::Pause)) {
        game_.resume();
        pausePage_ = 0;
        return;
    }
    if (input.pressed(Action::Cancel)) {
        if (pausePage_ != 0) {
            pausePage_ = 0;
            pauseSelection_ = 0;
        } else game_.resume();
        return;
    }

    if (pausePage_ == 0) {
        if (standaloneMode()) {
            constexpr std::size_t standaloneOptionCount = 4;
            if (input.pressed(Action::MoveUp))
                pauseSelection_ = pauseSelection_ == 0 ? standaloneOptionCount - 1 : pauseSelection_ - 1;
            if (input.pressed(Action::MoveDown)) pauseSelection_ = (pauseSelection_ + 1) % standaloneOptionCount;
            if (!input.pressed(Action::Confirm) && !input.pressed(Action::Interact)) return;
            if (pauseSelection_ == 0) game_.resume();
            else if (pauseSelection_ == 1) {
                const auto replaySource = replayMode_ ? saveSnapshot() : SaveData{};
                game_.resume();
                if (standaloneFightMode_) startCpuFight();
                else if (standaloneTrainingMode_) startStandaloneTraining();
                else if (replayMode_) startReplayChapter(replaySource);
            } else if (pauseSelection_ == 2) {
                pausePage_ = 3; pauseSelection_ = 0;
            } else {
                game_.resume();
                returnToTitleRequested_ = true;
            }
            return;
        }
        constexpr std::size_t optionCount = 9;
        if (input.pressed(Action::MoveUp))
            pauseSelection_ = pauseSelection_ == 0 ? optionCount - 1 : pauseSelection_ - 1;
        if (input.pressed(Action::MoveDown)) pauseSelection_ = (pauseSelection_ + 1) % optionCount;
        if (!input.pressed(Action::Confirm) && !input.pressed(Action::Interact)) return;
        switch (pauseSelection_) {
            case 0: game_.resume(); break;
            case 1:
                if (canManualSave()) {
                    manualSaveRequested_ = true;
                    saveStatus_ = "SAVING…";
                    saveStatusTime_ = 2.0f;
                } else {
                    saveStatus_ = "SAVE AVAILABLE DURING SAFE EXPLORATION";
                    saveStatusTime_ = 2.0f;
                }
                break;
            case 2:
                game_.resume();
                resetCurrentScene();
                showGameplayNotice("CHECKPOINT RESTARTED", 1.5f);
                break;
            case 3: pausePage_ = 1; pauseSelection_ = 0; break;
            case 4: pausePage_ = 2; pauseSelection_ = 0; break;
            case 5: pausePage_ = 4; pauseSelection_ = 0; break;
            case 6: pausePage_ = 5; pauseSelection_ = 0; break;
            case 7: pausePage_ = 3; pauseSelection_ = 0; break;
            case 8:
                game_.resume();
                returnToTitleRequested_ = true;
                break;
            default: break;
        }
        return;
    }

    if (pausePage_ == 5) {
        if (!input.pressed(Action::Confirm) && !input.pressed(Action::Interact)) return;
        if (rpgProgress_.weightedNecklaceAcquired) {
            const bool equipped = rpgProgress_.equippedAccessoryId == "alts_weighted_necklace";
            rpgProgress_.equippedAccessoryId = equipped ? std::string{} : "alts_weighted_necklace";
            manualSaveRequested_ = true;
            showGameplayNotice(equipped ? "ALT'S WEIGHTED NECKLACE • REMOVED" : "ALT'S WEIGHTED NECKLACE • EQUIPPED", 1.35f);
        }
        return;
    }
    if (pausePage_ != 3) return;
    constexpr std::size_t settingCount = 16;
    if (input.pressed(Action::MoveUp))
        pauseSelection_ = pauseSelection_ == 0 ? settingCount - 1 : pauseSelection_ - 1;
    if (input.pressed(Action::MoveDown)) pauseSelection_ = (pauseSelection_ + 1) % settingCount;
    if (!input.pressed(Action::Confirm) && !input.pressed(Action::Interact) &&
        !input.pressed(Action::MoveLeft) && !input.pressed(Action::MoveRight)) return;
    switch (pauseSelection_) {
        case 0: qolSettings_.holdToAdvanceDialogue = !qolSettings_.holdToAdvanceDialogue; break;
        case 1: qolSettings_.dialogueAutoAdvance = !qolSettings_.dialogueAutoAdvance; break;
        case 2: qolSettings_.dialogueSpeed = qolSettings_.dialogueSpeed == "slow" ? "normal" : qolSettings_.dialogueSpeed == "normal" ? "fast" : "slow"; break;
        case 3: qolSettings_.firstTimeHints = !qolSettings_.firstTimeHints; break;
        case 4: qolSettings_.reducedMotion = !qolSettings_.reducedMotion; break;
        case 5: qolSettings_.reducedCameraShake = !qolSettings_.reducedCameraShake; break;
        case 6: qolSettings_.reducedFlashes = !qolSettings_.reducedFlashes; break;
        case 7: qolSettings_.highContrastHud = !qolSettings_.highContrastHud; break;
        case 8: qolSettings_.cameraSensitivity = qolSettings_.cameraSensitivity < .9f ? 1.0f : qolSettings_.cameraSensitivity < 1.1f ? 1.25f : .75f; break;
        case 9: qolSettings_.invertCameraX = !qolSettings_.invertCameraX; break;
        case 10: qolSettings_.invertCameraY = !qolSettings_.invertCameraY; break;
        case 11: qolSettings_.gentleCameraRecenter = !qolSettings_.gentleCameraRecenter; break;
        case 12: qolSettings_.hudScale = qolSettings_.hudScale < .95f ? 1.0f : qolSettings_.hudScale < 1.05f ? 1.10f : .90f; break;
        case 13: qolSettings_.dialogueScale = qolSettings_.dialogueScale < 1.08f ? 1.15f : qolSettings_.dialogueScale < 1.22f ? 1.30f : 1.0f; break;
        case 14: qolSettings_.combatMessages = qolSettings_.combatMessages == "full" ? "minimal" : qolSettings_.combatMessages == "minimal" ? "off" : "full"; break;
        case 15: qolSettings_.objectiveDisplay = qolSettings_.objectiveDisplay == "full" ? "minimal" : qolSettings_.objectiveDisplay == "minimal" ? "off" : "full"; break;
        default: break;
    }
}

void RuntimeSession::recordObjective(const std::string& objective) {
    if (objective.empty()) return;
    if (!objectiveHistory_.empty() && objectiveHistory_.back() == objective) return;
    objectiveHistory_.push_back(objective);
    if (objectiveHistory_.size() > 8) objectiveHistory_.erase(objectiveHistory_.begin());
}

void RuntimeSession::rememberDialogueLine(const std::string& sourceId, std::size_t index,
                                          const std::string& speaker, const std::string& text) {
    if (text.empty()) return;
    const std::string key = sourceId + "#" + std::to_string(index);
    if (key == lastRecordedDialogueKey_) return;
    lastRecordedDialogueKey_ = key;
    recentDialogue_.push_back((speaker.empty() ? std::string{"…"} : speaker) + " • " + text);
    if (recentDialogue_.size() > 12) recentDialogue_.erase(recentDialogue_.begin());
}

void RuntimeSession::finishTransientDialogue() {
if (transientDialogueContinuation_ == 60) {
    const std::string finishedDialogue = transientDialogueId_;
    transientDialogueId_.clear();
    transientDialogueIndex_ = 0;
    transientDialogueContinuation_ = 0;
    if (exploration_.has(game_.scene().id)) {
        const auto& definition = exploration_.get(game_.scene().id);
        if (!definition.sequenceMarkers.empty() && sceneSequenceProgress_ >= definition.sequenceMarkers.size()) {
            if (game_.scene().id != "ch2_wade_shortcut" && !definition.completionDialogueId.empty() &&
                finishedDialogue != definition.completionDialogueId) {
                beginTransientDialogue(definition.completionDialogueId, 61);
            } else completeScene();
        }
    }
    return;
}

    const int continuation = transientDialogueContinuation_;
    transientDialogueId_.clear();
    transientDialogueIndex_ = 0;
    transientDialogueContinuation_ = 0;
    switch (continuation) {
        case 1: completeScene(); break;
        case 2: mainRouteReady_ = true; break;
        case 3:
            mainRouteFireCleared_ = true;
            disableBlocker("fallen_tree_center");
            routeProgress_ = 0; routeChallengeTime_ = 0.0f; routeHintStage_ = 0;
            showGameplayNotice("MAIN ROAD • FOUR WORK LANES AHEAD", 1.35f);
            break;
        case 4: completeScene(); break;
        case 5: startRunawayCartQte(); break;
        case 6: completeScene(); break;
        case 7: choiceKind_ = 1; choiceIndex_ = 0; break;
        case 10: startRoadsideFight(); break;
        case 11: resolveRoadsideEncounter(false); break;
        case 12:
            spectatorPassDelivered_ = true;
            roadsideEncounterResolved_ = true;
            showGameplayNotice("SIDE STORY COMPLETE • THE LOUDEST SEAT", 1.65f);
            break;
        case 20:
            signPuzzleStage_ = std::max(signPuzzleStage_, 1);
            showGameplayNotice("OPTIONAL STORY • THE SIGN THAT POINTS BACK", 1.65f);
            break;
        case 21:
            showGameplayNotice("SIDE STORY COMPLETE • WAYFINDER BADGE", 1.85f);
            break;
        case 30:
            terrainCollapseSeen_ = true;
            showGameplayNotice("DIRECT ROAD BLOCKED • HIGH TRAIL OPEN", 1.45f);
            break;
        case 31:
            setDetourBlockersActive(false);
            showGameplayNotice("TOURNAMENT ROAD REJOINED", 1.35f);
            completeScene();
            break;
        case 32:
            completeScene();
            break;
        case 70: resolveArenaRewardAndAdvance(); break;
        case 71:
            ploukeAwakeningFailureComplete_ = true;
            player_.energy = 8.0f;
            ploukeAwakeningVisualTime_ = std::max(ploukeAwakeningVisualTime_, 1.2f);
            triggerAbilityAnimation("fire_awakening_collapse", .55f);
            showGameplayNotice("FIRE AWAKENING FAILED • ENERGY EXHAUSTED", 1.45f);
            ploukeFinalClashActive_ = true;
            ploukeFinalClashTime_ = 5.5f;
            ploukeFinalClashIndex_ = 0;
            break;
        case 80: resolveOptionalTournamentFight(optionalArenaWon_); break;
        case 81:
            wadeLostFanStarted_ = true;
            showGameplayNotice("OPTIONAL • FIND WADE'S FAN IN THE MARKET", 1.45f);
            break;
        case 82:
            wadeLostFanFound_ = true;
            showGameplayNotice("OPTIONAL • LEAD THE FAN BACK TO WADE", 1.45f);
            break;
        case 83:
            wadeLostFanComplete_ = true;
            ++tournamentCard_.bonuses.focus;
            addUnique(rpgProgress_.profileUnlocks, "wade");
            manualSaveRequested_ = true;
            showGameplayNotice("WADE'S LOST FAN • FOCUS +1 • PROFILE UNLOCKED", 1.85f);
            break;
        case 84:
            fakeChampionContacted_ = true;
            showGameplayNotice("OPTIONAL • USE LENS OF TRUTH ON THE TRAINER", 1.45f);
            break;
        case 85:
            fakeChampionRevealed_ = true;
            startOptionalTournamentFight("ch2_fake_champion_optional");
            break;
        case 86: startOptionalTournamentFight("ch2_runaway_dummy_optional"); break;
        case 87: startOptionalTournamentFight("ch2_bark_practice_optional"); break;
        case 88:
            festivalFoodComplete_ = true;
            player_.hp = std::min(player_.maxHp, player_.hp + 12.0f);
            showGameplayNotice("FESTIVAL FOOD • HP RECOVERED", 1.25f);
            break;
        case 89:
            festivalPhotoComplete_ = true;
            addUnique(rpgProgress_.mementos, "Tournament Festival Photo");
            triggerAbilityAnimation("victory_pose", .65f);
            showGameplayNotice("FESTIVAL PHOTO • POSE SAVED TO STORY RECORD", 1.35f);
            break;
        case 90:
            missingPrizeStarted_ = true;
            showGameplayNotice("SIDE STORY • FIND THE MISSING PRIZE ENVELOPE", 1.45f);
            break;
        case 91:
            missingPrizeFound_ = true;
            showGameplayNotice("PRIZE ENVELOPE FOUND • RETURN TO THE WORKER", 1.35f);
            break;
        case 92:
            missingPrizeComplete_ = true;
            rpgProgress_.coins += 60;
            rpgProgress_.vendorDiscountPercent = std::max(rpgProgress_.vendorDiscountPercent, 10);
            addUnique(rpgProgress_.mementos, "Recovered Prize Seal");
            manualSaveRequested_ = true;
            showGameplayNotice("MISSING PRIZE • 60 COINS • VENDOR DISCOUNT", 1.85f);
            break;
        case 93: startOptionalTournamentFight("ch2_one_match_anyway_optional"); break;
        case 94:
            controlledFlameStarted_ = true;
            showGameplayNotice("CONTROLLED FLAME • WARM THE PLATE WITH FIRE BLAST", 1.55f);
            break;
        case 95:
            controlledFlameComplete_ = true;
            rpgProgress_.nextOfficialMealBoost = true;
            manualSaveRequested_ = true;
            showGameplayNotice("CONTROLLED FLAME • NEXT OFFICIAL MATCH MEAL READY", 1.85f);
            break;
        case 96:
            altRoverQuestStage_ = 1;
            showGameplayNotice("SIDE STORY • FOLLOW ALT & ROVER'S SERVICE-LANE TRAIL", 1.55f);
            break;
        case 97: altRoverQuestStage_ = 2; showGameplayNotice("TRAIL • CHECK THE BACKWARD SIGN", 1.15f); break;
        case 98: altRoverQuestStage_ = 3; showGameplayNotice("TRAIL • SERVICE AWNING", 1.15f); break;
        case 99: altRoverQuestStage_ = 4; showGameplayNotice("TRAIL • OUTER SERVICE GATE", 1.15f); break;
        case 100:
            altRoverQuestStage_ = 5;
            beginTransientDialogue("ch2_alt_rover_necklace", 101);
            break;
        case 101:
            altRoverQuestStage_ = 6;
            rpgProgress_.weightedNecklaceAcquired = true;
            rpgProgress_.equippedAccessoryId = "alts_weighted_necklace";
            addUnique(rpgProgress_.mementos, "Alt's Weighted Necklace");
            manualSaveRequested_ = true;
            showGameplayNotice("ALT'S WEIGHTED NECKLACE • EQUIPPED", 1.85f);
            break;
        case 61: completeScene(); break;
        default: break;
    }
}

void RuntimeSession::tickChoice(InputState& input) {
    const std::size_t optionCount = choiceKind_ == 20 ? 5 : choiceKind_ == 4 ? 3 : choiceKind_ == 31 ? 1 : 2;
    if (input.pressed(Action::MoveLeft) || input.pressed(Action::MoveUp))
        choiceIndex_ = choiceIndex_ == 0 ? optionCount - 1 : choiceIndex_ - 1;
    if (input.pressed(Action::MoveRight) || input.pressed(Action::MoveDown))
        choiceIndex_ = (choiceIndex_ + 1) % optionCount;
    if (!input.pressed(Action::Confirm) && !input.pressed(Action::Interact) && !input.pressed(Action::Cancel)) return;
    if ((choiceKind_ == 4 || choiceKind_ == 5 || choiceKind_ == 20 || choiceKind_ == 22 || choiceKind_ == 31) && input.pressed(Action::Cancel)) return;
    const bool second = input.pressed(Action::Cancel) || choiceIndex_ == 1;
    const int kind = choiceKind_;
    choiceKind_ = 0;
    if (kind == 1) {
        lostCompetitorHelped_ = !second; lostCompetitorDeclined_ = second;
        beginTransientDialogue(second ? "road_npc_lost_competitor_decline" : "road_npc_lost_competitor_help");
    } else if (kind == 2) {
        if (second) beginTransientDialogue("roadside_challenger_leave", 11); else beginTransientDialogue("roadside_challenger_intro", 10);
    } else if (kind == 3) {
        if (second) resolveRoadsideEncounter(false); else startRoadsideFight();
    } else if (kind == 4) {
        static const std::string routes[] = {"main", "forest", "cliff"};
        routeChoice_ = routes[std::min<std::size_t>(choiceIndex_, 2)];
        if (routeChoice_ == "main") { disableBlocker("fallen_tree_north"); disableBlocker("fallen_tree_south"); }
        else if (routeChoice_ == "forest") disableBlocker("fallen_tree_north");
        else { disableBlocker("fallen_tree_south"); cliffRouteHintLevel_ = 0; }
        completeScene();
    } else if (kind == 5) {
        lensRouteChosen_ = !second; southernDetourChosen_ = second;
        if (southernDetourChosen_) showGameplayNotice("SOUTH DETOUR • LONG WAY AROUND", 1.45f);
        else showGameplayNotice("LENS ROUTE • READ THE ROADBLOCK", 1.35f);
    } else if (kind == 20) {
        static const StoryStat stats[] = {StoryStat::Hp,StoryStat::Power,StoryStat::Defense,StoryStat::Speed,StoryStat::Focus};
        StoryProgressionSystem::applyBonusWheel(tournamentCard_,stats[std::min<std::size_t>(choiceIndex_,4)],tournamentBonusRoll_);
        tournamentCardRevealTime_=3.0f;
        tournamentBonusRoll_=0;
        showGameplayNotice("TOURNAMENT CARD UPDATED",1.15f);
        completeScene();
    } else if (kind == 22) {
        if (second) ++tournamentCard_.bonuses.speed; else ++tournamentCard_.bonuses.power;
        oneMatchComplete_ = true;
        manualSaveRequested_ = true;
        showGameplayNotice(second ? "ONE MATCH ANYWAY • SPEED +1" : "ONE MATCH ANYWAY • POWER +1", 1.65f);
    } else if (kind == 30) {
        if (second) resolveOptionalTournamentFight(false);
        else startStoryArena();
    } else if (kind == 31) {
        const bool retryMealBoost = activeMealBoost_;
        storyPlayerStocksLost_ = storyOpponentStocksLost_ = 0;
        startStoryArena();
        if (retryMealBoost) {
            activeMealBoost_ = true;
            configurePlayerProgression();
        }
    }
}

void RuntimeSession::startRunawayCartQte() {
    const auto* adventure = currentAdventure();
    if (!adventure) return;
    qteActive_ = true;
    qteId_ = adventure->runawayCart.id;
    qteIndex_ = 0;
    qteRemaining_ = adventure->runawayCart.durationSeconds;
    ++qteAttempts_;
}

void RuntimeSession::finishRunawayCartQte(bool success) {
    const auto* adventure = currentAdventure();
    if (!adventure) return;
    const auto& qte = adventure->runawayCart;
    qteActive_ = false;
    if (!success && qteAttempts_ <= qte.safeRetries) {
        startRunawayCartQte();
        return;
    }
    if (success || qte.failForward) {
        runawayCartSaved_ = true;
        beginTransientDialogue("runaway_cart_result", 6);
    }
}

void RuntimeSession::tickQte(InputState& input, float dt) {
    const auto* adventure = currentAdventure();
    if (!adventure) return;
    const auto& qte = adventure->runawayCart;
    qteRemaining_ = std::max(0.0f, qteRemaining_ - dt);
    if (qteRemaining_ <= 0.0f) { finishRunawayCartQte(false); return; }
    constexpr Action qteActions[] = {Action::MoveLeft, Action::MoveRight, Action::Jump};
    for (const auto action : qteActions) {
        if (!input.pressed(action)) continue;
        if (qteIndex_ >= qte.sequence.size() || action != qte.sequence[qteIndex_]) {
            finishRunawayCartQte(false);
            return;
        }
        ++qteIndex_;
        if (qteIndex_ >= qte.sequence.size()) finishRunawayCartQte(true);
        return;
    }
}

void RuntimeSession::startRoadsideFight() {
    const auto* adventure = currentAdventure();
    if (!adventure) return;
    const auto& encounter = adventure->roadsideFight;
    roadsideReturnPosition_ = playerPosition_;
    roadsideFightActive_ = true;
    roadsideFightIntroShown_ = true;
    combatReadyAnimationTime_ = 0.30f;
    roadsidePlayerKOs_ = 0;
    roadsideFoeKOs_ = 0;
    roadsideAiTimer_ = 0.55f;
    roadsideAiDecisionIndex_ = 0;
    opponentHeight_ = 0.0f;
    opponentVerticalVelocity_ = 0.0f;
    combatLensTimer_ = 0.0f;
    player_ = FighterState{"rrvvfo"};
    opponent_ = FighterState{encounter.opponentId};
    player_.energy = encounter.startingEnergy;
    opponent_.energy = encounter.startingEnergy;
    playerPosition_ = encounter.playerStart;
    opponentPosition_ = encounter.opponentStart;
    movementState_ = {};
    updateCombatFacing(0.0f, true);
    attackCooldown_ = 0.0f;
}

void RuntimeSession::resolveRoadsideEncounter(bool wonFight) {
    roadsideFightActive_ = false;
    triggerAbilityAnimation("combat_relax", 0.28f);
    spectatorPassWon_ = spectatorPassWon_ || wonFight;
    playerPosition_ = roadsideReturnPosition_;
    player_ = FighterState{"rrvvfo"};
    player_.energy = 45.0f;
    opponent_ = FighterState{"sage"};
    if (wonFight) {
        roadsideEncounterResolved_ = true;
        showGameplayNotice("PASS WON • OPTIONAL: RETURN IT TO THE LOST COMPETITOR", 1.85f);
        completeScene();
    } else {
        roadsideEncounterResolved_ = true;
        completeScene();
    }
}

void RuntimeSession::handleRoadsideAbilities(const InputState& input) {
    const auto* ability = pressedAbility(input);
    if (!ability) return;
    if (player_.energy < ability->energyCost) { showGameplayNotice("NOT ENOUGH ENERGY • " + std::to_string(static_cast<int>(player_.energy)) + " / " + std::to_string(static_cast<int>(ability->energyCost)), 1.0f); return; }
    if (ability->id == "fireBlast" && attackCooldown_ > 0.0f) { showGameplayNotice("FIRE BLAST • RECHARGING", 0.85f); return; }
    if (ability->id == "energyBeam" && attackCooldown_ > 0.0f) { showGameplayNotice("ENERGY BEAM • RECHARGING", 0.85f); return; }
    if (ability->id == "lensOfTruth" && player_.hp <= ability->hpCost) { showGameplayNotice("LENS OF TRUTH • NOT ENOUGH HP", 1.0f); return; }
    if (ability->id == "fireBlast" && attackCooldown_ <= 0.0f && player_.energy >= ability->energyCost &&
        CombatSystem::startAttack(player_, AttackKind::Projectile)) {
        player_.energy -= ability->energyCost;
        attackCooldown_ = 1.05f; // Browser 2.9A.40.7.2R Chapter-1 Fire Blast cooldown.
        if (storyArenaActive_) recordBattleAction(AttackKind::Projectile);
        triggerAbilityAnimation("fire_blast", 0.42f);
    } else if (ability->id == "energyBeam" && attackCooldown_ <= 0.0f && player_.energy >= ability->energyCost &&
        CombatSystem::startAttack(player_, AttackKind::Beam)) {
        player_.energy -= ability->energyCost;
        attackCooldown_ = 1.35f;
        pureEnergyBeamActive_ = fireAwakeningTime_ <= 0.0f;
        if (storyArenaActive_) { recordBattleAction(AttackKind::Beam); ++adventureRecords_.energyBeamUses; }
        triggerAbilityAnimation(fireAwakeningTime_ > 0.0f ? "solar_weave" : "energy_beam", .80f);
        showGameplayNotice(fireAwakeningTime_ > 0.0f ? "SOLAR WEAVE" : "PURE ENERGY BEAM", .65f);
    } else if (ability->id == "objectSwap" && player_.energy >= ability->energyCost) {
        player_.energy -= ability->energyCost;
        triggerAbilityAnimation("object_swap", 0.32f);
        std::swap(playerPosition_, opponentPosition_);
        updateCombatFacing(0.0f, true);
        player_.invulnerabilityTimer = std::max(player_.invulnerabilityTimer, 0.16f);
        if (storyArenaActive_) { battlePerformance_.actionVarietyMask |= (1u << 20u); ++adventureRecords_.objectSwaps; }
    } else if (ability->id == "lensOfTruth" && player_.energy >= ability->energyCost && player_.hp > ability->hpCost) {
        player_.energy -= ability->energyCost;
        player_.hp -= ability->hpCost;
        triggerAbilityAnimation("lens_activate", 0.30f);
        combatLensTimer_ = 4.0f;
        lensActive_ = true;
        scheduleLensBlindness(4.0f);
    } else if (ability->id == "fireAwakening" && fireAwakeningUnlocked_) {
        if (fireAwakeningTime_ > 0.0f) {
            showGameplayNotice("FIRE AWAKENING • ALREADY ACTIVE", .80f);
        } else {
            player_.energy -= ability->energyCost;
            fireAwakeningTime_ = 6.0f;
            triggerAbilityAnimation("fire_awakening", .72f);
            cameraImpulse_ = std::max(cameraImpulse_, 3.8f);
            showGameplayNotice("FIRE AWAKENING • ENERGY DRAINS", 1.15f);
        }
    }
}

void RuntimeSession::tickRoadsideFight(InputState& input, float dt) {
    FieldMovementConfig movement;
    movement.walkSpeed = 154.0f;
    movement.dashSpeed = 720.0f;
    movement.dashSeconds = 0.22f;
    movement.dashCooldownSeconds = 0.30f;
    const float speedReward = 1.0f + static_cast<float>(std::max(0, tournamentCard_.bonuses.speed)) * .01f;
    movement.walkSpeed *= speedReward * RpgProgressSystem::necklaceMovementMultiplier(rpgProgress_);
    movement.dashSpeed *= speedReward * RpgProgressSystem::necklaceDashMultiplier(rpgProgress_);
    const bool dashRequested = input.pressed(Action::Dash) || (bufferedDash_ && bufferedDashTime_ > 0.0f);
    if (dashRequested) {
        bool consumed = false;
        if (player_.pursuitWindow > 0.0f && CombatSystem::startPursuit(player_)) {
            playerPosition_ = {opponentPosition_.x - 78.0f, opponentPosition_.z};
            movementState_.dashCooldown = 0.0f;
            consumed = true;
        } else if (tryFlowCancel()) {
            consumed = true;
        } else if (movementState_.height > 0.0f) {
            CombatSystem::startAirDash(player_);
            consumed = true;
        } else {
            CombatSystem::startDash(player_);
            consumed = true;
        }
        if (consumed) { bufferedDash_ = false; bufferedDashTime_ = 0.0f; }
    }
    previousPlayerPosition_ = playerPosition_;
    InputState lockedMovementInput; lockedMovementInput.beginFrame();
    const InputState& movementInput = (player_.stunTimer > 0.0f || player_.knockdownTimer > 0.0f) ? lockedMovementInput : input;
    const float preLandingVelocity = movementState_.verticalVelocity;
    playerPosition_ = FieldMovementSystem::tick(map(), playerPosition_, movementState_, movementInput, dt, movement, disabledBlockers_);
    updateCombatFacing(dt);
    if (movementState_.landedThisFrame) {
        hardLanding_ = preLandingVelocity < -520.0f;
        landingAnimationTime_ = hardLanding_ ? 0.16f : 0.08f;
    }
    player_.airborne = movementState_.height > 0.0f || player_.pursuitTime > 0.0f;

    // Opponents use the same combat launch state but a lightweight shared runtime
    // height integration until their own 3D models land. This makes launcher /
    // pursuit reactions spatial instead of being invisible stat changes.
    if (opponent_.verticalVelocity != 0.0f) { opponentVerticalVelocity_ = opponent_.verticalVelocity; opponent_.verticalVelocity = 0.0f; }
    if (opponent_.airborne || opponentHeight_ > 0.0f) {
        opponentHeight_ = std::max(0.0f, opponentHeight_ + opponentVerticalVelocity_ * dt);
        opponentVerticalVelocity_ -= 980.0f * dt;
        if (opponentHeight_ <= 0.0f && opponentVerticalVelocity_ < 0.0f) { opponentHeight_ = 0.0f; opponentVerticalVelocity_ = 0.0f; opponent_.airborne = false; }
    }
    playerCharging_ = input.down(Action::Charge) && distance(previousPlayerPosition_, playerPosition_) < 0.2f;
    if (playerCharging_) CombatSystem::charge(player_, dt, true);
    combatLensTimer_ = std::max(0.0f, combatLensTimer_ - dt);
    lensActive_ = combatLensTimer_ > 0.0f;

    const bool blockNow = input.down(Action::Block);
    if (blockNow && !blockHeld_) CombatSystem::startBlock(player_);
    if (!blockNow && blockHeld_) CombatSystem::stopBlock(player_);
    blockHeld_ = blockNow;
    constexpr Action attacks[] = {Action::Light, Action::Heavy, Action::Launcher, Action::Grab};
    for (const auto action : attacks) if (input.pressed(action)) requestArenaAttack(action);
    handleRoadsideAbilities(input);
    if (input.pressed(Action::Counter) && CombatSystem::startCounter(player_)) triggerAbilityAnimation("counter", 0.216f);
    if (input.pressed(Action::Breaker) && CombatSystem::comboBreaker(player_)) triggerAbilityAnimation("breaker", 0.150f);

    Vec2 toward{playerPosition_.x - opponentPosition_.x, playerPosition_.z - opponentPosition_.z};
    float separation = std::max(0.001f, std::sqrt(toward.x * toward.x + toward.z * toward.z));
    roadsideAiTimer_ -= dt;
    if (roadsideAiTimer_ <= 0.0f) {
        roadsideAiTimer_ = 0.48f;
        const auto decision = CombatSystem::chooseAiAction(AiArchetype::Balanced, opponent_, player_, separation,
                                                           roadsideAiDecisionIndex_++);
        if (decision.block) CombatSystem::startBlock(opponent_); else CombatSystem::stopBlock(opponent_);
        if (opponent_.stunTimer <= 0.0f && decision.approach && separation > 105.0f) {
            opponentPosition_.x += toward.x / separation * 154.0f * 0.30f;
            opponentPosition_.z += toward.z / separation * 154.0f * 0.30f;
        }
        if (opponent_.stunTimer <= 0.0f && decision.retreat) {
            opponentPosition_.x -= toward.x / separation * 70.0f;
            opponentPosition_.z -= toward.z / separation * 70.0f;
        }
        if (opponent_.stunTimer <= 0.0f && decision.dash) CombatSystem::startDash(opponent_);
        if (opponent_.stunTimer <= 0.0f && decision.counter) CombatSystem::startCounter(opponent_);
        if (opponent_.stunTimer <= 0.0f && decision.attack != AttackKind::None) CombatSystem::startAttack(opponent_, decision.attack);
    }

    toward = {playerPosition_.x - opponentPosition_.x, playerPosition_.z - opponentPosition_.z};
    separation = std::max(0.001f, std::sqrt(toward.x * toward.x + toward.z * toward.z));
    if (CombatSystem::resolveMeleeClash(player_, opponent_, separation, 0.0f) == ClashResult::Draw) {
        player_.activeAttack = AttackKind::None;
        opponent_.activeAttack = AttackKind::None;
        player_.stunTimer = std::max(player_.stunTimer, 0.24f);
        opponent_.stunTimer = std::max(opponent_.stunTimer, 0.24f);
        emitCombatClashFeedback();
    } else {
        const AttackKind playerAttack = player_.activeAttack;
        const AttackKind opponentAttack = opponent_.activeAttack;
        const bool playerInRange = playerAttack == AttackKind::Projectile || playerAttack == AttackKind::Beam ||
            separation <= CombatSystem::attackFor(player_.id, playerAttack).range + 72.0f;
        const bool opponentInRange = separation <= CombatSystem::attackFor(opponent_.id, opponentAttack).range + 72.0f;
        const auto playerHit = CombatSystem::advanceAttack(player_, opponent_, dt, playerInRange,
            {opponentPosition_.x < 690.0f || opponentPosition_.x > 1015.0f, true});
        emitCombatFeedback(playerHit, playerAttack, true);
        if (playerHit.connected && !playerHit.blocked && !playerHit.countered) {
            Vec2 away{opponentPosition_.x-playerPosition_.x, opponentPosition_.z-playerPosition_.z};
            const float len=std::max(.001f,std::sqrt(away.x*away.x+away.z*away.z));
            const float push=std::min(120.0f,playerHit.knockback*.62f);
            opponentPosition_.x += away.x/len*push; opponentPosition_.z += away.z/len*push;
            opponentPosition_.x=std::clamp(opponentPosition_.x,map().bounds.minX+28.0f,map().bounds.maxX-28.0f);
            opponentPosition_.z=std::clamp(opponentPosition_.z,map().bounds.minZ+28.0f,map().bounds.maxZ-28.0f);
            if (playerHit.launched) { opponentHeight_=std::max(opponentHeight_,12.0f); opponentVerticalVelocity_=std::max(opponentVerticalVelocity_,opponent_.verticalVelocity); opponent_.verticalVelocity=0.0f; }
        }
        if (playerHit.connected && !playerHit.blocked && player_.flowCancelWindow > 0.0f &&
            !flowCancelLearned_ && qolSettings_.firstTimeHints)
            showGameplayNotice("FLOW CANCEL READY • DASH", 0.80f);
        tryBufferedArenaAttack();
        const auto opponentHit = CombatSystem::advanceAttack(opponent_, player_, dt, opponentInRange,
            {playerPosition_.x < 690.0f || playerPosition_.x > 1015.0f, true});
        emitCombatFeedback(opponentHit, opponentAttack, false);
        if (opponentHit.connected && !opponentHit.blocked && !opponentHit.countered) {
            Vec2 away{playerPosition_.x-opponentPosition_.x, playerPosition_.z-opponentPosition_.z};
            const float len=std::max(.001f,std::sqrt(away.x*away.x+away.z*away.z));
            const float push=std::min(110.0f,opponentHit.knockback*.58f);
            playerPosition_.x += away.x/len*push; playerPosition_.z += away.z/len*push;
            playerPosition_.x=std::clamp(playerPosition_.x,map().bounds.minX+28.0f,map().bounds.maxX-28.0f);
            playerPosition_.z=std::clamp(playerPosition_.z,map().bounds.minZ+28.0f,map().bounds.maxZ-28.0f);
            if (opponentHit.launched) { movementState_.height=std::max(movementState_.height,12.0f); movementState_.verticalVelocity=std::max(movementState_.verticalVelocity,player_.verticalVelocity); player_.verticalVelocity=0.0f; }
        }
    }
    updateCombatFacing(dt);
    if (opponent_.hp <= 0.0f) {
        ++roadsidePlayerKOs_;
        if (standaloneFightMode_) {
            startRoadsideFight();
            showGameplayNotice("RRVVFO WINS • REMATCH", 1.45f);
        } else resolveRoadsideEncounter(true);
    } else if (player_.hp <= 0.0f) {
        ++roadsideFoeKOs_;
        if (standaloneFightMode_) {
            startRoadsideFight();
            showGameplayNotice("CPU WINS • REMATCH", 1.45f);
        } else {
            roadsideFightActive_ = false;
            choiceKind_ = 3;
            choiceIndex_ = 0;
        }
    }
}

void RuntimeSession::tickRoadsideEncounter(InputState& input, float dt, const ExplorationDefinition& definition) {
    if (roadsideFightActive_) { tickRoadsideFight(input, dt); return; }
    if (roadsideEncounterResolved_) { completeScene(); return; }
    // The lost competitor stands just west of the clearing but well off-axis.
    // A global X threshold could skip the optional story while approaching him.
    if (distance(playerPosition_, definition.target) <= definition.radius) {
        if (lostCompetitorHelped_) { choiceKind_ = 2; choiceIndex_ = 0; }
        else resolveRoadsideEncounter(false);
    }
}

void RuntimeSession::tickAdventureNpcs(InputState& input, float, const ExplorationDefinition& definition) {
    if (!input.pressed(Action::Interact) || definition.adventureId.empty() || !adventures_.has(definition.adventureId)) return;
    const auto& adventure = adventures_.get(definition.adventureId);
    const AdventureNpcDefinition* nearest = nullptr;
    float nearestDistance = 100000.0f;
    for (const auto& npc : adventure.npcs) {
        const float d = distance(playerPosition_, adventureNpcPosition(npc));
        if (d <= npc.interactionRadius + 30.0f && d < nearestDistance) { nearest = &npc; nearestDistance = d; }
    }
    if (!nearest) return;
    if (nearest->id == "lost_competitor") {
        if (spectatorPassDelivered_) beginTransientDialogue("road_npc_lost_competitor_pass_repeat");
        else if (spectatorPassWon_) beginTransientDialogue("road_npc_lost_competitor_pass_delivered", 12);
        else if (lostCompetitorHelped_) beginTransientDialogue(nearest->helpedDialogueId);
        else if (lostCompetitorDeclined_) beginTransientDialogue(nearest->declinedDialogueId);
        else beginTransientDialogue(nearest->dialogueId, 7);
    } else if (nearest->id == adventure.sidePuzzle.npcId && game_.scene().id == "reach_tournament_outskirts") {
        if (signPuzzleStage_ >= 3) beginTransientDialogue(adventure.sidePuzzle.repeatDialogueId);
        else if (signPuzzleStage_ == 2) beginTransientDialogue("sign_quest_reminder_swap");
        else if (signPuzzleStage_ == 1) beginTransientDialogue("sign_quest_reminder_lens");
        else beginTransientDialogue(adventure.sidePuzzle.introDialogueId, 20);
    } else beginTransientDialogue(nearest->dialogueId);
}


void RuntimeSession::refreshFreeSwapObjects() {
    const auto& currentMap = map();
    if (freeSwapMapId_ == currentMap.id && !freeSwapObjects_.empty()) return;
    freeSwapMapId_ = currentMap.id;
    freeSwapObjects_.clear();
    for (const auto& landmark : currentMap.landmarks) {
        if (landmark.id.rfind("free_swap_", 0) == 0)
            freeSwapObjects_.push_back({landmark.id, landmark.position, true});
    }
}

bool RuntimeSession::tryFreeObjectSwap() {
    refreshFreeSwapObjects();
    if (objectSwapCooldownTime_ > 0.0f) {
        showGameplayNotice("OBJECT SWAP • RECHARGING " + std::to_string(static_cast<int>(std::ceil(objectSwapCooldownTime_))) + "s", .85f);
        return false;
    }
    constexpr float kFreeSwapRange = 225.0f;
    int bestIndex = -1;
    float bestDistance = kFreeSwapRange;
    for (std::size_t i = 0; i < freeSwapObjects_.size(); ++i) {
        if (!freeSwapObjects_[i].active) continue;
        const float d = distance(playerPosition_, freeSwapObjects_[i].position);
        if (d <= bestDistance) { bestDistance = d; bestIndex = static_cast<int>(i); }
    }
    if (bestIndex < 0) {
        showGameplayNotice("OBJECT SWAP • NO OBJECT IN RANGE", .85f);
        return false;
    }
    const auto& layout = AbilityHotbarCatalog::rrvvfoChapter1();
    const auto ability = std::find_if(layout.begin(), layout.end(), [](const AbilitySlotDefinition& slot){ return slot.id == "objectSwap"; });
    const float cost = ability == layout.end() ? 20.0f : ability->energyCost;
    if (player_.energy < cost) {
        showGameplayNotice("OBJECT SWAP • NOT ENOUGH ENERGY", .85f);
        return false;
    }
    player_.energy -= cost;
    auto& object = freeSwapObjects_[static_cast<std::size_t>(bestIndex)];
    objectSwapGhostFrom_ = playerPosition_;
    objectSwapGhostTo_ = object.position;
    const Vec2 old = playerPosition_;
    playerPosition_ = object.position;
    object.position = old;
    objectSwapPhaseTime_ = .12f;
    objectSwapCooldownTime_ = 4.5f;
    triggerAbilityAnimation("object_swap", .32f);
    showGameplayNotice("OBJECT SWAP", .55f);
    return true;
}

void RuntimeSession::scheduleLensBlindness(float delaySeconds) {
    lensBlindnessDelay_ = std::max(lensBlindnessDelay_, std::max(0.0f, delaySeconds));
    lensBlindnessTime_ = 0.0f;
}
bool RuntimeSession::tickAdventureSidePuzzle(const AbilitySlotDefinition* ability) {
    const auto* adventure = currentAdventure();
    if (!adventure || adventure->sidePuzzle.id.empty() || !ability || signPuzzleStage_ <= 0 || signPuzzleStage_ >= 3)
        return false;
    const auto& puzzle = adventure->sidePuzzle;
    if (signPuzzleStage_ == 1 && ability->id == "lensOfTruth" &&
        withinAbilityTarget(playerPosition_, puzzle.lensTarget, puzzle.radius)) {
        signPuzzleStage_ = 2;
        triggerAbilityAnimation("lens_activate", 0.30f);
        scheduleLensBlindness(.30f);
        beginTransientDialogue(puzzle.lensDialogueId);
        return true;
    }
    if (signPuzzleStage_ == 2 && ability->id == "objectSwap" &&
        withinAbilityTarget(playerPosition_, puzzle.swapTarget, puzzle.radius)) {
        signPuzzleStage_ = 3;
        triggerAbilityAnimation("object_swap", 0.32f);
        beginTransientDialogue(puzzle.completionDialogueId, 21);
        return true;
    }
    return false;
}

void RuntimeSession::tickExploration(InputState& input, float dt) {
    const auto& scene = game_.scene();
    if (!exploration_.has(scene.id)) return;
    const auto& definition = exploration_.get(scene.id);
    if (choiceKind_ != 0) { tickChoice(input); return; }
    if (qteActive_) { tickQte(input, dt); return; }
    if (!activeOptionalEncounterId_.empty()) { tickStoryArena(input, dt); return; }
    if (roadsideFightActive_) { tickRoadsideFight(input, dt); return; }

    tickAdventureNpcs(input, dt, definition);
    if (!transientDialogueId_.empty()) return;
    if (input.pressed(Action::Interact) && qolSettings_.firstTimeHints && !view_.objective.empty())
        showGameplayNotice("OBJECTIVE • " + view_.objective, 1.35f);

    FieldMovementConfig movement;
    movement.walkSpeed = 270.0f;
    const float speedReward = 1.0f + static_cast<float>(std::max(0, tournamentCard_.bonuses.speed)) * .01f;
    movement.walkSpeed *= speedReward * RpgProgressSystem::necklaceMovementMultiplier(rpgProgress_);
    movement.dashSpeed *= speedReward * RpgProgressSystem::necklaceDashMultiplier(rpgProgress_);
    previousPlayerPosition_ = playerPosition_;
    const float preLandingVelocity = movementState_.verticalVelocity;
    playerPosition_ = FieldMovementSystem::tick(map(), playerPosition_, movementState_, input, dt, movement, disabledBlockers_);
    updateExplorationFacing(dt);
    if (movementState_.landedThisFrame) {
        hardLanding_ = preLandingVelocity < -520.0f;
        landingAnimationTime_ = hardLanding_ ? 0.16f : 0.08f;
    }
    const bool explorationMoving = distance(previousPlayerPosition_, playerPosition_) > 0.35f;
    if (explorationMoving && !explorationWasMoving_) { explorationRunStartAnimationTime_ = 0.105f; explorationRunStopAnimationTime_ = 0.0f; }
    else if (!explorationMoving && explorationWasMoving_) { explorationRunStopAnimationTime_ = 0.120f; explorationRunStartAnimationTime_ = 0.0f; }
    explorationWasMoving_ = explorationMoving;
    const float movedThisFrame = distance(previousPlayerPosition_, playerPosition_);
    if (movedThisFrame > .01f && movedThisFrame <= movement.dashSpeed * dt + 2.0f) {
        necklaceTraversalAccumulator_ += movedThisFrame;
        if (necklaceTraversalAccumulator_ >= 650.0f) {
            necklaceTraversalAccumulator_ -= 650.0f;
            addNecklaceMastery(1.0f, "STORY TRAVERSAL");
        }
    }
    const auto* ability = pressedAbility(input);

    if (tickU15TournamentActivities(input, ability)) return;
    if (tickTournamentActivities(input, ability)) return;

    const bool storyOwnsObjectSwap = definition.rule == ExplorationRuleKind::UseAbilityPoint ||
        definition.rule == ExplorationRuleKind::SwapRelay || definition.rule == ExplorationRuleKind::TerrainDetour;
    if (ability && ability->id == "objectSwap" && !storyOwnsObjectSwap &&
        game_.scene().id != "reach_tournament_outskirts" && tryFreeObjectSwap()) return;

    if (game_.scene().id == "reach_tournament_outskirts" && tickAdventureSidePuzzle(ability)) return;

    if (relayReleaseTimer_ > 0.0f) {
        relayReleaseTimer_ = std::max(0.0f, relayReleaseTimer_ - dt);
        if (relayReleaseTimer_ <= 0.0f) {
            for (const auto& blocker : definition.blockersToDisable) disableBlocker(blocker);
            completeScene();
        }
        return;
    }

    switch (definition.rule) {
        case ExplorationRuleKind::ReachPoint:
            if (distance(playerPosition_, definition.target) <= definition.radius || playerPosition_.x >= definition.target.x) completeScene();
            break;
        case ExplorationRuleKind::InteractPoint:
            if (distance(playerPosition_, definition.target) <= definition.radius && input.pressed(Action::Interact)) completeScene();
            break;
        case ExplorationRuleKind::UseAbilityPoint:
            if (ability && ability->id == definition.requiredAbilityId && !withinAbilityTarget(playerPosition_, definition.target, definition.radius)) {
                showGameplayNotice("GET CLOSER TO THE " + std::string(ability->id == "objectSwap" ? "SWAP TARGET" : "ABILITY TARGET"), 1.0f);
            }
            if (ability && ability->id == definition.requiredAbilityId && withinAbilityTarget(playerPosition_, definition.target, definition.radius)) {
                triggerAbilityAnimation(ability->id == "objectSwap" ? "object_swap" : ability->id == "fireBlast" ? "fire_blast" : "lens_activate",
                                        ability->id == "objectSwap" ? 0.32f : ability->id == "fireBlast" ? 0.42f : 0.30f);
                if (scene.id == "transport_wheel_recovery") transportRescued_ = true;
                if (definition.swapPhysicalObject) {
                    const Vec2 old = playerPosition_;
                    playerPosition_ = farBankRockPosition_;
                    farBankRockPosition_ = old;
                }
                if (!definition.completionDialogueId.empty()) beginTransientDialogue(definition.completionDialogueId, 4);
                else completeScene();
            }
            break;
        case ExplorationRuleKind::ChooseRoute:
            routeChoiceIntroTime_ = std::max(0.0f, routeChoiceIntroTime_ - dt);
            if (routeChoiceIntroTime_ <= 0.0f) { choiceKind_ = 4; choiceIndex_ = 0; }
            break;
        case ExplorationRuleKind::RouteChallenge: {
            const auto* challenge = currentRouteChallenge(definition);
            if (!challenge) break;
            routeChallengeTime_ += dt;
            if (qolSettings_.firstTimeHints && routeHintStage_ == 0 && routeChallengeTime_ >= definition.routeHintFirstSeconds) {
                routeHintStage_ = 1;
                showGameplayNotice(routeChoice_ == "forest" ? "FOREST • FIND THE NEXT BLUE BELL" :
                                   routeChoice_ == "cliff" ? "CLIFF • TAKE THE LEDGES IN ORDER" :
                                   "MAIN ROAD • FOLLOW THE WORK LANES", 1.30f);
            } else if (qolSettings_.firstTimeHints && routeHintStage_ == 1 && routeChallengeTime_ >= definition.routeHintSecondSeconds) {
                routeHintStage_ = 2;
                showGameplayNotice(routeChoice_ == "forest" ? "FOREST • FOUR BELLS • ONE AFTER ANOTHER" :
                                   routeChoice_ == "cliff" ? "CLIFF • FIVE AIRBORNE LEDGES" :
                                   "MAIN ROAD • FOUR WORK BEATS AFTER THE FIRE BLOCK", 1.55f);
            }

            if (routeChoice_ == "main") {
                if (!mainRouteDialogueShown_ && distance(playerPosition_, Vec2{365.0f, 300.0f}) <= 165.0f) {
                    mainRouteDialogueShown_ = true;
                    beginTransientDialogue(definition.openingDialogueId, 2);
                    break;
                }
                if (!mainRouteReady_) break;
                if (!mainRouteFireCleared_) {
                    if (ability && ability->id == challenge->requiredAbilityId && !withinAbilityTarget(playerPosition_, challenge->abilityTarget, challenge->abilityRadius))
                        showGameplayNotice("FIRE BLAST • GET CLOSER TO THE FALLEN LOG", 1.0f);
                    if (ability && ability->id == challenge->requiredAbilityId &&
                        withinAbilityTarget(playerPosition_, challenge->abilityTarget, challenge->abilityRadius)) {
                        triggerAbilityAnimation("fire_blast", 0.42f);
                        beginTransientDialogue(definition.completionDialogueId, 3);
                    }
                    break;
                }
                if (routeProgress_ < definition.mainWorkMarkers.size() &&
                    distance(playerPosition_, definition.mainWorkMarkers[routeProgress_]) <= 62.0f) {
                    ++routeProgress_; routeChallengeTime_ = 0.0f; routeHintStage_ = 0;
                    showGameplayNotice("MAIN ROAD • WORK LANE " + std::to_string(routeProgress_) + " / 4", 1.0f);
                }
                if (routeProgress_ < definition.mainWorkMarkers.size()) break;
            } else if (routeChoice_ == "forest") {
                if (routeProgress_ < definition.forestBellMarkers.size() &&
                    distance(playerPosition_, definition.forestBellMarkers[routeProgress_]) <= 74.0f) {
                    ++routeProgress_; routeChallengeTime_ = 0.0f; routeHintStage_ = 0;
                    showGameplayNotice("BLUE BELL • " + std::to_string(routeProgress_) + " / 4", 1.05f);
                }
                if (routeProgress_ < definition.forestBellMarkers.size()) break;
            } else if (routeChoice_ == "cliff") {
                if (routeProgress_ < definition.jumpMarkers.size() &&
                    distance(playerPosition_, definition.jumpMarkers[routeProgress_]) <= definition.jumpMarkerRadius &&
                    (movementState_.height > 8.0f || movementState_.jumpStartedThisFrame)) {
                    cliffJumpComplete_[routeProgress_] = true;
                    ++routeProgress_; routeChallengeTime_ = 0.0f; routeHintStage_ = 0;
                    showGameplayNotice("CLIFF LEDGE • " + std::to_string(routeProgress_) + " / 5", 0.95f);
                }
                const bool ready = routeProgress_ >= definition.jumpMarkers.size();
                if (!ready && playerPosition_.x > challenge->finish.x) playerPosition_.x = challenge->finish.x - 22.0f;
                if (!ready) break;
            }

            if (distance(playerPosition_, challenge->finish) <= challenge->finishRadius || playerPosition_.x > challenge->finish.x) {
                disableBlocker("fallen_tree_center");
                disableBlocker("fallen_tree_north");
                disableBlocker("fallen_tree_south");
                if (routeChoice_ == "cliff" && !cliffRewardEarned_) {
                    cliffRewardEarned_ = true;
                    showGameplayNotice("SCENIC DISCOVERY • CLIFFSIDE VIEW", 1.20f);
                    beginTransientDialogue("cliff_overlook_reaction", 32);
                    break;
                }
                completeScene();
            }
            break;
        }
        case ExplorationRuleKind::TerrainDetour: {
            // Three readable broken-ground jumps. Each successful airborne crossing opens
            // only the corresponding collision gate; this keeps the flat collision core
            // while giving the authored 3D ledges real gameplay meaning.
            if (routeProgress_ < definition.jumpMarkers.size()) {
                const auto index = routeProgress_;
                if (distance(playerPosition_, definition.jumpMarkers[index]) <= definition.jumpMarkerRadius &&
                    (movementState_.height > 8.0f || movementState_.jumpStartedThisFrame)) {
                    ++routeProgress_;
                    disableBlocker("detour_jump_gate_" + std::to_string(index + 1));
                    showGameplayNotice("BROKEN GROUND • " + std::to_string(routeProgress_) + " / 3", 1.0f);
                }
                break;
            }
            if (!detourDashDone_) {
                if (distance(playerPosition_, definition.detourDashMarker) <= definition.detourDashRadius && movementState_.dashStartedThisFrame) {
                    detourDashDone_ = true;
                    disableBlocker("detour_dash_gate");
                    showGameplayNotice("GAP CLEARED • KEEP MOVING", 1.0f);
                } else if (distance(playerPosition_, definition.detourDashMarker) <= definition.detourDashRadius + 50.0f && qolSettings_.firstTimeHints) {
                    showGameplayNotice("WIDE GAP • DASH THROUGH", .85f);
                }
                break;
            }
            if (!detourSwapDone_) {
                if (ability && ability->id == definition.requiredAbilityId && withinAbilityTarget(playerPosition_, definition.shortcutPoint, definition.shortcutRadius)) {
                    detourSwapDone_ = true;
                    triggerAbilityAnimation("object_swap", .32f);
                    playerPosition_ = definition.swapDestination;
                    disableBlocker("detour_swap_gate");
                    showGameplayNotice("OBJECT SWAP • BACK TOWARD THE ROAD", 1.1f);
                } else if (ability && ability->id == definition.requiredAbilityId) {
                    showGameplayNotice("OBJECT SWAP • GET CLOSER TO THE HIGH ANCHOR", 1.0f);
                }
                break;
            }
            if (distance(playerPosition_, definition.target) <= definition.radius || playerPosition_.x >= definition.target.x) {
                if (!definition.completionDialogueId.empty()) beginTransientDialogue(definition.completionDialogueId, 31);
                else completeScene();
            }
            break;
        }
        case ExplorationRuleKind::SwapRelay:
            if (relayIndex_ < static_cast<int>(relayMarkers_.size()) && ability && ability->id == definition.requiredAbilityId) {
                const Vec2 target = relayMarkers_[relayIndex_];
                if (!withinAbilityTarget(playerPosition_, target, definition.radius)) showGameplayNotice("OBJECT SWAP • GET CLOSER TO THE ACTIVE TARGET", 1.0f);
                if (withinAbilityTarget(playerPosition_, target, definition.radius)) {
                    triggerAbilityAnimation("object_swap", 0.32f);
                    const Vec2 old = playerPosition_;
                    playerPosition_ = target;
                    relayMarkers_[relayIndex_] = old;
                    if (scene.id == "transport_wheel_recovery" && relayIndex_ == 0 && relayMarkers_.size() > 1) {
                        relayMarkers_[1] = old; // the second target is the actual place Rrvvfo came from.
                        ++relayIndex_;
                        showGameplayNotice("WHEEL RECOVERED • SWAP BACK TO THE RETURN ANCHOR", 1.55f);
                        break;
                    }
                    ++relayIndex_;
                    if (scene.id == "transport_wheel_recovery" && relayIndex_ >= static_cast<int>(relayMarkers_.size())) {
                        transportRescued_ = true;
                        beginTransientDialogue(definition.completionDialogueId, 4);
                        break;
                    }
                    if (relayIndex_ >= static_cast<int>(relayMarkers_.size())) {
                        if (definition.adventureId.empty()) completeScene();
                        else {
                            const auto* adventure = currentAdventure();
                            relayReleaseTimer_ = adventure ? adventure->gateReleaseDelaySeconds : 0.52f;
                            precisionSwapMastered_ = true;
                            showGameplayNotice("OBJECT SWAP MASTERY • PRECISION", 1.45f);
                        }
                    }
                }
            }
            break;
        case ExplorationRuleKind::QteSequence:
            break;
        case ExplorationRuleKind::RoadsideEncounter:
            tickRoadsideEncounter(input, dt, definition);
            break;
        case ExplorationRuleKind::MandatoryAbilityReveal:
            if (southernDetourChosen_) {
                if (playerPosition_.x > 1830.0f && playerPosition_.z > 430.0f) {
                    southernDetourComplete_ = true;
                    showGameplayNotice("WORLD DELIGHT • SOUTHERN DETOUR", 1.65f);
                    completeScene();
                }
                break;
            }
            if (ability && ability->id == definition.requiredAbilityId && withinAbilityTarget(playerPosition_, definition.target, definition.radius)) {
                player_.hp = std::max(1.0f, player_.hp - 1.0f);
                lensActive_ = true;
                triggerAbilityAnimation("lens_activate", 0.30f);
                scheduleLensBlindness(.30f);
                for (const auto& blocker : definition.blockersToDisable) disableBlocker(blocker);
                completeScene();
            }
            break;
case ExplorationRuleKind::InteractionSequence: {
    sceneSequenceStarted_ = true;
    sceneSequenceSeconds_ += dt;
    if (sceneSequenceProgress_ >= definition.sequenceMarkers.size()) { completeScene(); break; }
    const Vec2 target = definition.sequenceMarkers[sceneSequenceProgress_];
    const bool close = distance(playerPosition_, target) <= definition.sequenceRadius;
    if (close && (!definition.sequenceRequiresInteract || input.pressed(Action::Interact))) {
        const std::size_t completedIndex = sceneSequenceProgress_++;
        const std::string label = completedIndex < definition.sequenceLabels.size()
            ? definition.sequenceLabels[completedIndex] : "OBJECTIVE";
        showGameplayNotice(label + " • " + std::to_string(sceneSequenceProgress_) + " / " + std::to_string(definition.sequenceMarkers.size()), 1.0f);
        if (completedIndex < definition.sequenceDialogueIds.size() && !definition.sequenceDialogueIds[completedIndex].empty()) {
            beginTransientDialogue(definition.sequenceDialogueIds[completedIndex], 60);
            break;
        }
        if (sceneSequenceProgress_ >= definition.sequenceMarkers.size()) {
            if (!definition.completionDialogueId.empty()) beginTransientDialogue(definition.completionDialogueId, 60);
            else completeScene();
        }
    }
    break;
}
case ExplorationRuleKind::TimedCheckpointSequence: {
    sceneSequenceStarted_ = true;
    sceneSequenceSeconds_ += dt;
    if (sceneSequenceProgress_ >= definition.sequenceMarkers.size()) { completeScene(); break; }
    const Vec2 target = definition.sequenceMarkers[sceneSequenceProgress_];
    if (distance(playerPosition_, target) <= definition.sequenceRadius) {
        ++sceneSequenceProgress_;
        showGameplayNotice("CHECKPOINT • " + std::to_string(sceneSequenceProgress_) + " / " + std::to_string(definition.sequenceMarkers.size()), .75f);
        if (sceneSequenceProgress_ >= definition.sequenceMarkers.size()) {
            if (definition.sequenceTargetSeconds > 0.0f && sceneSequenceSeconds_ <= definition.sequenceTargetSeconds)
                showGameplayNotice("WADE SHORTCUT • TARGET BEAT", 1.35f);
            if (game_.scene().id == "ch2_wade_shortcut") {
                wadeRaceTargetBeaten_ = sceneSequenceSeconds_ <= definition.sequenceTargetSeconds;
                if (wadeRaceBestSeconds_ <= 0.0f || sceneSequenceSeconds_ < wadeRaceBestSeconds_)
                    wadeRaceBestSeconds_ = sceneSequenceSeconds_;
                beginTransientDialogue(wadeRaceTargetBeaten_ ? "ch2_wade_shortcut_win" : "ch2_wade_shortcut_loss", 60);
            } else if (!definition.completionDialogueId.empty()) beginTransientDialogue(definition.completionDialogueId, 60);
            else completeScene();
        }
    }
    break;
}

        case ExplorationRuleKind::CliffJumpRoute:
            break;
    }
}

bool RuntimeSession::tickTournamentActivities(InputState& input, const AbilitySlotDefinition* ability) {
    if (game_.story().chapterId != "rrvvfo_ch2" || game_.scene().kind != SceneKind::Exploration) return false;
    const std::size_t phaseIndex = game_.story().sceneIndex;
    if (phaseIndex < 6 || phaseIndex > 19) return false;
    const bool interact = input.pressed(Action::Interact) && ability == nullptr;
    const auto near = [&](Vec2 point, float radius = 115.0f) { return distance(playerPosition_, point) <= radius; };

    const Vec2 wadeSpot{520.0f, 690.0f};
    const Vec2 lostFanSpot{-840.0f, -650.0f};
    if (interact && near(wadeSpot)) {
        if (!wadeLostFanStarted_) beginTransientDialogue("ch2_wade_lost_fan_start", 81);
        else if (wadeLostFanFound_ && !wadeLostFanComplete_) beginTransientDialogue("ch2_wade_lost_fan_done", 83);
        else if (wadeLostFanComplete_) showGameplayNotice("WADE'S LOST FAN • COMPLETE", .90f);
        else showGameplayNotice("WADE'S FAN • CHECK THE FESTIVAL MARKET", 1.10f);
        return true;
    }
    if (interact && wadeLostFanStarted_ && !wadeLostFanFound_ && near(lostFanSpot)) {
        beginTransientDialogue("ch2_wade_lost_fan_market", 82);
        return true;
    }

    const Vec2 fakeSpot{-250.0f, -825.0f};
    if (near(fakeSpot)) {
        if (interact && !fakeChampionContacted_) {
            beginTransientDialogue("ch2_fake_champion_start", 84);
            return true;
        }
        if (fakeChampionContacted_ && !fakeChampionRevealed_ && ability && ability->id == "lensOfTruth") {
            if (player_.energy < ability->energyCost || player_.hp <= ability->hpCost) {
                showGameplayNotice("LENS OF TRUTH • NOT ENOUGH ENERGY / HP", 1.0f);
            } else {
                player_.energy -= ability->energyCost;
                player_.hp -= ability->hpCost;
                triggerAbilityAnimation("lens_activate", .30f);
                scheduleLensBlindness(.45f);
                beginTransientDialogue("ch2_fake_champion_reveal", 85);
            }
            return true;
        }
        if (interact && fakeChampionRevealed_ && !fakeChampionComplete_) {
            startOptionalTournamentFight("ch2_fake_champion_optional");
            return true;
        }
    }

    if (interact && near({-120.0f, 920.0f}) && !runawayDummyComplete_) {
        beginTransientDialogue("ch2_runaway_dummy_start", 86);
        return true;
    }
    if (interact && near({410.0f, 780.0f}) && phaseIndex >= 10 && game_.scene().id != "ch2_intermission_timing") {
        beginTransientDialogue("ch2_bark_practice_start", 87);
        return true;
    }
    if (interact && near({-880.0f, -700.0f}) && !festivalFoodComplete_) {
        beginTransientDialogue("ch2_festival_food", 88);
        return true;
    }
    if (interact && near({-210.0f, -870.0f}) && !festivalPhotoComplete_) {
        beginTransientDialogue("ch2_festival_photo", 89);
        return true;
    }
    if (interact && near({-1050.0f, 300.0f})) {
        RuntimeView bracket;
        bracket.chapterId = "rrvvfo_ch2";
        updateTournamentProgressView(bracket, phaseIndex);
        showGameplayNotice("BRACKET • " + bracket.tournamentPhase + " • NEXT: " +
                           bracket.nextTournamentMatch + " • " + bracket.bracketSummary, 2.8f);
        return true;
    }
    return false;
}

bool RuntimeSession::tickU15TournamentActivities(InputState& input, const AbilitySlotDefinition* ability) {
    if (game_.story().chapterId != "rrvvfo_ch2" || game_.scene().kind != SceneKind::Exploration) return false;
    const std::size_t phaseIndex = game_.story().sceneIndex;
    if (phaseIndex < 6 || phaseIndex > 19) return false;
    const bool interact = input.pressed(Action::Interact) && ability == nullptr;
    const auto near = [&](Vec2 point, float radius = 135.0f) { return distance(playerPosition_, point) <= radius; };

    // Missing Prize Envelope: a small investigation loop through the same hub,
    // with a useful economy reward rather than a disposable fetch counter.
    if (interact && !missingPrizeComplete_ && near({-1030.0f, 280.0f})) {
        if (!missingPrizeStarted_) beginTransientDialogue("ch2_missing_prize_start", 90);
        else if (missingPrizeFound_ && !missingPrizeComplete_) beginTransientDialogue("ch2_missing_prize_done", 92);
        else showGameplayNotice("PRIZE ENVELOPE • SEARCH THE MARKET-LANE CRATES", 1.15f);
        return true;
    }
    if (interact && missingPrizeStarted_ && !missingPrizeFound_ && near({760.0f, -720.0f})) {
        beginTransientDialogue("ch2_missing_prize_found", 91);
        return true;
    }

    // An eliminated contestant asks for one meaningful match. Winning offers a
    // single authored stat choice; the activity never becomes a repeatable farm.
    if (interact && !oneMatchComplete_ && phaseIndex >= 10 && near({825.0f, 660.0f})) {
        beginTransientDialogue("ch2_one_match_anyway_start", 93);
        return true;
    }

    // Fire Blast is used precisely on the failed warming plate. Merely pressing
    // Interact cannot solve it, and the hotbar ability remains the shared action.
    const Vec2 foodPlate{-600.0f, -520.0f};
    if (interact && near(foodPlate)) {
        if (!controlledFlameStarted_) beginTransientDialogue("ch2_controlled_flame_start", 94);
        else if (controlledFlameComplete_) showGameplayNotice("CONTROLLED FLAME • MEAL REWARD READY", .90f);
        else showGameplayNotice("USE FIRE BLAST ON THE WARMING PLATE", 1.15f);
        return true;
    }
    if (controlledFlameStarted_ && !controlledFlameComplete_ && near(foodPlate) &&
        ability && ability->id == "fireBlast") {
        if (player_.energy < ability->energyCost || player_.hp <= ability->hpCost) {
            showGameplayNotice("FIRE BLAST • NOT ENOUGH ENERGY", 1.0f);
        } else {
            player_.energy -= ability->energyCost;
            player_.hp -= ability->hpCost;
            triggerAbilityAnimation("fire_blast", .45f);
            beginTransientDialogue("ch2_controlled_flame_done", 95);
        }
        return true;
    }

    // Alt and Rover leave an ordered, physical trail around the tournament's
    // service edge. It culminates in story staging, not a generic arena fight.
    if (interact && altRoverQuestStage_ == 0 && near({430.0f, -780.0f})) {
        beginTransientDialogue("ch2_alt_rover_start", 96);
        return true;
    }
    if (interact && altRoverQuestStage_ == 1 && near({45.0f, -920.0f})) {
        beginTransientDialogue("ch2_alt_rover_cart", 97);
        return true;
    }
    if (interact && altRoverQuestStage_ == 2 && near({1130.0f, 185.0f})) {
        beginTransientDialogue("ch2_alt_rover_sign", 98);
        return true;
    }
    if (interact && altRoverQuestStage_ == 3 && near({970.0f, -680.0f})) {
        beginTransientDialogue("ch2_alt_rover_service", 99);
        return true;
    }
    if (interact && altRoverQuestStage_ == 4 && near({1250.0f, -840.0f})) {
        beginTransientDialogue("ch2_alt_rover_confrontation", 100);
        return true;
    }
    return false;
}

void RuntimeSession::advanceDialogue() {
    const auto& scene = game_.scene();
    if (!dialogue_.has(scene.id)) { completeScene(); return; }
    const auto& lines = dialogue_.get(scene.id);
    if (dialogueIndex_ + 1 < lines.size()) { ++dialogueIndex_; dialogueAutoAdvanceTime_ = 0.0f; }
    else completeScene();
}

void RuntimeSession::confirm() {
    if (!transientDialogueId_.empty()) {
        const auto& lines = dialogue_.get(transientDialogueId_);
        if (transientDialogueIndex_ + 1 < lines.size()) ++transientDialogueIndex_;
        else finishTransientDialogue();
    } else if (game_.mode() == GameMode::Cutscene) advanceDialogue();
    else if (trainingManualVisible_) {
        activateTrainingManualSelection();
    }
    else if (sceneComplete_) completeScene();
    syncView();
}

void RuntimeSession::completeScene() {
    recordObjective(view_.objective);
    addNecklaceMastery(.65f, "OBJECTIVE COMPLETE");
    const auto completedId = game_.scene().id;
    if (completedId == "ch2_tournament_aftermath") {
        fireAwakeningUnlocked_ = true;
        manualSaveRequested_ = true;
        saveStatus_ = "AUTOSAVING…";
        saveStatusTime_ = 2.0f;
        showGameplayNotice("FIRE AWAKENING • PLAYER TECHNIQUE UNLOCKED", 1.85f);
    }
    if (completedId == "ch2_registration_card" && !tournamentCard_.acquired) {
        tournamentCard_.acquired = true;
        tournamentCardRevealTime_ = 4.5f;
        showGameplayNotice("TOURNAMENT CARD ACQUIRED", 1.35f);
    }
    if (completedId == "sage_tutorial_spar" && standaloneTrainingMode_) {
        returnToTitleRequested_ = true;
        return;
    }
    if (game_.scene().kind == SceneKind::Cutscene &&
        std::find(seenCutscenes_.begin(), seenCutscenes_.end(), completedId) == seenCutscenes_.end())
        seenCutscenes_.push_back(completedId);
    if (game_.story().chapterId == "rrvvfo_ch2" &&
        (game_.scene().kind == SceneKind::Arena || completedId == "ch2_pre_plouke" || completedId == "ch2_lost_bracket")) {
        manualSaveRequested_ = true;
        saveStatus_ = "AUTOSAVING…";
        saveStatusTime_ = 1.5f;
    }
    if (completedId == "runaway_tournament_cart") showGameplayNotice("CHECKPOINT • TOURNAMENT ROAD", 1.15f);
    else if (completedId == "roadside_encounter") showGameplayNotice("CHECKPOINT • ROADSIDE", 1.15f);
    else if (completedId == "lens_roadblock_reveal") showGameplayNotice("CHECKPOINT • OUTSKIRTS AHEAD", 1.15f);

    const auto& chapter = game_.chapter();
    const bool finalScene = game_.story().sceneIndex + 1 >= chapter.openingFlow.size();
    const auto beforeChapter = game_.story().chapterId;
    const auto beforeIndex = game_.story().sceneIndex;

    if (finalScene && beforeChapter == "rrvvfo_ch1" && chapters_.has(chapter.nextChapterId)) {
        // Preserve every completed Chapter-1 flag before the internal
        // content boundary advances into Tournament Grounds.
        auto transitionSave = saveSnapshot();
        if (!containsFlag(transitionSave.story.flags, "ch1_complete_at_outskirts"))
            transitionSave.story.flags.push_back("ch1_complete_at_outskirts");
        game_.loadSave(transitionSave);
    }
    const bool returnFromCinematic = cinematicCameraBlend_ > .001f;
    const Vec2 returnFocus = cinematicCameraFocus_;
    const float returnYaw = cinematicCameraYawDegrees_;
    const float returnDistance = cinematicCameraDistance_;
    const float returnHeight = cinematicCameraHeight_;
    const float returnFov = cinematicCameraFovDegrees_;
    game_.advanceScene();

    if (game_.story().chapterId != beforeChapter) {
        // Internal chapter IDs are save/content boundaries, never a player-facing menu break.
        chapterComplete_ = false;
        sceneComplete_ = false;
        enterCurrentScene();
        if (returnFromCinematic) {
            cinematicCameraFocus_ = returnFocus;
            cinematicCameraYawDegrees_ = returnYaw;
            cinematicCameraDistance_ = returnDistance;
            cinematicCameraHeight_ = returnHeight;
            cinematicCameraFovDegrees_ = returnFov;
            cinematicCameraBlend_ = 1.0f;
            cinematicCameraReturnTime_ = .45f;
        }
        manualSaveRequested_ = true;
        saveStatus_ = "AUTOSAVING…";
        saveStatusTime_ = 2.0f;
        showGameplayNotice("STORY CONTINUES • TOURNAMENT GROUNDS", 1.10f);
        return;
    }
    if (game_.story().sceneIndex != beforeIndex) {
        enterCurrentScene();
        if (returnFromCinematic) {
            cinematicCameraFocus_ = returnFocus;
            cinematicCameraYawDegrees_ = returnYaw;
            cinematicCameraDistance_ = returnDistance;
            cinematicCameraHeight_ = returnHeight;
            cinematicCameraFovDegrees_ = returnFov;
            cinematicCameraBlend_ = 1.0f;
            cinematicCameraReturnTime_ = .45f;
        }
        return;
    }
    if (finalScene) {
        chapterComplete_ = true;
        sceneComplete_ = true;
    }
}

SaveData RuntimeSession::saveSnapshot(const std::string& inputPreset) const {
    auto data = game_.saveData();
    data.world.mapId = game_.chapter().primaryMap;
    data.world.position = roadsideFightActive_ ? roadsideReturnPosition_ : playerPosition_;
    data.world.routeChoice = routeChoice_;
    data.world.hp = player_.hp;
    data.world.energy = player_.energy;
    data.world.guard = player_.guard;

    // Free-swap props are physical world state in every chapter that authors them.
    data.world.objects.erase(std::remove_if(data.world.objects.begin(), data.world.objects.end(), [](const WorldObjectState& object){
        return object.id.rfind("free_swap_", 0) == 0;
    }), data.world.objects.end());
    for (const auto& object : freeSwapObjects_) data.world.objects.push_back(object);

    const auto addFlag = [&](const std::string& flag) {
        if (!containsFlag(data.story.flags, flag)) data.story.flags.push_back(flag);
    };

    if (game_.story().chapterId == "rrvvfo_ch1") {
        data.world.objects.erase(std::remove_if(data.world.objects.begin(), data.world.objects.end(), [](const WorldObjectState& object){
            return object.id == "far_bank_rock" || object.id.rfind("relay_marker_", 0) == 0;
        }), data.world.objects.end());
        data.world.objects.push_back({"far_bank_rock", farBankRockPosition_, true});
        for (std::size_t i = 0; i < relayMarkers_.size(); ++i)
            data.world.objects.push_back({"relay_marker_" + std::to_string(i), relayMarkers_[i], true});

        data.story.flags.erase(std::remove_if(data.story.flags.begin(), data.story.flags.end(), [](const std::string& flag){
            return flag.rfind("ch1_tutorial_checkpoint=", 0) == 0 ||
                   flag.rfind("ch1_route_progress=", 0) == 0 ||
                   flag.rfind("ch1_relay_index=", 0) == 0 ||
                   flag.rfind("ch1_cliff_mask=", 0) == 0 ||
                   flag == "ch1_spectator_pass_pending";
        }), data.story.flags.end());

        addFlag("ch1_tutorial_checkpoint=" + std::to_string(trainingCheckpointStep_));
        addFlag("ch1_route_progress=" + std::to_string(routeProgress_));
        addFlag("ch1_relay_index=" + std::to_string(std::max(0, relayIndex_)));
        unsigned cliffMask = 0;
        for (std::size_t i = 0; i < cliffJumpComplete_.size() && i < 31; ++i)
            if (cliffJumpComplete_[i]) cliffMask |= (1u << i);
        addFlag("ch1_cliff_mask=" + std::to_string(cliffMask));
        if (mainRouteFireCleared_) addFlag("ch1_main_fire_cleared");
        if (lensRouteChosen_) addFlag("ch1_lens_route_chosen");
        if (southernDetourChosen_) addFlag("ch1_southern_detour_chosen");
        if (southernDetourComplete_) { addFlag("ch1_southern_detour_complete"); addFlag("world_delight_southern_detour"); }
        if (terrainCollapseSeen_) addFlag("ch1_terrain_collapse_seen");
        if (detourDashDone_) addFlag("ch1_detour_dash_done");
        if (detourSwapDone_) addFlag("ch1_detour_swap_done");
        if (lostCompetitorHelped_) addFlag("ch1_lost_competitor_helped");
        if (lostCompetitorDeclined_) addFlag("ch1_lost_competitor_declined");
        if (roadsideEncounterResolved_) addFlag("ch1_roadside_encounter_resolved");
        if (spectatorPassWon_) addFlag("ch1_spectator_pass_won");
        if (spectatorPassWon_ && !spectatorPassDelivered_) addFlag("ch1_spectator_pass_pending");
        if (spectatorPassDelivered_) addFlag("ch1_spectator_pass_delivered");
        if (tutorialSkipped_) addFlag("ch1_tutorial_skipped");
        if (signPuzzleStage_ >= 1) addFlag("ch1_sign_that_points_back_started");
        if (signPuzzleStage_ >= 2) addFlag("ch1_sign_that_points_back_revealed");
        if (signPuzzleStage_ >= 3) { addFlag("ch1_sign_that_points_back_complete"); addFlag("ch1_wayfinder_badge"); }
        if (transportRescued_) { addFlag("ch1_transport_rescued"); addFlag("ch1_object_swap_token"); }
        if (runawayCartSaved_) addFlag(qteAttempts_ <= 1 ? "ch1_cart_perfect_intercept" : "ch1_cart_supplies_saved");
        if (precisionSwapMastered_) addFlag("ch1_precision_swap_mastered");
        if (flowCancelLearned_) addFlag("ch1_flow_cancel_learned");
        if (cliffRewardEarned_) {
            addFlag("ch1_road_dare_badge");
            addFlag("ch1_title_road_runner");
            addFlag("ch1_cliff_overlook_discovered");
        }
        if (chapterComplete_) addFlag("ch1_complete_at_outskirts");
        for (const auto& sceneId : seenCutscenes_) addFlag("seen_ch1_scene=" + sceneId);
    }

    // U11 tournament state is flag-based to preserve schema-6 U10 saves.
    data.story.flags.erase(std::remove_if(data.story.flags.begin(), data.story.flags.end(), [](const std::string& flag){
        return flag.rfind("ch2_wade_race_best_ms=", 0) == 0 || flag.rfind("ch2_bark_practice_wins=", 0) == 0;
    }), data.story.flags.end());
    if (wadeRaceBestSeconds_ > 0.0f) addFlag("ch2_wade_race_best_ms=" + std::to_string(static_cast<unsigned>(wadeRaceBestSeconds_ * 1000.0f + .5f)));
    if (wadeRaceTargetBeaten_) addFlag("ch2_wade_race_target_beaten");
    if (crackedRingIntroSeen_) addFlag("ch2_cracked_ring_intro_seen");
    if (wadeLostFanStarted_) addFlag("ch2_wade_lost_fan_started");
    if (wadeLostFanFound_) addFlag("ch2_wade_lost_fan_found");
    if (wadeLostFanComplete_) addFlag("ch2_wade_lost_fan_complete");
    if (fakeChampionContacted_) addFlag("ch2_fake_champion_contacted");
    if (fakeChampionRevealed_) addFlag("ch2_fake_champion_revealed");
    if (fakeChampionComplete_) addFlag("ch2_fake_champion_complete");
    if (runawayDummyComplete_) addFlag("ch2_runaway_dummy_complete");
    if (festivalFoodComplete_) addFlag("ch2_festival_food_complete");
    if (festivalPhotoComplete_) addFlag("ch2_festival_photo_complete");
    if (missingPrizeStarted_) addFlag("ch2_missing_prize_started");
    if (missingPrizeFound_) addFlag("ch2_missing_prize_found");
    if (missingPrizeComplete_) addFlag("ch2_missing_prize_complete");
    if (oneMatchComplete_) addFlag("ch2_one_match_anyway_complete");
    if (controlledFlameStarted_) addFlag("ch2_controlled_flame_started");
    if (controlledFlameComplete_) addFlag("ch2_controlled_flame_complete");
    data.story.flags.erase(std::remove_if(data.story.flags.begin(), data.story.flags.end(), [](const std::string& flag){
        return flag.rfind("ch2_alt_rover_stage=", 0) == 0;
    }), data.story.flags.end());
    if (altRoverQuestStage_ > 0) addFlag("ch2_alt_rover_stage=" + std::to_string(altRoverQuestStage_));
    if (barkPracticeWins_ > 0) addFlag("ch2_bark_practice_wins=" + std::to_string(barkPracticeWins_));
    if (fireAwakeningUnlocked_) addFlag("rrvvfo_fire_awakening_unlocked");
    for (const auto& sceneId : seenCutscenes_) addFlag("seen_scene=" + sceneId);

    // Generic authored sequence progress belongs to the current scene only.
    const std::string sceneProgressPrefix = "scene_progress=" + game_.story().chapterId + ":" + game_.scene().id + ":";
    data.story.flags.erase(std::remove_if(data.story.flags.begin(), data.story.flags.end(), [&](const std::string& flag){
        return flag.rfind(sceneProgressPrefix, 0) == 0;
    }), data.story.flags.end());
    if (sceneSequenceProgress_ > 0) data.story.flags.push_back(sceneProgressPrefix + std::to_string(sceneSequenceProgress_));

    data.tournamentCard = tournamentCard_;
    data.frontend.objectiveHistory = objectiveHistory_;
    data.frontend.currentArea = areaNameForPosition(data.world.position);
    data.frontend.currentObjective = view_.objective;
    const float ch1Scenes = static_cast<float>(chapters_.get("rrvvfo_ch1").openingFlow.size());
    const float ch2Scenes = static_cast<float>(chapters_.get("rrvvfo_ch2").openingFlow.size());
    const float completedScenes = game_.story().chapterId == "rrvvfo_ch2" ?
        ch1Scenes + static_cast<float>(game_.story().sceneIndex) : static_cast<float>(game_.story().sceneIndex);
    data.frontend.storyProgressPercent = std::clamp(100.0f * completedScenes / std::max(1.0f, ch1Scenes + ch2Scenes), 0.0f, 100.0f);
    data.frontend.playtimeSeconds = storyPlaytimeSeconds_;
    data.qol = qolSettings_;
    data.records = adventureRecords_;
    data.rpg = rpgProgress_;
    data.inputPreset = inputPreset;
    return data;
}

std::string RuntimeSession::areaNameForPosition(Vec2 position) const {
    const auto& zones = map().zones;
    if (zones.empty()) return map().name;
    const MapZone* nearest = &zones.front();
    float nearestDistance = distance(position, nearest->center);
    for (const auto& zone : zones) {
        const float d = distance(position, zone.center);
        if (d < nearestDistance) { nearest = &zone; nearestDistance = d; }
    }
    return nearest->name;
}

void RuntimeSession::syncView() {
    view_.mode = (roadsideFightActive_ || !activeOptionalEncounterId_.empty()) ? GameMode::ArenaCombat : game_.mode();
    view_.chapterId = game_.story().chapterId;
    view_.sceneId = game_.scene().id;
    updateTournamentProgressView(view_, game_.story().sceneIndex);
    view_.playerPosition = playerPosition_;
    view_.opponentPosition = opponentPosition_;
    view_.player = player_;
    view_.opponent = opponent_;
    view_.sparCleanHits = sparCleanHits_;
    view_.relayProgress = relayIndex_;
    view_.sceneComplete = sceneComplete_;
    const auto& chapterHotbar = game_.story().chapterId == "rrvvfo_ch1" ? AbilityHotbarCatalog::rrvvfoChapter1() :
        fireAwakeningUnlocked_ ? AbilityHotbarCatalog::rrvvfoPostChapter2() : AbilityHotbarCatalog::rrvvfoChapter2();
    const bool tutorialScene = training_.has(game_.scene().id);
    view_.hotbarVisible = !tutorialScene || (!trainingManualVisible_ && trainingStepIndex_ >= 4);
    const std::size_t desiredHotbarSize = !view_.hotbarVisible ? 0 :
        (tutorialScene && trainingStepIndex_ == 4 ? std::min<std::size_t>(2, chapterHotbar.size()) : chapterHotbar.size());
    if (desiredHotbarSize == 0) {
        view_.hotbar.clear();
    } else {
        // The Chapter-1 layout is immutable. Recopy it only when visibility/lesson size changes.
        if (view_.hotbar.size() < desiredHotbarSize) view_.hotbar = chapterHotbar;
        if (view_.hotbar.size() > desiredHotbarSize) view_.hotbar.resize(desiredHotbarSize);
        // The slot remains Energy Beam; only its active awakened expression is
        // Solar Weave. Shots of Agony is never inserted by Chapter 2.
        for (auto& ability : view_.hotbar) {
            if (ability.id == "energyBeam") {
                ability.label = fireAwakeningTime_ > 0.0f ? "SOLAR WEAVE" : "ENERGY BEAM";
                ability.stateLabel = fireAwakeningTime_ > 0.0f ? "AWAKENED" : "PURE ENERGY";
            }
        }
    }
    view_.showPlayerHealth = view_.mode == GameMode::ArenaCombat && !trainingManualVisible_;
    view_.showOpponentHealth = roadsideFightActive_ || storyArenaActive_ || (tutorialScene && !trainingManualVisible_ &&
        (trainingStepIndex_ == 1 || trainingStepIndex_ == 2 || trainingStepIndex_ >= 5));
    view_.showEnergy = roadsideFightActive_ || storyArenaActive_ || (tutorialScene && !trainingManualVisible_ && trainingStepIndex_ >= 3);
    view_.showGuard = roadsideFightActive_ || storyArenaActive_ || (tutorialScene && !trainingManualVisible_ &&
        (trainingStepIndex_ == 2 || trainingStepIndex_ >= 5));
    view_.routeChoice = routeChoice_;
    view_.presentationStageId = game_.scene().presentationStageId;
    view_.currentArea = areaNameForPosition(playerPosition_);
    view_.objectiveDetail.clear();
    view_.trainingManualVisible = trainingManualVisible_;
    view_.trainingManualSelection = trainingManualSelection_;
    view_.trainingCheckpointLabel = trainingCheckpointLabel(trainingCheckpointStep_);
    view_.trainingManualPageIndex = std::min(trainingManualPageIndex_, manual_.pages().size() - 1);
    view_.trainingManualPageCount = manual_.pages().size();
    view_.trainingManualPage = manual_.pages()[view_.trainingManualPageIndex];
    view_.trainingManualOptions = {"SAGE'S FULL CHALLENGE"};
    if (trainingCheckpointStep_ > 0) view_.trainingManualOptions.push_back("RESUME • " + trainingCheckpointLabel(trainingCheckpointStep_));
    view_.trainingManualOptions.push_back("QUICK ABILITY REFRESHER");
    view_.trainingManualOptions.push_back(standaloneTrainingMode_ ? "EXIT • RETURN TO MENU" : "SKIP • START THE ROAD");
    view_.trainingStepIndex = trainingStepIndex_;
    view_.trainingProgress = trainingProgress_;
    view_.trainingMovementDistance = trainingMovementDistance_;
    view_.playerHeight = movementState_.height;
    view_.opponentHeight = opponentHeight_;
    view_.playerDashing = movementState_.dashing;
    view_.opponentAttackTelegraphed = opponentAttackTelegraphed_;
    view_.opponentAttackAttempt = opponentAttackAttempts_;
    view_.opponentAttackSecondsRemaining = 0.0f;
    if (const auto* trainingStep = currentTrainingStep()) {
        const auto& trainingDefinition = training_.get(game_.scene().id);
        if (trainingStep->task == TrainingTaskKind::PerfectBlock) {
            const float delay = opponentAttackAttempts_ == 0 ? trainingDefinition.parryFirstAttackSeconds
                                                              : trainingDefinition.parryRetryAttackSeconds;
            view_.opponentAttackSecondsRemaining = std::max(0.0f, delay - opponentAttackTimer_);
        } else if (trainingStep->task == TrainingTaskKind::LensRead && lensActive_) {
            const float delay = opponentAttackAttempts_ == 0 ? trainingDefinition.lensFirstAttackSeconds
                                                              : trainingDefinition.lensRetryAttackSeconds;
            view_.opponentAttackSecondsRemaining = std::max(0.0f, delay - opponentAttackTimer_);
        }
    }
    view_.lensActive = lensActive_;
    view_.playerYawDegrees = playerYawDegrees_;
    view_.opponentYawDegrees = opponentYawDegrees_;
    view_.trainingStepCount = 0;
    view_.trainingRequired = 0;
    view_.trainingPrompt.clear();
    view_.dialogueVisible = false;
    view_.dialogueSpeaker.clear();
    view_.dialogueText.clear();
    view_.dialoguePortraitId.clear();
    view_.dialogueExpression.clear();
    view_.dialogueFocusActorId.clear();
    view_.dialogueCount = 0;
    view_.ambientActors.clear();
    view_.worldMarkers.clear();
    view_.choiceVisible = choiceKind_ != 0;
    view_.choiceIndex = choiceIndex_;
    view_.choiceOptions.clear();
    view_.choiceTitle.clear();
    view_.qteVisible = qteActive_ || ploukeFinalClashActive_;
    view_.qteTitle.clear();
    view_.qteSequence.clear();
    view_.qteIndex = qteIndex_;
    view_.qteSecondsRemaining = qteRemaining_;
    view_.qteAttempt = qteAttempts_;
    view_.pauseVisible = game_.mode() == GameMode::Pause;
    view_.pausePageTitle = pausePage_ == 1 ? "OBJECTIVE HISTORY" : pausePage_ == 2 ? "CONTROLS" :
                           pausePage_ == 3 ? "ACCESSIBILITY & QOL" : pausePage_ == 4 ? "RECENT DIALOGUE" :
                           pausePage_ == 5 ? "ACCESSORY / TRAINING ITEM" :
                           (game_.story().chapterId == "rrvvfo_ch2" ? "STORY MENU • TOURNAMENT GROUNDS" :
                                                                    "STORY MENU • TOURNAMENT ROAD");
    view_.pauseSections.clear();
    view_.pauseOptions.clear();
    view_.pauseSelection = pauseSelection_;
    view_.manualSaveAllowed = canManualSave();
    view_.saveStatus = saveStatus_;
    view_.objectiveHistory = objectiveHistory_;
    view_.recentDialogue = recentDialogue_;
    if (pausePage_ == 0) {
        view_.pauseSections = {
            "CURRENT OBJECTIVE • " + (objectiveBeforePause_.empty() ?
                (view_.chapterId == "rrvvfo_ch2" ? std::string{"Continue through the Tournament Grounds"} : std::string{"Continue Tournament Road"}) :
                objectiveBeforePause_),
            "CHECKPOINT • " + game_.story().checkpointId,
            "PARALLELS X 3.0R • SHARED STORY SAVE"
        };
        if (view_.chapterId == "rrvvfo_ch2") {
            view_.pauseSections.insert(view_.pauseSections.begin() + 1, "ROUND • " + view_.tournamentPhase);
            view_.pauseSections.insert(view_.pauseSections.begin() + 2, "NEXT • " + view_.nextTournamentMatch);
        }
        if (standaloneMode()) {
            const std::string sessionName = replayMode_ ? "STORY REPLAY • MAIN SAVE PROTECTED" :
                                            standaloneFightMode_ ? "FIGHT • RRVVFO VS CPU" : "TRAINING • SAGE'S CHALLENGE";
            const std::string restartName = replayMode_ ? "RESTART STORY REPLAY" :
                                            standaloneFightMode_ ? "RESTART FIGHT" : "RESTART TRAINING";
            view_.pauseSections = {sessionName};
            view_.pauseOptions = {"RESUME", restartName, "ACCESSIBILITY & QOL", "RETURN TO TITLE"};
        } else {
            view_.pauseOptions = {
                "RESUME", canManualSave() ? "SAVE GAME" : "SAVE GAME • UNAVAILABLE HERE",
                "RESTART FROM CHECKPOINT", "OBJECTIVE HISTORY", "CONTROLS", "RECENT DIALOGUE",
                "ACCESSORY / TRAINING ITEM", "ACCESSIBILITY & QOL", "RETURN TO TITLE"
            };
        }
    } else if (pausePage_ == 1) {
        if (!objectiveBeforePause_.empty()) view_.pauseSections.push_back("NOW • " + objectiveBeforePause_);
        if (view_.chapterId == "rrvvfo_ch2") {
            view_.pauseSections.push_back("ROUND • " + view_.tournamentPhase);
            view_.pauseSections.push_back("NEXT • " + view_.nextTournamentMatch);
            view_.pauseSections.push_back("BRACKET • " + view_.bracketSummary);
        }
        if (objectiveHistory_.empty()) view_.pauseSections.push_back("No completed objectives yet.");
        else for (auto it = objectiveHistory_.rbegin(); it != objectiveHistory_.rend(); ++it)
            view_.pauseSections.push_back("DONE • " + *it);
        view_.pauseSections.push_back("B / ESC • BACK");
    } else if (pausePage_ == 2) {
        view_.pauseSections = {
            "MOVE • WASD / LEFT STICK / CIRCLE PAD",
            "JUMP • SPACE / SOUTH BUTTON / B",
            "COMBAT • LIGHT / HEAVY / LAUNCHER / GRAB / GUARD",
            "ADVANCED • DASH / COUNTER / BREAKER / CHARGE",
            "ABILITIES • 1–5 OR HOLD LB/L + FACE BUTTONS",
            "B / ESC • BACK"
        };
        if (fireAwakeningUnlocked_) {
            view_.pauseSections.insert(view_.pauseSections.end() - 1,
                "FIRE AWAKENING • 35 ENERGY, THEN DRAINS • ENERGY BEAM BECOMES SOLAR WEAVE");
        } else if (game_.story().chapterId == "rrvvfo_ch2") {
            view_.pauseSections.insert(view_.pauseSections.end() - 1,
                "ENERGY POWER • CHARGE HIGH FOR A MODEST DAMAGE AND KNOCKBACK BONUS");
        }
    } else if (pausePage_ == 4) {
        if (recentDialogue_.empty()) view_.pauseSections.push_back("No dialogue recorded in this session yet.");
        else for (auto it = recentDialogue_.rbegin(); it != recentDialogue_.rend(); ++it)
            view_.pauseSections.push_back(*it);
        view_.pauseSections.push_back("B / ESC • BACK");
    } else if (pausePage_ == 5) {
        if (!rpgProgress_.weightedNecklaceAcquired) {
            view_.pauseSections = {"No accessory or training item acquired yet.", "ONE GAMEPLAY SLOT • NO INVENTORY BLOAT", "B / ESC • BACK"};
        } else {
            const bool equipped = rpgProgress_.equippedAccessoryId == "alts_weighted_necklace";
            const int mastery = static_cast<int>(rpgProgress_.weightedNecklaceMastery + .5f);
            const int penalty = static_cast<int>((1.0f - RpgProgressSystem::necklaceMovementMultiplier(rpgProgress_)) * 100.0f + .5f);
            view_.pauseSections = {
                "ALT'S WEIGHTED NECKLACE • TRAINING GEAR",
                "MASTERY • " + std::to_string(mastery) + "%",
                "POWER • +4%",
                penalty > 0 ? "WEIGHT • -" + std::to_string(penalty) + "% MOVE / DASH / CHARGE" : "WEIGHT PENALTIES • MASTERED",
                "B / ESC • BACK"
            };
            view_.pauseOptions = {equipped ? "REMOVE NECKLACE" : "EQUIP NECKLACE"};
        }
    } else {
        const auto onOff = [](bool value){ return value ? "ON" : "OFF"; };
        view_.pauseOptions = {
            "HOLD TO ADVANCE DIALOGUE • " + std::string(onOff(qolSettings_.holdToAdvanceDialogue)),
            "AUTO-ADVANCE • " + std::string(onOff(qolSettings_.dialogueAutoAdvance)),
            "DIALOGUE SPEED • " + qolSettings_.dialogueSpeed,
            "CONTEXTUAL HINTS • " + std::string(onOff(qolSettings_.firstTimeHints)),
            "REDUCED MOTION • " + std::string(onOff(qolSettings_.reducedMotion)),
            "REDUCED CAMERA SHAKE • " + std::string(onOff(qolSettings_.reducedCameraShake)),
            "REDUCED FLASHES • " + std::string(onOff(qolSettings_.reducedFlashes)),
            "HIGH-CONTRAST HUD • " + std::string(onOff(qolSettings_.highContrastHud)),
            "CAMERA SENSITIVITY • " + std::to_string(static_cast<int>(qolSettings_.cameraSensitivity * 100.0f + .5f)) + "%",
            "INVERT CAMERA X • " + std::string(onOff(qolSettings_.invertCameraX)),
            "INVERT CAMERA Y • " + std::string(onOff(qolSettings_.invertCameraY)),
            "GENTLE RECENTER • " + std::string(onOff(qolSettings_.gentleCameraRecenter)),
            "HUD SCALE • " + std::to_string(static_cast<int>(qolSettings_.hudScale * 100.0f + .5f)) + "%",
            "DIALOGUE SCALE • " + std::to_string(static_cast<int>(qolSettings_.dialogueScale * 100.0f + .5f)) + "%",
            "COMBAT MESSAGES • " + qolSettings_.combatMessages,
            "OBJECTIVE DISPLAY • " + qolSettings_.objectiveDisplay
        };
        view_.pauseSections = {"CONFIRM / LEFT / RIGHT • TOGGLE", "B / ESC • BACK"};
    }
    view_.holdToAdvanceDialogue = qolSettings_.holdToAdvanceDialogue;
    view_.reducedMotion = qolSettings_.reducedMotion;
    view_.reducedCameraShake = qolSettings_.reducedCameraShake;
    view_.reducedFlashes = qolSettings_.reducedFlashes;
    view_.highContrastHud = qolSettings_.highContrastHud;
    view_.largerText = qolSettings_.largerText || qolSettings_.dialogueScale > 1.05f;
    view_.combatMessages = qolSettings_.combatMessages;
    view_.hudScale = qolSettings_.hudScale;
    view_.dialogueScale = qolSettings_.dialogueScale;
    view_.objectiveDisplay = qolSettings_.objectiveDisplay;
    view_.cameraYawOffsetDegrees = userCameraYawOffset_;
    view_.cameraHeightOffset = userCameraHeightOffset_;
    view_.battleRankVisible = battleRankTime_ > 0.0f;
    view_.battleRank = battleRankLabel_;
    view_.battleRankScore = battleRankScore_;
    view_.adventureRecords = adventureRecords_;
    view_.rpgProgress = rpgProgress_;
    view_.necklaceMovementMultiplier = RpgProgressSystem::necklaceMovementMultiplier(rpgProgress_);
    view_.chapterComplete = chapterComplete_;
    view_.nextChapterId = game_.hasPendingChapter() ? game_.story().pendingChapterId : game_.chapter().nextChapterId;
    view_.roadsideFightActive = roadsideFightActive_;
    view_.nearbyInteractionLabel.clear();
    view_.gameplayNotice = gameplayNotice_;
    view_.bufferedCombatInput = hasBufferedCombatAction_ ? (bufferedCombatAction_ == Action::Light ? "LIGHT" : bufferedCombatAction_ == Action::Heavy ? "HEAVY" : bufferedCombatAction_ == Action::Launcher ? "LAUNCHER" : "GRAB") : "";
    view_.flowCancelReady = player_.flowCancelWindow > 0.0f;
    view_.hitFreezeSeconds = hitFreezeTime_;
    view_.cameraImpulse = cameraImpulse_ * (qolSettings_.reducedMotion ? 0.0f :
                                            qolSettings_.reducedCameraShake ? 0.30f : 1.0f);
    view_.impactFlash = impactFlash_ * (qolSettings_.reducedFlashes ? 0.20f : 1.0f);
    const bool importantCombatMessage = perfectBlockFlash_ || pursuitFinishFlash_ || guardBreakFlash_ || finalHitFlash_;
    view_.combatFeedback = qolSettings_.combatMessages == "off" ||
        (qolSettings_.combatMessages == "minimal" && !importantCombatMessage) ? std::string{} : combatFeedback_;
    view_.perfectBlockFlash = perfectBlockFlash_;
    view_.pursuitFinishFlash = pursuitFinishFlash_;
    view_.guardBreakFlash = guardBreakFlash_;
    view_.finalHitFlash = finalHitFlash_;
    view_.playerAnimation = resolvePlayerAnimation();
    // Combat retreat is now its own authored clip; never reverse-play the hub sprint.
    view_.playerAnimationSpeed = 1.0f;
    view_.playerFaceSeconds = adventureTime_;
    view_.playerMoving = distance(previousPlayerPosition_, playerPosition_) > 0.35f;
    view_.playerBlocking = player_.blocking;
    view_.playerCharging = playerCharging_;
    view_.playerAirborne = movementState_.height > 0.0f || player_.airborne;
    view_.playerFalling = movementState_.height > 0.0f && movementState_.verticalVelocity < -35.0f;

view_.cinematicCameraActive = cinematicCameraBlend_ > .001f;
view_.cinematicCameraBlend = cinematicCameraBlend_;
view_.cinematicCameraFocus = cinematicCameraFocus_;
view_.cinematicCameraYawDegrees = cinematicCameraYawDegrees_;
view_.cinematicCameraDistance = cinematicCameraDistance_;
view_.cinematicCameraHeight = cinematicCameraHeight_;
view_.cinematicCameraFovDegrees = cinematicCameraFovDegrees_;
const bool nearDenseHubGeometry = view_.presentationStageId == "tournament-hub" &&
    (playerPosition_.x > 720.0f || playerPosition_.z < -610.0f || playerPosition_.z > 720.0f);
view_.cinematicCameraOcclusionRescue = cinematicCameraBlend_ <= .001f && view_.playerMoving && nearDenseHubGeometry;
view_.cinematicExpression = cinematicExpression_;
view_.cinematicWorldEvent = cinematicWorldEvent_;
if (ploukeAwakeningVisualTime_ > 0.0f || ploukeFinalClashActive_) {
    // The failed transformation is a real final-match story beat, not a HUD
    // notification. Hold the same readable close composition on every target.
    view_.cinematicCameraActive = true;
    view_.cinematicCameraBlend = 1.0f;
    view_.cinematicCameraFocus = playerPosition_;
    view_.cinematicCameraYawDegrees = 26.0f;
    view_.cinematicCameraDistance = 580.0f;
    view_.cinematicCameraHeight = 260.0f;
    view_.cinematicCameraFovDegrees = 37.0f;
}
view_.tournamentCard = tournamentCard_;
view_.tournamentCardVisible = tournamentCard_.acquired && tournamentCardRevealTime_ > 0.0f;
view_.tournamentBonusRoll = tournamentBonusRoll_;
view_.playerStocksLost = storyPlayerStocksLost_;
view_.opponentStocksLost = storyOpponentStocksLost_;
const std::string visibleEncounterId = activeOptionalEncounterId_.empty() ? view_.sceneId : activeOptionalEncounterId_;
view_.stockTarget = arenaEncounters_.has(visibleEncounterId) ? arenaEncounters_.get(visibleEncounterId).stockTarget : 0;
view_.recommendedLevel = arenaEncounters_.has(visibleEncounterId) ? arenaEncounters_.get(visibleEncounterId).recommendedLevel : 0;
view_.officialTournamentMatch = arenaEncounters_.has(visibleEncounterId) && arenaEncounters_.get(visibleEncounterId).official;
view_.optionalTournamentFight = !activeOptionalEncounterId_.empty();
view_.optionalTournamentActivity = activeOptionalEncounterId_;
view_.ploukeAwakeningFailureActive = ploukeAwakeningVisualTime_ > 0.0f;
view_.wadeRaceBestSeconds = wadeRaceBestSeconds_;
view_.wadeRaceTargetBeaten = wadeRaceTargetBeaten_;
view_.energyPowerMultiplier = CombatSystem::energyPowerMultiplier(player_);
view_.energyChargePulse = playerCharging_ ? (.30f + .70f * std::abs(std::sin(adventureTime_ * (4.5f + player_.energy * .025f)))) *
                                              (.55f + .45f * player_.energy / 100.0f) : 0.0f;
view_.pureEnergyBeamActive = player_.activeAttack == AttackKind::Beam && fireAwakeningTime_ <= 0.0f;
view_.fireAwakeningUnlocked = fireAwakeningUnlocked_;
view_.fireAwakeningActive = fireAwakeningTime_ > 0.0f;
view_.fireAwakeningSeconds = fireAwakeningTime_;
view_.solarWeaveActive = player_.activeAttack == AttackKind::Beam && fireAwakeningTime_ > 0.0f;
view_.tournamentWorldState.clear();
updateTournamentProgressView(view_, game_.story().sceneIndex);
if (view_.chapterId == "rrvvfo_ch2") {
    if (view_.sceneId == "ch2_tournament_aftermath" || view_.sceneId == "ch2_plouke_reveal")
        view_.tournamentWorldState = "cleanup";
    else if (view_.sceneId == "ch2_pre_plouke" || view_.sceneId == "ch2_vs_plouke")
        view_.tournamentWorldState = "final";
    else if (view_.sceneId == "ch2_opening_ceremony" || view_.sceneId.rfind("ch2_vs_",0)==0 ||
             view_.sceneId.rfind("ch2_intermission_",0)==0 || view_.sceneId == "ch2_bark_pouki")
        view_.tournamentWorldState = "tournament";
    else
        view_.tournamentWorldState = "festival";
}
    if (choiceKind_ == 20) {
        view_.choiceTitle = "LEVEL UP • BONUS +" + std::to_string(tournamentBonusRoll_) + " • CHOOSE A STAT";
        view_.choiceOptions = {"HP","POWER","DEFENSE","SPEED","FOCUS"};
    } else if (choiceKind_ == 1) {
        view_.choiceTitle = "HELP HIM?";
        view_.choiceOptions = {"HELP HIM", "DON’T HELP"};
    } else if (choiceKind_ == 2) {
        view_.choiceTitle = "ROADSIDE CHALLENGER";
        view_.choiceOptions = {"FIGHT HIM", "LEAVE"};
    } else if (choiceKind_ == 3) {
        view_.choiceTitle = "OPTIONAL ENCOUNTER";
        view_.choiceOptions = {"TRY AGAIN", "LEAVE ENCOUNTER"};
    } else if (choiceKind_ == 4) {
        view_.choiceTitle = "CHOOSE A ROUTE";
        view_.choiceOptions = {"MAIN ROAD • FIRE / WORK", "FOREST • BELLS / NAVIGATION", "CLIFF • JUMPS / VIEW"};
    } else if (choiceKind_ == 5) {
        view_.choiceTitle = "FINAL ROADBLOCK";
        view_.choiceOptions = {"USE LENS OF TRUTH", "TAKE SOUTH DETOUR"};
    } else if (choiceKind_ == 22) {
        view_.choiceTitle = "ONE MATCH ANYWAY • CHOOSE YOUR TRAINING REWARD";
        view_.choiceOptions = {"POWER +1", "SPEED +1"};
    } else if (choiceKind_ == 30) {
        view_.choiceTitle = "OPTIONAL FIGHT LOST";
        view_.choiceOptions = {"RETRY", "LEAVE"};
    } else if (choiceKind_ == 31) {
        view_.choiceTitle = "OFFICIAL MATCH LOST • NO FORFEIT";
        view_.choiceOptions = {"RETRY"};
    }

    if (const auto* adventure = currentAdventure()) {
        for (const auto& npc : adventure->npcs) {
            const auto position = adventureNpcPosition(npc);
            const bool interactable = distance(playerPosition_, position) <= npc.interactionRadius + 30.0f;
            float npcYaw = 0.0f;
            if (interactable) { const Vec2 toPlayer{playerPosition_.x-position.x, playerPosition_.z-position.z}; npcYaw = yawForDirection(toPlayer); }
            view_.ambientActors.push_back({npc.id, npc.characterPresentationId, position, npcYaw, interactable});
            if (distance(playerPosition_, position) <= npc.interactionRadius + 30.0f)
                view_.nearbyInteractionLabel = npc.label;
        }
        const auto& life = adventure->ambientLife;
        for (int i = 0; i < life.birdCount; ++i) {
            const float travel = std::fmod(adventureTime_ * life.birdSpeed + static_cast<float>(i) * 570.0f,
                                           life.birdTravelDistance);
            const Vec2 position{life.birdStartX + travel,
                life.birdStartZ + static_cast<float>(i) * life.birdZSpacing +
                std::sin(adventureTime_ * 1.7f + static_cast<float>(i)) * 55.0f};
            view_.worldMarkers.push_back({"road_bird_" + std::to_string(i), position, "bird", false});
        }
        const float cartTravel = std::fmod(adventureTime_ * life.deliveryCartSpeed, life.deliveryCartTravelDistance);
        view_.worldMarkers.push_back({"delivery_cart", {life.deliveryCartStart.x + cartTravel, life.deliveryCartStart.z}, "delivery-cart", false});
        if (runawayCartSaved_) {
            const Vec2 savedCartPosition = view_.sceneId == "reach_tournament_outskirts"
                ? Vec2{1815.0f, -185.0f} : life.savedCartPosition;
            view_.worldMarkers.push_back({"saved_supply_cart", savedCartPosition, "parked-cart", true});
        }
        view_.worldMarkers.push_back({"far_bank_rock", farBankRockPosition_, "swap-rock", false});
        if (!adventure->sidePuzzle.id.empty()) {
            const auto& puzzle = adventure->sidePuzzle;
            if (signPuzzleStage_ == 1)
                view_.worldMarkers.push_back({puzzle.id, puzzle.lensTarget, "hidden-sign", false});
            else if (signPuzzleStage_ == 2) {
                view_.worldMarkers.push_back({puzzle.id, puzzle.lensTarget, "revealed-sign", false});
                view_.worldMarkers.push_back({puzzle.id + "_post", puzzle.swapTarget, "sign-post", false});
            } else if (signPuzzleStage_ >= 3)
                view_.worldMarkers.push_back({puzzle.id, puzzle.swapTarget, "corrected-sign", true});
        }
        if (view_.sceneId == "selected_route_adventure" && routeChoice_ == "main") {
            const auto& definition = exploration_.get(view_.sceneId);
            for (std::size_t i = 0; i < definition.mainWorkMarkers.size(); ++i)
                view_.worldMarkers.push_back({"main_work_" + std::to_string(i + 1), definition.mainWorkMarkers[i], "work-lane", i < routeProgress_});
        }
        if (view_.sceneId == "selected_route_adventure" && routeChoice_ == "forest") {
            const auto& definition = exploration_.get(view_.sceneId);
            for (std::size_t i = 0; i < definition.forestBellMarkers.size(); ++i)
                view_.worldMarkers.push_back({"forest_bell_" + std::to_string(i + 1), definition.forestBellMarkers[i], "blue-bell", i < routeProgress_});
        }
        if (view_.sceneId == "selected_route_adventure" && routeChoice_ == "cliff") {
            const auto& definition = exploration_.get(view_.sceneId);
            for (std::size_t i = 0; i < definition.jumpMarkers.size(); ++i) {
                const bool complete = i < cliffJumpComplete_.size() && cliffJumpComplete_[i];
                view_.worldMarkers.push_back({"cliff_jump_" + std::to_string(i + 1), definition.jumpMarkers[i], "cliff-jump", complete});
            }
        }
        if (view_.sceneId == "swap_relay_trial") {
            for (std::size_t i = 0; i < relayMarkers_.size(); ++i) {
                view_.worldMarkers.push_back({"relay_" + std::to_string(i + 1), relayMarkers_[i], "swap-relay", static_cast<int>(i) < relayIndex_});
            }
        }
        if (view_.sceneId == "collapsed_tournament_road_detour") {
            const auto& definition = exploration_.get(view_.sceneId);
            for (std::size_t i=0;i<definition.jumpMarkers.size();++i)
                view_.worldMarkers.push_back({"collapse_jump_"+std::to_string(i+1),definition.jumpMarkers[i],"cliff-jump",i<routeProgress_});
            view_.worldMarkers.push_back({"collapse_dash",definition.detourDashMarker,"work-lane",detourDashDone_});
            view_.worldMarkers.push_back({"collapse_swap",definition.shortcutPoint,"swap-relay",detourSwapDone_});
        }
        if (view_.sceneId == "transport_wheel_recovery") {
            if (!relayMarkers_.empty() && relayIndex_ == 0) view_.worldMarkers.push_back({"transport_wheel", relayMarkers_[0], "transport-wheel", false});
            if (relayMarkers_.size() > 1 && relayIndex_ > 0) view_.worldMarkers.push_back({"transport_return", relayMarkers_[1], "return-anchor", relayIndex_ > 1});
        }
        if (qteActive_) {
            view_.qteTitle = adventure->runawayCart.title;
            view_.qteSequence = adventure->runawayCart.sequence;
        }
    }

view_.objectSwapCooldownSeconds = objectSwapCooldownTime_;
view_.objectSwapPhaseSeconds = objectSwapPhaseTime_;
view_.objectSwapGhostFrom = objectSwapGhostFrom_;
view_.objectSwapGhostTo = objectSwapGhostTo_;
view_.objectSwapTargetLocked = false;
view_.objectSwapTargetPosition = {};
if (game_.mode() == GameMode::Exploration && objectSwapCooldownTime_ <= 0.0f) {
    constexpr float kFreeSwapRange = 225.0f;
    float best = kFreeSwapRange;
    for (const auto& object : freeSwapObjects_) {
        const float d = distance(playerPosition_, object.position);
        if (object.active && d <= best) { best = d; view_.objectSwapTargetLocked = true; view_.objectSwapTargetPosition = object.position; }
    }
}
if (ploukeFinalClashActive_) {
    view_.qteTitle = "FINAL BEAM CLASH • SPEND WHAT IS LEFT";
    view_.qteSequence = {Action::Charge, Action::Ability2, Action::Light, Action::Heavy};
    view_.qteIndex = ploukeFinalClashIndex_;
    view_.qteSecondsRemaining = ploukeFinalClashTime_;
    view_.qteAttempt = 1;
    view_.worldMarkers.push_back({"plouke_final_beam_clash",
        {(playerPosition_.x + opponentPosition_.x) * .5f, (playerPosition_.z + opponentPosition_.z) * .5f},
        "beam-clash", ploukeFinalClashIndex_ >= 3});
}
view_.lensBlindnessAmount = lensBlindnessTime_ <= 0.0f ? 0.0f : std::clamp(lensBlindnessTime_ / lensBlindnessDuration_, 0.0f, 1.0f);
if (view_.objectSwapTargetLocked)
    view_.worldMarkers.push_back({"rrvvfo_free_swap_lock", view_.objectSwapTargetPosition, "object-swap-lock", false});
if (objectSwapPhaseTime_ > 0.0f) {
    view_.worldMarkers.push_back({"rrvvfo_swap_ghost_from", objectSwapGhostFrom_, "object-swap-ghost", false});
    view_.worldMarkers.push_back({"rrvvfo_swap_ghost_to", objectSwapGhostTo_, "object-swap-ghost", true});
    view_.worldMarkers.push_back({"rrvvfo_swap_phase", playerPosition_, "object-swap-phase", true});
}
for (const auto& object : freeSwapObjects_)
    view_.worldMarkers.push_back({object.id, object.position, "free-swap-object", false});
view_.storySequenceProgress = sceneSequenceProgress_;
view_.storySequenceSeconds = sceneSequenceSeconds_;
view_.storySequenceCount = 0;
if (exploration_.has(view_.sceneId)) {
    const auto& sequenceDefinition = exploration_.get(view_.sceneId);
    if (!sequenceDefinition.sequenceMarkers.empty()) {
        view_.storySequenceCount = sequenceDefinition.sequenceMarkers.size();
        for (std::size_t i = 0; i < sequenceDefinition.sequenceMarkers.size(); ++i) {
            view_.worldMarkers.push_back({"story_sequence_" + std::to_string(i), sequenceDefinition.sequenceMarkers[i],
                i < sceneSequenceProgress_ ? "sequence-complete" : i == sceneSequenceProgress_ ? "sequence-active" : "sequence-upcoming",
                i < sceneSequenceProgress_});
        }
        if (sceneSequenceProgress_ < sequenceDefinition.sequenceMarkers.size() &&
            distance(playerPosition_, sequenceDefinition.sequenceMarkers[sceneSequenceProgress_]) <= sequenceDefinition.sequenceRadius + 25.0f &&
            sequenceDefinition.sequenceRequiresInteract) {
            view_.nearbyInteractionLabel = sceneSequenceProgress_ < sequenceDefinition.sequenceLabels.size()
                ? sequenceDefinition.sequenceLabels[sceneSequenceProgress_] : "CHECK";
        }
    }
}
if (view_.chapterId == "rrvvfo_ch2" && view_.presentationStageId == "tournament-hub") {
    const std::size_t phaseIndex = game_.story().sceneIndex;
    const auto actorAlreadyStaged = [&](const std::string& id) {
        return std::any_of(cutsceneActors_.begin(), cutsceneActors_.end(), [&](const RuntimeActorView& actor){ return actor.id == id; });
    };
    const auto addHubActor = [&](const std::string& id, const std::string& presentationId, Vec2 position,
                                 float yaw, bool important = false, const std::string& animation = "idle") {
        if (actorAlreadyStaged(id)) return;
        view_.ambientActors.push_back({id, presentationId, position, yaw, false, animation, important});
    };
    const float life = qolSettings_.reducedMotion ? 0.0f : std::sin(adventureTime_ * .72f) * 16.0f;
    const Vec2 announcerPosition = phaseIndex < 9 ? Vec2{-1110.0f, 10.0f} :
                                    phaseIndex >= 22 ? Vec2{820.0f, -540.0f} : Vec2{1110.0f, -230.0f};
    const Vec2 workerPosition = phaseIndex <= 2 ? Vec2{-1175.0f, 190.0f + life} :
                                phaseIndex >= 22 ? Vec2{430.0f, -780.0f} : Vec2{-1050.0f + life, 300.0f};
    const Vec2 medicPosition = phaseIndex < 16 ? Vec2{650.0f, 980.0f - life} : Vec2{570.0f, 900.0f - life};
    const Vec2 fanPosition = phaseIndex < 9 ? Vec2{-720.0f + life, -620.0f} : Vec2{250.0f, -760.0f + life};
    addHubActor("tournament_announcer", "tournament_announcer", announcerPosition, 70.0f, true);
    addHubActor("registration_worker", "registration_worker", workerPosition, -20.0f, false, phaseIndex <= 2 ? "work" : "walk");
    addHubActor("festival_vendor", "festival_vendor", {-670.0f, -690.0f}, 150.0f);
    addHubActor("tournament_medic", "tournament_medic", medicPosition, -80.0f, true, phaseIndex < 16 ? "walk" : "work");
    addHubActor("nervous_competitor", "nervous_competitor", {-400.0f, 870.0f}, 110.0f, false, "warmup");
    addHubActor("spectator_fan", "spectator_fan", fanPosition, -10.0f, false, phaseIndex < 9 ? "walk" : "cheer");
    if (phaseIndex >= 12) addHubActor("hamual", "hamual", {50.0f, -730.0f}, 25.0f, true, "recover");
    if (phaseIndex >= 14) addHubActor("daniel", "daniel", {430.0f, -680.0f}, -30.0f, true);
    if (phaseIndex >= 16) addHubActor("bark", "bark", {410.0f, 780.0f}, 165.0f, true, "practice");
    if (phaseIndex >= 18) addHubActor("wade", "wade", {520.0f, 690.0f}, -155.0f, true);
    if (phaseIndex >= 19 && phaseIndex <= 21) addHubActor("plouke", "plouke", {790.0f, 900.0f}, -95.0f, true, "still");

    if (phaseIndex >= 6 && phaseIndex <= 19) {
        if (!missingPrizeComplete_)
            addHubActor("prize_worker", "registration_worker", {-980.0f, 350.0f}, -35.0f, true, "work");
        if (!oneMatchComplete_ && phaseIndex >= 10)
            addHubActor("eliminated_fighter", "eliminated_fighter", {825.0f, 660.0f}, 155.0f, true, "warmup");
        if (!controlledFlameComplete_)
            addHubActor("controlled_flame_vendor", "festival_vendor", {-600.0f, -520.0f}, 145.0f, true, "work");
        if (altRoverQuestStage_ == 0)
            addHubActor("maintenance_foreman", "tournament_maintenance_worker", {430.0f, -780.0f}, -25.0f, true, "work");
        else if (altRoverQuestStage_ == 1)
            addHubActor("rover_trail", "rover", {70.0f, -895.0f}, 90.0f, true, "walk");
        else if (altRoverQuestStage_ == 2)
            addHubActor("alt_sign", "alt", {1160.0f, 205.0f}, -85.0f, true, "idle");
        else if (altRoverQuestStage_ == 3)
            addHubActor("rover_service", "rover", {985.0f, -690.0f}, 130.0f, true, "work");
        else if (altRoverQuestStage_ == 4) {
            addHubActor("alt_confrontation", "alt", {1250.0f, -840.0f}, -90.0f, true, "combat_ready");
            addHubActor("rover_confrontation", "rover", {1320.0f, -770.0f}, -110.0f, true, "idle");
            addHubActor("missing_maintenance_worker", "tournament_maintenance_worker", {1175.0f, -790.0f}, 60.0f, true, "recover");
        }
    }

    if (view_.tournamentWorldState == "festival") {
        view_.worldMarkers.push_back({"festival_delivery_cart", {-700.0f,-820.0f}, "delivery-cart", false});
    } else if (view_.tournamentWorldState == "tournament") {
        view_.worldMarkers.push_back({"medical_supply_cart", {650.0f,900.0f}, "parked-cart", true});
        view_.worldMarkers.push_back({"bracket_update", {-1050.0f,300.0f}, "work-lane", true});
    } else if (view_.tournamentWorldState == "final") {
        view_.worldMarkers.push_back({"final_waiting_supply", {650.0f,980.0f}, "parked-cart", true});
    } else if (view_.tournamentWorldState == "cleanup") {
        view_.worldMarkers.push_back({"cleanup_cart", {410.0f,-790.0f}, "delivery-cart", true});
        view_.worldMarkers.push_back({"repair_marker", {-540.0f,760.0f}, "work-lane", true});
    }

    if (phaseIndex >= 6 && phaseIndex <= 19) {
        if (!wadeLostFanComplete_) view_.worldMarkers.push_back({"wade_lost_fan_optional", wadeLostFanFound_ ? Vec2{520.0f,690.0f} : Vec2{-840.0f,-650.0f}, "optional", wadeLostFanComplete_});
        if (!fakeChampionComplete_) view_.worldMarkers.push_back({"fake_champion_optional", {-250.0f,-825.0f}, "optional", fakeChampionComplete_});
        if (!runawayDummyComplete_) view_.worldMarkers.push_back({"runaway_dummy_optional", {-120.0f,920.0f}, "optional", false});
        if (!missingPrizeComplete_)
            view_.worldMarkers.push_back({"missing_prize_optional", missingPrizeStarted_ && !missingPrizeFound_ ? Vec2{760.0f,-720.0f} : Vec2{-1030.0f,280.0f}, "optional", missingPrizeFound_});
        if (!oneMatchComplete_ && phaseIndex >= 10)
            view_.worldMarkers.push_back({"one_match_anyway_optional", {825.0f,660.0f}, "optional", false});
        if (!controlledFlameComplete_)
            view_.worldMarkers.push_back({"controlled_flame_optional", {-600.0f,-520.0f}, "optional", controlledFlameStarted_});
        const Vec2 altTarget = altRoverQuestStage_ == 0 ? Vec2{430.0f,-780.0f} :
                               altRoverQuestStage_ == 1 ? Vec2{45.0f,-920.0f} :
                               altRoverQuestStage_ == 2 ? Vec2{1130.0f,185.0f} :
                               altRoverQuestStage_ == 3 ? Vec2{970.0f,-680.0f} : Vec2{1250.0f,-840.0f};
        if (altRoverQuestStage_ < 5)
            view_.worldMarkers.push_back({"alt_rover_optional", altTarget, "optional", false});

        const auto offerInteraction = [&](Vec2 point, const std::string& labelText) {
            if (view_.nearbyInteractionLabel.empty() && distance(playerPosition_, point) <= 155.0f)
                view_.nearbyInteractionLabel = labelText;
        };
        if (!missingPrizeComplete_) offerInteraction(missingPrizeStarted_ && !missingPrizeFound_ ? Vec2{760.0f,-720.0f} : Vec2{-1030.0f,280.0f}, "PRIZE TRAIL");
        if (!oneMatchComplete_ && phaseIndex >= 10) offerInteraction({825.0f,660.0f}, "ELIMINATED FIGHTER");
        if (!controlledFlameComplete_) offerInteraction({-600.0f,-520.0f}, "FOOD STAND");
        if (altRoverQuestStage_ < 5) offerInteraction(altTarget, altRoverQuestStage_ == 0 ? "MAINTENANCE FOREMAN" : "ALT & ROVER TRAIL");
    }
}
    // Lightweight shared ability VFX markers keep the three Chapter-1 techniques
    // readable on every renderer tier. They are presentation only; gameplay remains
    // authoritative in the existing ability/combat systems.
    if (playerActionAnimation_ == "fire_blast" && playerActionAnimationTime_ > 0.0f) {
        Vec2 effect = {playerPosition_.x + 105.0f, playerPosition_.z};
        if (view_.opponentVisible || roadsideFightActive_) {
            effect = {(playerPosition_.x + opponentPosition_.x) * 0.5f,
                      (playerPosition_.z + opponentPosition_.z) * 0.5f};
        }
        view_.worldMarkers.push_back({"rrvvfo_fire_blast_fx", effect, "fire-blast", false});
    }
    if (playerActionAnimation_ == "object_swap" && playerActionAnimationTime_ > 0.0f)
        view_.worldMarkers.push_back({"rrvvfo_object_swap_fx", playerPosition_, "object-swap-fx", false});
    if (lensActive_ || (playerActionAnimation_ == "lens_activate" && playerActionAnimationTime_ > 0.0f))
        view_.worldMarkers.push_back({"rrvvfo_lens_fx", playerPosition_, "lens-fx", lensActive_});
    if (playerCharging_)
        view_.worldMarkers.push_back({"rrvvfo_energy_charge", playerPosition_, "energy-charge", player_.energy >= 80.0f});
    if (player_.activeAttack == AttackKind::Beam || playerActionAnimation_ == "energy_beam") {
        const Vec2 effect{(playerPosition_.x + opponentPosition_.x) * .5f,
                          (playerPosition_.z + opponentPosition_.z) * .5f};
        view_.worldMarkers.push_back({"rrvvfo_energy_beam", effect, "energy-beam", false});
    }
    if (playerActionAnimation_ == "solar_weave" || (fireAwakeningTime_ > 0.0f && player_.activeAttack == AttackKind::Beam)) {
        const Vec2 effect{(playerPosition_.x + opponentPosition_.x) * .5f,
                          (playerPosition_.z + opponentPosition_.z) * .5f};
        view_.worldMarkers.push_back({"rrvvfo_solar_weave", effect, "solar-weave", true});
    }
    if (fireAwakeningTime_ > 0.0f)
        view_.worldMarkers.push_back({"rrvvfo_fire_awakening", playerPosition_, "fire-awakening", true});
    if (ploukeAwakeningVisualTime_ > 0.0f)
        view_.worldMarkers.push_back({"rrvvfo_unstable_awakening", playerPosition_, "unstable-awakening", ploukeAwakeningFailureComplete_});
    if (player_.pursuitWindow > 0.0f || player_.pursuitTime > 0.0f ||
        player_.pursuitFollowupWindow > 0.0f || player_.pursuitFinishWindow > 0.0f)
        view_.worldMarkers.push_back({"rrvvfo_pursuit_lock", opponentPosition_, "pursuit-lock", player_.pursuitTime > 0.0f});
    if (playerActionAnimation_ == "flow_cancel" && playerActionAnimationTime_ > 0.0f)
        view_.worldMarkers.push_back({"rrvvfo_flow_cancel_fx", playerPosition_, "flow-cancel-fx", true});
    if (movementState_.dashStartedThisFrame)
        view_.worldMarkers.push_back({"rrvvfo_dash_dust", playerPosition_, "dash-dust", false});
    if (movementState_.landedThisFrame && hardLanding_)
        view_.worldMarkers.push_back({"rrvvfo_land_dust", playerPosition_, "landing-dust", true});

    if (game_.mode() == GameMode::Cutscene && cutscenes_.has(view_.sceneId)) {
        for (const auto& actor : cutsceneActors_) view_.ambientActors.push_back(actor);
    }

    if (!transientDialogueId_.empty()) {
        if (game_.mode() == GameMode::Exploration && exploration_.has(view_.sceneId)) {
            const auto& definition = exploration_.get(view_.sceneId);
            view_.objective = definition.objective;
            view_.objectiveDetail = definition.detail;
            if (spectatorPassWon_ && !spectatorPassDelivered_) {
                if (!view_.objectiveDetail.empty()) view_.objectiveDetail += " • ";
                view_.objectiveDetail += "OPTIONAL: Return the spectator pass to the lost competitor.";
            }
        }
        const auto& lines = dialogue_.get(transientDialogueId_);
        view_.dialogueVisible = !lines.empty();
        view_.dialogueIndex = std::min(transientDialogueIndex_, lines.empty() ? std::size_t{0} : lines.size() - 1);
        view_.dialogueCount = lines.size();
        if (!lines.empty()) {
            const auto& line = lines[view_.dialogueIndex];
            view_.dialogueSpeaker = line.speaker;
            view_.dialogueText = line.text;
            view_.dialoguePortraitId = line.portraitId;
            view_.dialogueExpression = line.expression;
            view_.dialogueFocusActorId = line.focusActorId;
            rememberDialogueLine(transientDialogueId_, view_.dialogueIndex, line.speaker, line.text);
        }
        view_.opponentVisible = roadsideFightActive_ || storyArenaActive_;
        return;
    }

    if (game_.mode() == GameMode::Cutscene) {
        view_.objective = chapterComplete_ ?
            (view_.chapterId == "rrvvfo_ch2" ? "TOURNAMENT COMPLETE • STORY CONTINUES LATER" :
                                               "TOURNAMENT ROAD COMPLETE • ENTERING THE GROUNDS") :
            "DIRECTED SCENE • CONTINUE";
        view_.opponentVisible = cutsceneOpponentVisible_;
        if (chapterComplete_) {
            view_.dialogueVisible = false;
        } else if (dialogue_.has(view_.sceneId)) {
            const auto& lines = dialogue_.get(view_.sceneId);
            view_.dialogueVisible = !lines.empty();
            view_.dialogueIndex = std::min(dialogueIndex_, lines.empty() ? std::size_t{0} : lines.size() - 1);
            view_.dialogueCount = lines.size();
            if (!lines.empty()) {
                view_.dialogueSpeaker = lines[view_.dialogueIndex].speaker;
                view_.dialogueText = lines[view_.dialogueIndex].text;
                view_.dialoguePortraitId = lines[view_.dialogueIndex].portraitId;
                view_.dialogueExpression = lines[view_.dialogueIndex].expression;
                view_.dialogueFocusActorId = lines[view_.dialogueIndex].focusActorId;
                rememberDialogueLine(view_.sceneId, view_.dialogueIndex,
                                     lines[view_.dialogueIndex].speaker, lines[view_.dialogueIndex].text);
            }
        } else {
            view_.dialogueVisible = false;
            view_.dialogueSpeaker.clear();
            view_.dialogueText.clear();
            view_.dialoguePortraitId.clear();
            view_.dialogueExpression.clear();
            view_.dialogueFocusActorId.clear();
            view_.dialogueCount = 0;
        }
    } else if (game_.mode() == GameMode::ArenaCombat) {
        view_.dialogueVisible = false;
        view_.opponentVisible = true;
        if (training_.has(view_.sceneId)) {
            const auto& definition = training_.get(view_.sceneId);
            view_.trainingStepCount = definition.steps.size();
            if (trainingManualVisible_) {
                view_.objective = "THE SAGE'S CHALLENGE • CHOOSE YOUR TRAINING";
                view_.objectiveDetail = "Take the full challenge, jump to abilities, or skip and start the road.";
            } else if (const auto* step = currentTrainingStep()) {
                view_.objective = step->objective;
                view_.objectiveDetail = step->prompt;
                if (step->task == TrainingTaskKind::Movement) {
                    view_.objectiveDetail = "MOVE " + std::to_string(static_cast<int>(std::min(trainingMovementDistance_, definition.meaningfulMovementDistance))) + " / " +
                                            std::to_string(static_cast<int>(definition.meaningfulMovementDistance)) +
                                            " • JUMP • DASH WHILE MOVING";
                } else if (step->task == TrainingTaskKind::BasicAttacks) {
                    view_.objectiveDetail = "Light, Heavy, and Launcher count when their animation begins. Grab must connect.";
                } else if (step->task == TrainingTaskKind::LensRead && !lensActive_) {
                    view_.objectiveDetail = player_.energy < definition.lensRequiredEnergy
                        ? "CHARGE " + std::to_string(static_cast<int>(player_.energy)) + " / " +
                          std::to_string(static_cast<int>(definition.lensRequiredEnergy))
                        : "ACTIVATE LENS OF TRUTH, THEN REACT TO THE PREDICTION";
                }
                view_.trainingRequired = step->requiredCount;
                view_.trainingPrompt = opponentAttackTelegraphed_
                    ? (step->task == TrainingTaskKind::LensRead ? "REACT NOW • DODGE OR PERFECT PARRY" : "BLOCK NOW")
                    : step->prompt;
            } else {
                view_.objective = "SAGE TRAINING COMPLETE • PRESS ENTER / INTERACT";
            }
        } else if (storyArenaActive_) {
            if (ploukeFinalClashActive_) {
                view_.objective = "FINAL BEAM CLASH • SPEND WHAT IS LEFT";
                view_.objectiveDetail = "Follow the four-step Energy rhythm. Plouke wins the official result either way.";
            } else {
                view_.objective = view_.officialTournamentMatch ? "OFFICIAL TOURNAMENT MATCH" : "PRACTICE RING";
                view_.objectiveDetail = (view_.officialTournamentMatch ? "FIRST TO " : "TARGET • ") +
                    std::to_string(std::max(1, view_.stockTarget)) + " STOCK";
            }
        }
    } else if (game_.mode() == GameMode::Exploration) {
        view_.opponentVisible = roadsideFightActive_ || !activeOptionalEncounterId_.empty();
        if (!activeOptionalEncounterId_.empty()) {
            view_.objective = "OPTIONAL FIGHT • FIRST TO 1 STOCK";
            view_.objectiveDetail = "B / CANCEL leaves safely. Official tournament matches never allow Run.";
            return;
        }
        if (roadsideFightActive_) {
            view_.objective = standaloneFightMode_ ? "FIGHT • RRVVFO VS CPU" : "OPTIONAL ROAD FIGHT • FIRST TO 1 KO";
            view_.objectiveDetail = standaloneFightMode_
                ? "Walk, jump, dash, guard, attack, Pursuit and Flow Cancel. Pause to return to the menu."
                : "Both fighters use the shared combat system. Losing offers a rematch or a safe exit.";
            return;
        }
        if (qteActive_) {
            view_.objective = "INTERCEPT THE RUNAWAY CART";
            view_.objectiveDetail = "RIGHT • JUMP • LEFT • RIGHT";
            return;
        }
        if (exploration_.has(view_.sceneId)) {
            const auto& definition = exploration_.get(view_.sceneId);
            view_.opponentVisible = definition.companionVisible;
            view_.objective = definition.objective;
            view_.objectiveDetail = definition.detail;
            if (definition.rule == ExplorationRuleKind::RouteChallenge) {
                if (const auto* challenge = currentRouteChallenge(definition)) view_.objective = challenge->objective;
                if (routeChoice_ == "main" && mainRouteFireCleared_) view_.objective += " • " + std::to_string(routeProgress_) + " / 4";
                else if (routeChoice_ == "forest") view_.objective += " • BELLS " + std::to_string(routeProgress_) + " / 4";
            } else if (definition.rule == ExplorationRuleKind::SwapRelay) {
                const std::string prefix = view_.sceneId == "sage_object_swap_field_trial" ? "SAGE FIELD ANCHORS • " :
                                           view_.sceneId == "transport_wheel_recovery" ? "TRANSPORT RESCUE • " : "OBJECT SWAP RELAY • ";
                view_.objective = prefix + std::to_string(relayIndex_) + " / " + std::to_string(definition.relayMarkers.size());
            }
            if (view_.sceneId == "lens_roadblock_reveal" && southernDetourChosen_)
                view_.objectiveDetail = "Follow the worn south trail around the roadblock, then reconnect with Tournament Road.";
            if (view_.sceneId == "collapsed_tournament_road_detour") {
                if (routeProgress_ < 3) view_.objectiveDetail = "Climb the broken ledges. Jump each gap in order.";
                else if (!detourDashDone_) view_.objectiveDetail = "The next break is wider. Dash through it.";
                else if (!detourSwapDone_) view_.objectiveDetail = "Use Object Swap on the high anchor beyond the broken section.";
                else view_.objectiveDetail = "Drop back toward the original Tournament Road.";
            }
            if (view_.sceneId == "roadside_encounter" && lostCompetitorHelped_ && !spectatorPassWon_)
                view_.objectiveDetail = "The fighter with the spectator pass is ahead in the practice clearing.";
            if (spectatorPassWon_ && !spectatorPassDelivered_) {
                if (!view_.objectiveDetail.empty()) view_.objectiveDetail += " • ";
                view_.objectiveDetail += "OPTIONAL: Return the spectator pass to the lost competitor.";
            }
            if (view_.sceneId == "reach_tournament_outskirts" && signPuzzleStage_ == 1) {
                view_.objectiveDetail = "OPTIONAL • THE SIGN THAT POINTS BACK • USE LENS ON THE TURNED SIGN";
            } else if (view_.sceneId == "reach_tournament_outskirts" && signPuzzleStage_ == 2) {
                view_.objectiveDetail = "OPTIONAL • THE SIGN THAT POINTS BACK • OBJECT SWAP IT TO THE EMPTY POST";
            }
        } else view_.objective = "TRAINING REGION • EXPLORE";
    }
}

} // namespace px
