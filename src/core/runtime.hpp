#pragma once
#include "content/adventure_registry.hpp"
#include "content/arena_encounter_registry.hpp"
#include "content/chapter_registry.hpp"
#include "content/combat_manual_registry.hpp"
#include "content/cutscene_registry.hpp"
#include "content/dialogue_registry.hpp"
#include "content/exploration_registry.hpp"
#include "content/map_registry.hpp"
#include "content/training_registry.hpp"
#include "core/ability_hotbar.hpp"
#include "core/combat.hpp"
#include "core/field_movement.hpp"
#include "core/game.hpp"
#include "core/input.hpp"
#include "core/save.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace px {

struct RuntimeActorView {
    std::string id;
    std::string presentationId;
    Vec2 position{};
    float yawDegrees{0.0f};
    bool interactable{false};
    std::string animation{"idle"};
    bool important{false};
};

struct RuntimeMarkerView {
    std::string id;
    Vec2 position{};
    std::string kind;
    bool complete{false};
};

struct RuntimeView {
    GameMode mode{GameMode::Title};
    std::string chapterId;
    std::string sceneId;
    std::string objective;
    std::string objectiveDetail;
    std::string currentArea;
    std::string routeChoice;
    std::string presentationStageId;
    Vec2 playerPosition{};
    Vec2 opponentPosition{};
    bool opponentVisible{false};
    FighterState player{"rrvvfo"};
    FighterState opponent{"sage"};
    bool dialogueVisible{false};
    std::string dialogueSpeaker;
    std::string dialogueText;
    std::string dialoguePortraitId;
    std::string dialogueExpression;
    std::string dialogueFocusActorId;
    std::size_t dialogueIndex{0};
    std::size_t dialogueCount{0};
    int sparCleanHits{0};
    int relayProgress{0};
    bool sceneComplete{false};
    bool trainingManualVisible{false};
    CombatManualPage trainingManualPage;
    std::size_t trainingManualPageIndex{0};
    std::size_t trainingManualPageCount{0};
    std::vector<std::string> trainingManualOptions;
    std::size_t trainingManualSelection{0};
    std::string trainingCheckpointLabel;
    std::size_t trainingStepIndex{0};
    std::size_t trainingStepCount{0};
    int trainingProgress{0};
    int trainingRequired{0};
    float trainingMovementDistance{0.0f};
    std::string trainingPrompt;
    float playerHeight{0.0f};
    float opponentHeight{0.0f};
    bool playerDashing{false};
    bool opponentAttackTelegraphed{false};
    float opponentAttackSecondsRemaining{0.0f};
    int opponentAttackAttempt{0};
    bool lensActive{false};
    float playerYawDegrees{0.0f};
    float opponentYawDegrees{180.0f};
    AbilityHotbarLayout hotbar{};
    bool hotbarVisible{true};
    bool showPlayerHealth{false};
    bool showOpponentHealth{false};
    bool showEnergy{false};
    bool showGuard{false};
    std::vector<RuntimeActorView> ambientActors;
    std::vector<RuntimeMarkerView> worldMarkers;
    bool choiceVisible{false};
    std::string choiceTitle;
    std::vector<std::string> choiceOptions;
    std::size_t choiceIndex{0};
    bool qteVisible{false};
    std::string qteTitle;
    std::vector<Action> qteSequence;
    std::size_t qteIndex{0};
    float qteSecondsRemaining{0.0f};
    int qteAttempt{0};
    bool pauseVisible{false};
    std::string pausePageTitle;
    std::vector<std::string> pauseSections;
    std::vector<std::string> pauseOptions;
    std::size_t pauseSelection{0};
    bool manualSaveAllowed{false};
    std::string saveStatus;
    std::vector<std::string> objectiveHistory;
    std::vector<std::string> recentDialogue;
    bool holdToAdvanceDialogue{true};
    bool reducedMotion{false};
    bool reducedCameraShake{false};
    bool reducedFlashes{false};
    bool highContrastHud{false};
    bool largerText{false};
    std::string combatMessages{"full"};
    bool chapterComplete{false};
    std::string nextChapterId;
    bool roadsideFightActive{false};
    std::string nearbyInteractionLabel;
    std::string gameplayNotice;
    std::string bufferedCombatInput;
    bool flowCancelReady{false};
    // Shared presentation feedback. These are renderer-agnostic signals derived
    // from the same combat simulation on Mac, Linux and 3DS.
    float hitFreezeSeconds{0.0f};
    float cameraImpulse{0.0f};
    float impactFlash{0.0f};
    std::string combatFeedback;
    bool perfectBlockFlash{false};
    bool pursuitFinishFlash{false};
    bool guardBreakFlash{false};
    bool finalHitFlash{false};

    // Renderer-agnostic animation state. Platform shells only consume this;
    // they never infer gameplay states independently.
    std::string playerAnimation{"idle"};
    float playerAnimationSpeed{1.0f};
    float playerFaceSeconds{0.0f};
    bool playerMoving{false};
    bool playerBlocking{false};
    bool playerCharging{false};
    bool playerAirborne{false};
    std::string tournamentWorldState;
    bool playerFalling{false};

// U7 shared cinematic state. Renderers consume these values directly;
// story content never reaches into a platform camera implementation.
bool cinematicCameraActive{false};
float cinematicCameraBlend{0.0f};
Vec2 cinematicCameraFocus{};
float cinematicCameraYawDegrees{38.0f};
float cinematicCameraDistance{900.0f};
float cinematicCameraHeight{410.0f};
float cinematicCameraFovDegrees{43.0f};
bool cinematicCameraOcclusionRescue{false};
std::string cinematicExpression;
std::string cinematicWorldEvent;

// U7B ability presentation. Gold is Object Swap, purple is Lens.
float objectSwapCooldownSeconds{0.0f};
bool objectSwapTargetLocked{false};
Vec2 objectSwapTargetPosition{};
float objectSwapPhaseSeconds{0.0f};
Vec2 objectSwapGhostFrom{};
Vec2 objectSwapGhostTo{};
float lensBlindnessAmount{0.0f};
std::size_t storySequenceProgress{0};
std::size_t storySequenceCount{0};
float storySequenceSeconds{0.0f};
TournamentCardState tournamentCard{"rrvvfo"};
bool tournamentCardVisible{false};
int tournamentBonusRoll{0};
int playerStocksLost{0};
int opponentStocksLost{0};
int stockTarget{0};
int recommendedLevel{0};
bool officialTournamentMatch{false};
std::string tournamentPhase;
std::string nextTournamentMatch;
std::string bracketSummary;
float wadeRaceBestSeconds{0.0f};
bool wadeRaceTargetBeaten{false};
bool ploukeAwakeningFailureActive{false};
bool optionalTournamentFight{false};
std::string optionalTournamentActivity;
float energyPowerMultiplier{1.0f};
float energyChargePulse{0.0f};
bool pureEnergyBeamActive{false};
bool fireAwakeningUnlocked{false};
bool fireAwakeningActive{false};
float fireAwakeningSeconds{0.0f};
bool solarWeaveActive{false};
bool battleRankVisible{false};
std::string battleRank;
int battleRankScore{0};
AdventureRecordsState adventureRecords;
RpgProgressState rpgProgress;
float necklaceMovementMultiplier{1.0f};
float hudScale{1.0f};
float dialogueScale{1.0f};
std::string objectiveDisplay{"full"};
float cameraYawOffsetDegrees{0.0f};
float cameraHeightOffset{0.0f};
};

// One platform-independent gameplay session. macOS, Windows/Linux and 3DS shells
// all feed semantic Actions into this class instead of owning chapter-specific gameplay.
class RuntimeSession {
public:
    RuntimeSession(const ChapterRegistry& chapters,
                   const MapRegistry& maps,
                   const CutsceneRegistry& cutscenes,
                   const DialogueRegistry& dialogue,
                   const ExplorationRegistry& exploration,
                   const TrainingRegistry& training,
                   const AdventureRegistry& adventures);

    void startChapter(const std::string& chapterId);
    void startReplayChapter(const SaveData& source);
    void startCpuFight();
    void startStandaloneTraining();
    void loadSnapshot(const SaveData& data);
    void tick(InputState& input, float dt);
    void confirm();
    void resetCurrentScene();
    bool consumeManualSaveRequest();
    bool consumeReturnToTitleRequest();
    void notifyManualSaveResult(bool success);
    void notifyManualSaveUnavailable();
    bool canManualSave() const;
    void setQolSettings(const QolSettings& settings) { qolSettings_ = settings; if (sceneCheckpointValid_) sceneCheckpointSnapshot_.qol = settings; syncView(); }
    const QolSettings& qolSettings() const { return qolSettings_; }
    bool standaloneMode() const { return standaloneFightMode_ || standaloneTrainingMode_ || replayMode_; }
    bool replayMode() const { return replayMode_; }

    const RuntimeView& view() const { return view_; }
    const MapDefinition& map() const;
    SaveData saveSnapshot(const std::string& inputPreset = "modern") const;
    const std::vector<std::string>& disabledBlockers() const { return disabledBlockers_; }

private:
    void enterCurrentScene();
    void syncView();
    void advanceDialogue();
    void tickCutscene(InputState& input, float dt);
    void applyCutsceneActions(float dt);
    bool cutsceneBeatReadyToAdvance() const;
    Vec2 cutsceneActorPosition(const std::string& actorId) const;
    void tickArena(InputState& input, float dt);
void startStoryArena();
void tickStoryArena(InputState& input, float dt);
void resetStoryArenaStock();
    void finishStoryArena(bool playerWon);
    void tickPloukeFinalClash(InputState& input, float dt);
    void finishPloukeFinalClash(bool strongFinish);
void resolveArenaRewardAndAdvance();
    void configurePlayerProgression();
    void resetBattlePerformance();
    void recordBattleAction(AttackKind kind);
    void finalizeBattleRank(bool playerWon);
    void addNecklaceMastery(float amount, const char* reason);
    const ArenaEncounterDefinition& activeArenaEncounter() const;
    void startOptionalTournamentFight(const std::string& encounterId);
    void resolveOptionalTournamentFight(bool won);
    void tickExploration(InputState& input, float dt);
bool tickTournamentActivities(InputState& input, const AbilitySlotDefinition* ability);
    bool tickU15TournamentActivities(InputState& input, const AbilitySlotDefinition* ability);
    void tickTransientDialogue(InputState& input, float dt);
    void tickPause(InputState& input);
    bool dialogueAdvanceRequested(const InputState& input, float dt);
    void recordObjective(const std::string& objective);
    void rememberDialogueLine(const std::string& sourceId, std::size_t index,
                              const std::string& speaker, const std::string& text);
    void beginTransientDialogue(const std::string& id, int continuation = 0);
    void finishTransientDialogue();
    void tickChoice(InputState& input);
    void tickQte(InputState& input, float dt);
    void startRunawayCartQte();
    void finishRunawayCartQte(bool success);
    void tickRoadsideEncounter(InputState& input, float dt, const ExplorationDefinition& definition);
    void startRoadsideFight();
    void tickRoadsideFight(InputState& input, float dt);
    void resolveRoadsideEncounter(bool wonFight);
    void tickAdventureNpcs(InputState& input, float dt, const ExplorationDefinition& definition);
    bool tickAdventureSidePuzzle(const AbilitySlotDefinition* ability);
void refreshFreeSwapObjects();
bool tryFreeObjectSwap();
void scheduleLensBlindness(float delaySeconds);
    const AdventureDefinition* currentAdventure() const;
    Vec2 adventureNpcPosition(const AdventureNpcDefinition& npc) const;
    void completeScene();
    struct ArenaAttackAttempt {
        bool started{false};
        HitResult hit{};
    };
    ArenaAttackAttempt performArenaAttack(Action action);
    ArenaAttackAttempt requestArenaAttack(Action action, bool allowBuffer = true);
    bool tryBufferedArenaAttack();
    bool tryFlowCancel();
    void clearCombatInputBuffer();
    void queueCombatInputsDuringFreeze(const InputState& input);
    void emitCombatFeedback(const HitResult& result, AttackKind kind, bool playerAttacker);
    void emitCombatClashFeedback();
    void triggerAbilityAnimation(const std::string& clip, float seconds);
    std::string resolvePlayerAnimation() const;
    void updateExplorationFacing(float dt);
    void updateCombatFacing(float dt, bool snap = false);
    bool playerBackpedaling() const;
    void showGameplayNotice(const std::string& text, float seconds = 1.25f);
    HitResult advancePlayerAttack(float dt);
    void handleRoadsideAbilities(const InputState& input);
    void advanceTrainingStep();
    void beginTrainingAt(std::size_t stepIndex);
    void activateTrainingManualSelection();
    void updateTrainingCheckpoint();
    static std::string trainingCheckpointLabel(std::size_t stepIndex);
    void updateTrainingProgress();
    const TrainingStepDefinition* currentTrainingStep() const;
    void tickTrainingOpponent(float dt, bool blockPressedThisFrame);
    const AbilitySlotDefinition* pressedAbility(const InputState& input) const;
    bool blockerDisabled(const std::string& id) const;
    void disableBlocker(const std::string& id);
    void enableBlocker(const std::string& id);
    void setTerrainCollapseActive(bool active);
    void setDetourBlockersActive(bool active);
    std::string areaNameForPosition(Vec2 position) const;
    const RouteChallenge* currentRouteChallenge(const ExplorationDefinition& definition) const;

    [[maybe_unused]] const ChapterRegistry& chapters_;
    const MapRegistry& maps_;
    const CutsceneRegistry& cutscenes_;
    const DialogueRegistry& dialogue_;
    const ExplorationRegistry& exploration_;
    const TrainingRegistry& training_;
    const AdventureRegistry& adventures_;
    CombatManualRegistry manual_;
ArenaEncounterRegistry arenaEncounters_{};
bool storyArenaActive_{false};
int storyPlayerStocksLost_{0};
int storyOpponentStocksLost_{0};
float storyArenaAiTimer_{0.0f};
unsigned storyArenaAiDecisionIndex_{0};
bool storyArenaRewardPending_{false};
bool pendingArenaSceneComplete_{false};
bool ploukeHighPerformance_{false};
float ploukeFinalSeconds_{0.0f};
bool ploukeAwakeningAttempted_{false};
bool ploukeAwakeningFailureComplete_{false};
float ploukeAwakeningVisualTime_{0.0f};
bool ploukeFinalClashActive_{false};
float ploukeFinalClashTime_{0.0f};
std::size_t ploukeFinalClashIndex_{0};
TournamentCardState tournamentCard_{"rrvvfo"};
AdventureRecordsState adventureRecords_{};
RpgProgressState rpgProgress_{};
BattlePerformance battlePerformance_{};
float battleRankTime_{0.0f};
std::string battleRankLabel_;
int battleRankScore_{0};
std::string pendingRankDialogueId_;
int pendingRankDialogueContinuation_{0};
int pendingRankChoiceKind_{0};
int tournamentBonusRoll_{0};
bool activeMealBoost_{false};
float tournamentCardRevealTime_{0.0f};
std::string activeOptionalEncounterId_;
Vec2 optionalArenaReturnPosition_{};
bool optionalArenaWon_{false};
std::string pendingOptionalDialogueId_;
    Game game_;
    Vec2 playerPosition_{};
    Vec2 opponentPosition_{};
    SaveData sceneCheckpointSnapshot_{};
    bool sceneCheckpointValid_{false};
    FighterState player_{"rrvvfo"};
    FighterState opponent_{"sage"};
    bool blockHeld_{false};
    bool sceneComplete_{false};
    std::size_t dialogueIndex_{0};
    int sparCleanHits_{0};
    float attackCooldown_{0.0f};
    bool hasBufferedCombatAction_{false};
    Action bufferedCombatAction_{Action::Light};
    float bufferedCombatTime_{0.0f};
    float gameplayNoticeTime_{0.0f};
    float hitFreezeTime_{0.0f};
    float cameraImpulse_{0.0f};
    float impactFlash_{0.0f};
    float combatFeedbackTime_{0.0f};
    std::string combatFeedback_;
    bool perfectBlockFlash_{false};
    bool pursuitFinishFlash_{false};
    bool guardBreakFlash_{false};
    bool finalHitFlash_{false};
    bool bufferedDash_{false};
    float bufferedDashTime_{0.0f};
    std::string playerActionAnimation_;
    float playerActionAnimationTime_{0.0f};
    float landingAnimationTime_{0.0f};
    float combatReadyAnimationTime_{0.0f};
    float explorationRunStartAnimationTime_{0.0f};
    float explorationRunStopAnimationTime_{0.0f};
    bool explorationWasMoving_{false};
    bool hardLanding_{false};
    bool playerCharging_{false};
    bool pureEnergyBeamActive_{false};
    bool fireAwakeningUnlocked_{false};
    float fireAwakeningTime_{0.0f};
    std::string gameplayNotice_;
    bool flowCancelLearned_{false};
    int cliffRouteHintLevel_{0};
    int routeHintStage_{0};
    std::size_t routeProgress_{0};
    float routeChallengeTime_{0.0f};
    float routeChoiceIntroTime_{0.0f};
std::size_t sceneSequenceProgress_{0};
float sceneSequenceSeconds_{0.0f};
bool sceneSequenceStarted_{false};
    bool mainRouteFireCleared_{false};
    bool lensRouteChosen_{false};
    bool southernDetourChosen_{false};
    bool southernDetourComplete_{false};
    bool terrainCollapseSeen_{false};
    bool detourDashDone_{false};
    bool detourSwapDone_{false};
    std::string routeChoice_;
    std::vector<std::string> disabledBlockers_;
    std::vector<Vec2> relayMarkers_;
    int relayIndex_{0};
    FieldMovementState movementState_{};
    bool trainingManualVisible_{false};
    std::size_t trainingManualSelection_{0};
    std::size_t trainingManualPageIndex_{0};
    std::size_t trainingCheckpointStep_{0};
    std::size_t trainingStepIndex_{0};
    int trainingProgress_{0};
    unsigned trainingSignals_{0};
    Vec2 previousPlayerPosition_{};
    float opponentAttackTimer_{0.0f};
    int opponentAttackAttempts_{0};
    bool opponentAttackTelegraphed_{false};
    bool trainingTimedDefenseArmed_{false};
    bool lensActive_{false};
    bool trainingEvadeAttempt_{false};
    bool lensTrialActive_{false};
    float lensTrialTimer_{0.0f};
    float lensTrialHp_{100.0f};
    float trainingMovementDistance_{0.0f};
    float playerYawDegrees_{0.0f};
    float opponentYawDegrees_{180.0f};
    bool cutsceneOpponentVisible_{false};
    std::vector<RuntimeActorView> cutsceneActors_;
float cutsceneBeatTime_{0.0f};
std::size_t cutsceneBeatIndex_{static_cast<std::size_t>(-1)};
float seenSceneSkipHoldTime_{0.0f};
bool cinematicCameraActive_{false};
float cinematicCameraBlend_{0.0f};
float cinematicCameraReturnTime_{0.0f};
Vec2 cinematicCameraFocus_{};
float cinematicCameraYawDegrees_{38.0f};
float cinematicCameraDistance_{900.0f};
float cinematicCameraHeight_{410.0f};
float cinematicCameraFovDegrees_{43.0f};
std::string cinematicExpression_;
std::string cinematicWorldEvent_;
    std::string transientDialogueId_;
    std::size_t transientDialogueIndex_{0};
    int transientDialogueContinuation_{0};
    int choiceKind_{0};
    std::size_t choiceIndex_{0};
    bool lostCompetitorHelped_{false};
    bool lostCompetitorDeclined_{false};
    bool roadsideEncounterResolved_{false};
    bool spectatorPassWon_{false};
    bool spectatorPassDelivered_{false};
    bool tutorialSkipped_{false};
    int signPuzzleStage_{0};
    bool transportRescued_{false};
    bool runawayCartSaved_{false};
    bool precisionSwapMastered_{false};
    bool cliffRewardEarned_{false};
    float wadeRaceBestSeconds_{0.0f};
    bool wadeRaceTargetBeaten_{false};
    bool crackedRingIntroSeen_{false};
    bool wadeLostFanStarted_{false};
    bool wadeLostFanFound_{false};
    bool wadeLostFanComplete_{false};
    bool fakeChampionContacted_{false};
    bool fakeChampionRevealed_{false};
    bool fakeChampionComplete_{false};
    bool runawayDummyComplete_{false};
    bool festivalFoodComplete_{false};
    bool festivalPhotoComplete_{false};
    bool missingPrizeStarted_{false};
    bool missingPrizeFound_{false};
    bool missingPrizeComplete_{false};
    bool oneMatchComplete_{false};
    bool controlledFlameStarted_{false};
    bool controlledFlameComplete_{false};
    int altRoverQuestStage_{0};
    int barkPracticeWins_{0};
    bool roadsideFightActive_{false};
    bool roadsideFightIntroShown_{false};
    Vec2 roadsideReturnPosition_{};
    std::vector<std::string> seenCutscenes_;
    float roadsideAiTimer_{0.0f};
    unsigned roadsideAiDecisionIndex_{0};
    float opponentHeight_{0.0f};
    float opponentVerticalVelocity_{0.0f};
    float combatLensTimer_{0.0f};
    int roadsidePlayerKOs_{0};
    int roadsideFoeKOs_{0};
    bool qteActive_{false};
    std::string qteId_;
    std::size_t qteIndex_{0};
    float qteRemaining_{0.0f};
    int qteAttempts_{0};
    float adventureTime_{0.0f};
    float storyPlaytimeSeconds_{0.0f};
    float necklaceTraversalAccumulator_{0.0f};
    float userCameraYawOffset_{0.0f};
    float userCameraHeightOffset_{0.0f};
    float dialogueAutoAdvanceTime_{0.0f};
    std::vector<bool> cliffJumpComplete_;
    Vec2 farBankRockPosition_{230.0f, 0.0f};
std::string freeSwapMapId_;
std::vector<WorldObjectState> freeSwapObjects_;
float objectSwapCooldownTime_{0.0f};
float objectSwapPhaseTime_{0.0f};
Vec2 objectSwapGhostFrom_{};
Vec2 objectSwapGhostTo_{};
float lensBlindnessDelay_{0.0f};
float lensBlindnessTime_{0.0f};
float lensBlindnessDuration_{0.65f};
    float relayReleaseTimer_{0.0f};
    bool mainRouteDialogueShown_{false};
    bool mainRouteReady_{false};
    bool chapterComplete_{false};
    bool standaloneFightMode_{false};
    bool standaloneTrainingMode_{false};
    bool replayMode_{false};
    std::string objectiveBeforePause_;
    std::vector<std::string> objectiveHistory_;
    QolSettings qolSettings_{};
    std::size_t pauseSelection_{0};
    int pausePage_{0};
    bool manualSaveRequested_{false};
    bool returnToTitleRequested_{false};
    std::string saveStatus_;
    float saveStatusTime_{0.0f};
    float dialogueHoldTime_{0.0f};
    std::vector<std::string> recentDialogue_;
    std::string lastRecordedDialogueKey_;
    RuntimeView view_{};
};

} // namespace px
