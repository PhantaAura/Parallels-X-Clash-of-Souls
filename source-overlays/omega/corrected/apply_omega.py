#!/usr/bin/env python3
"""Apply the Parallels X 3.0R Omega source overlay to the exact 0.4H.5-candidate source tree.

This patcher is intentionally strict. It refuses to edit a checkout when expected 0.4H.5
source anchors are missing so a newer/different tree is not silently damaged.

Usage:
    python3 apply_omega.py /path/to/Parallels-X-Clash-of-Souls
    python3 apply_omega.py /path/to/repo --check-only
"""
from __future__ import annotations

import argparse
import os
import shutil
import sys
from pathlib import Path

BASE_TAG = "v0.4H.5-candidate"
BASE_COMMIT = "851b8613fac6451f62ef492ad0ee454622096f63"


class PatchError(RuntimeError):
    pass


def load(path: Path) -> str:
    if not path.is_file():
        raise PatchError(f"required file missing: {path}")
    return path.read_text(encoding="utf-8")


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise PatchError(f"{label}: expected exactly one baseline anchor, found {count}")
    return text.replace(old, new, 1)


def replace_all_checked(text: str, old: str, new: str, expected: int, label: str) -> str:
    count = text.count(old)
    if count != expected:
        raise PatchError(f"{label}: expected {expected} baseline anchors, found {count}")
    return text.replace(old, new)


def write_if_changed(path: Path, text: str, check_only: bool, changes: list[str]) -> None:
    current = load(path)
    if current == text:
        return
    changes.append(str(path))
    if not check_only:
        path.write_text(text, encoding="utf-8")


def patch_runtime_hpp(root: Path, check_only: bool, changes: list[str]) -> None:
    path = root / "src/core/runtime.hpp"
    text = load(path)
    text = replace_once(
        text,
        "    void startChapter(const std::string& chapterId);\n"
        "    void startCpuFight();\n",
        "    void startChapter(const std::string& chapterId);\n"
        "    void startReplayChapter(const SaveData& source);\n"
        "    void startCpuFight();\n",
        "runtime.hpp replay entry",
    )
    text = replace_once(
        text,
        "    bool canManualSave() const;\n"
        "    bool standaloneMode() const { return standaloneFightMode_ || standaloneTrainingMode_; }\n",
        "    bool canManualSave() const;\n"
        "    void setQolSettings(const QolSettings& settings) { qolSettings_ = settings; syncView(); }\n"
        "    bool standaloneMode() const { return standaloneFightMode_ || standaloneTrainingMode_ || replayMode_; }\n"
        "    bool replayMode() const { return replayMode_; }\n",
        "runtime.hpp nonpersistent replay/qol",
    )
    text = replace_once(
        text,
        "    float landingAnimationTime_{0.0f};\n"
        "    bool playerCharging_{false};\n",
        "    float landingAnimationTime_{0.0f};\n"
        "    float combatReadyAnimationTime_{0.0f};\n"
        "    bool hardLanding_{false};\n"
        "    bool playerCharging_{false};\n",
        "runtime.hpp animation transition state",
    )
    text = replace_once(
        text,
        "    bool standaloneFightMode_{false};\n"
        "    bool standaloneTrainingMode_{false};\n",
        "    bool standaloneFightMode_{false};\n"
        "    bool standaloneTrainingMode_{false};\n"
        "    bool replayMode_{false};\n",
        "runtime.hpp replay flag",
    )
    text = replace_once(
        text,
        "    Game game_;\n"
        "    Vec2 playerPosition_{};\n"
        "    Vec2 opponentPosition_{};\n",
        "    Game game_;\n"
        "    Vec2 playerPosition_{};\n"
        "    Vec2 opponentPosition_{};\n"
        "    SaveData sceneCheckpointSnapshot_{};\n"
        "    bool sceneCheckpointValid_{false};\n",
        "runtime.hpp stable scene checkpoint snapshot",
    )
    write_if_changed(path, text, check_only, changes)


def patch_runtime_cpp(root: Path, check_only: bool, changes: list[str]) -> None:
    path = root / "src/core/runtime.cpp"
    text = load(path)

    # Story/replay/QoL lifecycle.
    text = replace_once(
        text,
        "void RuntimeSession::startChapter(const std::string& chapterId) {\n"
        "    standaloneFightMode_ = false;\n"
        "    standaloneTrainingMode_ = false;\n",
        "void RuntimeSession::startChapter(const std::string& chapterId) {\n"
        "    standaloneFightMode_ = false;\n"
        "    standaloneTrainingMode_ = false;\n"
        "    replayMode_ = false;\n",
        "runtime.cpp startChapter replay reset",
    )
    text = replace_once(
        text,
        "    landingAnimationTime_ = 0.0f;\n"
        "    playerCharging_ = false;\n"
        "    flowCancelLearned_ = false;\n",
        "    landingAnimationTime_ = 0.0f;\n"
        "    combatReadyAnimationTime_ = 0.0f;\n"
        "    hardLanding_ = false;\n"
        "    playerCharging_ = false;\n"
        "    flowCancelLearned_ = false;\n",
        "runtime.cpp startChapter animation reset",
    )
    text = replace_once(
        text,
        "    landingAnimationTime_ = 0.0f;\n"
        "    playerCharging_ = false;\n"
        "    cliffRouteHintLevel_ = 0;\n",
        "    landingAnimationTime_ = 0.0f;\n"
        "    combatReadyAnimationTime_ = 0.0f;\n"
        "    hardLanding_ = false;\n"
        "    playerCharging_ = false;\n"
        "    cliffRouteHintLevel_ = 0;\n",
        "runtime.cpp loadSnapshot animation reset",
    )
    text = replace_once(
        text,
        "    objectiveHistory_.clear();\n"
        "    qolSettings_ = {};\n"
        "    pauseSelection_ = 0;\n",
        "    objectiveHistory_.clear();\n"
        "    // QoL belongs to the player, not the chapter. Preserve it when Story/Fight/Training restarts.\n"
        "    pauseSelection_ = 0;\n",
        "runtime.cpp preserve global qol",
    )
    replay_fn = r'''void RuntimeSession::startReplayChapter(const SaveData& source) {
    const auto sourceQol = source.qol;
    std::vector<std::string> sourceSeen;
    constexpr const char* seenPrefix = "seen_ch1_scene=";
    for (const auto& flag : source.story.flags) {
        if (flag.rfind(seenPrefix, 0) == 0)
            sourceSeen.push_back(flag.substr(std::char_traits<char>::length(seenPrefix)));
    }
    startChapter("rrvvfo_ch1");
    replayMode_ = true;
    qolSettings_ = sourceQol;
    seenCutscenes_ = std::move(sourceSeen);
    showGameplayNotice("CHAPTER REPLAY • STORY SAVE PROTECTED", 1.65f);
    syncView();
}

'''
    text = replace_once(
        text,
        "void RuntimeSession::startCpuFight() {\n",
        replay_fn + "void RuntimeSession::startCpuFight() {\n",
        "runtime.cpp replay function",
    )
    text = replace_once(
        text,
        "void RuntimeSession::loadSnapshot(const SaveData& data) {\n"
        "    standaloneFightMode_ = false;\n"
        "    standaloneTrainingMode_ = false;\n",
        "void RuntimeSession::loadSnapshot(const SaveData& data) {\n"
        "    standaloneFightMode_ = false;\n"
        "    standaloneTrainingMode_ = false;\n"
        "    replayMode_ = false;\n",
        "runtime.cpp load replay reset",
    )
    text = replace_once(
        text,
        "    if (training_.has(game_.scene().id)) {\n"
        "        beginTrainingAt(trainingCheckpointStep_);\n"
        "        trainingManualVisible_ = true;\n"
        "        trainingManualSelection_ = trainingCheckpointStep_ == 0 ? 0 : 1;\n"
        "    }\n"
        "    syncView();\n"
        "}\n\n"
        "const MapDefinition& RuntimeSession::map() const {\n",
        "    if (training_.has(game_.scene().id)) {\n"
        "        beginTrainingAt(trainingCheckpointStep_);\n"
        "        trainingManualVisible_ = true;\n"
        "        trainingManualSelection_ = trainingCheckpointStep_ == 0 ? 0 : 1;\n"
        "    }\n"
        "    syncView();\n"
        "    // A loaded manual save is itself a valid restart point for this session.\n"
        "    sceneCheckpointSnapshot_ = saveSnapshot(data.inputPreset);\n"
        "    sceneCheckpointValid_ = true;\n"
        "}\n\n"
        "const MapDefinition& RuntimeSession::map() const {\n",
        "runtime.cpp loaded-save checkpoint capture",
    )

    # Stable scene checkpoints: scene entry is the restart authority. A manual loaded save replaces it at load end.
    text = replace_once(
        text,
        "    previousPlayerPosition_ = playerPosition_;\n"
        "    syncView();\n"
        "}\n\n"
        "void RuntimeSession::resetCurrentScene() {\n"
        "    if (training_.has(game_.scene().id)) {\n"
        "        enterCurrentScene();\n"
        "        beginTrainingAt(trainingCheckpointStep_);\n"
        "        trainingManualVisible_ = true;\n"
        "        trainingManualSelection_ = trainingCheckpointStep_ == 0 ? 0 : 1;\n"
        "        syncView();\n"
        "        return;\n"
        "    }\n"
        "    enterCurrentScene();\n"
        "}\n",
        "    previousPlayerPosition_ = playerPosition_;\n"
        "    syncView();\n"
        "    sceneCheckpointSnapshot_ = saveSnapshot();\n"
        "    sceneCheckpointValid_ = true;\n"
        "}\n\n"
        "void RuntimeSession::resetCurrentScene() {\n"
        "    if (training_.has(game_.scene().id)) {\n"
        "        enterCurrentScene();\n"
        "        beginTrainingAt(trainingCheckpointStep_);\n"
        "        trainingManualVisible_ = true;\n"
        "        trainingManualSelection_ = trainingCheckpointStep_ == 0 ? 0 : 1;\n"
        "        syncView();\n"
        "        return;\n"
        "    }\n"
        "    if (sceneCheckpointValid_) {\n"
        "        const auto checkpoint = sceneCheckpointSnapshot_;\n"
        "        loadSnapshot(checkpoint);\n"
        "        showGameplayNotice(\"CHECKPOINT RESTORED\", 1.25f);\n"
        "        return;\n"
        "    }\n"
        "    enterCurrentScene();\n"
        "}\n",
        "runtime.cpp real scene checkpoint restart",
    )

    # enter/tick animation state.
    # This exact reset block occurs in enterCurrentScene too; patch the remaining one only.
    text = replace_once(
        text,
        "    playerActionAnimation_.clear();\n"
        "    playerActionAnimationTime_ = 0.0f;\n"
        "    landingAnimationTime_ = 0.0f;\n"
        "    playerCharging_ = false;\n\n"
        "    const auto& scene = game_.scene();\n",
        "    playerActionAnimation_.clear();\n"
        "    playerActionAnimationTime_ = 0.0f;\n"
        "    landingAnimationTime_ = 0.0f;\n"
        "    combatReadyAnimationTime_ = 0.0f;\n"
        "    hardLanding_ = false;\n"
        "    playerCharging_ = false;\n\n"
        "    const auto& scene = game_.scene();\n",
        "runtime.cpp enter scene animation reset",
    )
    text = replace_once(
        text,
        "    landingAnimationTime_ = std::max(0.0f, landingAnimationTime_ - dt);\n"
        "    bufferedCombatTime_ = std::max(0.0f, bufferedCombatTime_ - dt);\n",
        "    landingAnimationTime_ = std::max(0.0f, landingAnimationTime_ - dt);\n"
        "    if (landingAnimationTime_ <= 0.0f) hardLanding_ = false;\n"
        "    combatReadyAnimationTime_ = std::max(0.0f, combatReadyAnimationTime_ - dt);\n"
        "    bufferedCombatTime_ = std::max(0.0f, bufferedCombatTime_ - dt);\n",
        "runtime.cpp animation timers",
    )
    text = replace_once(
        text,
        "    beginTrainingAt(start);\n"
        "    trainingManualVisible_ = false;\n",
        "    beginTrainingAt(start);\n"
        "    trainingManualVisible_ = false;\n"
        "    combatReadyAnimationTime_ = 0.30f;\n",
        "runtime.cpp training combat-ready",
    )

    # Capture hard landings in Sage arena and roadside combat, then add the missing exploration landing feedback.
    combat_move_old = (
        "    playerPosition_ = FieldMovementSystem::tick(map(), playerPosition_, movementState_, movementInput, dt, movement, disabledBlockers_);\n"
        "    updateCombatFacing(dt);\n"
        "    if (movementState_.landedThisFrame) landingAnimationTime_ = 0.13f;\n"
    )
    combat_move_new = (
        "    const float preLandingVelocity = movementState_.verticalVelocity;\n"
        "    playerPosition_ = FieldMovementSystem::tick(map(), playerPosition_, movementState_, movementInput, dt, movement, disabledBlockers_);\n"
        "    updateCombatFacing(dt);\n"
        "    if (movementState_.landedThisFrame) {\n"
        "        hardLanding_ = preLandingVelocity < -520.0f;\n"
        "        landingAnimationTime_ = hardLanding_ ? 0.22f : 0.13f;\n"
        "    }\n"
    )
    text = replace_all_checked(text, combat_move_old, combat_move_new, 2, "runtime.cpp combat hard landings")

    text = replace_once(
        text,
        "    previousPlayerPosition_ = playerPosition_;\n"
        "    playerPosition_ = FieldMovementSystem::tick(map(), playerPosition_, movementState_, input, dt, movement, disabledBlockers_);\n"
        "    updateExplorationFacing(dt);\n"
        "    const auto* ability = pressedAbility(input);\n",
        "    previousPlayerPosition_ = playerPosition_;\n"
        "    const float preLandingVelocity = movementState_.verticalVelocity;\n"
        "    playerPosition_ = FieldMovementSystem::tick(map(), playerPosition_, movementState_, input, dt, movement, disabledBlockers_);\n"
        "    updateExplorationFacing(dt);\n"
        "    if (movementState_.landedThisFrame) {\n"
        "        hardLanding_ = preLandingVelocity < -520.0f;\n"
        "        landingAnimationTime_ = hardLanding_ ? 0.22f : 0.13f;\n"
        "    }\n"
        "    const auto* ability = pressedAbility(input);\n",
        "runtime.cpp exploration hard landing",
    )

    # Combat entry flourish.
    text = replace_once(
        text,
        "void RuntimeSession::startRoadsideFight() {\n"
        "    const auto* adventure = currentAdventure();\n",
        "void RuntimeSession::startRoadsideFight() {\n"
        "    const auto* adventure = currentAdventure();\n",
        "runtime.cpp roadside function anchor",
    )
    text = replace_once(
        text,
        "    roadsideFightActive_ = true;\n"
        "    roadsideFightIntroShown_ = true;\n",
        "    roadsideFightActive_ = true;\n"
        "    roadsideFightIntroShown_ = true;\n"
        "    combatReadyAnimationTime_ = 0.30f;\n",
        "runtime.cpp roadside combat-ready",
    )

    text = replace_once(
        text,
        "void RuntimeSession::resolveRoadsideEncounter(bool wonFight) {\n"
        "    roadsideFightActive_ = false;\n"
        "    spectatorPassWon_ = spectatorPassWon_ || wonFight;\n",
        "void RuntimeSession::resolveRoadsideEncounter(bool wonFight) {\n"
        "    roadsideFightActive_ = false;\n"
        "    triggerAbilityAnimation(\"combat_relax\", 0.28f);\n"
        "    spectatorPassWon_ = spectatorPassWon_ || wonFight;\n",
        "runtime.cpp combat exit relax",
    )

    # Exact 7.2R story-only Fire Blast cost/damage/cooldown entry point.
    text = replace_once(
        text,
        "    if (ability->id == \"fireBlast\" && player_.energy >= ability->energyCost &&\n"
        "        CombatSystem::startAttack(player_, AttackKind::Projectile)) {\n"
        "        player_.energy -= ability->energyCost;\n"
        "        triggerAbilityAnimation(\"fire_blast\", 0.42f);\n"
        "    } else if (ability->id == \"objectSwap\" && player_.energy >= ability->energyCost) {\n",
        "    if (ability->id == \"fireBlast\" && attackCooldown_ <= 0.0f && player_.energy >= ability->energyCost &&\n"
        "        CombatSystem::startAttack(player_, AttackKind::Projectile)) {\n"
        "        player_.energy -= ability->energyCost;\n"
        "        attackCooldown_ = 1.05f; // Browser 2.9A.40.7.2R Chapter-1 Fire Blast cooldown.\n"
        "        triggerAbilityAnimation(\"fire_blast\", 0.42f);\n"
        "    } else if (ability->id == \"objectSwap\" && player_.energy >= ability->energyCost) {\n",
        "runtime.cpp Fire Blast cooldown",
    )

    # Animation selection: hub idle/run are separate from combat stance/locomotion.
    old_resolve = '''std::string RuntimeSession::resolvePlayerAnimation() const {
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
'''
    new_resolve = '''std::string RuntimeSession::resolvePlayerAnimation() const {
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
    if (landingAnimationTime_ > 0.0f) return hardLanding_ ? "hard_land" : "land";
    const bool moving = distance(previousPlayerPosition_, playerPosition_) > 0.35f;
    if (moving) {
        if (!combat) return "run";
        return playerBackpedaling() ? "combat_retreat" : "combat_advance";
    }
    return combat ? "fighting_stance" : "idle";
}
'''
    text = replace_once(text, old_resolve, new_resolve, "runtime.cpp animation state machine")
    text = replace_once(
        text,
        "    view_.playerAnimation = resolvePlayerAnimation();\n"
        "    view_.playerAnimationSpeed = playerBackpedaling() && view_.playerAnimation == \"run\" ? -0.72f : 1.0f;\n",
        "    view_.playerAnimation = resolvePlayerAnimation();\n"
        "    // Combat retreat is now its own authored clip; never reverse-play the hub sprint.\n"
        "    view_.playerAnimationSpeed = 1.0f;\n",
        "runtime.cpp positive animation playback",
    )

    # Safe snapshots: an optional arena can never serialize its arena position as a road coordinate.
    text = replace_once(
        text,
        "    data.world.position = playerPosition_;\n",
        "    data.world.position = roadsideFightActive_ ? roadsideReturnPosition_ : playerPosition_;\n",
        "runtime.cpp safe road save position",
    )

    write_if_changed(path, text, check_only, changes)


def patch_ability_hotbar(root: Path, check_only: bool, changes: list[str]) -> None:
    path = root / "src/core/ability_hotbar.cpp"
    text = load(path)
    text = replace_all_checked(text, '"fireBlast", "FIRE BLAST", "FIRE", 28.0f, 0.0f,',
                               '"fireBlast", "FIRE BLAST", "FIRE", 22.0f, 0.0f,', 2,
                               "ability_hotbar Fire Blast 7.2R cost")
    write_if_changed(path, text, check_only, changes)


def patch_combat(root: Path, check_only: bool, changes: list[str]) -> None:
    path = root / "src/core/combat.cpp"
    text = load(path)
    old = '''const AttackDefinition& CombatSystem::attackFor(const std::string& fighterId, AttackKind kind) {
    const auto index = indexOf(kind);
    if (index >= std::size(kDefaultAttacks)) throw std::out_of_range("Unknown attack kind");
    if (fighterId != "sage" && fighterId != "plouke") return kDefaultAttacks[index];
'''
    new = '''const AttackDefinition& CombatSystem::attackFor(const std::string& fighterId, AttackKind kind) {
    const auto index = indexOf(kind);
    if (index >= std::size(kDefaultAttacks)) throw std::out_of_range("Unknown attack kind");
    // Browser 2.9A.40.7.2R Chapter-1 Rrvvfo Fire Blast combat profile.
    // Projectile travel speed/radius are presentation/simulation fields still pending the projectile-object port;
    // do not fake those values by changing unrelated melee geometry.
    if (fighterId == "rrvvfo" && kind == AttackKind::Projectile) {
        static const AttackDefinition rrvvfoFireBlast = [] {
            auto value = kDefaultAttacks[indexOf(AttackKind::Projectile)];
            value.damage = 15.0f;
            value.guardDamage = 9.0f;
            return value;
        }();
        return rrvvfoFireBlast;
    }
    if (fighterId != "sage" && fighterId != "plouke") return kDefaultAttacks[index];
'''
    text = replace_once(text, old, new, "combat.cpp Rrvvfo Fire Blast profile")
    write_if_changed(path, text, check_only, changes)


def patch_save_schema(root: Path, check_only: bool, changes: list[str]) -> None:
    hpp = root / "src/core/save.hpp"
    text = load(hpp)
    text = replace_once(text, "    static constexpr int kSchemaVersion = 4;\n",
                        "    static constexpr int kSchemaVersion = 5;\n", "save.hpp schema 5")
    write_if_changed(hpp, text, check_only, changes)

    cpp = root / "src/core/save.cpp"
    text = load(cpp)
    text = replace_once(
        text,
        "    if (data.schemaVersion != 2 && data.schemaVersion != 3 && data.schemaVersion != SaveData::kSchemaVersion)\n"
        "        throw std::runtime_error(\"Unsupported save schema\");\n",
        "    if (data.schemaVersion != 2 && data.schemaVersion != 3 && data.schemaVersion != 4 &&\n"
        "        data.schemaVersion != SaveData::kSchemaVersion)\n"
        "        throw std::runtime_error(\"Unsupported save schema\");\n",
        "save.cpp schema 4 migration",
    )
    write_if_changed(cpp, text, check_only, changes)

    game = root / "src/core/game.cpp"
    text = load(game)
    old = '''void Game::loadSave(const SaveData& data) {
    if (!chapters_.has(data.story.chapterId)) throw std::runtime_error("Save references unknown chapter");
    const auto& def = chapters_.get(data.story.chapterId);
    if (data.story.sceneIndex >= def.openingFlow.size()) throw std::runtime_error("Save scene index is out of range");
    save_ = data;
    syncModeToScene();
}
'''
    new = '''void Game::loadSave(const SaveData& data) {
    if (!chapters_.has(data.story.chapterId)) throw std::runtime_error("Save references unknown chapter");
    const auto& def = chapters_.get(data.story.chapterId);
    SaveData resolved = data;

    // Omega schema migration: checkpoint IDs are stable content identities, while numeric scene indexes
    // can shift when Chapter 1 gains missing browser beats. Prefer the stable ID whenever it resolves.
    if (!data.story.checkpointId.empty()) {
        const auto checkpoint = std::find_if(def.openingFlow.begin(), def.openingFlow.end(), [&](const SceneStep& step) {
            return step.checkpointId == data.story.checkpointId;
        });
        if (checkpoint != def.openingFlow.end())
            resolved.story.sceneIndex = static_cast<std::size_t>(std::distance(def.openingFlow.begin(), checkpoint));
    }
    if (resolved.story.sceneIndex >= def.openingFlow.size())
        throw std::runtime_error("Save scene index/checkpoint is out of range");
    resolved.story.checkpointId = def.openingFlow[resolved.story.sceneIndex].checkpointId;
    save_ = resolved;
    syncModeToScene();
}
'''
    text = replace_once(text, old, new, "game.cpp stable checkpoint migration")
    text = replace_once(text, "#include <stdexcept>\n", "#include <algorithm>\n#include <stdexcept>\n", "game.cpp algorithm include")
    write_if_changed(game, text, check_only, changes)



def patch_exploration_content(root: Path, check_only: bool, changes: list[str]) -> None:
    hpp = root / "src/content/exploration_registry.hpp"
    text = load(hpp)
    text = replace_once(
        text,
        "    std::vector<Vec2> jumpMarkers;\n"
        "    float jumpMarkerRadius{72.0f};\n",
        "    std::vector<Vec2> jumpMarkers;\n"
        "    float jumpMarkerRadius{72.0f};\n"
        "    // Omega keeps the browser-authoritative route counts/order while mapping them into the native 3D road.\n"
        "    std::vector<Vec2> mainWorkMarkers;\n"
        "    std::vector<Vec2> forestBellMarkers;\n"
        "    float routeHintFirstSeconds{18.0f};\n"
        "    float routeHintSecondSeconds{36.0f};\n",
        "exploration.hpp Omega route sequences",
    )
    write_if_changed(hpp, text, check_only, changes)

    cpp = root / "src/content/exploration_registry.cpp"
    text = load(cpp)
    text = replace_once(
        text,
        "    routeAdventure.jumpMarkers = {{300.0f, 330.0f}, {355.0f, 400.0f}, {412.0f, 350.0f}};\n"
        "    routeAdventure.jumpMarkerRadius = 72.0f;\n",
        "    // 7.2R parity counts: Main = four work-lane beats, Forest = four sequential bells, Cliff = five ledges.\n"
        "    // These native 3D positions preserve the existing route geography; exact browser pixel coordinates are not fabricated.\n"
        "    routeAdventure.mainWorkMarkers = {{305.0f, 42.0f}, {340.0f, -34.0f}, {378.0f, 38.0f}, {414.0f, 0.0f}};\n"
        "    routeAdventure.forestBellMarkers = {{286.0f, -242.0f}, {327.0f, -322.0f}, {371.0f, -398.0f}, {418.0f, -350.0f}};\n"
        "    routeAdventure.jumpMarkers = {{286.0f, 305.0f}, {325.0f, 365.0f}, {360.0f, 425.0f}, {397.0f, 388.0f}, {425.0f, 342.0f}};\n"
        "    routeAdventure.jumpMarkerRadius = 72.0f;\n"
        "    routeAdventure.routeHintFirstSeconds = 18.0f;\n"
        "    routeAdventure.routeHintSecondSeconds = 36.0f;\n",
        "exploration.cpp browser route counts",
    )
    old_transport = '''    auto transport = roadScene(
        "transport_wheel_recovery", ExplorationRuleKind::UseAbilityPoint,
        "RECOVER THE TRANSPORT WHEEL", "Use [2] Object Swap near the marked wheel beyond the broken ledge.");
    transport.target = {735.0f, 40.0f};
    transport.radius = 250.0f;
    transport.requiredAbilityId = "objectSwap";
    transport.completionDialogueId = "transport_wheel_result";
    scenes_.emplace(transport.sceneId, transport);
'''
    new_transport = '''    auto transport = roadScene(
        "transport_wheel_recovery", ExplorationRuleKind::SwapRelay,
        "RECOVER THE TRANSPORT WHEEL", "Swap to the stranded wheel, then use the return anchor to get back.");
    transport.radius = 250.0f;
    transport.requiredAbilityId = "objectSwap";
    transport.relayMarkers = {{770.0f, -210.0f}, {0.0f, 0.0f}};
    transport.completionDialogueId = "transport_wheel_result";
    scenes_.emplace(transport.sceneId, transport);
'''
    text = replace_once(text, old_transport, new_transport, "exploration.cpp two-sided transport")
    write_if_changed(cpp, text, check_only, changes)


def patch_map_for_southern_detour(root: Path, check_only: bool, changes: list[str]) -> None:
    path = root / "src/content/map_registry.cpp"
    text = load(path)
    text = replace_once(
        text,
        '{"lens_roadblock_south", {1045.0f, 1120.0f, 175.0f, 720.0f}, true}',
        '{"lens_roadblock_south", {1045.0f, 1120.0f, 175.0f, 430.0f}, true}',
        "map southern detour corridor",
    )
    write_if_changed(path, text, check_only, changes)


def patch_runtime_routes(root: Path, check_only: bool, changes: list[str]) -> None:
    hpp = root / "src/core/runtime.hpp"
    text = load(hpp)
    text = replace_once(
        text,
        "    int cliffRouteHintLevel_{0};\n"
        "    std::string routeChoice_;\n",
        "    int cliffRouteHintLevel_{0};\n"
        "    int routeHintStage_{0};\n"
        "    std::size_t routeProgress_{0};\n"
        "    float routeChallengeTime_{0.0f};\n"
        "    bool mainRouteFireCleared_{false};\n"
        "    bool southernDetourChosen_{false};\n"
        "    bool southernDetourComplete_{false};\n"
        "    std::string routeChoice_;\n",
        "runtime.hpp route progress state",
    )
    write_if_changed(hpp, text, check_only, changes)

    path = root / "src/core/runtime.cpp"
    text = load(path)
    # Generic numeric flag helper, used for save-safe route/relay state.
    anchor = '''std::size_t savedTutorialStep(const std::vector<std::string>& flags) {
    constexpr const char* prefix = "ch1_tutorial_checkpoint=";
    for (const auto& flag : flags) {
        if (flag.rfind(prefix, 0) != 0) continue;
        const auto value = static_cast<std::size_t>(std::stoul(flag.substr(std::char_traits<char>::length(prefix))));
        return std::min<std::size_t>(value, 6);
    }
    return 0;
}
'''
    helper = anchor + '''
std::size_t savedUnsignedFlag(const std::vector<std::string>& flags, const char* prefix, std::size_t maximum = 64) {
    const auto prefixLength = std::char_traits<char>::length(prefix);
    for (const auto& flag : flags) {
        if (flag.rfind(prefix, 0) != 0) continue;
        try { return std::min<std::size_t>(static_cast<std::size_t>(std::stoul(flag.substr(prefixLength))), maximum); }
        catch (...) { return 0; }
    }
    return 0;
}
'''
    text = replace_once(text, anchor, helper, "runtime.cpp numeric save flag helper")

    text = replace_once(
        text,
        "    cliffRouteHintLevel_ = 0;\n"
        "    disabledBlockers_.clear();\n",
        "    cliffRouteHintLevel_ = 0;\n"
        "    routeHintStage_ = 0;\n"
        "    routeProgress_ = 0;\n"
        "    routeChallengeTime_ = 0.0f;\n"
        "    mainRouteFireCleared_ = false;\n"
        "    southernDetourChosen_ = false;\n"
        "    southernDetourComplete_ = false;\n"
        "    disabledBlockers_.clear();\n",
        "runtime.cpp chapter route reset",
    )
    # Load persistent route flags before entering scene.
    text = replace_once(
        text,
        "    cliffRewardEarned_ = containsFlag(data.story.flags, \"ch1_road_dare_badge\");\n"
        "    flowCancelLearned_ = containsFlag(data.story.flags, \"ch1_flow_cancel_learned\");\n",
        "    cliffRewardEarned_ = containsFlag(data.story.flags, \"ch1_road_dare_badge\");\n"
        "    flowCancelLearned_ = containsFlag(data.story.flags, \"ch1_flow_cancel_learned\");\n"
        "    mainRouteFireCleared_ = containsFlag(data.story.flags, \"ch1_main_fire_cleared\");\n"
        "    southernDetourChosen_ = containsFlag(data.story.flags, \"ch1_southern_detour_chosen\");\n"
        "    southernDetourComplete_ = containsFlag(data.story.flags, \"ch1_southern_detour_complete\");\n",
        "runtime.cpp load route flags",
    )
    # Restore relay marker locations from save objects and route progress after enterCurrentScene rebuilt defaults.
    text = replace_once(
        text,
        "    enterCurrentScene();\n"
        "    playerPosition_ = data.world.position;\n",
        "    enterCurrentScene();\n"
        "    routeProgress_ = savedUnsignedFlag(data.story.flags, \"ch1_route_progress=\", 8);\n"
        "    const auto cliffMask = savedUnsignedFlag(data.story.flags, \"ch1_cliff_mask=\", 31);\n"
        "    if (!cliffJumpComplete_.empty()) {\n"
        "        for (std::size_t i = 0; i < cliffJumpComplete_.size(); ++i) cliffJumpComplete_[i] = (cliffMask & (1u << i)) != 0;\n"
        "    }\n"
        "    if (game_.scene().id == \"selected_route_adventure\" && routeChoice_ == \"main\" && (mainRouteFireCleared_ || routeProgress_ > 0)) {\n"
        "        mainRouteDialogueShown_ = true; mainRouteReady_ = true;\n"
        "        if (mainRouteFireCleared_) disableBlocker(\"fallen_tree_center\");\n"
        "    }\n"
        "    if (exploration_.has(game_.scene().id) && exploration_.get(game_.scene().id).rule == ExplorationRuleKind::SwapRelay) {\n"
        "        relayIndex_ = static_cast<int>(savedUnsignedFlag(data.story.flags, \"ch1_relay_index=\", relayMarkers_.size()));\n"
        "        for (std::size_t i = 0; i < relayMarkers_.size(); ++i) {\n"
        "            const auto id = \"relay_marker_\" + std::to_string(i);\n"
        "            const auto savedMarker = std::find_if(data.world.objects.begin(), data.world.objects.end(), [&](const WorldObjectState& object){ return object.id == id; });\n"
        "            if (savedMarker != data.world.objects.end()) relayMarkers_[i] = savedMarker->position;\n"
        "        }\n"
        "        if (game_.scene().id == \"swap_relay_trial\" && relayIndex_ >= static_cast<int>(relayMarkers_.size())) relayReleaseTimer_ = 0.01f;\n"
        "    }\n"
        "    playerPosition_ = data.world.position;\n",
        "runtime.cpp restore route/relay substate",
    )

    # Enter route scene with clean counters; Lens scene exposes the browser choice instead of forcing Lens.
    text = replace_once(
        text,
        "        if (scene.id == \"selected_route_adventure\") {\n"
        "            mainRouteDialogueShown_ = false;\n"
        "            mainRouteReady_ = routeChoice_ != \"main\";\n"
        "            cliffJumpComplete_.assign(definition.jumpMarkers.size(), false);\n"
        "        }\n",
        "        if (scene.id == \"selected_route_adventure\") {\n"
        "            mainRouteDialogueShown_ = false;\n"
        "            mainRouteReady_ = routeChoice_ != \"main\";\n"
        "            routeProgress_ = 0; routeChallengeTime_ = 0.0f; routeHintStage_ = 0;\n"
        "            cliffJumpComplete_.assign(definition.jumpMarkers.size(), false);\n"
        "        }\n"
        "        if (scene.id == \"lens_roadblock_reveal\" && !southernDetourChosen_ && !southernDetourComplete_) {\n"
        "            choiceKind_ = 5; choiceIndex_ = 0;\n"
        "        }\n",
        "runtime.cpp route/lens entry state",
    )

    # Main-route Fire Blast dialogue no longer completes the entire route; it opens the four work beats.
    text = replace_once(
        text,
        "        case 3:\n"
        "            disableBlocker(\"fallen_tree_center\");\n"
        "            disableBlocker(\"fallen_tree_north\");\n"
        "            disableBlocker(\"fallen_tree_south\");\n"
        "            completeScene();\n"
        "            break;\n",
        "        case 3:\n"
        "            mainRouteFireCleared_ = true;\n"
        "            disableBlocker(\"fallen_tree_center\");\n"
        "            routeProgress_ = 0; routeChallengeTime_ = 0.0f; routeHintStage_ = 0;\n"
        "            showGameplayNotice(\"MAIN ROAD • FOUR WORK LANES AHEAD\", 1.35f);\n"
        "            break;\n",
        "runtime.cpp Main route post-Fire Blast continuation",
    )

    # Add the Lens-vs-detour choice to shared choice handling.
    text = replace_once(
        text,
        "    if (choiceKind_ == 4 && input.pressed(Action::Cancel)) return;\n",
        "    if ((choiceKind_ == 4 || choiceKind_ == 5) && input.pressed(Action::Cancel)) return;\n",
        "runtime.cpp protected route choices",
    )
    text = replace_once(
        text,
        "        completeScene();\n"
        "    }\n"
        "}\n\n"
        "void RuntimeSession::startRunawayCartQte() {\n",
        "        completeScene();\n"
        "    } else if (kind == 5) {\n"
        "        southernDetourChosen_ = second;\n"
        "        if (southernDetourChosen_) showGameplayNotice(\"SOUTH DETOUR • LONG WAY AROUND\", 1.45f);\n"
        "        else showGameplayNotice(\"LENS ROUTE • READ THE ROADBLOCK\", 1.35f);\n"
        "    }\n"
        "}\n\n"
        "void RuntimeSession::startRunawayCartQte() {\n",
        "runtime.cpp Lens-vs-detour choice result",
    )

    # Replace the route/relay/lens switch section as one coherent state machine.
    old_switch = '''        case ExplorationRuleKind::RouteChallenge: {
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
'''
    new_switch = '''        case ExplorationRuleKind::RouteChallenge: {
            const auto* challenge = currentRouteChallenge(definition);
            if (!challenge) break;
            routeChallengeTime_ += dt;
            if (routeHintStage_ == 0 && routeChallengeTime_ >= definition.routeHintFirstSeconds) {
                routeHintStage_ = 1;
                showGameplayNotice(routeChoice_ == "forest" ? "FOREST • LISTEN FOR THE NEXT BLUE BELL" :
                                   routeChoice_ == "cliff" ? "CLIFF • TAKE THE LEDGES IN ORDER" :
                                   "MAIN ROAD • FOLLOW THE WORK LANES", 1.30f);
            } else if (routeHintStage_ == 1 && routeChallengeTime_ >= definition.routeHintSecondSeconds) {
                routeHintStage_ = 2;
                showGameplayNotice(routeChoice_ == "forest" ? "FOREST • FOUR BELLS • ONE AFTER ANOTHER" :
                                   routeChoice_ == "cliff" ? "CLIFF • FIVE AIRBORNE LEDGES" :
                                   "MAIN ROAD • FOUR WORK BEATS AFTER THE FIRE BLOCK", 1.55f);
            }

            if (routeChoice_ == "main") {
                if (!mainRouteDialogueShown_ && playerPosition_.x > 235.0f) {
                    mainRouteDialogueShown_ = true;
                    beginTransientDialogue(definition.openingDialogueId, 2);
                    break;
                }
                if (!mainRouteReady_) break;
                if (!mainRouteFireCleared_) {
                    if (ability && ability->id == challenge->requiredAbilityId &&
                        withinAbilityTarget(playerPosition_, challenge->abilityTarget, challenge->abilityRadius)) {
                        triggerAbilityAnimation("fire_blast", 0.42f);
                        beginTransientDialogue(definition.completionDialogueId, 3);
                    }
                    break;
                }
                if (routeProgress_ < definition.mainWorkMarkers.size() &&
                    distance(playerPosition_, definition.mainWorkMarkers[routeProgress_]) <= 62.0f) {
                    ++routeProgress_;
                    showGameplayNotice("MAIN ROAD • WORK LANE " + std::to_string(routeProgress_) + " / 4", 1.0f);
                }
                if (routeProgress_ < definition.mainWorkMarkers.size()) break;
            } else if (routeChoice_ == "forest") {
                if (routeProgress_ < definition.forestBellMarkers.size() &&
                    distance(playerPosition_, definition.forestBellMarkers[routeProgress_]) <= 74.0f) {
                    ++routeProgress_;
                    showGameplayNotice("BLUE BELL • " + std::to_string(routeProgress_) + " / 4", 1.05f);
                }
                if (routeProgress_ < definition.forestBellMarkers.size()) break;
            } else if (routeChoice_ == "cliff") {
                if (routeProgress_ < definition.jumpMarkers.size() &&
                    distance(playerPosition_, definition.jumpMarkers[routeProgress_]) <= definition.jumpMarkerRadius &&
                    (movementState_.height > 8.0f || movementState_.jumpStartedThisFrame)) {
                    cliffJumpComplete_[routeProgress_] = true;
                    ++routeProgress_;
                    showGameplayNotice("CLIFF LEDGE • " + std::to_string(routeProgress_) + " / 5", 0.95f);
                }
                const bool ready = routeProgress_ >= definition.jumpMarkers.size();
                if (!ready && playerPosition_.x > 430.0f) playerPosition_.x = 410.0f;
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
                if (playerPosition_.x > 1130.0f && playerPosition_.z > 430.0f) {
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
                for (const auto& blocker : definition.blockersToDisable) disableBlocker(blocker);
                completeScene();
            }
            break;
'''
    text = replace_once(text, old_switch, new_switch, "runtime.cpp full route/relay/lens parity state machine")

    # Persist relay markers and route state. This also fixes the pre-Omega mid-relay save/reload bug.
    text = replace_once(
        text,
        "    data.world.objects.erase(std::remove_if(data.world.objects.begin(), data.world.objects.end(), [](const WorldObjectState& object){\n"
        "        return object.id == \"far_bank_rock\";\n"
        "    }), data.world.objects.end());\n"
        "    data.world.objects.push_back({\"far_bank_rock\", farBankRockPosition_, true});\n",
        "    data.world.objects.erase(std::remove_if(data.world.objects.begin(), data.world.objects.end(), [](const WorldObjectState& object){\n"
        "        return object.id == \"far_bank_rock\" || object.id.rfind(\"relay_marker_\", 0) == 0;\n"
        "    }), data.world.objects.end());\n"
        "    data.world.objects.push_back({\"far_bank_rock\", farBankRockPosition_, true});\n"
        "    for (std::size_t i = 0; i < relayMarkers_.size(); ++i)\n"
        "        data.world.objects.push_back({\"relay_marker_\" + std::to_string(i), relayMarkers_[i], true});\n",
        "runtime.cpp persist relay marker geometry",
    )
    text = replace_once(
        text,
        "    data.story.flags.erase(std::remove_if(data.story.flags.begin(), data.story.flags.end(), [](const std::string& flag){\n"
        "        return flag.rfind(\"ch1_tutorial_checkpoint=\", 0) == 0;\n"
        "    }), data.story.flags.end());\n",
        "    data.story.flags.erase(std::remove_if(data.story.flags.begin(), data.story.flags.end(), [](const std::string& flag){\n"
        "        return flag.rfind(\"ch1_tutorial_checkpoint=\", 0) == 0 ||\n"
        "               flag.rfind(\"ch1_route_progress=\", 0) == 0 ||\n"
        "               flag.rfind(\"ch1_relay_index=\", 0) == 0 ||\n"
        "               flag.rfind(\"ch1_cliff_mask=\", 0) == 0;\n"
        "    }), data.story.flags.end());\n",
        "runtime.cpp replace dynamic progress flags",
    )
    text = replace_once(
        text,
        "    addFlag(\"ch1_tutorial_checkpoint=\" + std::to_string(trainingCheckpointStep_));\n",
        "    addFlag(\"ch1_tutorial_checkpoint=\" + std::to_string(trainingCheckpointStep_));\n"
        "    addFlag(\"ch1_route_progress=\" + std::to_string(routeProgress_));\n"
        "    addFlag(\"ch1_relay_index=\" + std::to_string(std::max(0, relayIndex_)));\n"
        "    unsigned cliffMask = 0;\n"
        "    for (std::size_t i = 0; i < cliffJumpComplete_.size() && i < 31; ++i) if (cliffJumpComplete_[i]) cliffMask |= (1u << i);\n"
        "    addFlag(\"ch1_cliff_mask=\" + std::to_string(cliffMask));\n"
        "    if (mainRouteFireCleared_) addFlag(\"ch1_main_fire_cleared\");\n"
        "    if (southernDetourChosen_) addFlag(\"ch1_southern_detour_chosen\");\n"
        "    if (southernDetourComplete_) { addFlag(\"ch1_southern_detour_complete\"); addFlag(\"world_delight_southern_detour\"); }\n",
        "runtime.cpp persist route state flags",
    )

    # Choice/presentation markers and objective progress.
    text = replace_once(
        text,
        "    } else if (choiceKind_ == 4) {\n"
        "        view_.choiceTitle = \"CHOOSE A ROUTE\";\n"
        "        view_.choiceOptions = {\"MAIN ROAD\", \"FOREST SHORTCUT\", \"CLIFF ROUTE\"};\n"
        "    }\n",
        "    } else if (choiceKind_ == 4) {\n"
        "        view_.choiceTitle = \"CHOOSE A ROUTE\";\n"
        "        view_.choiceOptions = {\"MAIN ROAD\", \"FOREST SHORTCUT\", \"CLIFF ROUTE\"};\n"
        "    } else if (choiceKind_ == 5) {\n"
        "        view_.choiceTitle = \"FINAL ROADBLOCK\";\n"
        "        view_.choiceOptions = {\"USE LENS OF TRUTH\", \"TAKE SOUTH DETOUR\"};\n"
        "    }\n",
        "runtime.cpp Lens choice UI",
    )
    text = replace_once(
        text,
        "        if (view_.sceneId == \"selected_route_adventure\" && routeChoice_ == \"cliff\") {\n",
        "        if (view_.sceneId == \"selected_route_adventure\" && routeChoice_ == \"main\") {\n"
        "            const auto& definition = exploration_.get(view_.sceneId);\n"
        "            for (std::size_t i = 0; i < definition.mainWorkMarkers.size(); ++i)\n"
        "                view_.worldMarkers.push_back({\"main_work_\" + std::to_string(i + 1), definition.mainWorkMarkers[i], \"work-lane\", i < routeProgress_});\n"
        "        }\n"
        "        if (view_.sceneId == \"selected_route_adventure\" && routeChoice_ == \"forest\") {\n"
        "            const auto& definition = exploration_.get(view_.sceneId);\n"
        "            for (std::size_t i = 0; i < definition.forestBellMarkers.size(); ++i)\n"
        "                view_.worldMarkers.push_back({\"forest_bell_\" + std::to_string(i + 1), definition.forestBellMarkers[i], \"blue-bell\", i < routeProgress_});\n"
        "        }\n"
        "        if (view_.sceneId == \"selected_route_adventure\" && routeChoice_ == \"cliff\") {\n",
        "runtime.cpp route world markers",
    )
    text = replace_once(
        text,
        "        if (view_.sceneId == \"transport_wheel_recovery\") {\n"
        "            view_.worldMarkers.push_back({\"transport_wheel\", adventure->transportWheel, \"transport-wheel\", false});\n"
        "        }\n",
        "        if (view_.sceneId == \"transport_wheel_recovery\") {\n"
        "            if (!relayMarkers_.empty() && relayIndex_ == 0) view_.worldMarkers.push_back({\"transport_wheel\", relayMarkers_[0], \"transport-wheel\", false});\n"
        "            if (relayMarkers_.size() > 1 && relayIndex_ > 0) view_.worldMarkers.push_back({\"transport_return\", relayMarkers_[1], \"return-anchor\", relayIndex_ > 1});\n"
        "        }\n",
        "runtime.cpp transport return marker",
    )
    text = replace_once(
        text,
        "            if (definition.rule == ExplorationRuleKind::RouteChallenge) {\n"
        "                if (const auto* challenge = currentRouteChallenge(definition)) view_.objective = challenge->objective;\n"
        "            } else if (definition.rule == ExplorationRuleKind::SwapRelay) {\n"
        "                view_.objective = (view_.sceneId == \"sage_object_swap_field_trial\" ? \"SAGE FIELD ANCHORS • \" : \"OBJECT SWAP RELAY • \") +\n"
        "                                  std::to_string(relayIndex_) + \" / \" + std::to_string(definition.relayMarkers.size());\n"
        "            }\n",
        "            if (definition.rule == ExplorationRuleKind::RouteChallenge) {\n"
        "                if (const auto* challenge = currentRouteChallenge(definition)) view_.objective = challenge->objective;\n"
        "                if (routeChoice_ == \"main\" && mainRouteFireCleared_) view_.objective += \" • \" + std::to_string(routeProgress_) + \" / 4\";\n"
        "                else if (routeChoice_ == \"forest\") view_.objective += \" • BELLS \" + std::to_string(routeProgress_) + \" / 4\";\n"
        "            } else if (definition.rule == ExplorationRuleKind::SwapRelay) {\n"
        "                const std::string prefix = view_.sceneId == \"sage_object_swap_field_trial\" ? \"SAGE FIELD ANCHORS • \" :\n"
        "                                           view_.sceneId == \"transport_wheel_recovery\" ? \"TRANSPORT RESCUE • \" : \"OBJECT SWAP RELAY • \";\n"
        "                view_.objective = prefix + std::to_string(relayIndex_) + \" / \" + std::to_string(definition.relayMarkers.size());\n"
        "            }\n"
        "            if (view_.sceneId == \"lens_roadblock_reveal\" && southernDetourChosen_)\n"
        "                view_.objectiveDetail = \"Go far south, pass the roadblock, then reconnect with Tournament Road.\";\n",
        "runtime.cpp route objective progress",
    )

    # NPCs turn toward Rrvvfo when he is close enough to interact, giving the road more authored life.
    text = replace_once(
        text,
        "            view_.ambientActors.push_back({npc.id, npc.characterPresentationId, position, 0.0f,\n"
        "                distance(playerPosition_, position) <= npc.interactionRadius});\n",
        "            const bool interactable = distance(playerPosition_, position) <= npc.interactionRadius;\n"
        "            float npcYaw = 0.0f;\n"
        "            if (interactable) { const Vec2 toPlayer{playerPosition_.x-position.x, playerPosition_.z-position.z}; npcYaw = yawForDirection(toPlayer); }\n"
        "            view_.ambientActors.push_back({npc.id, npc.characterPresentationId, position, npcYaw, interactable});\n",
        "runtime.cpp NPC turn-to-face",
    )

    write_if_changed(path, text, check_only, changes)


def patch_core_tests_milestone_b(root: Path, check_only: bool, changes: list[str]) -> None:
    path = root / "tests/core_tests.cpp"
    text = load(path)
    text = replace_once(
        text,
        "    assert(exploration.get(\"selected_route_adventure\").routeChallenges.size() == 3);\n"
        "    assert(exploration.get(\"selected_route_adventure\").jumpMarkers.size() == 3);\n",
        "    assert(exploration.get(\"selected_route_adventure\").routeChallenges.size() == 3);\n"
        "    assert(exploration.get(\"selected_route_adventure\").mainWorkMarkers.size() == 4);\n"
        "    assert(exploration.get(\"selected_route_adventure\").forestBellMarkers.size() == 4);\n"
        "    assert(exploration.get(\"selected_route_adventure\").jumpMarkers.size() == 5);\n"
        "    assert(exploration.get(\"selected_route_adventure\").routeHintFirstSeconds == 18.0f);\n"
        "    assert(exploration.get(\"selected_route_adventure\").routeHintSecondSeconds == 36.0f);\n"
        "    assert(exploration.get(\"transport_wheel_recovery\").rule == px::ExplorationRuleKind::SwapRelay);\n"
        "    assert(exploration.get(\"transport_wheel_recovery\").relayMarkers.size() == 2);\n"
        "    {\n"
        "        px::SaveData checkpoint;\n"
        "        checkpoint.story.chapterId = \"rrvvfo_ch1\";\n"
        "        checkpoint.story.sceneIndex = 10;\n"
        "        checkpoint.story.checkpointId = ch1.openingFlow[10].checkpointId;\n"
        "        checkpoint.story.flags = {\"ch1_route_progress=2\"};\n"
        "        checkpoint.world.mapId = ch1.primaryMap;\n"
        "        checkpoint.world.routeChoice = \"forest\";\n"
        "        checkpoint.world.position = {250.0f, -350.0f};\n"
        "        px::RuntimeSession restarted(chapters, maps, cutscenes, dialogue, exploration, training, adventures);\n"
        "        restarted.loadSnapshot(checkpoint);\n"
        "        const auto restartPoint = restarted.view().playerPosition;\n"
        "        px::InputState checkpointInput;\n"
        "        moveToward(restarted, checkpointInput, {410.0f, -350.0f}, 30.0f, 80);\n"
        "        assert(px::distance(restarted.view().playerPosition, restartPoint) > 20.0f);\n"
        "        restarted.resetCurrentScene();\n"
        "        assert(px::distance(restarted.view().playerPosition, restartPoint) < 0.01f);\n"
        "        assert(restarted.view().objective.find(\"BELLS 2 / 4\") != std::string::npos);\n"
        "    }\n",
        "core tests Omega route counts/checkpoint restart",
    )
    write_if_changed(path, text, check_only, changes)



def patch_runtime_pause_polish(root: Path, check_only: bool, changes: list[str]) -> None:
    path = root / "src/core/runtime.cpp"
    text = load(path)
    old = '''        if (standaloneMode()) {
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
'''
    new = '''        if (standaloneMode()) {
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
'''
    text = replace_once(text, old, new, "runtime.cpp replay-safe standalone pause")

    text = replace_once(
        text,
        "    constexpr std::size_t settingCount = 7;\n",
        "    constexpr std::size_t settingCount = 8;\n",
        "runtime.cpp settings count",
    )
    text = replace_once(
        text,
        "        case 6: qolSettings_.largerText = !qolSettings_.largerText; break;\n"
        "        default: break;\n",
        "        case 6: qolSettings_.largerText = !qolSettings_.largerText; break;\n"
        "        case 7:\n"
        "            qolSettings_.combatMessages = qolSettings_.combatMessages == \"full\" ? \"important\" :\n"
        "                                          qolSettings_.combatMessages == \"important\" ? \"off\" : \"full\";\n"
        "            break;\n"
        "        default: break;\n",
        "runtime.cpp combat message setting",
    )

    old_view = '''        if (standaloneMode()) {
            view_.pauseSections = {standaloneFightMode_ ? "FIGHT • RRVVFO VS CPU" : "TRAINING • SAGE'S CHALLENGE"};
            view_.pauseOptions = {
                "RESUME", standaloneFightMode_ ? "RESTART FIGHT" : "RESTART TRAINING", "RETURN TO TITLE"
            };
        } else {
'''
    new_view = '''        if (standaloneMode()) {
            const std::string sessionName = replayMode_ ? "CHAPTER REPLAY • STORY SAVE PROTECTED" :
                                            standaloneFightMode_ ? "FIGHT • RRVVFO VS CPU" : "TRAINING • SAGE'S CHALLENGE";
            const std::string restartName = replayMode_ ? "RESTART CHAPTER REPLAY" :
                                            standaloneFightMode_ ? "RESTART FIGHT" : "RESTART TRAINING";
            view_.pauseSections = {sessionName};
            view_.pauseOptions = {"RESUME", restartName, "ACCESSIBILITY & QOL", "RETURN TO TITLE"};
        } else {
'''
    text = replace_once(text, old_view, new_view, "runtime.cpp replay pause labels")
    text = replace_once(
        text,
        "            \"HIGH-CONTRAST HUD • \" + std::string(onOff(qolSettings_.highContrastHud)),\n"
        "            \"LARGER TEXT • \" + std::string(onOff(qolSettings_.largerText))\n",
        "            \"HIGH-CONTRAST HUD • \" + std::string(onOff(qolSettings_.highContrastHud)),\n"
        "            \"LARGER TEXT • \" + std::string(onOff(qolSettings_.largerText)),\n"
        "            \"COMBAT MESSAGES • \" + std::string(qolSettings_.combatMessages == \"full\" ? \"FULL\" :\n"
        "                                                   qolSettings_.combatMessages == \"important\" ? \"IMPORTANT\" : \"OFF\")\n",
        "runtime.cpp combat message option UI",
    )
    write_if_changed(path, text, check_only, changes)



def patch_linux_compat(root: Path, check_only: bool, changes: list[str]) -> None:
    path = root / "src/platform/linux/sdl_compat.hpp"
    text = load(path)
    text = replace_once(
        text,
        "struct SDL_ControllerDeviceEvent {\n"
        "    Uint32 type;\n"
        "    Uint32 timestamp;\n"
        "    Sint32 which;\n"
        "};\n"
        "union SDL_Event {\n",
        "struct SDL_ControllerDeviceEvent {\n"
        "    Uint32 type;\n"
        "    Uint32 timestamp;\n"
        "    Sint32 which;\n"
        "};\n"
        "struct SDL_MouseButtonEvent {\n"
        "    Uint32 type;\n"
        "    Uint32 timestamp;\n"
        "    Uint32 windowID;\n"
        "    Uint32 which;\n"
        "    Uint8 button;\n"
        "    Uint8 state;\n"
        "    Uint8 clicks;\n"
        "    Uint8 padding1;\n"
        "    Sint32 x;\n"
        "    Sint32 y;\n"
        "};\n"
        "union SDL_Event {\n",
        "sdl_compat mouse struct",
    )
    text = replace_once(
        text,
        "    SDL_ControllerDeviceEvent cdevice;\n"
        "    Uint8 padding[56];\n",
        "    SDL_ControllerDeviceEvent cdevice;\n"
        "    SDL_MouseButtonEvent button;\n"
        "    Uint8 padding[56];\n",
        "sdl_compat mouse event union",
    )
    text = replace_once(
        text,
        "constexpr Uint32 SDL_KEYUP = 0x301u;\n",
        "constexpr Uint32 SDL_KEYUP = 0x301u;\n"
        "constexpr Uint32 SDL_MOUSEBUTTONDOWN = 0x401u;\n"
        "constexpr Uint32 SDL_MOUSEBUTTONUP = 0x402u;\n",
        "sdl_compat mouse event constants",
    )
    text = replace_once(
        text,
        "constexpr Sint32 SDLK_UP = 1073741906;\n",
        "constexpr Sint32 SDLK_UP = 1073741906;\n"
        "constexpr Sint32 SDLK_LSHIFT = 1073742049;\n"
        "constexpr Sint32 SDLK_RSHIFT = 1073742053;\n"
        "constexpr Uint8 SDL_BUTTON_LEFT = 1;\n"
        "constexpr Uint8 SDL_BUTTON_RIGHT = 3;\n",
        "sdl_compat shift/mouse button constants",
    )
    write_if_changed(path, text, check_only, changes)


def patch_linux_main(root: Path, check_only: bool, changes: list[str]) -> None:
    path = root / "src/platform/linux/main.cpp"
    text = load(path)
    old_save_io = r'''px::SaveData loadDevelopmentSave(const std::filesystem::path& path, bool allowBackup = true) {
    std::ifstream input(path);
    if (!input) return allowBackup ? loadDevelopmentSave(path.string() + ".bak", false) : px::SaveData{};
    const std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    try { return px::SaveCodec::deserialize(text); }
    catch (const std::exception& error) {
        std::cerr << "Linux development save ignored: " << error.what() << '\n';
        if (allowBackup) return loadDevelopmentSave(path.string() + ".bak", false);
        return {};
    }
}

bool writeDevelopmentSave(const std::filesystem::path& path, const px::SaveData& save) {
    try {
        std::filesystem::create_directories(path.parent_path());
        const auto temporary = path.string() + ".tmp";
        const auto backup = path.string() + ".bak";
        if (std::filesystem::exists(path))
            std::filesystem::copy_file(path, backup, std::filesystem::copy_options::overwrite_existing);
        {
            std::ofstream output(temporary, std::ios::trunc);
            output << px::SaveCodec::serialize(save);
            if (!output) return false;
        }
        std::filesystem::rename(temporary, path);
        return true;
    } catch (const std::exception& error) {
        std::cerr << "Could not write Linux development save: " << error.what() << '\n';
        return false;
    }
}
'''
    new_save_io = r'''bool validSaveLocation(const px::SaveData& save) {
    if (save.story.chapterId.empty()) return true;
    px::ChapterRegistry chapters;
    if (!chapters.has(save.story.chapterId)) return false;
    const auto& chapter = chapters.get(save.story.chapterId);
    if (!save.story.checkpointId.empty()) {
        const auto checkpoint = std::find_if(chapter.openingFlow.begin(), chapter.openingFlow.end(), [&](const px::SceneStep& step) {
            return step.checkpointId == save.story.checkpointId;
        });
        if (checkpoint != chapter.openingFlow.end()) return true;
    }
    return save.story.sceneIndex < chapter.openingFlow.size();
}

bool readValidatedSave(const std::filesystem::path& path, px::SaveData& output) {
    std::ifstream input(path);
    if (!input) return false;
    const std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    try {
        output = px::SaveCodec::deserialize(text);
        return validSaveLocation(output);
    } catch (const std::exception& error) {
        std::cerr << "Linux save ignored: " << error.what() << '\n';
        return false;
    }
}

px::SaveData loadDevelopmentSave(const std::filesystem::path& path, bool allowBackup = true) {
    px::SaveData output;
    if (readValidatedSave(path, output)) return output;
    if (allowBackup && readValidatedSave(path.string() + ".bak", output)) return output;
    return {};
}

bool writeDevelopmentSave(const std::filesystem::path& path, const px::SaveData& save) {
    try {
        std::filesystem::create_directories(path.parent_path());
        const auto temporary = path.string() + ".tmp";
        const auto backup = path.string() + ".bak";
        px::SaveData validPrevious;
        if (readValidatedSave(path, validPrevious)) {
            std::ofstream backupOutput(backup, std::ios::trunc);
            backupOutput << px::SaveCodec::serialize(validPrevious);
            if (!backupOutput) return false;
        }
        {
            std::ofstream output(temporary, std::ios::trunc);
            output << px::SaveCodec::serialize(save);
            if (!output) return false;
        }
        if (std::filesystem::exists(path)) std::filesystem::remove(path);
        std::filesystem::rename(temporary, path);
        return true;
    } catch (const std::exception& error) {
        std::cerr << "Could not write Linux save: " << error.what() << '\n';
        return false;
    }
}
'''
    text = replace_once(text, old_save_io, new_save_io, "linux validated backup recovery")
    text = replace_once(
        text,
        "namespace {\n\nstruct Arguments {\n"
        "    std::string review;\n"
        "    std::string screenshot;\n"
        "    std::string saveDirectory{\"dev-saves/linux\"};\n",
        "namespace {\n\n"
        "std::string defaultSaveDirectory() {\n"
        "    if (const char* xdg = std::getenv(\"XDG_DATA_HOME\"))\n"
        "        return (std::filesystem::path(xdg) / \"ParallelsX/ClashOfSouls\").string();\n"
        "    if (const char* home = std::getenv(\"HOME\"))\n"
        "        return (std::filesystem::path(home) / \".local/share/ParallelsX/ClashOfSouls\").string();\n"
        "    return \"dev-saves/linux\";\n"
        "}\n\n"
        "struct Arguments {\n"
        "    std::string review;\n"
        "    std::string screenshot;\n"
        "    std::string saveDirectory{defaultSaveDirectory()};\n",
        "linux main production save directory",
    )
    text = replace_once(
        text,
        "Arguments parseArguments(int argc, char** argv) {\n"
        "    Arguments args;\n"
        "    args.assetRoot = std::filesystem::absolute(argv[0]).parent_path().parent_path().string();\n",
        "Arguments parseArguments(int argc, char** argv) {\n"
        "    Arguments args;\n"
        "    const auto executableDir = std::filesystem::absolute(argv[0]).parent_path();\n"
        "    args.assetRoot = std::filesystem::exists(executableDir / \"assets\")\n"
        "        ? executableDir.string() : executableDir.parent_path().string();\n",
        "linux main packaged asset root",
    )
    text = replace_once(
        text,
        "        case SDLK_SPACE: return px::Action::Jump;\n",
        "        case SDLK_SPACE: return px::Action::Jump;\n"
        "        case SDLK_LSHIFT: case SDLK_RSHIFT: return px::Action::Dash;\n",
        "linux main shift dash",
    )
    text = replace_once(
        text,
        "          runtime(chapters, maps, cutscenes, dialogue, exploration, training, adventures),\n"
        "          menu(menus, routes, recap, save) {\n"
        "        const auto& rrvvfo = characters.get(\"rrvvfo\");\n",
        "          runtime(chapters, maps, cutscenes, dialogue, exploration, training, adventures),\n"
        "          menu(menus, routes, recap, save) {\n"
        "        menu.setReducedMotion(save.qol.reducedMotion);\n"
        "        const auto& rrvvfo = characters.get(\"rrvvfo\");\n",
        "linux menu reduced motion",
    )
    old_menu = '''    if (outcome == px::MenuOutcome::BeginStory || outcome == px::MenuOutcome::ReplayChapter) {
        state.runtime.startChapter("rrvvfo_ch1");
        state.gameplay = true;
    } else if (outcome == px::MenuOutcome::ContinueStory) {
'''
    new_menu = '''    if (outcome == px::MenuOutcome::BeginStory) {
        state.runtime.startChapter("rrvvfo_ch1");
        state.runtime.setQolSettings(state.save.qol);
        state.gameplay = true;
    } else if (outcome == px::MenuOutcome::ReplayChapter) {
        state.runtime.startReplayChapter(state.save);
        state.gameplay = true;
    } else if (outcome == px::MenuOutcome::ContinueStory) {
'''
    text = replace_once(text, old_menu, new_menu, "linux main isolated replay")
    text = replace_once(
        text,
        "        if (mode == px::MenuModeId::ArenaBattle) state.runtime.startCpuFight();\n"
        "        else if (mode == px::MenuModeId::Training) state.runtime.startStandaloneTraining();\n"
        "        state.gameplay = true;\n",
        "        if (mode == px::MenuModeId::ArenaBattle) state.runtime.startCpuFight();\n"
        "        else if (mode == px::MenuModeId::Training) state.runtime.startStandaloneTraining();\n"
        "        state.runtime.setQolSettings(state.save.qol);\n"
        "        state.gameplay = true;\n",
        "linux main standalone QoL",
    )
    # Mouse gameplay mapping must be handled before keyboard/controller down/up filter.
    text = replace_once(
        text,
        "            const bool down = event.type == SDL_KEYDOWN || event.type == SDL_CONTROLLERBUTTONDOWN;\n"
        "            const bool up = event.type == SDL_KEYUP || event.type == SDL_CONTROLLERBUTTONUP;\n"
        "            if (!down && !up) continue;\n",
        "            if (state.gameplay && (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)) {\n"
        "                const bool held = event.type == SDL_MOUSEBUTTONDOWN;\n"
        "                if (event.button.button == SDL_BUTTON_LEFT) state.input.set(px::Action::Light, held);\n"
        "                else if (event.button.button == SDL_BUTTON_RIGHT) state.input.set(px::Action::Block, held);\n"
        "                continue;\n"
        "            }\n"
        "            const bool down = event.type == SDL_KEYDOWN || event.type == SDL_CONTROLLERBUTTONDOWN;\n"
        "            const bool up = event.type == SDL_KEYUP || event.type == SDL_CONTROLLERBUTTONUP;\n"
        "            if (!down && !up) continue;\n",
        "linux main mouse gameplay",
    )
    # Give schema-5 save a new filename while retaining v4/v0.4G migration.
    text = replace_once(
        text,
        'std::filesystem::path(arguments.saveDirectory) / "ParallelsX-0.4H-GOLDEN-GATE-QOL-linux-dev.save";',
        'std::filesystem::path(arguments.saveDirectory) / "ParallelsX-Omega-save-v5.txt";',
        "linux main schema5 filename",
    )
    text = replace_once(
        text,
        'const std::filesystem::path legacySavePath =\n        std::filesystem::path(arguments.saveDirectory) / "ParallelsX-0.4G-GOLD-linux-dev.save";\n'
        '    const bool currentSaveExists = std::filesystem::exists(savePath) || std::filesystem::exists(savePath.string() + ".bak");\n'
        '    ApplicationState state(reviewRun ? px::SaveData{} : loadDevelopmentSave(currentSaveExists ? savePath : legacySavePath), arguments.assetRoot);',
        'const std::filesystem::path legacySavePath =\n        std::filesystem::path(arguments.saveDirectory) / "ParallelsX-0.4H-GOLDEN-GATE-QOL-linux-dev.save";\n'
        '    const std::filesystem::path olderLegacySavePath =\n        std::filesystem::path(arguments.saveDirectory) / "ParallelsX-0.4G-GOLD-linux-dev.save";\n'
        '    const bool currentSaveExists = std::filesystem::exists(savePath) || std::filesystem::exists(savePath.string() + ".bak");\n'
        '    const bool legacySaveExists = std::filesystem::exists(legacySavePath) || std::filesystem::exists(legacySavePath.string() + ".bak");\n'
        '    const auto selectedSavePath = currentSaveExists ? savePath : (legacySaveExists ? legacySavePath : olderLegacySavePath);\n'
        '    ApplicationState state(reviewRun ? px::SaveData{} : loadDevelopmentSave(selectedSavePath), arguments.assetRoot);',
        "linux main save migration chain",
    )
    write_if_changed(path, text, check_only, changes)


def patch_macos_main(root: Path, check_only: bool, changes: list[str]) -> None:
    path = root / "src/platform/macos/main.mm"
    text = load(path)
    old_read = '''static bool readMacSave(NSString* path, px::SaveData& out) {
    NSError* error=nil;
    NSString* encoded=[NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:&error];
    if(!encoded)return false;
    try{out=px::SaveCodec::deserialize(std::string(encoded.UTF8String?:""));return true;}
    catch(const std::exception& exception){NSLog(@"Ignoring unreadable Parallels X save at %@: %s",path,exception.what());return false;}
}
'''
    new_read = '''static bool validMacSaveLocation(const px::SaveData& save) {
    if(save.story.chapterId.empty())return true;
    px::ChapterRegistry chapters;
    if(!chapters.has(save.story.chapterId))return false;
    const auto& chapter=chapters.get(save.story.chapterId);
    if(!save.story.checkpointId.empty()){
        const auto checkpoint=std::find_if(chapter.openingFlow.begin(),chapter.openingFlow.end(),[&](const px::SceneStep& step){return step.checkpointId==save.story.checkpointId;});
        if(checkpoint!=chapter.openingFlow.end())return true;
    }
    return save.story.sceneIndex<chapter.openingFlow.size();
}

static bool readMacSave(NSString* path, px::SaveData& out) {
    NSError* error=nil;
    NSString* encoded=[NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:&error];
    if(!encoded)return false;
    try{
        out=px::SaveCodec::deserialize(std::string(encoded.UTF8String?:""));
        return validMacSaveLocation(out);
    }
    catch(const std::exception& exception){NSLog(@"Ignoring unreadable Parallels X save at %@: %s",path,exception.what());return false;}
}
'''
    text = replace_once(text, old_read, new_read, "mac semantic save validation")
    text = replace_once(
        text,
        '    if([NSFileManager.defaultManager fileExistsAtPath:path]){\n'
        '        [NSFileManager.defaultManager removeItemAtPath:backup error:nil];\n'
        '        [NSFileManager.defaultManager copyItemAtPath:path toPath:backup error:nil];\n'
        '    }\n',
        '    px::SaveData validPrevious;\n'
        '    if([NSFileManager.defaultManager fileExistsAtPath:path]&&readMacSave(path,validPrevious)){\n'
        '        [NSFileManager.defaultManager removeItemAtPath:backup error:nil];\n'
        '        [NSFileManager.defaultManager copyItemAtPath:path toPath:backup error:nil];\n'
        '    }\n',
        "mac preserve only validated backup",
    )
    text = replace_once(text, '@"save-v4.txt"]', '@"save-v5.txt"]', "mac schema5 save filename")
    text = replace_once(
        text,
        'static NSString* legacyMacSavePath() {\n'
        '    NSString* support=NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory,NSUserDomainMask,YES).firstObject;\n'
        '    return [[support stringByAppendingPathComponent:@"ParallelsX/ClashOfSouls"] stringByAppendingPathComponent:@"save-v3.txt"];\n'
        '}\n',
        'static NSString* legacyMacSavePath() {\n'
        '    NSString* support=NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory,NSUserDomainMask,YES).firstObject;\n'
        '    return [[support stringByAppendingPathComponent:@"ParallelsX/ClashOfSouls"] stringByAppendingPathComponent:@"save-v4.txt"];\n'
        '}\n\n'
        'static NSString* olderLegacyMacSavePath() {\n'
        '    NSString* support=NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory,NSUserDomainMask,YES).firstObject;\n'
        '    return [[support stringByAppendingPathComponent:@"ParallelsX/ClashOfSouls"] stringByAppendingPathComponent:@"save-v3.txt"];\n'
        '}\n',
        "mac save migration paths",
    )
    text = replace_once(
        text,
        '    path=legacyMacSavePath();\n'
        '    if(readMacSave(path,save))return save;\n'
        '    if(readMacSave([path stringByAppendingString:@".bak"],save))return save;\n'
        '    return {};\n',
        '    path=legacyMacSavePath();\n'
        '    if(readMacSave(path,save))return save;\n'
        '    if(readMacSave([path stringByAppendingString:@".bak"],save))return save;\n'
        '    path=olderLegacyMacSavePath();\n'
        '    if(readMacSave(path,save))return save;\n'
        '    if(readMacSave([path stringByAppendingString:@".bak"],save))return save;\n'
        '    return {};\n',
        "mac load migration chain",
    )
    old = '        if(outcome==px::MenuOutcome::BeginStory||outcome==px::MenuOutcome::ReplayChapter)session.startChapter("rrvvfo_ch1");\n'
    new = ('        if(outcome==px::MenuOutcome::BeginStory){session.startChapter("rrvvfo_ch1");session.setQolSettings(save.qol);}\n'
           '        else if(outcome==px::MenuOutcome::ReplayChapter)session.startReplayChapter(save);\n')
    text = replace_once(text, old, new, "mac isolated replay")
    text = replace_once(
        text,
        '            if(mode==px::MenuModeId::ArenaBattle)session.startCpuFight();\n'
        '            else if(mode==px::MenuModeId::Training)session.startStandaloneTraining();\n'
        '        }\n',
        '            if(mode==px::MenuModeId::ArenaBattle)session.startCpuFight();\n'
        '            else if(mode==px::MenuModeId::Training)session.startStandaloneTraining();\n'
        '            session.setQolSettings(save.qol);\n'
        '        }\n',
        "mac standalone QoL",
    )
    # Front-end motion preference should use the persisted value immediately.
    text = replace_once(
        text,
        '          menu(menuRegistry,storyRoutes,storyRecap,save) {\n'
        '        const auto& rrvvfo=characterPresentation.get("rrvvfo");\n',
        '          menu(menuRegistry,storyRoutes,storyRecap,save) {\n'
        '        menu.setReducedMotion(save.qol.reducedMotion);\n'
        '        const auto& rrvvfo=characterPresentation.get("rrvvfo");\n',
        "mac menu reduced motion",
    )
    write_if_changed(path, text, check_only, changes)


def patch_3ds_main(root: Path, check_only: bool, changes: list[str]) -> None:
    path = root / "src/platform/3ds/main.cpp"
    text = load(path)
    old_load_at = '''bool loadSaveAt(const char* path, px::SaveData& output) {
    std::string text;
    if (!readText(path, text)) return false;
    try {
        output = px::SaveCodec::deserialize(text);
        return output.story.chapterId.empty() || output.story.chapterId == "rrvvfo_ch1";
    } catch (...) {
        return false;
    }
}
'''
    new_load_at = '''bool validSaveLocation(const px::SaveData& save) {
    if (save.story.chapterId.empty()) return true;
    px::ChapterRegistry chapters;
    if (!chapters.has(save.story.chapterId)) return false;
    const auto& chapter = chapters.get(save.story.chapterId);
    if (!save.story.checkpointId.empty()) {
        const auto checkpoint = std::find_if(chapter.openingFlow.begin(), chapter.openingFlow.end(), [&](const px::SceneStep& step) {
            return step.checkpointId == save.story.checkpointId;
        });
        if (checkpoint != chapter.openingFlow.end()) return true;
    }
    return save.story.sceneIndex < chapter.openingFlow.size();
}

bool loadSaveAt(const char* path, px::SaveData& output) {
    std::string text;
    if (!readText(path, text)) return false;
    try {
        output = px::SaveCodec::deserialize(text);
        return validSaveLocation(output);
    } catch (...) {
        return false;
    }
}
'''
    text = replace_once(text, old_load_at, new_load_at, "3ds semantic save validation")
    text = replace_once(
        text,
        '    std::string previous;\n'
        '    if (readText(kSavePath, previous)) writeText(kBackupSavePath, previous);\n',
        '    std::string previous;\n'
        '    px::SaveData validPrevious;\n'
        '    if (loadSaveAt(kSavePath, validPrevious) && readText(kSavePath, previous)) writeText(kBackupSavePath, previous);\n',
        "3ds preserve only validated backup",
    )
    text = replace_once(text, 'constexpr const char* kSavePath = "sdmc:/3ds/ParallelsX/save-v4.txt";\n',
                        'constexpr const char* kSavePath = "sdmc:/3ds/ParallelsX/save-v5.txt";\n',
                        "3ds schema5 save path")
    text = replace_once(text, 'constexpr const char* kBackupSavePath = "sdmc:/3ds/ParallelsX/save-v4.txt.bak";\n',
                        'constexpr const char* kBackupSavePath = "sdmc:/3ds/ParallelsX/save-v5.txt.bak";\n',
                        "3ds schema5 backup path")
    text = replace_once(text, 'constexpr const char* kTemporarySavePath = "sdmc:/3ds/ParallelsX/save-v4.txt.tmp";\n',
                        'constexpr const char* kTemporarySavePath = "sdmc:/3ds/ParallelsX/save-v5.txt.tmp";\n'
                        'constexpr const char* kLegacyQolSavePath = "sdmc:/3ds/ParallelsX/save-v4.txt";\n',
                        "3ds schema5 temp/migration path")
    text = replace_once(
        text,
        "bool loadSave(px::SaveData& output) {\n"
        "    if (loadSaveAt(kSavePath, output)) return true;\n"
        "    if (loadSaveAt(kBackupSavePath, output)) return true;\n"
        "    return loadSaveAt(kLegacyGateSavePath, output);\n"
        "}\n",
        "bool loadSave(px::SaveData& output) {\n"
        "    if (loadSaveAt(kSavePath, output)) return true;\n"
        "    if (loadSaveAt(kBackupSavePath, output)) return true;\n"
        "    if (loadSaveAt(kLegacyQolSavePath, output)) return true;\n"
        "    return loadSaveAt(kLegacyGateSavePath, output);\n"
        "}\n",
        "3ds load schema migration chain",
    )
    old = '''    if (outcome == px::MenuOutcome::BeginStory || outcome == px::MenuOutcome::ReplayChapter)
        session.startChapter("rrvvfo_ch1");
    else if (outcome == px::MenuOutcome::ContinueStory) {
'''
    new = '''    if (outcome == px::MenuOutcome::BeginStory) {
        session.startChapter("rrvvfo_ch1");
        session.setQolSettings(save.qol);
    } else if (outcome == px::MenuOutcome::ReplayChapter)
        session.startReplayChapter(save);
    else if (outcome == px::MenuOutcome::ContinueStory) {
'''
    text = replace_once(text, old, new, "3ds isolated replay")
    text = replace_once(
        text,
        "        if (mode == px::MenuModeId::ArenaBattle) session.startCpuFight();\n"
        "        else if (mode == px::MenuModeId::Training) session.startStandaloneTraining();\n"
        "    }\n",
        "        if (mode == px::MenuModeId::ArenaBattle) session.startCpuFight();\n"
        "        else if (mode == px::MenuModeId::Training) session.startStandaloneTraining();\n"
        "        session.setQolSettings(save.qol);\n"
        "    }\n",
        "3ds standalone QoL",
    )
    text = replace_once(
        text,
        "    px::MenuState menu(menuRegistry, storyRoutes, storyRecap, save);\n",
        "    px::MenuState menu(menuRegistry, storyRoutes, storyRecap, save);\n"
        "    menu.setReducedMotion(save.qol.reducedMotion);\n",
        "3ds menu reduced motion",
    )
    write_if_changed(path, text, check_only, changes)


def patch_3ds_build_sync(root: Path, check_only: bool, changes: list[str]) -> None:
    path = root / "scripts/build-3ds.sh"
    text = load(path)
    text = replace_once(
        text,
        'make -C "$ROOT/src/platform/3ds" clean all\n',
        'mkdir -p "$ROOT/src/platform/3ds/romfs/assets/characters/rrvvfo"\n'
        'cp "$ROOT/assets/characters/rrvvfo/rrvvfo-dev.pxskel" "$ROOT/src/platform/3ds/romfs/assets/characters/rrvvfo/rrvvfo-dev.pxskel"\n'
        'make -C "$ROOT/src/platform/3ds" clean all\n',
        "3ds build sync shared Rrvvfo cooked asset",
    )
    write_if_changed(path, text, check_only, changes)


def patch_linux_workflow(root: Path, check_only: bool, changes: list[str]) -> None:
    path = root / ".github/workflows/linux-build.yml"
    text = load(path)
    text = replace_once(
        text,
        "          mkdir -p linux-package\n"
        "          cp build-linux/ParallelsX linux-package/ParallelsX\n"
        "          cp LINUX_BUILD_README.md linux-package/README.md\n"
        "          tar -C linux-package -czf Parallels-X-Clash-of-Souls-3.0R-Linux-x86_64.tar.gz .\n",
        "          mkdir -p linux-package/assets/characters/rrvvfo\n"
        "          cp build-linux/ParallelsX linux-package/ParallelsX\n"
        "          cp assets/characters/rrvvfo/rrvvfo-dev.pxskel linux-package/assets/characters/rrvvfo/rrvvfo-dev.pxskel\n"
        "          cp LINUX_BUILD_README.md linux-package/README.md\n"
        "          test -x linux-package/ParallelsX\n"
        "          test -s linux-package/assets/characters/rrvvfo/rrvvfo-dev.pxskel\n"
        "          tar -C linux-package -czf Parallels-X-Clash-of-Souls-3.0R-Linux-x86_64.tar.gz .\n",
        "linux workflow package required model",
    )
    write_if_changed(path, text, check_only, changes)


def patch_core_tests_milestone_a(root: Path, check_only: bool, changes: list[str]) -> None:
    path = root / "tests/core_tests.cpp"
    text = load(path)
    text = replace_once(
        text,
        '    assert(runtime.view().playerAnimation == "run" && runtime.view().playerAnimationSpeed < 0.0f);\n',
        '    assert(runtime.view().playerAnimation == "combat_retreat" && runtime.view().playerAnimationSpeed > 0.0f);\n',
        "core tests dedicated combat retreat",
    )
    text = replace_once(
        text,
        '    assert(hotbar.size() == 3 && hotbar[1].id == "objectSwap" && hotbar[1].energyCost == 20.0f);\n',
        '    assert(hotbar.size() == 3 && hotbar[0].id == "fireBlast" && hotbar[0].energyCost == 22.0f);\n'
        '    assert(hotbar[1].id == "objectSwap" && hotbar[1].energyCost == 20.0f);\n'
        '    const auto& storyFireBlast = px::CombatSystem::attackFor("rrvvfo", px::AttackKind::Projectile);\n'
        '    assert(storyFireBlast.damage == 15.0f && storyFireBlast.guardDamage == 9.0f);\n',
        "core tests Fire Blast parity",
    )
    # Add schema-4 checkpoint migration regression before the Chapter 2 foundation comment.
    marker = "    // Shared Chapter 2–4 foundation: Legacy progression math, protected card visibility and data-only quests.\n"
    migration_test = r'''    // Omega save migration: a stable checkpoint ID wins over a stale numeric index.
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

'''
    text = replace_once(text, marker, migration_test + marker, "core tests schema checkpoint migration")
    write_if_changed(path, text, check_only, changes)


def patch_animation_tests(root: Path, check_only: bool, changes: list[str]) -> None:
    path = root / "tests/skeletal_animation_tests.cpp"
    text = load(path)
    text = replace_once(
        text,
        '        {"idle", .750f}, {"fighting_stance", .420f}, {"run", .328f}, {"dash", .240f},\n'
        '        {"jump_start", .150f}, {"fall", .220f}, {"land", .130f},\n',
        '        {"idle", .750f}, {"fighting_stance", .420f}, {"combat_ready", .300f}, {"combat_relax", .280f},\n'
        '        {"run", .328f}, {"combat_advance", .320f}, {"combat_retreat", .340f}, {"dash", .240f},\n'
        '        {"jump_start", .150f}, {"fall", .220f}, {"land", .130f}, {"hard_land", .220f},\n',
        "animation tests Omega locomotion/transition clips",
    )
    write_if_changed(path, text, check_only, changes)

def copy_animation_author(root: Path, package_root: Path, check_only: bool, changes: list[str]) -> None:
    src = package_root / "replacement_files/scripts/author-rrvvfo-core-animations.py"
    dst = root / "scripts/author-rrvvfo-core-animations.py"
    if not src.is_file():
        raise PatchError(f"Omega replacement missing: {src}")
    current = load(dst)
    replacement = src.read_text(encoding="utf-8")
    # Require the known 0.4H.5 authoring marker before replacement.
    if "Legacy-timed core animation set" not in current and "RRVVFO" not in current.upper():
        raise PatchError("animation authoring script no longer resembles the audited 0.4H.5 source")
    if current != replacement:
        changes.append(str(dst))
        if not check_only:
            shutil.copy2(src, dst)
            os.chmod(dst, 0o755)


def verify_baseline(root: Path) -> None:
    required = [
        "src/core/runtime.cpp",
        "src/core/runtime.hpp",
        "src/core/save.cpp",
        "src/core/save.hpp",
        "src/core/game.cpp",
        "src/core/ability_hotbar.cpp",
        "src/core/combat.cpp",
        "src/platform/macos/main.mm",
        "src/platform/linux/main.cpp",
        "src/platform/linux/sdl_compat.hpp",
        "src/platform/3ds/main.cpp",
        "scripts/build-3ds.sh",
        "scripts/author-rrvvfo-core-animations.py",
        "tests/core_tests.cpp",
        "tests/skeletal_animation_tests.cpp",
        ".github/workflows/linux-build.yml",
    ]
    missing = [p for p in required if not (root / p).is_file()]
    if missing:
        raise PatchError("not a complete audited source checkout; missing: " + ", ".join(missing))
    handoff = root / "EVERYTHING_A_NEW_CHAT_NEEDS_TO_KNOW.md"
    if handoff.is_file():
        text = handoff.read_text(encoding="utf-8", errors="replace")
        if "0.4H.5" not in text:
            raise PatchError("handoff file does not identify the 0.4H.5 candidate baseline")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("repo", type=Path, help="path to the 0.4H.5-candidate checkout")
    parser.add_argument("--check-only", action="store_true", help="verify anchors and report files without writing")
    args = parser.parse_args()
    root = args.repo.expanduser().resolve()
    package_root = Path(__file__).resolve().parent
    try:
        verify_baseline(root)
        changes: list[str] = []
        copy_animation_author(root, package_root, args.check_only, changes)
        patch_runtime_hpp(root, args.check_only, changes)
        patch_runtime_cpp(root, args.check_only, changes)
        patch_ability_hotbar(root, args.check_only, changes)
        patch_combat(root, args.check_only, changes)
        patch_save_schema(root, args.check_only, changes)
        patch_exploration_content(root, args.check_only, changes)
        patch_map_for_southern_detour(root, args.check_only, changes)
        patch_runtime_routes(root, args.check_only, changes)
        patch_runtime_pause_polish(root, args.check_only, changes)
        patch_linux_compat(root, args.check_only, changes)
        patch_linux_main(root, args.check_only, changes)
        patch_macos_main(root, args.check_only, changes)
        patch_3ds_main(root, args.check_only, changes)
        patch_3ds_build_sync(root, args.check_only, changes)
        patch_linux_workflow(root, args.check_only, changes)
        patch_core_tests_milestone_a(root, args.check_only, changes)
        patch_core_tests_milestone_b(root, args.check_only, changes)
        patch_animation_tests(root, args.check_only, changes)
    except PatchError as exc:
        print(f"OMEGA PATCH REFUSED: {exc}", file=sys.stderr)
        return 2

    mode = "CHECK" if args.check_only else "APPLIED"
    print(f"Omega source overlay {mode} against {BASE_TAG} ({BASE_COMMIT[:12]}…)")
    if changes:
        for item in changes:
            try:
                print(" -", Path(item).relative_to(root))
            except ValueError:
                print(" -", item)
    else:
        print(" - no changes needed (already applied or identical)")
    if not args.check_only:
        print("NEXT: regenerate Rrvvfo animation JSON/PXSKEL, then run ./scripts/test.sh and all three platform builds.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
