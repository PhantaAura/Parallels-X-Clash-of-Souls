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
constexpr float kExplorationTurnDegreesPerSecond = 1080.0f;
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
    playerCharging_ = false;
    flowCancelLearned_ = false;
    cliffRouteHintLevel_ = 0;
    disabledBlockers_.clear();
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
    relayReleaseTimer_ = 0.0f;
    mainRouteDialogueShown_ = false;
    mainRouteReady_ = false;
    chapterComplete_ = false;
    objectiveHistory_.clear();
    qolSettings_ = {};
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
    game_.loadSave(data);
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
    playerCharging_ = false;
    cliffRouteHintLevel_ = 0;
    routeChoice_ = data.world.routeChoice;
    seenCutscenes_.clear();
    constexpr const char* seenPrefix = "seen_ch1_scene=";
    for (const auto& flag : data.story.flags)
        if (flag.rfind(seenPrefix, 0) == 0) seenCutscenes_.push_back(flag.substr(std::char_traits<char>::length(seenPrefix)));
    trainingCheckpointStep_ = savedTutorialStep(data.story.flags);
    lostCompetitorHelped_ = containsFlag(data.story.flags, "ch1_lost_competitor_helped");
    lostCompetitorDeclined_ = containsFlag(data.story.flags, "ch1_lost_competitor_declined");
    roadsideEncounterResolved_ = containsFlag(data.story.flags, "ch1_roadside_encounter_resolved");
    spectatorPassWon_ = containsFlag(data.story.flags, "ch1_spectator_pass_won");
    spectatorPassDelivered_ = containsFlag(data.story.flags, "ch1_spectator_pass_delivered") ||
                              (roadsideEncounterResolved_ && spectatorPassWon_);
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
    flowCancelLearned_ = containsFlag(data.story.flags, "ch1_flow_cancel_learned");
    chapterComplete_ = containsFlag(data.story.flags, "ch1_complete_at_outskirts");
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
    if (!routeChoice_.empty()) {
        if (routeChoice_ == "main") { disableBlocker("fallen_tree_north"); disableBlocker("fallen_tree_south"); }
        if (routeChoice_ == "forest") disableBlocker("fallen_tree_north");
        if (routeChoice_ == "cliff") disableBlocker("fallen_tree_south");
    }
    if (precisionSwapMastered_) {
        disableBlocker("swap_gate"); disableBlocker("swap_gate_forest"); disableBlocker("swap_gate_cliff");
    }
    if (data.story.sceneIndex > 10) {
        disableBlocker("fallen_tree_center"); disableBlocker("fallen_tree_north"); disableBlocker("fallen_tree_south");
    }
    if (data.story.sceneIndex > 11) {
        disableBlocker("swap_gate"); disableBlocker("swap_gate_forest"); disableBlocker("swap_gate_cliff");
    }
    if (data.story.sceneIndex > 19) {
        disableBlocker("lens_roadblock"); disableBlocker("lens_roadblock_north"); disableBlocker("lens_roadblock_south");
    }
    farBankRockPosition_ = {230.0f, 0.0f};
    const auto rock = std::find_if(data.world.objects.begin(), data.world.objects.end(), [](const WorldObjectState& object){
        return object.id == "far_bank_rock";
    });
    if (rock != data.world.objects.end()) farBankRockPosition_ = rock->position;
    enterCurrentScene();
    playerPosition_ = data.world.position;
    player_.hp = std::clamp(data.world.hp, 1.0f, player_.maxHp);
    player_.energy = std::clamp(data.world.energy, 0.0f, 100.0f);
    player_.guard = std::clamp(data.world.guard, 0.0f, 100.0f);
    if (training_.has(game_.scene().id)) {
        beginTrainingAt(trainingCheckpointStep_);
        trainingManualVisible_ = true;
        trainingManualSelection_ = trainingCheckpointStep_ == 0 ? 0 : 1;
    }
    syncView();
}

const MapDefinition& RuntimeSession::map() const {
    if (roadsideFightActive_) return maps_.get("roadside_arena");
    if (game_.scene().kind == SceneKind::Arena) return maps_.get("sage_training_arena");
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
            }
        }
    }
    if (scene.kind == SceneKind::Arena) {
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

    if (scene.kind == SceneKind::Exploration && exploration_.has(scene.id)) {
        const auto& definition = exploration_.get(scene.id);
        if (definition.hasPlayerStart) playerPosition_ = definition.playerStart;
        if (definition.companionVisible) opponentPosition_ = definition.companionPosition;
        if (definition.rule == ExplorationRuleKind::SwapRelay) {
            relayMarkers_ = definition.relayMarkers;
            relayIndex_ = 0;
        }
        if (definition.rule == ExplorationRuleKind::ChooseRoute) {
            choiceKind_ = 4;
            choiceIndex_ = 0;
        }
        if (scene.id == "selected_route_adventure") {
            mainRouteDialogueShown_ = false;
            mainRouteReady_ = routeChoice_ != "main";
            cliffJumpComplete_.assign(definition.jumpMarkers.size(), false);
        }
        if (definition.rule == ExplorationRuleKind::QteSequence && !definition.openingDialogueId.empty()) {
            beginTransientDialogue(definition.openingDialogueId, 5);
        }
    }
    previousPlayerPosition_ = playerPosition_;
    syncView();
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
    enterCurrentScene();
}

void RuntimeSession::tick(InputState& input, float dt) {
    dt = std::clamp(std::max(0.0f, dt), 0.0f, 0.1f);
    playerCharging_ = false;

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
        recordObjective(objectiveBeforePause_);
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
    if (seen && input.pressed(Action::Cancel)) { completeScene(); return; }
    if (dialogueAdvanceRequested(input, dt)) advanceDialogue();
}

bool RuntimeSession::dialogueAdvanceRequested(const InputState& input, float dt) {
    const bool pressed = input.pressed(Action::Confirm) || input.pressed(Action::Interact);
    const bool held = input.down(Action::Confirm) || input.down(Action::Interact);
    if (pressed) {
        dialogueHoldTime_ = 0.0f;
        return true;
    }
    if (!held || !qolSettings_.holdToAdvanceDialogue) {
        dialogueHoldTime_ = 0.0f;
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
        } else if (CombatSystem::flowCancel(player_)) {
            movementState_.dashCooldown = 0.0f;
            flowCancelLearned_ = true;
            showGameplayNotice("FLOW CANCEL • KEEP MOVING", 1.25f);
            hitFreezeTime_ = std::max(hitFreezeTime_, 3.0f * kFeedbackStepSeconds);
            cameraImpulse_ = std::max(cameraImpulse_, 3.2f);
            impactFlash_ = std::max(impactFlash_, 0.11f);
            combatFeedback_ = "FLOW CANCEL";
            combatFeedbackTime_ = 0.55f;
            consumed = true;
        }
        if (consumed) { bufferedDash_ = false; bufferedDashTime_ = 0.0f; }
    }
    FieldMovementConfig movement;
    movement.walkSpeed = 250.0f;
    previousPlayerPosition_ = playerPosition_;
    InputState lockedMovementInput; lockedMovementInput.beginFrame();
    const InputState& movementInput = (player_.stunTimer > 0.0f || player_.knockdownTimer > 0.0f) ? lockedMovementInput : input;
    playerPosition_ = FieldMovementSystem::tick(map(), playerPosition_, movementState_, movementInput, dt, movement, disabledBlockers_);
    updateCombatFacing(dt);
    if (movementState_.landedThisFrame) landingAnimationTime_ = 0.13f;
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
    if (resolvedHit.connected && !resolvedHit.blocked && player_.flowCancelWindow > 0.0f)
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

void RuntimeSession::showGameplayNotice(const std::string& text, float seconds) {
    gameplayNotice_ = text;
    gameplayNoticeTime_ = std::max(0.0f, seconds);
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
    int frames = std::max(1, result.hitstopFrames);
    float impulse = result.blocked ? 1.7f : 2.35f;
    float flash = result.blocked ? 0.055f : 0.075f;
    std::string label;

    if (result.perfectBlocked) {
        frames = std::max(frames, 7);
        impulse = 4.8f;
        flash = 0.16f;
        if (!playerAttacker) {
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
    if (playerCharging_) return "charge";
    if (player_.blocking) return "block";
    if (movementState_.dashing || player_.dashTime > 0.0f || player_.pursuitTime > 0.0f) return "dash";
    if (movementState_.height > 0.0f) return movementState_.verticalVelocity < -35.0f ? "fall" : "jump_start";
    if (landingAnimationTime_ > 0.0f) return "land";
    const bool moving = distance(previousPlayerPosition_, playerPosition_) > 0.35f;
    if (moving) return "run";
    return (game_.mode() == GameMode::ArenaCombat || roadsideFightActive_) ? "fighting_stance" : "idle";
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
    return {
        npc.basePosition.x + std::sin(adventureTime_ * 0.55f + npc.motionPhase) * 45.0f,
        npc.basePosition.z + std::cos(adventureTime_ * 0.48f + npc.motionPhase) * 24.0f
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
            constexpr std::size_t standaloneOptionCount = 3;
            if (input.pressed(Action::MoveUp))
                pauseSelection_ = pauseSelection_ == 0 ? standaloneOptionCount - 1 : pauseSelection_ - 1;
            if (input.pressed(Action::MoveDown)) pauseSelection_ = (pauseSelection_ + 1) % standaloneOptionCount;
            if (!input.pressed(Action::Confirm) && !input.pressed(Action::Interact)) return;
            if (pauseSelection_ == 0) game_.resume();
            else if (pauseSelection_ == 1) {
                game_.resume();
                if (standaloneFightMode_) startCpuFight();
                else startStandaloneTraining();
            } else {
                game_.resume();
                returnToTitleRequested_ = true;
            }
            return;
        }
        constexpr std::size_t optionCount = 7;
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
            case 5: pausePage_ = 3; pauseSelection_ = 0; break;
            case 6:
                game_.resume();
                returnToTitleRequested_ = true;
                break;
            default: break;
        }
        return;
    }

    if (pausePage_ != 3) return;
    constexpr std::size_t settingCount = 7;
    if (input.pressed(Action::MoveUp))
        pauseSelection_ = pauseSelection_ == 0 ? settingCount - 1 : pauseSelection_ - 1;
    if (input.pressed(Action::MoveDown)) pauseSelection_ = (pauseSelection_ + 1) % settingCount;
    if (!input.pressed(Action::Confirm) && !input.pressed(Action::Interact) &&
        !input.pressed(Action::MoveLeft) && !input.pressed(Action::MoveRight)) return;
    switch (pauseSelection_) {
        case 0: qolSettings_.holdToAdvanceDialogue = !qolSettings_.holdToAdvanceDialogue; break;
        case 1: qolSettings_.firstTimeHints = !qolSettings_.firstTimeHints; break;
        case 2: qolSettings_.reducedMotion = !qolSettings_.reducedMotion; break;
        case 3: qolSettings_.reducedCameraShake = !qolSettings_.reducedCameraShake; break;
        case 4: qolSettings_.reducedFlashes = !qolSettings_.reducedFlashes; break;
        case 5: qolSettings_.highContrastHud = !qolSettings_.highContrastHud; break;
        case 6: qolSettings_.largerText = !qolSettings_.largerText; break;
        default: break;
    }
}

void RuntimeSession::recordObjective(const std::string& objective) {
    if (objective.empty()) return;
    if (!objectiveHistory_.empty() && objectiveHistory_.back() == objective) return;
    objectiveHistory_.push_back(objective);
    if (objectiveHistory_.size() > 8) objectiveHistory_.erase(objectiveHistory_.begin());
}

void RuntimeSession::finishTransientDialogue() {
    const int continuation = transientDialogueContinuation_;
    transientDialogueId_.clear();
    transientDialogueIndex_ = 0;
    transientDialogueContinuation_ = 0;
    switch (continuation) {
        case 1: completeScene(); break;
        case 2: mainRouteReady_ = true; break;
        case 3:
            disableBlocker("fallen_tree_center");
            disableBlocker("fallen_tree_north");
            disableBlocker("fallen_tree_south");
            completeScene();
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
            completeScene();
            break;
        case 20:
            signPuzzleStage_ = std::max(signPuzzleStage_, 1);
            showGameplayNotice("OPTIONAL STORY • THE SIGN THAT POINTS BACK", 1.65f);
            break;
        case 21:
            showGameplayNotice("SIDE STORY COMPLETE • WAYFINDER BADGE", 1.85f);
            break;
        default: break;
    }
}

void RuntimeSession::tickChoice(InputState& input) {
    const std::size_t optionCount = choiceKind_ == 4 ? 3 : 2;
    if (input.pressed(Action::MoveLeft) || input.pressed(Action::MoveUp))
        choiceIndex_ = choiceIndex_ == 0 ? optionCount - 1 : choiceIndex_ - 1;
    if (input.pressed(Action::MoveRight) || input.pressed(Action::MoveDown))
        choiceIndex_ = (choiceIndex_ + 1) % optionCount;
    if (!input.pressed(Action::Confirm) && !input.pressed(Action::Interact) && !input.pressed(Action::Cancel)) return;
    if (choiceKind_ == 4 && input.pressed(Action::Cancel)) return;
    const bool second = input.pressed(Action::Cancel) || choiceIndex_ == 1;
    const int kind = choiceKind_;
    choiceKind_ = 0;
    if (kind == 1) {
        lostCompetitorHelped_ = !second;
        lostCompetitorDeclined_ = second;
        beginTransientDialogue(second ? "road_npc_lost_competitor_decline" : "road_npc_lost_competitor_help");
    } else if (kind == 2) {
        if (second) beginTransientDialogue("roadside_challenger_leave", 11);
        else beginTransientDialogue("roadside_challenger_intro", 10);
    } else if (kind == 3) {
        if (second) resolveRoadsideEncounter(false);
        else startRoadsideFight();
    } else if (kind == 4) {
        static const std::string routes[] = {"main", "forest", "cliff"};
        routeChoice_ = routes[std::min<std::size_t>(choiceIndex_, 2)];
        if (routeChoice_ == "main") {
            disableBlocker("fallen_tree_north");
            disableBlocker("fallen_tree_south");
        } else if (routeChoice_ == "forest") {
            disableBlocker("fallen_tree_north");
        } else {
            disableBlocker("fallen_tree_south");
            cliffRouteHintLevel_ = 0;
        }
        completeScene();
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
    spectatorPassWon_ = spectatorPassWon_ || wonFight;
    playerPosition_ = roadsideReturnPosition_;
    player_ = FighterState{"rrvvfo"};
    player_.energy = 45.0f;
    opponent_ = FighterState{"sage"};
    if (wonFight) {
        roadsideEncounterResolved_ = false;
        showGameplayNotice("PASS WON • RETURN IT TO THE LOST COMPETITOR", 1.85f);
    } else {
        roadsideEncounterResolved_ = true;
        completeScene();
    }
}

void RuntimeSession::handleRoadsideAbilities(const InputState& input) {
    const auto* ability = pressedAbility(input);
    if (!ability) return;
    if (ability->id == "fireBlast" && player_.energy >= ability->energyCost &&
        CombatSystem::startAttack(player_, AttackKind::Projectile)) {
        player_.energy -= ability->energyCost;
        triggerAbilityAnimation("fire_blast", 0.42f);
    } else if (ability->id == "objectSwap" && player_.energy >= ability->energyCost) {
        player_.energy -= ability->energyCost;
        triggerAbilityAnimation("object_swap", 0.32f);
        std::swap(playerPosition_, opponentPosition_);
        updateCombatFacing(0.0f, true);
        player_.invulnerabilityTimer = std::max(player_.invulnerabilityTimer, 0.16f);
    } else if (ability->id == "lensOfTruth" && player_.energy >= ability->energyCost && player_.hp > ability->hpCost) {
        player_.energy -= ability->energyCost;
        player_.hp -= ability->hpCost;
        triggerAbilityAnimation("lens_activate", 0.30f);
        combatLensTimer_ = 4.0f;
        lensActive_ = true;
    }
}

void RuntimeSession::tickRoadsideFight(InputState& input, float dt) {
    FieldMovementConfig movement;
    movement.walkSpeed = 154.0f;
    movement.dashSpeed = 720.0f;
    movement.dashSeconds = 0.22f;
    movement.dashCooldownSeconds = 0.30f;
    const bool dashRequested = input.pressed(Action::Dash) || (bufferedDash_ && bufferedDashTime_ > 0.0f);
    if (dashRequested) {
        bool consumed = false;
        if (player_.pursuitWindow > 0.0f && CombatSystem::startPursuit(player_)) {
            playerPosition_ = {opponentPosition_.x - 78.0f, opponentPosition_.z};
            movementState_.dashCooldown = 0.0f;
            consumed = true;
        } else if (CombatSystem::flowCancel(player_)) {
            movementState_.dashCooldown = 0.0f;
            flowCancelLearned_ = true;
            showGameplayNotice("FLOW CANCEL • KEEP MOVING", 1.25f);
            hitFreezeTime_ = std::max(hitFreezeTime_, 3.0f * kFeedbackStepSeconds);
            cameraImpulse_ = std::max(cameraImpulse_, 3.2f);
            impactFlash_ = std::max(impactFlash_, 0.11f);
            combatFeedback_ = "FLOW CANCEL";
            combatFeedbackTime_ = 0.55f;
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
    playerPosition_ = FieldMovementSystem::tick(map(), playerPosition_, movementState_, movementInput, dt, movement, disabledBlockers_);
    updateCombatFacing(dt);
    if (movementState_.landedThisFrame) landingAnimationTime_ = 0.13f;
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
        if (playerHit.connected && !playerHit.blocked && player_.flowCancelWindow > 0.0f)
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
    if (spectatorPassWon_ && !spectatorPassDelivered_) return;
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
        if (d <= npc.interactionRadius && d < nearestDistance) { nearest = &npc; nearestDistance = d; }
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

bool RuntimeSession::tickAdventureSidePuzzle(const AbilitySlotDefinition* ability) {
    const auto* adventure = currentAdventure();
    if (!adventure || adventure->sidePuzzle.id.empty() || !ability || signPuzzleStage_ <= 0 || signPuzzleStage_ >= 3)
        return false;
    const auto& puzzle = adventure->sidePuzzle;
    if (signPuzzleStage_ == 1 && ability->id == "lensOfTruth" &&
        withinAbilityTarget(playerPosition_, puzzle.lensTarget, puzzle.radius)) {
        signPuzzleStage_ = 2;
        triggerAbilityAnimation("lens_activate", 0.30f);
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
    if (roadsideFightActive_) { tickRoadsideFight(input, dt); return; }

    tickAdventureNpcs(input, dt, definition);
    if (!transientDialogueId_.empty()) return;

    FieldMovementConfig movement;
    movement.walkSpeed = 270.0f;
    previousPlayerPosition_ = playerPosition_;
    playerPosition_ = FieldMovementSystem::tick(map(), playerPosition_, movementState_, input, dt, movement, disabledBlockers_);
    updateExplorationFacing(dt);
    const auto* ability = pressedAbility(input);

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
            // The exact Legacy route panel is opened when this internal scene is entered.
            // The selected route is still traversed physically in the following scene.
            break;
        case ExplorationRuleKind::RouteChallenge: {
            const auto* challenge = currentRouteChallenge(definition);
            if (!challenge) break;
            if (routeChoice_ == "main") {
                if (!mainRouteDialogueShown_ && playerPosition_.x > 235.0f) {
                    mainRouteDialogueShown_ = true;
                    beginTransientDialogue(definition.openingDialogueId, 2);
                    break;
                }
                if (mainRouteReady_ && !blockerDisabled(challenge->blockerToDisable) && ability &&
                    ability->id == challenge->requiredAbilityId && withinAbilityTarget(playerPosition_, challenge->abilityTarget, challenge->abilityRadius)) {
                    triggerAbilityAnimation(ability->id == "objectSwap" ? "object_swap" : "fire_blast", ability->id == "objectSwap" ? 0.32f : 0.42f);
                    beginTransientDialogue(definition.completionDialogueId, 3);
                }
                break;
            }
            if (routeChoice_ == "cliff") {
                for (std::size_t i = 0; i < definition.jumpMarkers.size(); ++i) {
                    if (!cliffJumpComplete_[i] && distance(playerPosition_, definition.jumpMarkers[i]) <= definition.jumpMarkerRadius &&
                        (movementState_.height > 8.0f || movementState_.jumpStartedThisFrame)) cliffJumpComplete_[i] = true;
                }
                const bool ready = std::all_of(cliffJumpComplete_.begin(), cliffJumpComplete_.end(), [](bool done){ return done; });
                if (!ready && playerPosition_.x > 430.0f) {
                    playerPosition_.x = 410.0f;
                    ++cliffRouteHintLevel_;
                    const int left = static_cast<int>(std::count(cliffJumpComplete_.begin(), cliffJumpComplete_.end(), false));
                    if (cliffRouteHintLevel_ == 1) showGameplayNotice("HIGH ROAD • KEEP CLIMBING", 1.0f);
                    else if (cliffRouteHintLevel_ == 2) showGameplayNotice("HIGH ROAD • " + std::to_string(left) + " JUMPS LEFT", 1.15f);
                    else showGameplayNotice("JUMP FROM THE MARKED LEDGES • " + std::to_string(left) + " LEFT", 1.35f);
                }
                if (!ready) break;
            }
            if (distance(playerPosition_, challenge->finish) <= challenge->finishRadius || playerPosition_.x > 430.0f) {
                disableBlocker("fallen_tree_center");
                disableBlocker("fallen_tree_north");
                disableBlocker("fallen_tree_south");
                if (routeChoice_ == "cliff" && !cliffRewardEarned_) {
                    cliffRewardEarned_ = true;
                    showGameplayNotice("SCENIC DISCOVERY • CLIFFSIDE VIEW", 1.65f);
                }
                completeScene();
            }
            break;
        }
        case ExplorationRuleKind::SwapRelay:
            if (relayIndex_ < static_cast<int>(relayMarkers_.size()) && ability && ability->id == definition.requiredAbilityId) {
                const Vec2 target = relayMarkers_[relayIndex_];
                if (withinAbilityTarget(playerPosition_, target, definition.radius)) {
                    triggerAbilityAnimation("object_swap", 0.32f);
                    const Vec2 old = playerPosition_;
                    playerPosition_ = target;
                    relayMarkers_[relayIndex_] = old;
                    ++relayIndex_;
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
            if (ability && ability->id == definition.requiredAbilityId && withinAbilityTarget(playerPosition_, definition.target, definition.radius)) {
                player_.hp = std::max(1.0f, player_.hp - 1.0f);
                lensActive_ = true;
                triggerAbilityAnimation("lens_activate", 0.30f);
                for (const auto& blocker : definition.blockersToDisable) disableBlocker(blocker);
                completeScene();
            }
            break;
        case ExplorationRuleKind::CliffJumpRoute:
            break;
    }
}

void RuntimeSession::advanceDialogue() {
    const auto& scene = game_.scene();
    if (!dialogue_.has(scene.id)) { completeScene(); return; }
    const auto& lines = dialogue_.get(scene.id);
    if (dialogueIndex_ + 1 < lines.size()) ++dialogueIndex_;
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
    const auto completedId = game_.scene().id;
    if (completedId == "sage_tutorial_spar" && standaloneTrainingMode_) {
        returnToTitleRequested_ = true;
        return;
    }
    if (game_.scene().kind == SceneKind::Cutscene &&
        std::find(seenCutscenes_.begin(), seenCutscenes_.end(), completedId) == seenCutscenes_.end())
        seenCutscenes_.push_back(completedId);
    if (completedId == "runaway_tournament_cart") showGameplayNotice("CHECKPOINT • TOURNAMENT ROAD", 1.15f);
    else if (completedId == "roadside_encounter") showGameplayNotice("CHECKPOINT • ROADSIDE", 1.15f);
    else if (completedId == "lens_roadblock_reveal") showGameplayNotice("CHECKPOINT • OUTSKIRTS AHEAD", 1.15f);
    const auto& chapter = game_.chapter();
    const bool finalScene = game_.story().sceneIndex + 1 >= chapter.openingFlow.size();
    if (finalScene) {
        game_.advanceScene();
        chapterComplete_ = true;
        sceneComplete_ = true;
        return;
    }
    const auto beforeChapter = game_.story().chapterId;
    const auto before = game_.story().sceneIndex;
    game_.advanceScene();
    if (game_.story().chapterId != beforeChapter || game_.story().sceneIndex != before) enterCurrentScene();
}

SaveData RuntimeSession::saveSnapshot(const std::string& inputPreset) const {
    auto data = game_.saveData();
    data.world.mapId = game_.chapter().primaryMap;
    data.world.position = playerPosition_;
    data.world.routeChoice = routeChoice_;
    data.world.hp = player_.hp;
    data.world.energy = player_.energy;
    data.world.guard = player_.guard;
    data.world.objects.erase(std::remove_if(data.world.objects.begin(), data.world.objects.end(), [](const WorldObjectState& object){
        return object.id == "far_bank_rock";
    }), data.world.objects.end());
    data.world.objects.push_back({"far_bank_rock", farBankRockPosition_, true});
    data.story.flags.erase(std::remove_if(data.story.flags.begin(), data.story.flags.end(), [](const std::string& flag){
        return flag.rfind("ch1_tutorial_checkpoint=", 0) == 0;
    }), data.story.flags.end());
    const auto addFlag = [&](const std::string& flag) {
        if (!containsFlag(data.story.flags, flag)) data.story.flags.push_back(flag);
    };
    addFlag("ch1_tutorial_checkpoint=" + std::to_string(trainingCheckpointStep_));
    if (lostCompetitorHelped_) addFlag("ch1_lost_competitor_helped");
    if (lostCompetitorDeclined_) addFlag("ch1_lost_competitor_declined");
    if (roadsideEncounterResolved_) addFlag("ch1_roadside_encounter_resolved");
    if (spectatorPassWon_) addFlag("ch1_spectator_pass_won");
    if (spectatorPassDelivered_) addFlag("ch1_spectator_pass_delivered");
    if (tutorialSkipped_) addFlag("ch1_tutorial_skipped");
    if (signPuzzleStage_ >= 1) addFlag("ch1_sign_that_points_back_started");
    if (signPuzzleStage_ >= 2) addFlag("ch1_sign_that_points_back_revealed");
    if (signPuzzleStage_ >= 3) {
        addFlag("ch1_sign_that_points_back_complete");
        addFlag("ch1_wayfinder_badge");
    }
    if (transportRescued_) {
        addFlag("ch1_transport_rescued");
        addFlag("ch1_object_swap_token");
    }
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
    data.frontend.objectiveHistory = objectiveHistory_;
    data.qol = qolSettings_;
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
    view_.mode = roadsideFightActive_ ? GameMode::ArenaCombat : game_.mode();
    view_.chapterId = game_.story().chapterId;
    view_.sceneId = game_.scene().id;
    view_.playerPosition = playerPosition_;
    view_.opponentPosition = opponentPosition_;
    view_.player = player_;
    view_.opponent = opponent_;
    view_.sparCleanHits = sparCleanHits_;
    view_.relayProgress = relayIndex_;
    view_.sceneComplete = sceneComplete_;
    view_.hotbar = AbilityHotbarCatalog::rrvvfoChapter1();
    const bool tutorialScene = training_.has(game_.scene().id);
    view_.hotbarVisible = !tutorialScene || (!trainingManualVisible_ && trainingStepIndex_ >= 4);
    if (tutorialScene && trainingStepIndex_ == 4 && view_.hotbar.size() > 2) view_.hotbar.resize(2);
    if (!view_.hotbarVisible) view_.hotbar.clear();
    view_.showPlayerHealth = view_.mode == GameMode::ArenaCombat && !trainingManualVisible_;
    view_.showOpponentHealth = roadsideFightActive_ || (tutorialScene && !trainingManualVisible_ &&
        (trainingStepIndex_ == 1 || trainingStepIndex_ == 2 || trainingStepIndex_ >= 5));
    view_.showEnergy = roadsideFightActive_ || (tutorialScene && !trainingManualVisible_ && trainingStepIndex_ >= 3);
    view_.showGuard = roadsideFightActive_ || (tutorialScene && !trainingManualVisible_ &&
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
    view_.qteVisible = qteActive_;
    view_.qteTitle.clear();
    view_.qteSequence.clear();
    view_.qteIndex = qteIndex_;
    view_.qteSecondsRemaining = qteRemaining_;
    view_.qteAttempt = qteAttempts_;
    view_.pauseVisible = game_.mode() == GameMode::Pause;
    view_.pausePageTitle = pausePage_ == 1 ? "OBJECTIVE HISTORY" : pausePage_ == 2 ? "CONTROLS" :
                           pausePage_ == 3 ? "ACCESSIBILITY & QOL" : "STORY MENU • CHAPTER 1";
    view_.pauseSections.clear();
    view_.pauseOptions.clear();
    view_.pauseSelection = pauseSelection_;
    view_.manualSaveAllowed = canManualSave();
    view_.saveStatus = saveStatus_;
    view_.objectiveHistory = objectiveHistory_;
    if (pausePage_ == 0) {
        view_.pauseSections = {
            "CURRENT OBJECTIVE • " + (objectiveBeforePause_.empty() ? std::string{"Continue Chapter 1"} : objectiveBeforePause_),
            "CHECKPOINT • " + game_.story().checkpointId
        };
        if (standaloneMode()) {
            view_.pauseSections = {standaloneFightMode_ ? "FIGHT • RRVVFO VS CPU" : "TRAINING • SAGE'S CHALLENGE"};
            view_.pauseOptions = {
                "RESUME", standaloneFightMode_ ? "RESTART FIGHT" : "RESTART TRAINING", "RETURN TO TITLE"
            };
        } else {
            view_.pauseOptions = {
                "RESUME", canManualSave() ? "SAVE GAME" : "SAVE GAME • UNAVAILABLE HERE",
                "RESTART FROM CHECKPOINT", "OBJECTIVE HISTORY", "CONTROLS", "ACCESSIBILITY & QOL", "RETURN TO TITLE"
            };
        }
    } else if (pausePage_ == 1) {
        if (objectiveHistory_.empty()) view_.pauseSections.push_back("No completed objectives yet.");
        else for (auto it = objectiveHistory_.rbegin(); it != objectiveHistory_.rend(); ++it)
            view_.pauseSections.push_back("• " + *it);
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
    } else {
        const auto onOff = [](bool value){ return value ? "ON" : "OFF"; };
        view_.pauseOptions = {
            "HOLD TO ADVANCE DIALOGUE • " + std::string(onOff(qolSettings_.holdToAdvanceDialogue)),
            "FIRST-TIME HINTS • " + std::string(onOff(qolSettings_.firstTimeHints)),
            "REDUCED MOTION • " + std::string(onOff(qolSettings_.reducedMotion)),
            "REDUCED CAMERA SHAKE • " + std::string(onOff(qolSettings_.reducedCameraShake)),
            "REDUCED FLASHES • " + std::string(onOff(qolSettings_.reducedFlashes)),
            "HIGH-CONTRAST HUD • " + std::string(onOff(qolSettings_.highContrastHud)),
            "LARGER TEXT • " + std::string(onOff(qolSettings_.largerText))
        };
        view_.pauseSections = {"CONFIRM / LEFT / RIGHT • TOGGLE", "B / ESC • BACK"};
    }
    view_.holdToAdvanceDialogue = qolSettings_.holdToAdvanceDialogue;
    view_.reducedMotion = qolSettings_.reducedMotion;
    view_.reducedCameraShake = qolSettings_.reducedCameraShake;
    view_.reducedFlashes = qolSettings_.reducedFlashes;
    view_.highContrastHud = qolSettings_.highContrastHud;
    view_.largerText = qolSettings_.largerText;
    view_.combatMessages = qolSettings_.combatMessages;
    view_.chapterComplete = chapterComplete_;
    view_.nextChapterId = game_.hasPendingChapter() ? game_.story().pendingChapterId : game_.chapter().nextChapterId;
    view_.roadsideFightActive = roadsideFightActive_;
    view_.nearbyInteractionLabel.clear();
    view_.gameplayNotice = gameplayNotice_;
    view_.bufferedCombatInput = hasBufferedCombatAction_ ? (bufferedCombatAction_ == Action::Light ? "LIGHT" : bufferedCombatAction_ == Action::Heavy ? "HEAVY" : bufferedCombatAction_ == Action::Launcher ? "LAUNCHER" : "GRAB") : "";
    view_.flowCancelReady = player_.flowCancelWindow > 0.0f;
    view_.hitFreezeSeconds = hitFreezeTime_;
    view_.cameraImpulse = cameraImpulse_ * (qolSettings_.reducedCameraShake ? 0.30f : 1.0f);
    view_.impactFlash = impactFlash_ * (qolSettings_.reducedFlashes ? 0.20f : 1.0f);
    const bool importantCombatMessage = perfectBlockFlash_ || pursuitFinishFlash_ || guardBreakFlash_ || finalHitFlash_;
    view_.combatFeedback = qolSettings_.combatMessages == "off" ||
        (qolSettings_.combatMessages == "important" && !importantCombatMessage) ? std::string{} : combatFeedback_;
    view_.perfectBlockFlash = perfectBlockFlash_;
    view_.pursuitFinishFlash = pursuitFinishFlash_;
    view_.guardBreakFlash = guardBreakFlash_;
    view_.finalHitFlash = finalHitFlash_;
    view_.playerAnimation = resolvePlayerAnimation();
    view_.playerAnimationSpeed = playerBackpedaling() && view_.playerAnimation == "run" ? -0.72f : 1.0f;
    view_.playerFaceSeconds = adventureTime_;
    view_.playerMoving = distance(previousPlayerPosition_, playerPosition_) > 0.35f;
    view_.playerBlocking = player_.blocking;
    view_.playerCharging = playerCharging_;
    view_.playerAirborne = movementState_.height > 0.0f || player_.airborne;
    view_.playerFalling = movementState_.height > 0.0f && movementState_.verticalVelocity < -35.0f;

    if (choiceKind_ == 1) {
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
        view_.choiceOptions = {"MAIN ROAD", "FOREST SHORTCUT", "CLIFF ROUTE"};
    }

    if (const auto* adventure = currentAdventure()) {
        for (const auto& npc : adventure->npcs) {
            const auto position = adventureNpcPosition(npc);
            view_.ambientActors.push_back({npc.id, npc.characterPresentationId, position, 0.0f,
                distance(playerPosition_, position) <= npc.interactionRadius});
            if (distance(playerPosition_, position) <= npc.interactionRadius) view_.nearbyInteractionLabel = npc.label;
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
        if (runawayCartSaved_) view_.worldMarkers.push_back({"saved_supply_cart", life.savedCartPosition, "parked-cart", true});
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
        if (view_.sceneId == "transport_wheel_recovery") {
            view_.worldMarkers.push_back({"transport_wheel", adventure->transportWheel, "transport-wheel", false});
        }
        if (qteActive_) {
            view_.qteTitle = adventure->runawayCart.title;
            view_.qteSequence = adventure->runawayCart.sequence;
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

    if (game_.mode() == GameMode::Cutscene && cutscenes_.has(view_.sceneId)) {
        for (const auto& actor : cutscenes_.get(view_.sceneId).staging) {
            if (actor.actorId != "rrvvfo" && actor.actorId != "sage") {
                view_.ambientActors.push_back({actor.actorId, actor.actorId, actor.position, actor.yawDegrees, false});
            }
        }
    }

    if (!transientDialogueId_.empty()) {
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
        }
        view_.opponentVisible = roadsideFightActive_;
        return;
    }

    if (game_.mode() == GameMode::Cutscene) {
        view_.objective = chapterComplete_ ? "CHAPTER 1 COMPLETE • TOURNAMENT OUTSKIRTS" : "DIRECTED SCENE • CONTINUE";
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
        }
    } else if (game_.mode() == GameMode::Exploration) {
        view_.opponentVisible = roadsideFightActive_;
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
            } else if (definition.rule == ExplorationRuleKind::SwapRelay) {
                view_.objective = (view_.sceneId == "sage_object_swap_field_trial" ? "SAGE FIELD ANCHORS • " : "OBJECT SWAP RELAY • ") +
                                  std::to_string(relayIndex_) + " / " + std::to_string(definition.relayMarkers.size());
            }
            if (view_.sceneId == "roadside_encounter" && spectatorPassWon_ && !spectatorPassDelivered_) {
                view_.objective = "OPTIONAL • RETURN THE SPECTATOR PASS";
                view_.objectiveDetail = "Take the earned pass back to the lost competitor. He's just behind the clearing.";
            } else if (view_.sceneId == "reach_tournament_outskirts" && signPuzzleStage_ == 1) {
                view_.objectiveDetail = "OPTIONAL • THE SIGN THAT POINTS BACK • USE LENS ON THE TURNED SIGN";
            } else if (view_.sceneId == "reach_tournament_outskirts" && signPuzzleStage_ == 2) {
                view_.objectiveDetail = "OPTIONAL • THE SIGN THAT POINTS BACK • OBJECT SWAP IT TO THE EMPTY POST";
            }
        } else view_.objective = "CHAPTER 1 • TRAINING REGION";
    }
}

} // namespace px
