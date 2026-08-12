#!/usr/bin/env python3
from __future__ import annotations

import argparse
import pathlib
import re
import subprocess
import sys
from textwrap import dedent

BASE_SHA = "d84d6e944c6b90822da461b33e418f57b504fe43"
BATCH_ID = "u7-u10-20260811"


def run(cmd, cwd):
    return subprocess.run(cmd, cwd=cwd, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)


def read(path: pathlib.Path) -> str:
    return path.read_text(encoding="utf-8")


def write(path: pathlib.Path, content: str):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def ensure_contains(path: pathlib.Path, needle: str, label: str):
    text = read(path)
    if not contains_semantic(text, needle):
        raise RuntimeError(f"Preflight failed: {label} not found in {path}")


def whitespace_pattern(value: str) -> str:
    """Match authored text while ignoring formatting-only whitespace changes."""
    pieces = re.split(r"(\s+)", value)
    return "".join(r"\s+" if piece.isspace() else re.escape(piece) for piece in pieces if piece)


def semantic_spans(text: str, value: str) -> list[tuple[int, int]]:
    if not value:
        return []
    exact = []
    offset = 0
    while True:
        start = text.find(value, offset)
        if start < 0:
            break
        exact.append((start, start + len(value)))
        offset = start + max(1, len(value))
    if exact:
        return exact
    return [match.span() for match in re.finditer(whitespace_pattern(value), text, re.MULTILINE)]


def contains_semantic(text: str, value: str) -> bool:
    return bool(semantic_spans(text, value))


def unique_semantic_span(text: str, value: str, label: str, path: pathlib.Path) -> tuple[int, int]:
    spans = semantic_spans(text, value)
    if not spans:
        raise RuntimeError(f"Semantic anchor missing for {label}: {path}")
    if len(spans) != 1:
        raise RuntimeError(f"Semantic anchor ambiguous ({len(spans)} matches) for {label}: {path}")
    if text[spans[0][0]:spans[0][1]] != value:
        print(f"  tolerant anchor: {label}")
    return spans[0]


def replace_once(path: pathlib.Path, old: str, new: str, label: str):
    text = read(path)
    if contains_semantic(text, new):
        print(f"  already: {label}")
        return
    start, end = unique_semantic_span(text, old, label, path)
    write(path, text[:start] + new + text[end:])
    print(f"  patched: {label}")


def insert_before(path: pathlib.Path, marker: str, addition: str, token: str, label: str):
    text = read(path)
    if contains_semantic(text, token):
        print(f"  already: {label}")
        return
    start, _ = unique_semantic_span(text, marker, label, path)
    write(path, text[:start] + addition + text[start:])
    print(f"  inserted: {label}")


def insert_after(path: pathlib.Path, marker: str, addition: str, token: str, label: str):
    text = read(path)
    if contains_semantic(text, token):
        print(f"  already: {label}")
        return
    _, end = unique_semantic_span(text, marker, label, path)
    write(path, text[:end] + addition + text[end:])
    print(f"  inserted: {label}")


def replace_function(path: pathlib.Path, signature: str, replacement: str, label: str):
    text = read(path)
    if contains_semantic(text, replacement.strip()):
        print(f"  already: {label}")
        return
    start, _ = unique_semantic_span(text, signature, label, path)
    brace = text.find("{", start)
    if brace < 0:
        raise RuntimeError(f"Opening brace missing for {label}")

    i = brace
    depth = 0
    state = "code"
    while i < len(text):
        c = text[i]
        n = text[i + 1] if i + 1 < len(text) else ""
        if state == "code":
            if c == '"': state = "string"
            elif c == "'": state = "char"
            elif c == "/" and n == "/": state = "line_comment"; i += 1
            elif c == "/" and n == "*": state = "block_comment"; i += 1
            elif c == "{": depth += 1
            elif c == "}":
                depth -= 1
                if depth == 0:
                    end = i + 1
                    normalize = lambda line: re.sub(r"\s+", " ", line.strip())
                    wanted = [normalize(line) for line in replacement.splitlines() if line.strip()]
                    current = [normalize(line) for line in text[start:end].splitlines() if line.strip()]
                    cursor = 0
                    for line in current:
                        if cursor < len(wanted) and line == wanted[cursor]:
                            cursor += 1
                    if cursor == len(wanted):
                        print(f"  already: {label} (extended by later milestones)")
                        return
                    write(path, text[:start] + replacement.rstrip() + text[end:])
                    print(f"  replaced: {label}")
                    return
        elif state == "string":
            if c == "\\": i += 1
            elif c == '"': state = "code"
        elif state == "char":
            if c == "\\": i += 1
            elif c == "'": state = "code"
        elif state == "line_comment":
            if c == "\n": state = "code"
        elif state == "block_comment":
            if c == "*" and n == "/": state = "code"; i += 1
        i += 1
    raise RuntimeError(f"Could not find end of function for {label}")


def mark(root: pathlib.Path, name: str):
    state = root / ".parallels_x_update_state"
    state.mkdir(exist_ok=True)
    write(state / name, BATCH_ID + "\n")


def preflight(root: pathlib.Path):
    required = [
        "CMakeLists.txt",
        "src/core/runtime.cpp",
        "src/core/runtime.hpp",
        "src/core/save.cpp",
        "src/core/save.hpp",
        "src/core/types.hpp",
        "src/content/chapter_registry.cpp",
        "src/content/cutscene_registry.hpp",
        "src/content/cutscene_registry.cpp",
        "src/content/dialogue_registry.cpp",
        "src/content/exploration_registry.hpp",
        "src/content/exploration_registry.cpp",
        "src/content/map_registry.cpp",
        "src/content/world_presentation_registry.cpp",
        "src/platform/macos/main.mm",
        "src/platform/linux/presentation_renderer.cpp",
        "src/platform/3ds/world_renderer_3ds.cpp",
    ]
    for item in required:
        if not (root / item).exists():
            raise RuntimeError(f"Not a Parallels X U6 source tree; missing {item}")
    ensure_contains(root / "src/content/chapter_registry.cpp", "collapsed_tournament_road_detour", "U6 collapse scene")
    runtime_text = read(root / "src/core/runtime.cpp")
    if "BUILD • 3.0R / UPDATE 6 GOLDEN GATE" not in runtime_text and "BUILD • 3.0R / UPDATE 10 TOURNAMENT GOLDEN" not in runtime_text:
        raise RuntimeError("Preflight failed: U6/U10 Golden Gate build label not found in src/core/runtime.cpp")
    ensure_contains(root / "src/core/runtime.cpp", "Combat retreat is now its own authored clip", "positive-time combat retreat lock")
    ensure_contains(root / "src/core/ability_hotbar.cpp", "shotsOfAgony", "future Shots catalog")

    git = run(["git", "rev-parse", "HEAD"], root)
    if git.returncode == 0:
        head = git.stdout.strip()
        ancestor = run(["git", "merge-base", "--is-ancestor", BASE_SHA, "HEAD"], root)
        if head != BASE_SHA and ancestor.returncode != 0:
            raise RuntimeError(
                "This batch targets the U2-U6 Golden Gate branch.\n"
                f"Expected {BASE_SHA} as HEAD or ancestor, got {head}."
            )
    print("Preflight PASS — U6 Golden Gate source detected.")


def apply_u7a(root: pathlib.Path):
    print("\n=== U7A — CINEMATIC FOUNDATION ===")

    cutscene_hpp = root / "src/content/cutscene_registry.hpp"
    replace_once(
        cutscene_hpp,
        "enum class CutsceneTier { FieldDialogue = 1, Directed = 2, Major = 3 };\n",
        dedent('''\
        enum class CutsceneTier { FieldDialogue = 1, Directed = 2, Major = 3 };

        // Shared story-staging language. These actions are content data consumed by the
        // same RuntimeSession on Mac, Linux and 3DS; they are not a second cutscene engine.
        enum class CutsceneActionKind {
            MoveTo,
            FaceActor,
            PlayAnimation,
            LookAt,
            CameraTrack,
            CameraFocus,
            TriggerWorldEvent,
            Wait,
            Dialogue,
            Expression
        };

        struct CutsceneAction {
            std::size_t dialogueIndex{0};
            CutsceneActionKind kind{CutsceneActionKind::Wait};
            std::string actorId;
            std::string targetActorId;
            Vec2 position{};
            float speed{150.0f};
            float durationSeconds{0.0f};
            float cameraYawDegrees{38.0f};
            float cameraDistance{900.0f};
            float cameraHeight{410.0f};
            float cameraFovDegrees{43.0f};
            std::string cue;
        };
        '''),
        "shared cutscene action language",
    )
    replace_once(
        cutscene_hpp,
        "    std::vector<ActorStaging> staging;\n};",
        "    std::vector<ActorStaging> staging;\n    std::vector<CutsceneAction> actions;\n};",
        "CutsceneDefinition action list",
    )
    insert_after(cutscene_hpp, "#include <string>\n", "#include <cstddef>\n", "#include <cstddef>", "cstddef include")

    cutscene_cpp = root / "src/content/cutscene_registry.cpp"
    action_block = dedent('''\

        // U7A: retrofit existing Chapter 1 beats with movement/camera intent while
        // preserving every existing story line and scene identity.
        const auto setActions = [&](const std::string& id, std::vector<CutsceneAction> actions) {
            cutscenes_.at(id).actions = std::move(actions);
        };

        setActions("ch1_object_swap_setup", {
            {0, CutsceneActionKind::CameraTrack, "rrvvfo", "", {-1325.0f, 65.0f}, 0.0f, .20f, 28.0f, 720.0f, 285.0f, 39.0f, "low_walk_in"},
            {0, CutsceneActionKind::MoveTo, "rrvvfo", "", {-1325.0f, 65.0f}, 175.0f, .30f},
            {0, CutsceneActionKind::PlayAnimation, "sage", "", {}, 0.0f, .30f, 0,0,0,0, "set_training_post"},
            {1, CutsceneActionKind::Expression, "rrvvfo", "", {}, 0.0f, .20f, 0,0,0,0, "unimpressed"},
            {2, CutsceneActionKind::FaceActor, "sage", "rrvvfo"},
            {2, CutsceneActionKind::CameraFocus, "", "sage", {}, 0.0f, .15f, 34.0f, 760.0f, 310.0f, 41.0f, "anchor_explain"},
            {3, CutsceneActionKind::CameraFocus, "", "rrvvfo", {}, 0.0f, .15f, 30.0f, 700.0f, 290.0f, 40.0f, "reaction"}
        });

        setActions("ch1_opening_sage_setup", {
            {0, CutsceneActionKind::CameraTrack, "rrvvfo", "sage", {}, 0.0f, .20f, 38.0f, 850.0f, 360.0f, 42.0f, "walking_two_shot"},
            {1, CutsceneActionKind::FaceActor, "sage", "rrvvfo"},
            {2, CutsceneActionKind::Expression, "rrvvfo", "", {}, 0.0f, .10f, 0,0,0,0, "annoyed"},
            {4, CutsceneActionKind::CameraFocus, "", "rrvvfo", {}, 0.0f, .15f, 24.0f, 660.0f, 275.0f, 39.0f, "pride_close"},
            {6, CutsceneActionKind::TriggerWorldEvent, "sage", "", {}, 0.0f, .10f, 0,0,0,0, "manual_handoff"}
        });

        setActions("tournament_road_departure_dialogue", {
            {0, CutsceneActionKind::CameraFocus, "", "", {-785.0f, 0.0f}, 0.0f, .25f, 38.0f, 960.0f, 420.0f, 44.0f, "show_road"},
            {1, CutsceneActionKind::MoveTo, "rrvvfo", "", {-900.0f, 90.0f}, 155.0f, .40f},
            {2, CutsceneActionKind::CameraTrack, "rrvvfo", "", {}, 0.0f, .15f, 35.0f, 820.0f, 340.0f, 42.0f, "walk_past_sage"},
            {3, CutsceneActionKind::MoveTo, "rrvvfo", "", {-835.0f, 55.0f}, 180.0f, .35f},
            {3, CutsceneActionKind::Wait, "", "", {}, 0.0f, .18f}
        });

        setActions("tournament_checkpoint_dialogue", {
            {0, CutsceneActionKind::CameraTrack, "rrvvfo", "checkpoint_worker", {}, 0.0f, .12f, 38.0f, 760.0f, 320.0f, 41.0f, "checkpoint_two_shot"},
            {1, CutsceneActionKind::Expression, "rrvvfo", "", {}, 0.0f, .10f, 0,0,0,0, "confident"}
        });

        setActions("lens_manual_reaction", {
            {0, CutsceneActionKind::CameraFocus, "", "rrvvfo", {}, 0.0f, .12f, 25.0f, 650.0f, 280.0f, 39.0f, "lens_reaction"},
            {0, CutsceneActionKind::Expression, "rrvvfo", "", {}, 0.0f, .12f, 0,0,0,0, "annoyed"}
        });

        setActions("tournament_outskirts_arrival", {
            {0, CutsceneActionKind::MoveTo, "rrvvfo", "", {1990.0f, -10.0f}, 115.0f, .65f},
            {0, CutsceneActionKind::CameraTrack, "rrvvfo", "", {}, 0.0f, .20f, 32.0f, 980.0f, 390.0f, 43.0f, "long_approach"},
            {1, CutsceneActionKind::Expression, "rrvvfo", "", {}, 0.0f, .10f, 0,0,0,0, "dry"},
            {3, CutsceneActionKind::CameraFocus, "", "", {2050.0f, 0.0f}, 0.0f, .18f, 30.0f, 900.0f, 360.0f, 42.0f, "entrance_reveal"},
            {4, CutsceneActionKind::MoveTo, "rrvvfo", "", {2075.0f, 0.0f}, 165.0f, .45f},
            {4, CutsceneActionKind::TriggerWorldEvent, "", "", {}, 0.0f, .10f, 0,0,0,0, "cross_tournament_threshold"}
        });
    ''')
    insert_before(cutscene_cpp, "}\n\nconst CutsceneDefinition& CutsceneRegistry::get", action_block, "U7A: retrofit existing Chapter 1 beats", "Chapter 1 cinematic action data")
    insert_after(cutscene_cpp, "#include <stdexcept>\n", "#include <utility>\n", "#include <utility>", "cutscene utility include")

    runtime_hpp = root / "src/core/runtime.hpp"
    view_fields = dedent('''\

        // U7 shared cinematic state. Renderers consume these values directly;
        // story content never reaches into a platform camera implementation.
        bool cinematicCameraActive{false};
        Vec2 cinematicCameraFocus{};
        float cinematicCameraYawDegrees{38.0f};
        float cinematicCameraDistance{900.0f};
        float cinematicCameraHeight{410.0f};
        float cinematicCameraFovDegrees{43.0f};
        std::string cinematicExpression;
        std::string cinematicWorldEvent;
    ''')
    insert_before(runtime_hpp, "};\n\n// One platform-independent gameplay session", view_fields, "cinematicCameraActive", "RuntimeView cinematic state")
    private_methods = dedent('''\
        void applyCutsceneActions(float dt);
        bool cutsceneBeatReadyToAdvance() const;
        Vec2 cutsceneActorPosition(const std::string& actorId) const;
    ''')
    insert_after(runtime_hpp, "    void tickCutscene(InputState& input, float dt);\n", private_methods, "applyCutsceneActions", "cutscene runtime methods")
    private_fields = dedent('''\
        float cutsceneBeatTime_{0.0f};
        std::size_t cutsceneBeatIndex_{static_cast<std::size_t>(-1)};
        bool cinematicCameraActive_{false};
        Vec2 cinematicCameraFocus_{};
        float cinematicCameraYawDegrees_{38.0f};
        float cinematicCameraDistance_{900.0f};
        float cinematicCameraHeight_{410.0f};
        float cinematicCameraFovDegrees_{43.0f};
        std::string cinematicExpression_;
        std::string cinematicWorldEvent_;
    ''')
    insert_after(runtime_hpp, "    bool cutsceneOpponentVisible_{false};\n", private_fields, "cutsceneBeatTime_", "cutscene runtime state")

    runtime_cpp = root / "src/core/runtime.cpp"
    cutscene_impl = dedent('''\
    void RuntimeSession::tickCutscene(InputState& input, float dt) {
        const auto& id = game_.scene().id;
        const bool seen = std::find(seenCutscenes_.begin(), seenCutscenes_.end(), id) != seenCutscenes_.end();
        if (seen && input.pressed(Action::Cancel)) { completeScene(); return; }
        applyCutsceneActions(dt);
        if (dialogueAdvanceRequested(input, dt) && cutsceneBeatReadyToAdvance()) advanceDialogue();
    }
    ''')
    replace_function(runtime_cpp, "void RuntimeSession::tickCutscene(InputState& input, float dt)", cutscene_impl, "shared cutscene action execution")

    action_impl = dedent('''\

    Vec2 RuntimeSession::cutsceneActorPosition(const std::string& actorId) const {
        if (actorId.empty() || actorId == "rrvvfo" || actorId == game_.chapter().playableCharacter) return playerPosition_;
        if (actorId == "sage") return opponentPosition_;
        if (cutscenes_.has(game_.scene().id)) {
            for (const auto& actor : cutscenes_.get(game_.scene().id).staging)
                if (actor.actorId == actorId) return actor.position;
        }
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
        }
        cutsceneBeatTime_ += dt;
        const bool firstFrame = cutsceneBeatTime_ <= dt + .0001f;
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
            switch (action.kind) {
                case CutsceneActionKind::MoveTo:
                    if (action.actorId == "rrvvfo" || action.actorId == game_.chapter().playableCharacter)
                        moveActor(action, playerPosition_, playerYawDegrees_);
                    else if (action.actorId == "sage")
                        moveActor(action, opponentPosition_, opponentYawDegrees_);
                    break;
                case CutsceneActionKind::FaceActor:
                case CutsceneActionKind::LookAt: {
                    const Vec2 target = cutsceneActorPosition(action.targetActorId);
                    if (action.actorId == "rrvvfo" || action.actorId == game_.chapter().playableCharacter)
                        playerYawDegrees_ = turnTowardDegrees(playerYawDegrees_, yawForDirection({target.x-playerPosition_.x,target.z-playerPosition_.z}), kExplorationTurnDegreesPerSecond*dt);
                    else if (action.actorId == "sage")
                        opponentYawDegrees_ = turnTowardDegrees(opponentYawDegrees_, yawForDirection({target.x-opponentPosition_.x,target.z-opponentPosition_.z}), kExplorationTurnDegreesPerSecond*dt);
                    break;
                }
                case CutsceneActionKind::PlayAnimation:
                    if (firstFrame && (action.actorId == "rrvvfo" || action.actorId == game_.chapter().playableCharacter) && !action.cue.empty())
                        triggerAbilityAnimation(action.cue, std::max(.10f, action.durationSeconds));
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
                    if (firstFrame) cinematicWorldEvent_ = action.cue;
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
            if (action.kind == CutsceneActionKind::Wait) minimumTime = std::max(minimumTime, action.durationSeconds);
            if (action.kind == CutsceneActionKind::MoveTo) {
                const Vec2 actor = cutsceneActorPosition(action.actorId);
                if (distance(actor, action.position) > 14.0f) return false;
            }
        }
        return cutsceneBeatTime_ >= minimumTime;
    }
    ''')
    insert_before(runtime_cpp, "bool RuntimeSession::dialogueAdvanceRequested", action_impl, "Vec2 RuntimeSession::cutsceneActorPosition", "cinematic action helpers")

    # Reset shared cinematic state on every scene entry.
    reset_block = dedent('''\
        cutsceneBeatTime_ = 0.0f;
        cutsceneBeatIndex_ = static_cast<std::size_t>(-1);
        cinematicCameraActive_ = false;
        cinematicCameraFocus_ = {};
        cinematicCameraYawDegrees_ = 38.0f;
        cinematicCameraDistance_ = 900.0f;
        cinematicCameraHeight_ = 410.0f;
        cinematicCameraFovDegrees_ = 43.0f;
        cinematicExpression_.clear();
        cinematicWorldEvent_.clear();
    ''')
    insert_after(runtime_cpp, "    cutsceneOpponentVisible_ = false;\n", reset_block, "cutsceneBeatIndex_ = static_cast", "cinematic scene reset")

    # Copy the shared camera state into RuntimeView immediately before choice rendering.
    sync_insert = dedent('''\
        view_.cinematicCameraActive = cinematicCameraActive_ && game_.mode() == GameMode::Cutscene && !qolSettings_.reducedMotion;
        view_.cinematicCameraFocus = cinematicCameraFocus_;
        view_.cinematicCameraYawDegrees = cinematicCameraYawDegrees_;
        view_.cinematicCameraDistance = cinematicCameraDistance_;
        view_.cinematicCameraHeight = cinematicCameraHeight_;
        view_.cinematicCameraFovDegrees = cinematicCameraFovDegrees_;
        view_.cinematicExpression = cinematicExpression_;
        view_.cinematicWorldEvent = cinematicWorldEvent_;
    ''')
    insert_before(runtime_cpp, "    if (choiceKind_ == 1) {", sync_insert, "view_.cinematicCameraActive", "RuntimeView cinematic sync")

    # Mac camera consumes the shared override; stage camera remains fallback.
    mac = root / "src/platform/macos/main.mm"
    replace_once(
        mac,
        "    const float yaw=stage.camera.yawDegrees*kPi/180.0f;\n    const vector_float3 target={(float)focusX,stage.camera.targetHeight,(float)focusZ};\n    const vector_float3 eye={focusX+std::sin(yaw)*stage.camera.baseDistance,stage.camera.height,focusZ+std::cos(yaw)*stage.camera.baseDistance};\n    SceneUniforms uniforms{};uniforms.viewProjection=simd_mul(perspective(stage.camera.fovDegrees,aspect,stage.camera.nearPlane,stage.camera.farPlane),lookAt(eye,target));",
        dedent('''\
            const float cameraFocusX=v.cinematicCameraActive?v.cinematicCameraFocus.x:focusX;
            const float cameraFocusZ=v.cinematicCameraActive?v.cinematicCameraFocus.z:focusZ;
            const float cameraYaw=v.cinematicCameraActive?v.cinematicCameraYawDegrees:stage.camera.yawDegrees;
            const float cameraDistance=v.cinematicCameraActive?v.cinematicCameraDistance:stage.camera.baseDistance;
            const float cameraHeight=v.cinematicCameraActive?v.cinematicCameraHeight:stage.camera.height;
            const float cameraFov=v.cinematicCameraActive?v.cinematicCameraFovDegrees:stage.camera.fovDegrees;
            const float yaw=cameraYaw*kPi/180.0f;
            const vector_float3 target={(float)cameraFocusX,stage.camera.targetHeight,(float)cameraFocusZ};
            const vector_float3 eye={cameraFocusX+std::sin(yaw)*cameraDistance,cameraHeight,cameraFocusZ+std::cos(yaw)*cameraDistance};
            SceneUniforms uniforms{};uniforms.viewProjection=simd_mul(perspective(cameraFov,aspect,stage.camera.nearPlane,stage.camera.farPlane),lookAt(eye,target));'''),
        "macOS shared cinematic camera",
    )

    # Linux: create a camera copy so software renderer receives the exact same override.
    linux = root / "src/platform/linux/presentation_renderer.cpp"
    linux_old = "SoftwareWorldCanvas world(renderer_, stage, focusX, focusZ);"
    linux_new = dedent('''\
        WorldPresentationDefinition cameraStage = stage;
        if (view.cinematicCameraActive) {
            cameraStage.camera.yawDegrees = view.cinematicCameraYawDegrees;
            cameraStage.camera.baseDistance = view.cinematicCameraDistance;
            cameraStage.camera.height = view.cinematicCameraHeight;
            cameraStage.camera.fovDegrees = view.cinematicCameraFovDegrees;
            focusX = view.cinematicCameraFocus.x;
            focusZ = view.cinematicCameraFocus.z;
        }
        SoftwareWorldCanvas world(renderer_, cameraStage, focusX, focusZ);''')
    if linux_old in read(linux):
        replace_once(linux, linux_old, linux_new, "Linux shared cinematic camera")
    else:
        print("  note: Linux camera constructor anchor differs; source verifier will flag if cinematic override is absent")

    docs = root / "docs/U7A_CINEMATIC_FOUNDATION.md"
    write(docs, dedent('''\
    # U7A — Cinematic Foundation

    Locked scope:
    - shared MoveTo / FaceActor / PlayAnimation / LookAt / CameraTrack / CameraFocus / TriggerWorldEvent / Wait / Dialogue / Expression actions;
    - Chapter 1 retrofit only; no story rewrite;
    - cutscenes hand control back to the same RuntimeSession;
    - Reduced Motion falls back to the authored stage camera;
    - existing 0.75 second Legacy idle and dedicated combat_retreat assets are untouched;
    - Tournament arrival is staged as movement toward the threshold, not a player-facing chapter-complete menu.

    Milestone gate: BUG CHECK -> exact-behavior simplification when safe -> Chapter 1 regression -> continue.
    '''))
    mark(root, "U7A")


def apply_u7b(root: pathlib.Path):
    print("\n=== U7B — ABILITY + RRVVFO PRESENTATION ===")
    runtime_hpp = root / "src/core/runtime.hpp"
    ability_view = dedent('''\

        // U7B ability presentation. Gold is Object Swap, purple is Lens.
        float objectSwapCooldownSeconds{0.0f};
        bool objectSwapTargetLocked{false};
        Vec2 objectSwapTargetPosition{};
        float objectSwapPhaseSeconds{0.0f};
        Vec2 objectSwapGhostFrom{};
        Vec2 objectSwapGhostTo{};
        float lensBlindnessAmount{0.0f};
    ''')
    insert_before(runtime_hpp, "};\n\n// One platform-independent gameplay session", ability_view, "objectSwapCooldownSeconds", "ability presentation view")
    methods = dedent('''\
        void refreshFreeSwapObjects();
        bool tryFreeObjectSwap();
        void scheduleLensBlindness(float delaySeconds);
    ''')
    insert_after(runtime_hpp, "    bool tickAdventureSidePuzzle(const AbilitySlotDefinition* ability);\n", methods, "tryFreeObjectSwap", "free Object Swap methods")
    fields = dedent('''\
        std::string freeSwapMapId_;
        std::vector<WorldObjectState> freeSwapObjects_;
        float objectSwapCooldownTime_{0.0f};
        float objectSwapPhaseTime_{0.0f};
        Vec2 objectSwapGhostFrom_{};
        Vec2 objectSwapGhostTo_{};
        float lensBlindnessDelay_{0.0f};
        float lensBlindnessTime_{0.0f};
        float lensBlindnessDuration_{0.65f};
    ''')
    insert_after(runtime_hpp, "    Vec2 farBankRockPosition_{230.0f, 0.0f};\n", fields, "freeSwapMapId_", "free Object Swap runtime state")

    runtime_cpp = root / "src/core/runtime.cpp"
    # Reset only when a fresh chapter/session starts; scene transitions keep swapped props where they were.
    reset = dedent('''\
        freeSwapMapId_.clear();
        freeSwapObjects_.clear();
        objectSwapCooldownTime_ = 0.0f;
        objectSwapPhaseTime_ = 0.0f;
        lensBlindnessDelay_ = 0.0f;
        lensBlindnessTime_ = 0.0f;
    ''')
    insert_after(runtime_cpp, "    farBankRockPosition_ = {230.0f, 0.0f};\n", reset, "freeSwapMapId_.clear", "U7B chapter ability reset")

    # Scene entry keeps local fun props alive and seeds them once per map.
    insert_before(runtime_cpp, "    previousPlayerPosition_ = playerPosition_;\n    syncView();\n    sceneCheckpointSnapshot_ = saveSnapshot();", "    refreshFreeSwapObjects();\n", "refreshFreeSwapObjects();", "seed free swap props on scene entry")

    helper_impl = dedent('''\

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
    ''')
    insert_before(runtime_cpp, "bool RuntimeSession::tickAdventureSidePuzzle", helper_impl, "void RuntimeSession::refreshFreeSwapObjects", "free Object Swap + Lens helpers")

    # Timers update independently from hit freeze; blindness is presentation drawback, never a soft-lock.
    timer_block = '''    objectSwapCooldownTime_ = std::max(0.0f, objectSwapCooldownTime_ - dt);
    objectSwapPhaseTime_ = std::max(0.0f, objectSwapPhaseTime_ - dt);
    if (lensBlindnessDelay_ > 0.0f) {
        lensBlindnessDelay_ = std::max(0.0f, lensBlindnessDelay_ - dt);
        if (lensBlindnessDelay_ <= 0.0f) lensBlindnessTime_ = lensBlindnessDuration_;
    } else {
        lensBlindnessTime_ = std::max(0.0f, lensBlindnessTime_ - dt);
    }
'''
    # Older generated batches could place these frame timers after the first
    # playerCharging_ reset in startChapter(), where dt does not exist. Remove
    # that misplaced copy before applying the function-scoped tick anchor.
    runtime_text = read(runtime_cpp)
    tick_start = runtime_text.find("void RuntimeSession::tick(InputState& input, float dt)")
    misplaced = runtime_text.find("objectSwapCooldownTime_ = std::max", 0, tick_start)
    if misplaced >= 0:
        misplaced_end = runtime_text.find("    flowCancelLearned_ = false;", misplaced, tick_start)
        if misplaced_end < 0:
            raise RuntimeError("Could not bound misplaced ability presentation timers")
        line_start = runtime_text.rfind("\n", 0, misplaced) + 1
        write(runtime_cpp, runtime_text[:line_start] + runtime_text[misplaced_end:])
        print("  repaired: ability timers moved out of chapter initialization")
    tick_marker = "void RuntimeSession::tick(InputState& input, float dt) {\n    dt = std::clamp(std::max(0.0f, dt), 0.0f, 0.1f);\n    playerCharging_ = false;\n"
    insert_after(runtime_cpp, tick_marker, timer_block, "objectSwapCooldownTime_ = std::max", "ability presentation timers")

    # Free swaps are only fallback exploration actions. Required/puzzle swap rules keep authority and ignore the free-use cooldown.
    free_swap_hook = '''    const bool storyOwnsObjectSwap = definition.rule == ExplorationRuleKind::UseAbilityPoint ||
        definition.rule == ExplorationRuleKind::SwapRelay || definition.rule == ExplorationRuleKind::TerrainDetour;
    if (ability && ability->id == "objectSwap" && !storyOwnsObjectSwap &&
        game_.scene().id != "reach_tournament_outskirts" && tryFreeObjectSwap()) return;

'''
    runtime_text = read(runtime_cpp)
    exploration_start = runtime_text.find("void RuntimeSession::tickExploration(InputState& input, float dt)")
    misplaced = runtime_text.find("const bool storyOwnsObjectSwap", 0, exploration_start)
    if misplaced >= 0:
        hook_end = runtime_text.find(" && tryFreeObjectSwap()) return;", misplaced, exploration_start)
        if hook_end < 0:
            raise RuntimeError("Could not bound misplaced free Object Swap hook")
        hook_end += len(" && tryFreeObjectSwap()) return;")
        line_start = runtime_text.rfind("\n", 0, misplaced) + 1
        while hook_end < len(runtime_text) and runtime_text[hook_end] == "\n":
            hook_end += 1
        write(runtime_cpp, runtime_text[:line_start] + runtime_text[hook_end:])
        print("  repaired: free Object Swap hook moved out of training")
    insert_before(runtime_cpp,
        "    if (game_.scene().id == \"reach_tournament_outskirts\" && tickAdventureSidePuzzle(ability)) return;",
        free_swap_hook, "storyOwnsObjectSwap", "free Object Swap fallback hook")

    # Lens blindness gets scheduled from every current Chapter-1 Lens path.
    replace_once(runtime_cpp,
        "        triggerAbilityAnimation(\"lens_activate\", 0.30f);\n        beginTransientDialogue(puzzle.lensDialogueId);",
        "        triggerAbilityAnimation(\"lens_activate\", 0.30f);\n        scheduleLensBlindness(.30f);\n        beginTransientDialogue(puzzle.lensDialogueId);",
        "Lens side-puzzle blindness")
    replace_once(runtime_cpp,
        "                lensActive_ = true;\n                triggerAbilityAnimation(\"lens_activate\", 0.30f);\n                for (const auto& blocker : definition.blockersToDisable)",
        "                lensActive_ = true;\n                triggerAbilityAnimation(\"lens_activate\", 0.30f);\n                scheduleLensBlindness(.30f);\n                for (const auto& blocker : definition.blockersToDisable)",
        "Lens roadblock blindness")
    replace_once(runtime_cpp,
        "        combatLensTimer_ = 4.0f;\n        lensActive_ = true;",
        "        combatLensTimer_ = 4.0f;\n        lensActive_ = true;\n        scheduleLensBlindness(4.0f);",
        "combat Lens blindness")
    # Training Lens cue: schedule after the active read window instead of immediately obscuring the lesson.
    lens_training_old = "                lensActive_ = true;\n                triggerAbilityAnimation(\"lens_activate\", 0.30f);\n                trainingSignals_ |= 2u;"
    lens_training_new = "                lensActive_ = true;\n                triggerAbilityAnimation(\"lens_activate\", 0.30f);\n                scheduleLensBlindness(1.10f);\n                trainingSignals_ |= 2u;"
    if lens_training_old in read(runtime_cpp):
        replace_once(runtime_cpp, lens_training_old, lens_training_new, "training Lens recovery blindness")

    # Persist the fun swap objects so the physical exchange remains real across save/load.
    save_anchor = "    for (std::size_t i = 0; i < relayMarkers_.size(); ++i)\n        data.world.objects.push_back({\"relay_marker_\" + std::to_string(i), relayMarkers_[i], true});\n"
    save_add = dedent('''\
        data.world.objects.erase(std::remove_if(data.world.objects.begin(), data.world.objects.end(), [](const WorldObjectState& object){
            return object.id.rfind("free_swap_", 0) == 0;
        }), data.world.objects.end());
        for (const auto& object : freeSwapObjects_) data.world.objects.push_back(object);
    ''')
    insert_after(runtime_cpp, save_anchor, save_add, "object.id.rfind(\"free_swap_\"", "persist free swap objects")

    load_anchor = "    routeProgress_ = savedUnsignedFlag(data.story.flags, \"ch1_route_progress=\", 8);\n"
    load_add = dedent('''\
        refreshFreeSwapObjects();
        for (auto& object : freeSwapObjects_) {
            const auto savedObject = std::find_if(data.world.objects.begin(), data.world.objects.end(), [&](const WorldObjectState& saved){ return saved.id == object.id; });
            if (savedObject != data.world.objects.end()) object = *savedObject;
        }
    ''')
    insert_before(runtime_cpp, load_anchor, load_add, "savedObject = std::find_if", "restore free swap objects")

    # RuntimeView markers: lock target, gold exchange ghosts, purple blindness state.
    sync_ability = dedent('''\
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
    ''')
    insert_before(runtime_cpp, "    // Lightweight shared ability VFX markers", sync_ability, "view_.objectSwapCooldownSeconds", "ability presentation markers")

    # Ch1 safe/fun free-swap props. Short range prevents them from bypassing authored blockers.
    map_cpp = root / "src/content/map_registry.cpp"
    landmark_add = dedent('''\
        ,{"free_swap_road_crate_a", "Loose Supply Crate", {-585.0f, 265.0f}}
        ,{"free_swap_road_crate_b", "Loose Supply Crate", {815.0f, 300.0f}}
        ,{"free_swap_outskirts_barrel", "Tournament Barrel", {1510.0f, -305.0f}}
    ''')
    insert_before(map_cpp, "    };\n    maps_.emplace(training.id, training);", landmark_add, "free_swap_road_crate_a", "safe Chapter-1 free-swap props")

    # Correct the ability color language on macOS and add cheap gold phase markers.
    mac = root / "src/platform/macos/main.mm"
    replace_once(mac,
        "{.50f,.94f,1.0f,.52f}",
        "{1.0f,.78f,.12f,.56f}",
        "Object Swap gold marker")
    replace_once(mac,
        "{1.0f,.74f,.18f,.65f}",
        "{.63f,.22f,.92f,.68f}",
        "Lens purple marker")
    marker_anchor = "        }else if(marker.kind==\"pursuit-lock\"){"
    marker_add = dedent('''\
            }else if(marker.kind=="object-swap-lock"){
                pushCylinder(vertices,{marker.position.x,42.0f,marker.position.z,54.0f,84.0f,54.0f,0.0f},{1.0f,.78f,.12f,.38f},16);
            }else if(marker.kind=="object-swap-ghost"||marker.kind=="object-swap-phase"){
                pushCylinder(vertices,{marker.position.x,76.0f,marker.position.z,46.0f,152.0f,46.0f,0.0f},{1.0f,.72f,.08f,.28f},14);
            }else if(marker.kind=="free-swap-object"){
                pushBox(vertices,{marker.position.x,26.0f,marker.position.z,46.0f,52.0f,46.0f,0.0f},{.60f,.42f,.25f,1.0f});
    ''')
    insert_before(mac, marker_anchor, marker_add, "marker.kind==\"object-swap-lock\"", "macOS gold swap lock/phase")
    # Purple post-Lens visibility loss through fog, never a hard-black screen.
    replace_once(mac,
        "uniforms.fogParameters=(vector_float4){stage.fogNear,stage.fogFar,0,0};uniforms.fogColor=(vector_float4){stage.fogColor.r,stage.fogColor.g,stage.fogColor.b,1};",
        dedent('''\
        const float lensBlind=std::clamp(v.lensBlindnessAmount,0.0f,1.0f);
        const float fogNear=stage.fogNear*(1.0f-lensBlind*.92f);
        const float fogFar=stage.fogFar*(1.0f-lensBlind*.72f);
        uniforms.fogParameters=(vector_float4){fogNear,fogFar,0,0};
        uniforms.fogColor=(vector_float4){stage.fogColor.r*(1.0f-lensBlind*.55f)+.16f*lensBlind,
                                           stage.fogColor.g*(1.0f-lensBlind*.72f)+.04f*lensBlind,
                                           stage.fogColor.b*(1.0f-lensBlind*.40f)+.24f*lensBlind,1};'''),
        "macOS purple Lens blindness fog")

    # 3DS marker color strings/logic vary by renderer; make the existing shared colors gold/purple when anchors exist.
    three = root / "src/platform/3ds/world_renderer_3ds.cpp"
    three_text = read(three)
    three_text = three_text.replace("0.45f, 0.91f, 1.0f", "1.0f, 0.78f, 0.12f")
    three_text = three_text.replace("1.0f, 0.74f, 0.18f", "0.63f, 0.22f, 0.92f")
    if three_text != read(three):
        write(three, three_text)
        print("  patched: 3DS gold/purple ability palette")

    write(root / "docs/U7B_ABILITY_PRESENTATION.md", dedent('''\
    # U7B — Ability + Rrvvfo Presentation

    Object Swap is a literal position exchange, never a dash.

    Free-use rules:
    - 225-unit local targeting radius;
    - 4.5 second cooldown;
    - 20 Energy;
    - sparse props only in safe/local spaces;
    - required story swaps keep authority and do not inherit the free-use cooldown;
    - swapped props persist in save data.

    Visual language:
    - Object Swap = gold/yellow target lock, simultaneous brief gold/translucent phase, dual ghosts, instantaneous exchange;
    - Lens of Truth = purple reveal, then 0.65 second recoverable visibility loss after the active read window;
    - Fire remains red/orange;
    - Shots of Agony remains blue and is NOT unlocked, equipped, taught or tested in Chapter 1. U7 only future-proofs translucent rendering.
    '''))
    mark(root, "U7B")


def apply_u8(root: pathlib.Path):
    print("\n=== U8 — TOURNAMENT HUB / CONTINUOUS STORY ===")

    # Generic exploration sequence rules are shared systems; tournament content only supplies data.
    exploration_hpp = root / "src/content/exploration_registry.hpp"
    replace_once(
        exploration_hpp,
        "    MandatoryAbilityReveal\n};",
        "    MandatoryAbilityReveal,\n    InteractionSequence,\n    TimedCheckpointSequence\n};",
        "shared interaction/timed sequence rules",
    )
    sequence_fields = dedent('''\
        // Shared authored sequence data. Any chapter may use these; they are not tournament-specific engine code.
        std::vector<Vec2> sequenceMarkers;
        std::vector<std::string> sequenceLabels;
        std::vector<std::string> sequenceDialogueIds;
        float sequenceRadius{82.0f};
        float sequenceTargetSeconds{0.0f};
        bool sequenceRequiresInteract{true};
    ''')
    insert_before(exploration_hpp, "    std::string encounterId;\n", sequence_fields, "sequenceMarkers", "generic exploration sequence data")

    runtime_hpp = root / "src/core/runtime.hpp"
    sequence_view = dedent('''\
        std::size_t storySequenceProgress{0};
        std::size_t storySequenceCount{0};
        float storySequenceSeconds{0.0f};
    ''')
    insert_before(runtime_hpp, "};\n\n// One platform-independent gameplay session", sequence_view, "storySequenceProgress", "story sequence view")
    sequence_fields_runtime = dedent('''\
        std::size_t sceneSequenceProgress_{0};
        float sceneSequenceSeconds_{0.0f};
        bool sceneSequenceStarted_{false};
    ''')
    insert_after(runtime_hpp, "    float routeChoiceIntroTime_{0.0f};\n", sequence_fields_runtime, "sceneSequenceProgress_", "story sequence runtime state")

    runtime_cpp = root / "src/core/runtime.cpp"
    # Reset sequence on scene entry.
    insert_after(runtime_cpp, "    sceneComplete_ = false;\n", "    sceneSequenceProgress_ = 0;\n    sceneSequenceSeconds_ = 0.0f;\n    sceneSequenceStarted_ = false;\n", "sceneSequenceProgress_ = 0", "scene sequence reset")

    # Handle shared sequence rules in the exploration switch.
    sequence_cases = dedent('''\
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
                    if (!definition.completionDialogueId.empty()) beginTransientDialogue(definition.completionDialogueId, 60);
                    else completeScene();
                }
            }
            break;
        }
    ''')
    insert_before(runtime_cpp, "        case ExplorationRuleKind::CliffJumpRoute:\n            break;", sequence_cases, "case ExplorationRuleKind::InteractionSequence", "generic exploration sequence runtime")

    # Continuation 60 means: dialogue belongs to a sequence; finish the scene only when all markers are done.
    finish_anchor = "void RuntimeSession::finishTransientDialogue()"
    # Insert a tiny branch inside the function by its first continuation switch anchor if available.
    finish_text = read(runtime_cpp)
    if "transientDialogueContinuation_ == 60" not in finish_text:
        sig = finish_text.find(finish_anchor)
        if sig < 0: raise RuntimeError("finishTransientDialogue missing")
        brace = finish_text.find("{", sig)
        addition = dedent('''\

        if (transientDialogueContinuation_ == 60) {
            transientDialogueId_.clear();
            transientDialogueIndex_ = 0;
            transientDialogueContinuation_ = 0;
            if (exploration_.has(game_.scene().id)) {
                const auto& definition = exploration_.get(game_.scene().id);
                if (!definition.sequenceMarkers.empty() && sceneSequenceProgress_ >= definition.sequenceMarkers.size()) completeScene();
            }
            return;
        }
        ''')
        write(runtime_cpp, finish_text[:brace+1] + addition + finish_text[brace+1:])
        print("  inserted: sequence dialogue continuation")

    # Sequence markers/labels into view.
    sequence_sync = dedent('''\
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
    ''')
    insert_before(runtime_cpp, "    // Lightweight shared ability VFX markers", sequence_sync, "view_.storySequenceProgress", "story sequence presentation")

    # Persist generic per-scene progress without deleting Chapter-1 history.
    generic_save = dedent('''\
        const std::string sceneProgressPrefix = "scene_progress=" + game_.story().chapterId + ":" + game_.scene().id + ":";
        data.story.flags.erase(std::remove_if(data.story.flags.begin(), data.story.flags.end(), [&](const std::string& flag){
            return flag.rfind(sceneProgressPrefix, 0) == 0;
        }), data.story.flags.end());
        if (sceneSequenceProgress_ > 0) data.story.flags.push_back(sceneProgressPrefix + std::to_string(sceneSequenceProgress_));
    ''')
    insert_before(runtime_cpp, "    data.frontend.objectiveHistory = objectiveHistory_;", generic_save, "sceneProgressPrefix", "generic sequence save progress")

    generic_load = dedent('''\
        const std::string sceneProgressPrefix = "scene_progress=" + game_.story().chapterId + ":" + game_.scene().id + ":";
        for (const auto& flag : data.story.flags) {
            if (flag.rfind(sceneProgressPrefix, 0) != 0) continue;
            try { sceneSequenceProgress_ = static_cast<std::size_t>(std::stoul(flag.substr(sceneProgressPrefix.size()))); }
            catch (...) { sceneSequenceProgress_ = 0; }
            break;
        }
    ''')
    insert_before(runtime_cpp, "    routeProgress_ = savedUnsignedFlag(data.story.flags, \"ch1_route_progress=\", 8);", generic_load, "sceneProgressPrefix = \"scene_progress=\" + game_.story", "generic sequence load progress")

    # Ch2 loading must not inherit Chapter-1 complete UI state.
    replace_once(runtime_cpp,
        "    chapterComplete_ = containsFlag(data.story.flags, \"ch1_complete_at_outskirts\");",
        "    chapterComplete_ = game_.story().chapterId == \"rrvvfo_ch1\" && containsFlag(data.story.flags, \"ch1_complete_at_outskirts\");",
        "chapter-complete scope")

    # Seamless internal chapter transition: final Ch1 scene enters Ch2 immediately if registered.
    complete_replacement = dedent('''\
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
        game_.advanceScene();

        if (game_.story().chapterId != beforeChapter) {
            // Internal chapter IDs are save/content boundaries, never a player-facing menu break.
            chapterComplete_ = false;
            sceneComplete_ = false;
            enterCurrentScene();
            manualSaveRequested_ = true;
            saveStatus_ = "AUTOSAVING…";
            saveStatusTime_ = 2.0f;
            showGameplayNotice("STORY CONTINUES • TOURNAMENT GROUNDS", 1.10f);
            return;
        }
        if (game_.story().sceneIndex != beforeIndex) {
            enterCurrentScene();
            return;
        }
        if (finalScene) {
            chapterComplete_ = true;
            sceneComplete_ = true;
        }
    }
    ''')
    replace_function(runtime_cpp, "void RuntimeSession::completeScene()", complete_replacement, "continuous internal chapter transition")

    # Tournament map and safe free-swap props.
    map_cpp = root / "src/content/map_registry.cpp"
    tournament_map = dedent('''\

        MapDefinition tournament;
        tournament.id = "tournament_grounds";
        tournament.name = "Tournament Grounds";
        tournament.bounds = {-1450.0f, 1450.0f, -980.0f, 980.0f};
        tournament.playerStart = {-1280.0f, 40.0f};
        tournament.zones = {
            {"entrance_plaza", "Tournament Entrance Plaza", {-1120.0f, 0.0f}, "arrival and crowd threshold"},
            {"registration_plaza", "Registration Plaza", {-760.0f, 0.0f}, "cards, bracket and official entry"},
            {"market_street", "Market Street", {-260.0f, -520.0f}, "vendors and festival side stories"},
            {"practice_grounds", "Practice Grounds", {-120.0f, 430.0f}, "sparring, race and cracked ring"},
            {"spectator_district", "Spectator District", {420.0f, -460.0f}, "fans, rumors and match reactions"},
            {"main_arena_gate", "Main Arena Gate", {930.0f, 0.0f}, "official bracket reporting"},
            {"contestant_lane", "Contestant Lane", {500.0f, 420.0f}, "medical tent, waiting tent and intermissions"}
        };
        tournament.links = {
            {"entrance_plaza","registration_plaza","main"}, {"registration_plaza","market_street","south_loop"},
            {"registration_plaza","practice_grounds","north_loop"}, {"market_street","spectator_district","south_loop"},
            {"practice_grounds","contestant_lane","north_loop"}, {"spectator_district","main_arena_gate","main"},
            {"contestant_lane","main_arena_gate","main"}, {"practice_grounds","market_street","service_lane"}
        };
        tournament.blockers = {
            {"arena_gate_lock", {850.0f, 1030.0f, -180.0f, 180.0f}, true}
        };
        tournament.landmarks = {
            {"tournament_entrance", "Tournament Entrance", {-1280.0f, 0.0f}},
            {"registration_desk", "Registration Desk", {-760.0f, 0.0f}},
            {"bracket_board", "Bracket Board", {-630.0f, 170.0f}},
            {"practice_ring", "Practice Ring", {-80.0f, 430.0f}},
            {"waiting_tent", "Contestant Waiting Tent", {470.0f, 455.0f}},
            {"medical_tent", "Medical Tent", {650.0f, 500.0f}},
            {"main_arena_gate", "Main Arena Gate", {930.0f, 0.0f}},
            {"free_swap_market_crate", "Market Supply Crate", {-300.0f, -390.0f}},
            {"free_swap_practice_barrel", "Practice Barrel", {85.0f, 300.0f}},
            {"free_swap_spectator_box", "Spectator Supply Box", {455.0f, -330.0f}}
        };
        maps_.emplace(tournament.id, tournament);

        MapDefinition tournamentArena;
        tournamentArena.id = "tournament_arena";
        tournamentArena.name = "Tournament Main Ring";
        tournamentArena.bounds = {-720.0f, 720.0f, -560.0f, 560.0f};
        tournamentArena.playerStart = {-300.0f, 0.0f};
        tournamentArena.zones = {{"official_ring", "Main Tournament Ring", {0.0f,0.0f}, "first-to-three stock combat and ring-outs"}};
        tournamentArena.landmarks = {{"ring_center", "Ring Center", {0.0f,0.0f}}};
        maps_.emplace(tournamentArena.id, tournamentArena);
    ''')
    insert_before(map_cpp, "    maps_.emplace(\"tangai_dojo\"", tournament_map, "tournament.id = \"tournament_grounds\"", "Tournament Grounds + arena maps")

    # Tournament presentation helper/stages.
    world_cpp = root / "src/content/world_presentation_registry.cpp"
    tournament_helper = dedent('''\

    void addTournamentHub(WorldPresentationDefinition& hub) {
        box(hub,"tg_ground",0,-24,0,2800,48,1840,rgb(117,151,83));
        box(hub,"tg_main_walk",-100,4,0,2440,8,260,rgb(215,192,145));
        box(hub,"tg_market_walk",-250,5,-470,1500,10,220,rgb(199,169,119));
        box(hub,"tg_practice_walk",-150,5,430,1640,10,220,rgb(199,169,119));
        for(int i=0;i<7;++i){
            const float x=-1080.0f+i*330.0f;
            cylinder(hub,"tg_banner_post_"+std::to_string(i),x,72,-135,12,144,12,rgb(86,61,45));
            box(hub,"tg_banner_"+std::to_string(i),x,105,-135,8,58,74,rgb(204,48,39,.92f),PresentationDetailTier::Full);
        }
        box(hub,"registration_counter",-760,42,0,210,84,70,rgb(130,83,49));
        box(hub,"bracket_board",-630,110,170,150,190,22,rgb(235,224,192));
        box(hub,"market_awning_a",-420,105,-560,260,18,160,rgb(214,67,52));
        box(hub,"market_awning_b",-80,105,-560,260,18,160,rgb(63,122,207));
        cylinder(hub,"practice_ring",-80,8,430,430,16,430,rgb(216,199,155));
        cylinder(hub,"practice_ring_inner",-80,12,430,365,8,365,rgb(178,157,116));
        box(hub,"waiting_tent",470,70,455,260,140,170,rgb(227,222,204),PresentationDetailTier::Essential);
        box(hub,"medical_tent",650,70,500,230,140,160,rgb(232,232,224),PresentationDetailTier::Full);
        box(hub,"arena_gate_a",930,105,-155,56,210,56,rgb(124,51,43));
        box(hub,"arena_gate_b",930,105,155,56,210,56,rgb(124,51,43));
        box(hub,"arena_gate_top",930,205,0,56,45,360,rgb(177,61,48));
        for(int i=0;i<18;++i){
            const float x=-1150.0f+(i%9)*270.0f;
            const float z=i<9?-760.0f:760.0f;
            tree(hub,"tg_tree_"+std::to_string(i),x,z,.72f+(i%3)*.06f,PresentationDetailTier::Full);
        }
        hub.ambientActors = {
            {"announcer","ambient_worker",{-760,0,100,1,1,1,-90},PresentationDetailTier::Essential},
            {"tournament_fan_a","ambient_fan",{-330,0,-360,1,1,1,10},PresentationDetailTier::Full},
            {"tournament_fan_b","ambient_fan",{370,0,-420,1,1,1,-20},PresentationDetailTier::Full},
            {"practice_fighter","road_fighter",{-10,0,510,1,1,1,180},PresentationDetailTier::Full}
        };
    }

    void addTournamentArena(WorldPresentationDefinition& arena) {
        box(arena,"arena_floor_base",0,-20,0,1380,40,1020,rgb(88,111,73));
        box(arena,"arena_ring",0,10,0,900,20,700,rgb(222,205,163));
        box(arena,"arena_ring_center",0,22,0,760,8,560,rgb(236,222,183));
        for(const float x:{-450.0f,450.0f}) for(const float z:{-350.0f,350.0f})
            cylinder(arena,"arena_corner_"+std::to_string((int)x)+"_"+std::to_string((int)z),x,62,z,20,124,20,rgb(157,49,42));
        box(arena,"arena_stands_n",0,110,-500,1260,220,170,rgb(83,76,68));
        box(arena,"arena_stands_s",0,110,500,1260,220,170,rgb(83,76,68));
    }
    ''')
    insert_before(world_cpp, "\n} // namespace\n\nWorldPresentationRegistry::WorldPresentationRegistry()", tournament_helper, "void addTournamentHub", "tournament presentation helpers")
    tournament_stages = dedent('''\

        WorldPresentationDefinition hub;
        hub.id="tournament-hub"; hub.displayName="Tournament Grounds";
        hub.camera={35.0f,44.0f,980.0f,850.0f,1180.0f,420.0f,44.0f,8.0f,4300.0f,0,0,1380,900,true};
        hub.clearColor=rgb(123,181,218); hub.fogColor=rgb(193,218,205); hub.fogNear=1150; hub.fogFar=3200;
        addTournamentHub(hub); stages_.emplace(hub.id,std::move(hub));

        WorldPresentationDefinition arena;
        arena.id="tournament-arena"; arena.displayName="Main Tournament Ring";
        arena.camera={32.0f,42.0f,960.0f,850.0f,1080.0f,390.0f,46.0f,8.0f,3000.0f,0,0,620,500,true};
        arena.clearColor=rgb(124,181,220); arena.fogColor=rgb(190,216,213); arena.fogNear=900; arena.fogFar=2600;
        addTournamentArena(arena); stages_.emplace(arena.id,std::move(arena));
    ''')
    insert_before(world_cpp, "}\n\nconst WorldPresentationDefinition& WorldPresentationRegistry::get", tournament_stages, "hub.id=\"tournament-hub\"", "tournament presentation stages")

    # Chapter 2 U8 skeleton. U9 replaces this flow with the complete tournament loop.
    chapter_cpp = root / "src/content/chapter_registry.cpp"
    ch2 = dedent('''\

        chapters_.emplace("rrvvfo_ch2", ChapterDefinition{
            "rrvvfo_ch2", "Tournament Grounds", "tournament_grounds", "rrvvfo",
            {
                {SceneKind::Cutscene, "tournament_gate_walk_in", "rrvvfo-ch2-arrival", "tournament-hub"},
                {SceneKind::Cutscene, "ch2_arrival_delay", "rrvvfo-ch2-delay", "tournament-hub"},
                {SceneKind::Exploration, "ch2_lost_bracket", "rrvvfo-ch2-bracket", "tournament-hub"},
                {SceneKind::Cutscene, "ch2_u8_gate", "rrvvfo-ch2-u8-gate", "tournament-hub"}
            }, ""
        });
    ''')
    insert_before(chapter_cpp, "\n}\n\nconst ChapterDefinition& ChapterRegistry::get", ch2, "chapters_.emplace(\"rrvvfo_ch2\"", "Chapter 2 continuous hub skeleton")

    # U8 exploration content: varied three-card bracket recovery.
    exploration_cpp = root / "src/content/exploration_registry.cpp"
    ch2_explore = dedent('''\

        ExplorationDefinition bracket;
        bracket.sceneId="ch2_lost_bracket"; bracket.rule=ExplorationRuleKind::InteractionSequence;
        bracket.objective="THE LOST BRACKET • FIND THREE CONTESTANT CARDS";
        bracket.detail="Search the grounds while registration is delayed.";
        bracket.hasPlayerStart=true; bracket.playerStart={-760.0f,40.0f};
        bracket.sequenceMarkers={{-370.0f,-500.0f},{-20.0f,-360.0f},{250.0f,120.0f}};
        bracket.sequenceLabels={"WADE'S CARD","BARK'S CARD","QUALIFIER CARD"};
        bracket.sequenceDialogueIds={"ch2_bracket_wade","ch2_bracket_bark","ch2_bracket_qualifier"};
        bracket.sequenceRadius=92.0f; bracket.sequenceRequiresInteract=true;
        bracket.completionDialogueId="ch2_bracket_return";
        scenes_.emplace(bracket.sceneId,bracket);
    ''')
    insert_before(exploration_cpp, "}\n\nconst ExplorationDefinition& ExplorationRegistry::get", ch2_explore, "bracket.sceneId=\"ch2_lost_bracket\"", "Lost Bracket exploration")

    # Exact rewritten Chapter 2 dialogue used for U8 arrival and bracket.
    dialogue_cpp = root / "src/content/dialogue_registry.cpp"
    ch2_dialogue = dedent('''\

        scenes_.emplace("tournament_gate_walk_in", std::vector<DialogueLine>{
            {"THE SAGE", "Hey. How was the road? I had some important business to take care of.", "sage", "casual", "sage"},
            {"RRVVFO", "We both know what that was, Mr. Sage the Great.", "rrvvfo", "sarcastic", "rrvvfo"}
        });
        scenes_.emplace("ch2_arrival_delay", std::vector<DialogueLine>{
            {"ANNOUNCER", "Registration delay! Nobody panic unless you’re holding part of the bracket!"},
            {"RRVVFO", "Not my problem."},
            {"THE SAGE", "You’re bored, right? Go do that."},
            {"RRVVFO", "Whatever. I’ve got nothing better to be doing."},
            {"ANNOUNCER", "Disaster! Three contestant cards escaped the bracket board!"},
            {"RRVVFO", "Aren’t you the announcer I used to watch in those World Tournaments on TV when I was younger? So I guess your clumsiness wasn’t a character."},
            {"ANNOUNCER", "HEY! HURTFUL!"},
            {"RRVVFO", "I’m helping you. I can be as rude as I want."}
        });
        scenes_.emplace("ch2_bracket_wade", std::vector<DialogueLine>{
            {"TOURNAMENT FAN", "A contestant card blew into my souvenir bag. It says Wade."},
            {"RRVVFO", "Of course his card traveled faster than everybody else’s."}
        });
        scenes_.emplace("ch2_bracket_bark", std::vector<DialogueLine>{
            {"RRVVFO", "Bark’s card landed on the upper market walkway."},
            {"FOOD VENDOR", "I said it went up. I did not say the wind respected stairs."}
        });
        scenes_.emplace("ch2_bracket_qualifier", std::vector<DialogueLine>{
            {"OLD COMPETITOR", "There it is—caught on the maintenance cart!"},
            {"RRVVFO", "The bracket paperwork is officially faster than the staff."}
        });
        scenes_.emplace("ch2_bracket_return", std::vector<DialogueLine>{
            {"ANNOUNCER", "Wade, Bark, and the unreadable qualifier! The bracket lives!"},
            {"RRVVFO", "I’m about to win."}
        });
        scenes_.emplace("ch2_u8_gate", std::vector<DialogueLine>{
            {"ANNOUNCER", "Registration is moving again. Competitors, stay near the practice grounds."}
        });
    ''')
    insert_before(dialogue_cpp, "}\n\nconst std::vector<DialogueLine>& DialogueRegistry::get", ch2_dialogue, "scenes_.emplace(\"tournament_gate_walk_in\"", "Chapter 2 arrival/bracket dialogue")

    # Cinematic arrival actions: walk through threshold, Sage joins from another direction, control stays in Story.
    cutscene_cpp = root / "src/content/cutscene_registry.cpp"
    ch2_cutscene_defs = dedent('''\

        cutscenes_.emplace("tournament_gate_walk_in", CutsceneDefinition{
            "tournament_gate_walk_in", CutsceneTier::Major,
            {{"threshold","","Walk through the tournament entrance instead of cutting to a menu.","long_track","walk"},
             {"sage_rejoins","SAGE","Sage approaches from another direction while both keep moving.","walking_two_shot","walk"}},
            {{"rrvvfo",{-1280.0f,40.0f},90.0f,true},{"sage",{-1140.0f,-180.0f},20.0f,true}},
            {
                {0,CutsceneActionKind::MoveTo,"rrvvfo","",{-1120.0f,40.0f},145.0f,.70f},
                {0,CutsceneActionKind::CameraTrack,"rrvvfo","",{},0,.20f,32.0f,980.0f,390.0f,43.0f,"entrance_reveal"},
                {0,CutsceneActionKind::MoveTo,"sage","",{-1050.0f,-30.0f},125.0f,.70f},
                {1,CutsceneActionKind::CameraTrack,"rrvvfo","sage",{},0,.20f,36.0f,840.0f,340.0f,42.0f,"walking_two_shot"},
                {1,CutsceneActionKind::MoveTo,"rrvvfo","",{-1010.0f,30.0f},150.0f,.55f},
                {1,CutsceneActionKind::MoveTo,"sage","",{-940.0f,-20.0f},145.0f,.55f}
            }
        });
        cutscenes_.emplace("ch2_arrival_delay", CutsceneDefinition{
            "ch2_arrival_delay", CutsceneTier::Directed,
            {{"announcer_delay","ANNOUNCER","Registration trouble becomes an active hub problem.","registration_wide","crowd_react"}},
            {{"rrvvfo",{-820.0f,40.0f},90.0f,true},{"sage",{-740.0f,-80.0f},30.0f,true}},
            {{0,CutsceneActionKind::CameraFocus,"","",{-760.0f,0.0f},0,.15f,35.0f,900.0f,360.0f,42.0f,"registration"}}
        });
        cutscenes_.emplace("ch2_u8_gate", CutsceneDefinition{
            "ch2_u8_gate", CutsceneTier::FieldDialogue,
            {{"practice_call","ANNOUNCER","Send the player toward the practice grounds without a menu break.","practice_pan","point"}},
            {{"rrvvfo",{-520.0f,30.0f},80.0f,true}},
            {{0,CutsceneActionKind::CameraFocus,"","",{-120.0f,430.0f},0,.15f,35.0f,960.0f,390.0f,43.0f,"practice_grounds"}}
        });
    ''')
    insert_before(cutscene_cpp, "// U7A: retrofit existing Chapter 1 beats", ch2_cutscene_defs, "cutscenes_.emplace(\"tournament_gate_walk_in\"", "Chapter 2 arrival cutscenes")

    write(root / "docs/U8_TOURNAMENT_HUB.md", dedent('''\
    # U8 — Tournament Hub / Continuous Story

    - Chapter 1 final scene automatically enters rrvvfo_ch2 through Game::advanceScene; no menu or mission-select screen.
    - Tournament Grounds is one authored hub with entrance, registration, market, practice, spectator, contestant and arena-gate districts.
    - The Lost Bracket teaches the hub with three distinct recoveries.
    - Safe local free-Object-Swap props exist in hub districts; none cross the arena gate or story blockers.
    - Internal chapter IDs remain save/content boundaries only.
    - Exact rewritten Chapter 2 arrival/bracket dialogue is content data, not platform code.
    '''))
    mark(root, "U8")


def apply_u9(root: pathlib.Path):
    print("\n=== U9 — TOURNAMENT CORE GAMEPLAY + PROGRESSION ===")

    # Data-only arena encounter registry. Runtime/combat remain shared.
    arena_hpp = dedent('''\
    #pragma once
    #include "core/combat.hpp"
    #include "core/world.hpp"
    #include <string>
    #include <unordered_map>

    namespace px {

    enum class StoryArenaResolution { PlayerVictory, PloukeStoryFinal };

    struct ArenaEncounterDefinition {
        std::string sceneId;
        std::string mapId;
        std::string opponentId;
        std::string opponentLabel;
        AiArchetype ai{AiArchetype::Balanced};
        Vec2 playerStart{};
        Vec2 opponentStart{};
        Rect ringBounds{};
        int stockTarget{3};
        float startingEnergy{45.0f};
        float opponentMoveSpeed{154.0f};
        int recommendedLevel{1};
        int xpReward{0};
        bool official{true};
        StoryArenaResolution resolution{StoryArenaResolution::PlayerVictory};
        std::string postDialogueId;
    };

    class ArenaEncounterRegistry {
    public:
        ArenaEncounterRegistry();
        const ArenaEncounterDefinition& get(const std::string& sceneId) const;
        bool has(const std::string& sceneId) const;
    private:
        std::unordered_map<std::string, ArenaEncounterDefinition> encounters_;
    };

    } // namespace px
    ''')
    arena_cpp = dedent('''\
    #include "content/arena_encounter_registry.hpp"
    #include <stdexcept>

    namespace px {

    ArenaEncounterRegistry::ArenaEncounterRegistry() {
        const Rect ring{-450.0f,450.0f,-350.0f,350.0f};
        encounters_.emplace("ch2_practice_brawl", ArenaEncounterDefinition{
            "ch2_practice_brawl","tournament_arena","practice_fighter","Practice Ring Fighter",AiArchetype::Balanced,
            {-280.0f,0.0f},{280.0f,0.0f},ring,1,45.0f,154.0f,1,45,false,StoryArenaResolution::PlayerVictory,"ch2_practice_brawl_result"});
        encounters_.emplace("ch2_vs_hamual", ArenaEncounterDefinition{
            "ch2_vs_hamual","tournament_arena","hamual","Hamual",AiArchetype::Aggressive,
            {-300.0f,0.0f},{300.0f,0.0f},ring,3,55.0f,158.0f,1,120,true,StoryArenaResolution::PlayerVictory,"ch2_hamual_result"});
        encounters_.emplace("ch2_vs_daniel", ArenaEncounterDefinition{
            "ch2_vs_daniel","tournament_arena","daniel","Daniel",AiArchetype::Defensive,
            {-300.0f,0.0f},{300.0f,0.0f},ring,3,58.0f,148.0f,2,150,true,StoryArenaResolution::PlayerVictory,"ch2_daniel_result"});
        encounters_.emplace("ch2_vs_wade", ArenaEncounterDefinition{
            "ch2_vs_wade","tournament_arena","wade","Wade",AiArchetype::Aggressive,
            {-300.0f,0.0f},{300.0f,0.0f},ring,3,62.0f,212.0f,3,190,true,StoryArenaResolution::PlayerVictory,"ch2_wade_result"});
        encounters_.emplace("ch2_vs_plouke", ArenaEncounterDefinition{
            "ch2_vs_plouke","tournament_arena","plouke","Plouke",AiArchetype::Adaptive,
            {-300.0f,0.0f},{300.0f,0.0f},ring,3,65.0f,138.0f,4,240,true,StoryArenaResolution::PloukeStoryFinal,""});
    }

    const ArenaEncounterDefinition& ArenaEncounterRegistry::get(const std::string& id) const {
        const auto it=encounters_.find(id); if(it==encounters_.end()) throw std::out_of_range("Unknown arena encounter: "+id); return it->second;
    }
    bool ArenaEncounterRegistry::has(const std::string& id) const { return encounters_.find(id)!=encounters_.end(); }

    } // namespace px
    ''')
    write(root / "src/content/arena_encounter_registry.hpp", arena_hpp)
    write(root / "src/content/arena_encounter_registry.cpp", arena_cpp)

    cmake = root / "CMakeLists.txt"
    insert_after(cmake, "    src/content/adventure_registry.cpp\n", "    src/content/arena_encounter_registry.cpp\n", "arena_encounter_registry.cpp", "arena encounter source in px_core")

    # Save schema 6: Tournament Card is shared save data, not chapter-local UI state.
    save_hpp = root / "src/core/save.hpp"
    insert_after(save_hpp, "#include \"core/types.hpp\"\n", "#include \"core/story_progression.hpp\"\n", "story_progression.hpp", "Tournament Card save include")
    replace_once(save_hpp, "static constexpr int kSchemaVersion = 5;", "static constexpr int kSchemaVersion = 6;", "save schema 6")
    insert_after(save_hpp, "    QolSettings qol;\n", "    TournamentCardState tournamentCard{\"rrvvfo\"};\n", "tournamentCard", "Tournament Card save field")

    save_cpp = root / "src/core/save.cpp"
    card_serialize = dedent('''\
        out << "cardOwner=" << data.tournamentCard.ownerId << '\\n';
        out << "cardLevel=" << data.tournamentCard.level << '\\n';
        out << "cardXp=" << data.tournamentCard.xp << '\\n';
        out << "cardPending=" << data.tournamentCard.pendingBonusChoices << '\\n';
        out << "cardAcquired=" << (data.tournamentCard.acquired ? 1 : 0) << '\\n';
        out << "cardBonusHp=" << data.tournamentCard.bonuses.hp << '\\n';
        out << "cardBonusPower=" << data.tournamentCard.bonuses.power << '\\n';
        out << "cardBonusDefense=" << data.tournamentCard.bonuses.defense << '\\n';
        out << "cardBonusSpeed=" << data.tournamentCard.bonuses.speed << '\\n';
        out << "cardBonusFocus=" << data.tournamentCard.bonuses.focus << '\\n';
    ''')
    broken_card_serialize = dedent('''\
        out << "cardOwner=" << data.tournamentCard.ownerId << '\\n';
        out << "cardLevel=" << data.tournamentCard.level << '\\n';
        out << "cardXp=" << data.tournamentCard.xp << '\\n';
        out << "cardPending=" << data.tournamentCard.pendingBonusChoices << '\\n';
        out << "cardAcquired=" << (data.tournamentCard.acquired ? 1 : 0) << '\\n';
        for (std::size_t i=0;i<data.tournamentCard.bonuses.size();++i)
            out << "cardBonus" << i << '=' << data.tournamentCard.bonuses[i] << '\\n';
    ''')
    if contains_semantic(read(save_cpp), broken_card_serialize):
        replace_once(save_cpp, broken_card_serialize, card_serialize, "repair Tournament Card serialization")
    else:
        insert_before(save_cpp, "    for (const auto& flag : data.story.flags) out << \"flag=\" << flag << '\\n';", card_serialize, "cardBonusHp=", "Tournament Card serialization")
    card_deserialize = dedent('''\
        else if (key == "cardOwner") data.tournamentCard.ownerId = value;
        else if (key == "cardLevel") data.tournamentCard.level = std::stoi(value);
        else if (key == "cardXp") data.tournamentCard.xp = std::stoi(value);
        else if (key == "cardPending") data.tournamentCard.pendingBonusChoices = std::stoi(value);
        else if (key == "cardAcquired") data.tournamentCard.acquired = value != "0";
        else if (key == "cardBonusHp" || key == "cardBonus0") data.tournamentCard.bonuses.hp = std::stoi(value);
        else if (key == "cardBonusPower" || key == "cardBonus1") data.tournamentCard.bonuses.power = std::stoi(value);
        else if (key == "cardBonusDefense" || key == "cardBonus2") data.tournamentCard.bonuses.defense = std::stoi(value);
        else if (key == "cardBonusSpeed" || key == "cardBonus3") data.tournamentCard.bonuses.speed = std::stoi(value);
        else if (key == "cardBonusFocus" || key == "cardBonus4") data.tournamentCard.bonuses.focus = std::stoi(value);
    ''')
    broken_card_deserialize = dedent('''\
        else if (key == "cardOwner") data.tournamentCard.ownerId = value;
        else if (key == "cardLevel") data.tournamentCard.level = std::stoi(value);
        else if (key == "cardXp") data.tournamentCard.xp = std::stoi(value);
        else if (key == "cardPending") data.tournamentCard.pendingBonusChoices = std::stoi(value);
        else if (key == "cardAcquired") data.tournamentCard.acquired = value != "0";
        else if (key.rfind("cardBonus",0)==0) {
            const auto index=static_cast<std::size_t>(std::stoul(key.substr(9)));
            if(index<data.tournamentCard.bonuses.size()) data.tournamentCard.bonuses[index]=std::stoi(value);
        }
    ''')
    if contains_semantic(read(save_cpp), broken_card_deserialize):
        replace_once(save_cpp, broken_card_deserialize, card_deserialize, "repair Tournament Card deserialization")
    else:
        insert_before(save_cpp, "        else if (key == \"flag\") data.story.flags.push_back(value);", card_deserialize, "key == \"cardBonusHp\"", "Tournament Card deserialization")
    replace_once(save_cpp,
        "    if (data.schemaVersion != 2 && data.schemaVersion != 3 && data.schemaVersion != 4 &&\n        data.schemaVersion != SaveData::kSchemaVersion)",
        "    if (data.schemaVersion != 2 && data.schemaVersion != 3 && data.schemaVersion != 4 &&\n        data.schemaVersion != 5 && data.schemaVersion != SaveData::kSchemaVersion)",
        "schema-5 migration support")

    runtime_hpp = root / "src/core/runtime.hpp"
    insert_after(runtime_hpp, "#include \"content/adventure_registry.hpp\"\n", "#include \"content/arena_encounter_registry.hpp\"\n", "arena_encounter_registry.hpp", "arena registry include")
    tournament_view = dedent('''\
        TournamentCardState tournamentCard{"rrvvfo"};
        bool tournamentCardVisible{false};
        int tournamentBonusRoll{0};
        int playerStocksLost{0};
        int opponentStocksLost{0};
        int stockTarget{0};
        int recommendedLevel{0};
        bool officialTournamentMatch{false};
    ''')
    insert_before(runtime_hpp, "};\n\n// One platform-independent gameplay session", tournament_view, "officialTournamentMatch", "tournament runtime view")
    methods = dedent('''\
        void startStoryArena();
        void tickStoryArena(InputState& input, float dt);
        void resetStoryArenaStock();
        void finishStoryArena(bool playerWon);
        void resolveArenaRewardAndAdvance();
    ''')
    insert_after(runtime_hpp, "    void tickArena(InputState& input, float dt);\n", methods, "tickStoryArena", "shared story arena methods")
    fields = dedent('''\
        ArenaEncounterRegistry arenaEncounters_{};
        bool storyArenaActive_{false};
        int storyPlayerStocksLost_{0};
        int storyOpponentStocksLost_{0};
        float storyArenaAiTimer_{0.0f};
        unsigned storyArenaAiDecisionIndex_{0};
        bool storyArenaRewardPending_{false};
        bool pendingArenaSceneComplete_{false};
        bool ploukeHighPerformance_{false};
        TournamentCardState tournamentCard_{"rrvvfo"};
        int tournamentBonusRoll_{0};
        float tournamentCardRevealTime_{0.0f};
    ''')
    insert_after(runtime_hpp, "    CombatManualRegistry manual_;\n", fields, "ArenaEncounterRegistry arenaEncounters_", "story arena/card state")

    runtime_cpp = root / "src/core/runtime.cpp"
    # Load/save the Tournament Card.
    insert_after(runtime_cpp, "    game_.loadSave(data);\n", "    tournamentCard_ = data.tournamentCard;\n", "tournamentCard_ = data.tournamentCard", "load Tournament Card")
    insert_before(runtime_cpp, "    data.frontend.objectiveHistory = objectiveHistory_;", "    data.tournamentCard = tournamentCard_;\n", "data.tournamentCard = tournamentCard_", "save Tournament Card")
    # Reveal timer.
    insert_after(runtime_cpp, "    objectSwapPhaseTime_ = std::max(0.0f, objectSwapPhaseTime_ - dt);\n", "    tournamentCardRevealTime_ = std::max(0.0f, tournamentCardRevealTime_ - dt);\n", "tournamentCardRevealTime_ = std::max", "Tournament Card reveal timer")

    # Arena map selection is content-driven.
    map_replacement = dedent('''\
    const MapDefinition& RuntimeSession::map() const {
        if (roadsideFightActive_) return maps_.get("roadside_arena");
        if (game_.scene().kind == SceneKind::Arena) {
            if (arenaEncounters_.has(game_.scene().id)) return maps_.get(arenaEncounters_.get(game_.scene().id).mapId);
            return maps_.get("sage_training_arena");
        }
        return maps_.get(game_.chapter().primaryMap);
    }
    ''')
    replace_function(runtime_cpp, "const MapDefinition& RuntimeSession::map() const", map_replacement, "data-driven arena map")

    # Story Arena initialization alongside existing Sage training path.
    arena_branch_old = dedent('''\
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
    ''')
    arena_branch_new = dedent('''\
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
    ''')
    replace_once(runtime_cpp, arena_branch_old, arena_branch_new, "story Arena initialization")

    # Arena delegate before the existing tutorial arena logic.
    insert_after(runtime_cpp, "void RuntimeSession::tickArena(InputState& input, float dt) {\n", "    if (arenaEncounters_.has(game_.scene().id)) { tickStoryArena(input, dt); return; }\n", "tickStoryArena(input, dt)", "story arena delegate")

    arena_impl = dedent('''\

    void RuntimeSession::startStoryArena() {
        const auto& encounter = arenaEncounters_.get(game_.scene().id);
        storyArenaActive_ = true;
        storyPlayerStocksLost_ = 0;
        storyOpponentStocksLost_ = 0;
        storyArenaAiTimer_ = .45f;
        storyArenaAiDecisionIndex_ = 0;
        storyArenaRewardPending_ = false;
        pendingArenaSceneComplete_ = false;
        player_ = FighterState{game_.chapter().playableCharacter};
        opponent_ = FighterState{encounter.opponentId};
        player_.energy = encounter.startingEnergy;
        opponent_.energy = encounter.startingEnergy;
        playerPosition_ = encounter.playerStart;
        opponentPosition_ = encounter.opponentStart;
        movementState_ = {};
        opponentHeight_ = 0.0f;
        opponentVerticalVelocity_ = 0.0f;
        combatReadyAnimationTime_ = .30f;
        updateCombatFacing(0.0f, true);
    }

    void RuntimeSession::resetStoryArenaStock() {
        const auto& encounter = arenaEncounters_.get(game_.scene().id);
        player_ = FighterState{game_.chapter().playableCharacter};
        opponent_ = FighterState{encounter.opponentId};
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

    void RuntimeSession::tickStoryArena(InputState& input, float dt) {
        const auto& encounter = arenaEncounters_.get(game_.scene().id);
        if (!storyArenaActive_) { startStoryArena(); return; }

        FieldMovementConfig movement;
        movement.walkSpeed = 250.0f;
        movement.dashSpeed = 720.0f;
        movement.dashSeconds = .22f;
        movement.dashCooldownSeconds = .30f;
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
                ploukeHighPerformance_=storyOpponentStocksLost_>=2;
                finishStoryArena(false); return;
            }
            if(storyOpponentStocksLost_>=encounter.stockTarget){finishStoryArena(true);return;}
            if(storyPlayerStocksLost_>=encounter.stockTarget){
                storyPlayerStocksLost_=0;storyOpponentStocksLost_=0;
                resetStoryArenaStock();showGameplayNotice("OFFICIAL MATCH • TRY AGAIN",1.25f);return;
            }
            resetStoryArenaStock();
        }
    }

    void RuntimeSession::finishStoryArena(bool playerWon) {
        const auto& encounter=arenaEncounters_.get(game_.scene().id);
        storyArenaActive_=false;
        pendingArenaSceneComplete_=true;
        storyArenaRewardPending_=playerWon||encounter.resolution==StoryArenaResolution::PloukeStoryFinal;
        triggerAbilityAnimation("combat_relax",.28f);
        if(encounter.resolution==StoryArenaResolution::PloukeStoryFinal){
            beginTransientDialogue(ploukeHighPerformance_?"ch2_plouke_final_ringout":"ch2_plouke_final_exhausted",70);
        }else if(!encounter.postDialogueId.empty()) beginTransientDialogue(encounter.postDialogueId,70);
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
    ''')
    insert_before(runtime_cpp, "void RuntimeSession::showGameplayNotice", arena_impl, "void RuntimeSession::startStoryArena", "shared story arena implementation")

    # Transient dialogue 70 -> reward/advance.
    finish_text = read(runtime_cpp)
    if "case 70: resolveArenaRewardAndAdvance(); break;" not in finish_text:
        replace_once(runtime_cpp, "        case 32:\n            completeScene();\n            break;\n", "        case 32:\n            completeScene();\n            break;\n        case 70: resolveArenaRewardAndAdvance(); break;\n", "arena-result dialogue continuation")

    # Bonus wheel choice extends the existing choice system without changing route/optional choices.
    tick_choice = dedent('''\
    void RuntimeSession::tickChoice(InputState& input) {
        const std::size_t optionCount = choiceKind_ == 20 ? 5 : choiceKind_ == 4 ? 3 : 2;
        if (input.pressed(Action::MoveLeft) || input.pressed(Action::MoveUp))
            choiceIndex_ = choiceIndex_ == 0 ? optionCount - 1 : choiceIndex_ - 1;
        if (input.pressed(Action::MoveRight) || input.pressed(Action::MoveDown))
            choiceIndex_ = (choiceIndex_ + 1) % optionCount;
        if (!input.pressed(Action::Confirm) && !input.pressed(Action::Interact) && !input.pressed(Action::Cancel)) return;
        if ((choiceKind_ == 4 || choiceKind_ == 5 || choiceKind_ == 20) && input.pressed(Action::Cancel)) return;
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
        }
    }
    ''')
    replace_function(runtime_cpp, "void RuntimeSession::tickChoice(InputState& input)", tick_choice, "Tournament Card bonus wheel choice")

    # Card/stock view plus bonus wheel UI.
    card_sync = dedent('''\
        view_.tournamentCard = tournamentCard_;
        view_.tournamentCardVisible = tournamentCard_.acquired && tournamentCardRevealTime_ > 0.0f;
        view_.tournamentBonusRoll = tournamentBonusRoll_;
        view_.playerStocksLost = storyPlayerStocksLost_;
        view_.opponentStocksLost = storyOpponentStocksLost_;
        view_.stockTarget = arenaEncounters_.has(view_.sceneId) ? arenaEncounters_.get(view_.sceneId).stockTarget : 0;
        view_.recommendedLevel = arenaEncounters_.has(view_.sceneId) ? arenaEncounters_.get(view_.sceneId).recommendedLevel : 0;
        view_.officialTournamentMatch = arenaEncounters_.has(view_.sceneId) && arenaEncounters_.get(view_.sceneId).official;
    ''')
    insert_before(runtime_cpp, "    if (choiceKind_ == 1) {", card_sync, "view_.tournamentCard = tournamentCard_", "Tournament Card/stock view")
    insert_before(runtime_cpp, "    if (choiceKind_ == 1) {", "    if (choiceKind_ == 20) {\n        view_.choiceTitle = \"LEVEL UP • BONUS +\" + std::to_string(tournamentBonusRoll_) + \" • CHOOSE A STAT\";\n        view_.choiceOptions = {\"HP\",\"POWER\",\"DEFENSE\",\"SPEED\",\"FOCUS\"};\n    } else ", "view_.choiceTitle = \"LEVEL UP • BONUS +\"", "Tournament Card choice presentation")
    # Above insertion changes `else if` structure only if followed by `if`; normalize exact resulting token.
    text = read(runtime_cpp)
    text = text.replace("    } else     if (choiceKind_ == 1) {", "    } else if (choiceKind_ == 1) {")
    write(runtime_cpp, text)

    # Registration acquires the physical Tournament Card for Rrvvfo after Bark/Wade reunion and prep.
    completion_hook = "    const auto completedId = game_.scene().id;\n"
    acquisition = '''    if (completedId == "ch2_registration_card" && !tournamentCard_.acquired) {
        tournamentCard_.acquired = true;
        tournamentCardRevealTime_ = 4.5f;
        showGameplayNotice("TOURNAMENT CARD ACQUIRED", 1.35f);
    }
'''
    insert_after(runtime_cpp, completion_hook, acquisition, "completedId == \"ch2_registration_card\"", "Tournament Card acquisition")

    # Full Chapter 2 U9 flow replaces the U8 holding gate.
    chapter_cpp = root / "src/content/chapter_registry.cpp"
    old_ch2 = dedent('''\
        chapters_.emplace("rrvvfo_ch2", ChapterDefinition{
            "rrvvfo_ch2", "Tournament Grounds", "tournament_grounds", "rrvvfo",
            {
                {SceneKind::Cutscene, "tournament_gate_walk_in", "rrvvfo-ch2-arrival", "tournament-hub"},
                {SceneKind::Cutscene, "ch2_arrival_delay", "rrvvfo-ch2-delay", "tournament-hub"},
                {SceneKind::Exploration, "ch2_lost_bracket", "rrvvfo-ch2-bracket", "tournament-hub"},
                {SceneKind::Cutscene, "ch2_u8_gate", "rrvvfo-ch2-u8-gate", "tournament-hub"}
            }, ""
        });
    ''')
    new_ch2 = dedent('''\
        chapters_.emplace("rrvvfo_ch2", ChapterDefinition{
            "rrvvfo_ch2", "Tournament Grounds", "tournament_grounds", "rrvvfo",
            {
                {SceneKind::Cutscene, "tournament_gate_walk_in", "rrvvfo-ch2-arrival", "tournament-hub"},
                {SceneKind::Cutscene, "ch2_arrival_delay", "rrvvfo-ch2-delay", "tournament-hub"},
                {SceneKind::Exploration, "ch2_lost_bracket", "rrvvfo-ch2-bracket", "tournament-hub"},
                {SceneKind::Cutscene, "ch2_practice_brawl_intro", "rrvvfo-ch2-practice-intro", "tournament-hub"},
                {SceneKind::Arena, "ch2_practice_brawl", "rrvvfo-ch2-practice", "tournament-arena"},
                {SceneKind::Cutscene, "ch2_ninja_reunion", "rrvvfo-ch2-reunion", "tournament-hub"},
                {SceneKind::Exploration, "ch2_wade_shortcut", "rrvvfo-ch2-wade-race", "tournament-hub"},
                {SceneKind::Exploration, "ch2_cracked_ring", "rrvvfo-ch2-cracked-ring", "tournament-hub"},
                {SceneKind::Cutscene, "ch2_registration_card", "rrvvfo-ch2-card", "tournament-hub"},
                {SceneKind::Cutscene, "ch2_opening_ceremony", "rrvvfo-ch2-ceremony", "tournament-hub"},
                {SceneKind::Cutscene, "ch2_hailey_plouke", "rrvvfo-ch2-hailey-plouke", "tournament-arena"},
                {SceneKind::Arena, "ch2_vs_hamual", "rrvvfo-ch2-hamual", "tournament-arena"},
                {SceneKind::Exploration, "ch2_intermission_stillness", "rrvvfo-ch2-clue-stillness", "tournament-hub"},
                {SceneKind::Arena, "ch2_vs_daniel", "rrvvfo-ch2-daniel", "tournament-arena"},
                {SceneKind::Exploration, "ch2_intermission_positioning", "rrvvfo-ch2-clue-position", "tournament-hub"},
                {SceneKind::Cutscene, "ch2_bark_pouki", "rrvvfo-ch2-bark-pouki", "tournament-arena"},
                {SceneKind::Exploration, "ch2_intermission_timing", "rrvvfo-ch2-clue-timing", "tournament-hub"},
                {SceneKind::Arena, "ch2_vs_wade", "rrvvfo-ch2-wade", "tournament-arena"},
                {SceneKind::Exploration, "ch2_intermission_edge", "rrvvfo-ch2-clue-edge", "tournament-hub"},
                {SceneKind::Cutscene, "ch2_pre_plouke", "rrvvfo-ch2-pre-final", "tournament-hub"},
                {SceneKind::Arena, "ch2_vs_plouke", "rrvvfo-ch2-plouke", "tournament-arena"},
                {SceneKind::Cutscene, "ch2_plouke_reveal", "rrvvfo-ch2-reveal", "tournament-hub"},
                {SceneKind::Cutscene, "ch2_tournament_aftermath", "rrvvfo-ch2-aftermath", "tournament-hub"}
            }, "rrvvfo_ch3"
        });
    ''')
    replace_once(chapter_cpp, old_ch2, new_ch2, "complete Chapter 2 tournament flow")

    # U9 hub activities: Wade race, cracked ring, and four playable Plouke clue checks.
    exploration_cpp = root / "src/content/exploration_registry.cpp"
    u9_explore = dedent('''\

        ExplorationDefinition wadeRace;
        wadeRace.sceneId="ch2_wade_shortcut"; wadeRace.rule=ExplorationRuleKind::TimedCheckpointSequence;
        wadeRace.objective="WADE'S SHORTCUT • FIVE DISTRICTS"; wadeRace.detail="Finish the route. Beat 24 seconds for the optional target.";
        wadeRace.hasPlayerStart=true; wadeRace.playerStart={-650.0f,80.0f};
        wadeRace.sequenceMarkers={{-420,-430},{-80,-520},{180,-120},{-40,430},{430,420}};
        wadeRace.sequenceRadius=95; wadeRace.sequenceTargetSeconds=24.0f; wadeRace.sequenceRequiresInteract=false;
        wadeRace.completionDialogueId="ch2_wade_shortcut_result"; scenes_.emplace(wadeRace.sceneId,wadeRace);

        ExplorationDefinition cracked;
        cracked.sceneId="ch2_cracked_ring"; cracked.rule=ExplorationRuleKind::InteractionSequence;
        cracked.objective="THE CRACKED RING • INSPECT THREE SUPPORTS"; cracked.detail="Bark and Wade are watching the practice ring.";
        cracked.hasPlayerStart=true; cracked.playerStart={-260,430};
        cracked.sequenceMarkers={{-260,520},{-75,620},{110,510}}; cracked.sequenceLabels={"WEST SUPPORT","SOUTH SUPPORT","EAST SUPPORT"};
        cracked.sequenceDialogueIds={"ch2_crack_west","ch2_crack_south","ch2_crack_east"}; cracked.sequenceRadius=82;
        cracked.completionDialogueId="ch2_cracked_ring_result"; scenes_.emplace(cracked.sceneId,cracked);

        const auto addClue=[&](std::string id,std::string objective,Vec2 point,std::string dialogue){
            ExplorationDefinition clue; clue.sceneId=id; clue.rule=ExplorationRuleKind::InteractionSequence; clue.objective=objective;
            clue.detail="Observe the clue before returning to the arena gate."; clue.hasPlayerStart=true; clue.playerStart={430,0};
            clue.sequenceMarkers={point}; clue.sequenceLabels={"OBSERVE"}; clue.sequenceDialogueIds={dialogue}; clue.sequenceRadius=95;
            scenes_.emplace(clue.sceneId,clue);
        };
        addClue("ch2_intermission_stillness","PLOUKE STUDY • STILLNESS",{560,-420},"ch2_clue_stillness");
        addClue("ch2_intermission_positioning","PLOUKE STUDY • POSITIONING",{-80,430},"ch2_clue_positioning");
        addClue("ch2_intermission_timing","PLOUKE STUDY • TIMING",{470,455},"ch2_clue_timing");
        addClue("ch2_intermission_edge","PLOUKE STUDY • RING EDGE",{840,80},"ch2_clue_edge");
    ''')
    insert_before(exploration_cpp, "}\n\nconst ExplorationDefinition& ExplorationRegistry::get", u9_explore, "wadeRace.sceneId=\"ch2_wade_shortcut\"", "Tournament prep + clue gameplay")

    dialogue_cpp = root / "src/content/dialogue_registry.cpp"
    u9_dialogue = dedent('''\

        scenes_.emplace("ch2_practice_brawl_intro", std::vector<DialogueLine>{
            {"THE SAGE", "Stay near the practice ring. I need to check something before your first brawl."},
            {"RRVVFO", "He’s probably spying on ladies again. Perv."},
            {"PRACTICE RING FIGHTER", "Alright, let’s fight. A little training’s good for ya, boy."},
            {"RRVVFO", "Alright. Let’s do it."}
        });
        scenes_.emplace("ch2_practice_brawl_result", std::vector<DialogueLine>{{"PRACTICE RING FIGHTER","Not bad. That card system will make more sense once registration catches up."}});
        scenes_.emplace("ch2_ninja_reunion", std::vector<DialogueLine>{
            {"WADE", "There you are! We’ve been looking all over—"},
            {"BARK", "We just got here, Wade. We didn’t even start looking yet."},
            {"RRVVFO", "Wade, you really are dense, huh?"},
            {"WADE", "Looks like a big ninja reunion. The three of us."},
            {"RRVVFO", "But only one of us can win."},
            {"BARK", "It’s me."},
            {"WADE", "It’s me. I’m the most tactical."},
            {"BARK", "In what world?"},
            {"RRVVFO", "In what world?"}
        });
        scenes_.emplace("ch2_wade_shortcut_result", std::vector<DialogueLine>{{"WADE","Okay, that shortcut was faster than I thought."},{"BARK","The east support is cracked. Look."}});
        scenes_.emplace("ch2_crack_west", std::vector<DialogueLine>{{"RRVVFO","The damage is pushing outward. Something hit this from inside the ring."}});
        scenes_.emplace("ch2_crack_south", std::vector<DialogueLine>{{"RRVVFO","Footprints stop halfway. That’s not normal."}});
        scenes_.emplace("ch2_crack_east", std::vector<DialogueLine>{{"WADE","These shoe marks are different from the other ones."},{"RRVVFO","Yeah. More than one pattern."}});
        scenes_.emplace("ch2_cracked_ring_result", std::vector<DialogueLine>{
            {"RRVVFO","We fixed them, but it has to be a fighter. All the spectators are kept away from the rings until the tournament starts."},
            {"RRVVFO","We shouldn’t make a big deal out of it yet and worry the guests."},
            {"RRVVFO","After the tournament, I’ll figure out who did it. Don’t stress about it, Bark."},
            {"BARK","Alright. You find the culprit. I’m gonna practice."}
        });
        scenes_.emplace("ch2_registration_card", std::vector<DialogueLine>{
            {"REGISTRATION STAFF","Cards, please."},
            {"RRVVFO","What card?"},
            {"WADE","I thought you had ours."},
            {"BARK","Why would he have ours?"},
            {"REGISTRATION STAFF","Spare Tournament Cards. One each. Keep them with you."}
        });
        scenes_.emplace("ch2_opening_ceremony", std::vector<DialogueLine>{
            {"ANNOUNCER","Welcome to the local tournament! Hamual, Daniel, Hailey, Bark, Wade, Pouki, Plouke—and first-time entrant Rrvvfo!"},
            {"CROWD","The former champion Hamual towers over the entrance line. Daniel looks ordinary enough to be tournament staff."},
            {"RRVVFO","And Sage is missing. Shocking."},
            {"ANNOUNCER","First preliminary: Hailey versus Plouke!"}
        });
        scenes_.emplace("ch2_hailey_plouke", std::vector<DialogueLine>{
            {"HAILEY","Stop staring and defend yourself!"},
            {"RRVVFO","This guy reminds me of the Sage. I hope they never meet."},
            {"PLOUKE","I might not have won if it weren’t for that pebble I placed in front of me, just in case I got distracted."},
            {"ANNOUNCER","Official matches are first to three. A knockout or crossing the ring boundary removes a stock."},
            {"RRVVFO","So ring-outs matter. I should stay away from the edge."}
        });
        scenes_.emplace("ch2_hamual_intro", std::vector<DialogueLine>{{"ANNOUNCER","Opening round: Rrvvfo versus former champion Hamual!"}});
        scenes_.emplace("ch2_hamual_result", std::vector<DialogueLine>{{"ANNOUNCER","Rrvvfo advances!"},{"RRVVFO","Told you."}});
        scenes_.emplace("ch2_daniel_intro", std::vector<DialogueLine>{{"ANNOUNCER","Next match: Rrvvfo versus Daniel!"}});
        scenes_.emplace("ch2_daniel_result", std::vector<DialogueLine>{{"ANNOUNCER","Rrvvfo advances again!"}});
        scenes_.emplace("ch2_bark_pouki", std::vector<DialogueLine>{
            {"ANNOUNCER","Pouki wins! Bark held the center, survived the guard break, and nearly landed one final counter!"},
            {"BARK","He changed rhythm every time I settled. My last counter was the first opening he gave me."},
            {"RRVVFO","If he got Bark that easily, he must be really strong."},
            {"WADE","You still have to beat me first."}
        });
        scenes_.emplace("ch2_wade_intro", std::vector<DialogueLine>{{"WADE","Guess the bracket really wanted this."},{"RRVVFO","You may be fast, but you’re slow in the brain."},{"WADE","I’m fast, not slow."}});
        scenes_.emplace("ch2_wade_result", std::vector<DialogueLine>{{"WADE","You won! Yay!"},{"RRVVFO","I don’t think that’s supposed to be your reaction."},{"ANNOUNCER","Plouke has defeated Pouki in the opposite semifinal. The final is set!"}});
        scenes_.emplace("ch2_clue_stillness", std::vector<DialogueLine>{{"OLD COMPETITOR","Plouke barely moves until the other fighter commits first."},{"RRVVFO","So he waits for people to slip up."}});
        scenes_.emplace("ch2_clue_positioning", std::vector<DialogueLine>{{"WORKER","Every fighter who faces Plouke ends up standing exactly where he wants."},{"RRVVFO","So I have to be a little more cautious."}});
        scenes_.emplace("ch2_clue_timing", std::vector<DialogueLine>{{"BARK","Plouke doesn’t overpower people immediately. He waits until their strongest option becomes predictable."},{"RRVVFO","If he has Bark on edge, I should take him a little more seriously."}});
        scenes_.emplace("ch2_clue_edge", std::vector<DialogueLine>{{"WADE","Plouke always looks at the edge. Maybe he’s in love with it."},{"RRVVFO","Maybe that gave me some clues. He probably rings people out a lot."},{"RRVVFO","I should observe the matches."}});
        scenes_.emplace("ch2_pre_plouke", std::vector<DialogueLine>{
            {"BARK","Stillness, positioning, timing, and the ring edge. We verified every pattern Plouke uses."},
            {"WADE","Don’t chase his retreat. Cut through the center and make him choose first."},
            {"RRVVFO","And Sage is still gone. Great timing."},
            {"PLOUKE","You used too much energy reaching this round."},
            {"RRVVFO","I can win this with my hands tied up."},
            {"PLOUKE","That confidence is exactly why you’re tired."},
            {"RRVVFO","Keep talking. It’ll make losing more embarrassing."}
        });
        scenes_.emplace("ch2_plouke_final_ringout", std::vector<DialogueLine>{
            {"RRVVFO","I beat you in the beam! Haha—"},{"RRVVFO","WAIT, I’M ON THE GRASS! AHH!"},{"PLOUKE","I beat you."},
            {"ANNOUNCER","Rrvvfo wins the beam clash—but Plouke wins by ring-out!"}
        });
        scenes_.emplace("ch2_plouke_final_exhausted", std::vector<DialogueLine>{
            {"RRVVFO","No! I lost the clash... I’m out of energy."},{"PLOUKE","The match is over."},{"ANNOUNCER","Plouke wins the tournament!"}
        });
        scenes_.emplace("ch2_plouke_reveal", std::vector<DialogueLine>{
            {"RRVVFO","Who are you?"},{"PLOUKE","You really did skim the disguise section."},{"RRVVFO","...No."},{"THE SAGE","Plouke was me."},
            {"RRVVFO","Now that explains what you were doing during your fight with Hailey."},{"RRVVFO","I hate how planned ahead you are."}
        });
        scenes_.emplace("ch2_tournament_aftermath", std::vector<DialogueLine>{{"ANNOUNCER","The local tournament is complete. Grounds remain open while staff begins cleanup."}});
    ''')
    insert_before(dialogue_cpp, "}\n\nconst std::vector<DialogueLine>& DialogueRegistry::get", u9_dialogue, "scenes_.emplace(\"ch2_ninja_reunion\"", "Tournament core dialogue")

    # Cutscene data for U9 mandatory story beats.
    cutscene_cpp = root / "src/content/cutscene_registry.cpp"
    u9_cutscenes = dedent('''\

        cutscenes_.emplace("ch2_practice_brawl_intro", CutsceneDefinition{"ch2_practice_brawl_intro",CutsceneTier::Directed,
            {{"sage_exit","SAGE","Sage slips behind the waiting tent.","practice_wide","walk"},{"fighter_steps","PRACTICE RING FIGHTER","The practice fighter steps into the ring.","fighter_medium","ready"}},
            {{"rrvvfo",{-240,430},0,true},{"sage",{340,455},90,true},{"practice_fighter",{-20,520},180,true}},
            {{0,CutsceneActionKind::MoveTo,"sage","",{470,455},145,.50f},{1,CutsceneActionKind::CameraFocus,"","",{-80,430},0,.15f,32,760,320,41,"practice"}}});
        cutscenes_.emplace("ch2_ninja_reunion", CutsceneDefinition{"ch2_ninja_reunion",CutsceneTier::Directed,
            {{"reunion","WADE","Wade and Bark enter from the plaza while Rrvvfo keeps moving.","walking_three_shot","walk"}},
            {{"rrvvfo",{-200,410},0,true},{"wade",{-480,300},70,true},{"bark",{-520,360},65,true}},
            {{0,CutsceneActionKind::CameraTrack,"rrvvfo","",{},0,.15f,35,860,350,42,"reunion"}}});
        cutscenes_.emplace("ch2_registration_card", CutsceneDefinition{"ch2_registration_card",CutsceneTier::Directed,
            {{"cards","REGISTRATION STAFF","All three ninjas receive spare Tournament Cards.","counter_three_shot","card_handoff"}},
            {{"rrvvfo",{-800,20},90,true},{"wade",{-860,-60},80,true},{"bark",{-850,90},95,true}},
            {{0,CutsceneActionKind::TriggerWorldEvent,"","",{},0,.10f,0,0,0,0,"tournament_card_handoff"},{0,CutsceneActionKind::CameraFocus,"","",{-760,0},0,.15f,35,760,310,40,"card"}}});
        cutscenes_.emplace("ch2_opening_ceremony", CutsceneDefinition{"ch2_opening_ceremony",CutsceneTier::Major,
            {{"lineup","ANNOUNCER","Introduce the bracket and first preliminary.","arena_establish","lineup"}},
            {{"rrvvfo",{700,100},90,true}},{{0,CutsceneActionKind::CameraFocus,"","",{0,0},0,.25f,30,1080,430,44,"ceremony"}}});
        cutscenes_.emplace("ch2_hailey_plouke", CutsceneDefinition{"ch2_hailey_plouke",CutsceneTier::Major,
            {{"prelim","","Show Plouke's stillness and pebble ring-out visually.","spectator_track","fight_observe"}},
            {{"rrvvfo",{520,-430},140,true},{"hailey",{-150,0},90,true},{"plouke",{150,0},-90,true}},
            {{0,CutsceneActionKind::TriggerWorldEvent,"","",{},0,.10f,0,0,0,0,"plouke_pebble_ringout"}}});
        cutscenes_.emplace("ch2_bark_pouki", CutsceneDefinition{"ch2_bark_pouki",CutsceneTier::Major,
            {{"bark_center","","Bark controls center.","ring_wide","block"},{"pouki_break","","Pouki changes rhythm and breaks defense.","ring_track","heavy"},{"last_counter","","Bark nearly lands the final counter.","reaction_close","counter"}},
            {{"rrvvfo",{520,-430},140,true},{"bark",{-180,0},90,true},{"pouki",{180,0},-90,true},{"wade",{600,-380},150,true}},
            {{0,CutsceneActionKind::Wait,"","",{},0,.35f},{1,CutsceneActionKind::TriggerWorldEvent,"","",{},0,.10f,0,0,0,0,"bark_guard_break"},{2,CutsceneActionKind::TriggerWorldEvent,"","",{},0,.10f,0,0,0,0,"bark_last_counter"}}});
        cutscenes_.emplace("ch2_pre_plouke", CutsceneDefinition{"ch2_pre_plouke",CutsceneTier::Directed,
            {{"quiet_prep","BARK","Quiet contestant-lane preparation before the final.","bench_three_shot","idle"}},
            {{"rrvvfo",{500,455},20,true},{"bark",{430,500},-20,true},{"wade",{590,500},20,true}},
            {{0,CutsceneActionKind::CameraTrack,"rrvvfo","",{},0,.15f,28,780,315,40,"quiet_prep"},{2,CutsceneActionKind::Expression,"rrvvfo","",{},0,.10f,0,0,0,0,"focused"}}});
        cutscenes_.emplace("ch2_plouke_reveal", CutsceneDefinition{"ch2_plouke_reveal",CutsceneTier::Major,
            {{"reveal","SAGE","Plouke reveals himself as Sage after the final.","reveal_orbit","reveal"}},
            {{"rrvvfo",{720,80},90,true},{"sage",{820,80},-90,true}},
            {{3,CutsceneActionKind::TriggerWorldEvent,"sage","",{},0,.10f,0,0,0,0,"plouke_to_sage"},{4,CutsceneActionKind::CameraFocus,"","rrvvfo",{},0,.15f,24,660,280,39,"reaction"}}});
        cutscenes_.emplace("ch2_tournament_aftermath", CutsceneDefinition{"ch2_tournament_aftermath",CutsceneTier::Directed,
            {{"cleanup","ANNOUNCER","Tournament transitions to cleanup and later investigation state.","hub_wide","cleanup"}},
            {{"rrvvfo",{600,100},90,true}},{{0,CutsceneActionKind::CameraFocus,"","",{0,0},0,.20f,35,1000,400,43,"cleanup"}}});
    ''')
    insert_before(cutscene_cpp, "// U7A: retrofit existing Chapter 1 beats", u9_cutscenes, "cutscenes_.emplace(\"ch2_practice_brawl_intro\"", "Tournament core cutscenes")

    write(root / "docs/U9_TOURNAMENT_CORE.md", dedent('''\
    # U9 — Minimum Complete Tournament Loop

    Scope is intentionally narrow:
    - Practice brawl introduces real tournament combat.
    - Required official order: Rrvvfo vs Hamual -> Rrvvfo vs Daniel -> Bark vs Pouki spectator event -> Rrvvfo vs Wade -> Plouke final.
    - Hailey vs Plouke is the opening spectator preliminary.
    - Official player fights are first-to-three stocks; KO or leaving the authored ring boundary removes a stock.
    - Losing a mandatory pre-final official match safely retries the match; Run/forfeit is not offered.
    - Plouke always wins the tournament story. Player performance selects the established beam/ring-out vs exhausted resolution.
    - Tournament Card uses the existing StoryProgressionSystem and save schema 6; all stats rise on level, then +1/+2/+3 is assigned to one stat.
    - No Shots of Agony unlock. No separate tournament engine.
    '''))
    mark(root, "U9")


def apply_u10(root: pathlib.Path):
    print("\n=== U10 — TOURNAMENT GOLDEN POLISH ===")

    runtime_hpp = root / "src/core/runtime.hpp"
    insert_before(runtime_hpp, "    bool playerFalling{false};\n", "    std::string tournamentWorldState;\n", "tournamentWorldState", "Tournament world-state view")

    runtime_cpp = root / "src/core/runtime.cpp"

    # U10 Golden Gate: Chapter-1 transient fields may never overwrite/pollute a Chapter-2 save.
    # Keep arbitrary story flags (including completed Chapter 1 history), then update only the active chapter's transient data.
    save_snapshot = dedent('''\
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

        // Generic authored sequence progress belongs to the current scene only.
        const std::string sceneProgressPrefix = "scene_progress=" + game_.story().chapterId + ":" + game_.scene().id + ":";
        data.story.flags.erase(std::remove_if(data.story.flags.begin(), data.story.flags.end(), [&](const std::string& flag){
            return flag.rfind(sceneProgressPrefix, 0) == 0;
        }), data.story.flags.end());
        if (sceneSequenceProgress_ > 0) data.story.flags.push_back(sceneProgressPrefix + std::to_string(sceneSequenceProgress_));

        data.tournamentCard = tournamentCard_;
        data.frontend.objectiveHistory = objectiveHistory_;
        data.qol = qolSettings_;
        data.inputPreset = inputPreset;
        return data;
    }
    ''')
    replace_function(runtime_cpp, "SaveData RuntimeSession::saveSnapshot(const std::string& inputPreset) const", save_snapshot, "chapter-safe Golden Gate save snapshot")

    # Tournament world state is shared presentation data, derived from Story state rather than platform code.
    tournament_state = dedent('''\
        view_.tournamentWorldState.clear();
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
    ''')
    insert_before(runtime_cpp, "    if (choiceKind_ == 20) {", tournament_state, "view_.tournamentWorldState.clear", "shared Tournament world-state presentation")

    # Lightweight world-life changes use already-supported marker types. They do not create a separate hub simulation.
    tournament_markers = dedent('''\
        if (view_.chapterId == "rrvvfo_ch2" && view_.presentationStageId == "tournament-hub") {
            if (view_.tournamentWorldState == "festival") {
                view_.worldMarkers.push_back({"festival_delivery_cart", {-180.0f,-540.0f}, "delivery-cart", false});
            } else if (view_.tournamentWorldState == "tournament") {
                view_.worldMarkers.push_back({"medical_supply_cart", {650.0f,500.0f}, "parked-cart", true});
                view_.worldMarkers.push_back({"bracket_update", {-630.0f,170.0f}, "work-lane", true});
            } else if (view_.tournamentWorldState == "final") {
                view_.worldMarkers.push_back({"final_waiting_supply", {470.0f,455.0f}, "parked-cart", true});
            } else if (view_.tournamentWorldState == "cleanup") {
                view_.worldMarkers.push_back({"cleanup_cart", {260.0f,-430.0f}, "delivery-cart", true});
                view_.worldMarkers.push_back({"repair_marker", {-80.0f,430.0f}, "work-lane", true});
            }
        }
    ''')
    insert_before(runtime_cpp, "    // Lightweight shared ability VFX markers", tournament_markers, "festival_delivery_cart", "Tournament hub world-life states")

    replace_once(runtime_cpp,
        '            "BUILD • 3.0R / UPDATE 6 GOLDEN GATE"',
        '            "BUILD • 3.0R / UPDATE 10 TOURNAMENT GOLDEN"',
        "U10 pause build label")

    # macOS: stop exposing internal chapter numbers in the normal objective header, and show a real compact Tournament Card reveal.
    mac = root / "src/platform/macos/main.mm"
    replace_once(mac,
        '_objectivePanel.kicker=ns("CHAPTER 1 • "+v.currentArea);',
        '_objectivePanel.kicker=ns(v.currentArea);',
        "continuous Story objective header")

    old_notice = '''    const bool hasCombatFeedback=!v.combatFeedback.empty();
    _noticePanel.hidden=(v.gameplayNotice.empty()&&!hasCombatFeedback)||v.dialogueVisible||v.trainingManualVisible||v.choiceVisible||v.qteVisible||v.pauseVisible;
    _noticePanel.kicker=hasCombatFeedback?@"COMBAT IMPACT":(v.flowCancelReady?@"COMBAT TIMING":@"ROAD MOMENT");_noticePanel.body=ns(hasCombatFeedback?v.combatFeedback:v.gameplayNotice);[_noticePanel setNeedsDisplay:YES];'''
    new_notice = '''    const bool hasCombatFeedback=!v.combatFeedback.empty();
    const bool showTournamentCard=v.tournamentCardVisible;
    _noticePanel.hidden=(v.gameplayNotice.empty()&&!hasCombatFeedback&&!showTournamentCard)||v.dialogueVisible||v.trainingManualVisible||v.choiceVisible||v.qteVisible||v.pauseVisible;
    if(showTournamentCard){
        const auto stats=px::StoryProgressionSystem::statsFor(v.tournamentCard);
        _noticePanel.kicker=@"TOURNAMENT CARD";
        _noticePanel.body=ns("RRVVFO  •  LV "+std::to_string(v.tournamentCard.level)+"  •  XP "+std::to_string(v.tournamentCard.xp)+
            "  •  HP "+std::to_string(stats.hp)+"  PWR "+std::to_string(stats.power)+"  DEF "+std::to_string(stats.defense)+
            "  SPD "+std::to_string(stats.speed)+"  FOC "+std::to_string(stats.focus));
    }else{
        _noticePanel.kicker=hasCombatFeedback?@"COMBAT IMPACT":(v.flowCancelReady?@"COMBAT TIMING":@"STORY MOMENT");
        _noticePanel.body=ns(hasCombatFeedback?v.combatFeedback:v.gameplayNotice);
    }
    [_noticePanel setNeedsDisplay:YES];'''
    replace_once(mac, old_notice, new_notice, "macOS Tournament Card reveal")

    # Old 3DS: card reveal lives on the touch screen; no future abilities are shown.
    ui3 = root / "src/platform/3ds/legacy_ui_3ds.cpp"
    old_hotbar = '''    if (view.hotbarVisible && !view.hotbar.empty()) {'''
    new_hotbar = '''    if (view.tournamentCardVisible) {
        const auto stats = StoryProgressionSystem::statsFor(view.tournamentCard);
        remakePanel(10, 87, 300, 91, kPaper, kYellow, true);
        label("TOURNAMENT CARD", 24, 96, .29f, kRed);
        fitted("RRVVFO   LV " + std::to_string(view.tournamentCard.level) + "   XP " + std::to_string(view.tournamentCard.xp),
               24, 117, 272, .34f, .26f, kInk);
        fitted("HP " + std::to_string(stats.hp) + "  PWR " + std::to_string(stats.power) + "  DEF " + std::to_string(stats.defense),
               24, 139, 272, .27f, .22f, kBlue);
        fitted("SPD " + std::to_string(stats.speed) + "  FOC " + std::to_string(stats.focus),
               24, 158, 272, .27f, .22f, kBlue);
    } else if (view.hotbarVisible && !view.hotbar.empty()) {'''
    replace_once(ui3, old_hotbar, new_hotbar, "3DS bottom-screen Tournament Card")

    # More authored place-making: waiting/medical/photo/best-time/repair dressing is presentation only.
    world_cpp = root / "src/content/world_presentation_registry.cpp"
    gold_dressing = '''    // U10 Golden Pass: readable tournament districts and intermission landmarks.
    box(hub,"tg_waiting_tent",470,55,455,210,110,170,rgb(224,207,170),PresentationDetailTier::Essential);
    box(hub,"tg_waiting_flag",470,120,455,190,32,8,rgb(199,54,45),PresentationDetailTier::Full);
    box(hub,"tg_medical_tent",650,55,500,190,110,150,rgb(231,231,221),PresentationDetailTier::Essential);
    box(hub,"tg_medical_cross",650,80,420,34,34,8,rgb(199,54,45),PresentationDetailTier::Full);
    box(hub,"tg_photo_stand",170,45,-555,150,90,60,rgb(207,160,75),PresentationDetailTier::Full);
    box(hub,"tg_race_best_time_board",60,66,360,130,132,20,rgb(78,62,48),PresentationDetailTier::Essential);
    box(hub,"tg_bracket_update_board",-630,82,170,170,164,20,rgb(76,61,47),PresentationDetailTier::Essential);
    for(int i=0;i<5;++i) {
        box(hub,"tg_final_banner_"+std::to_string(i),420+i*95,98,-115,54,70,8,rgb(192,54+(i%2)*18,47),PresentationDetailTier::Full);
    }
'''
    insert_before(world_cpp, "        hub.ambientActors = {", gold_dressing, "tg_waiting_tent", "Tournament Golden district dressing")

    # Add extra non-blocking crowd silhouettes to make intermissions feel populated without expensive simulation.
    crowd_anchor = '        {"practice_fighter","road_fighter",{-10,0,510,1,1,1,180},PresentationDetailTier::Full}\n'
    extra_crowd = '''        {"practice_fighter","road_fighter",{-10,0,510,1,1,1,180},PresentationDetailTier::Full},
        {"tournament_fan_d","ambient_fan",{250,0,-610,1,1,1,20},PresentationDetailTier::Full},
        {"tournament_fan_e","ambient_fan",{540,0,-580,1,1,1,-25},PresentationDetailTier::Full},
        {"practice_student_a","ambient_student",{-20,0,610,1,1,1,170},PresentationDetailTier::Full},
        {"practice_student_b","ambient_student",{140,0,575,1,1,1,-165},PresentationDetailTier::Full},
        {"medical_worker","checkpoint_worker",{680,0,420,1,1,1,30},PresentationDetailTier::Full}
'''
    replace_once(world_cpp, crowd_anchor, extra_crowd, "Tournament Golden crowd layer")

    # Keep U9 fixed opponent identities/expected levels. Document the soft-scaling cap rather than silently changing combat data.
    write(root / "docs/U10_TOURNAMENT_GOLDEN.md", dedent('''\
    # U10 — Tournament Golden Pass

    ## Presentation
    - Tournament hub now reads as entrance / registration / market / practice / spectator / contestant / arena-gate districts.
    - Waiting tent, medical tent, photo stand, bracket update board, race best-time board, extra crowd silhouettes and cleanup/repair states add life without a second hub engine.
    - Intermissions visibly change between festival, active tournament, final preparation and cleanup.
    - macOS and Old 3DS receive a Tournament Card reveal using the same saved card data. The card never spoils future abilities.
    - Normal Story objective headers favor the current area rather than internal chapter numbers.

    ## Gameplay locks
    - Official opponent identities remain Hamual -> Daniel -> Wade -> Plouke, with Bark vs Pouki as the spectator set piece.
    - Fixed recommended levels remain authoritative. If future balancing adds soft scaling, it may only add +1 opponent level when the player is severely overleveled; U10 does not silently scale today.
    - No Run/forfeit command is introduced to official matches.
    - Plouke remains the story winner.
    - Shots of Agony remains unavailable until Rrvvfo actually learns it later in Story.

    ## Save safety
    - Chapter-1 transient progression is updated only while Chapter 1 is active.
    - Crossing into Chapter 2 preserves completed Chapter-1 flags instead of re-authoring them from reset runtime fields.
    - Tournament Card and physical free-swap props remain portable SaveData state.
    '''))

    write(root / "docs/U7_U10_GOLDEN_GATES.md", dedent('''\
    # U7–U10 Milestone Gates

    Every milestone follows: BUG CHECK -> exact 1:1 simplification where provably safe -> REGRESSION CHECK -> CONTINUE.

    ## U7A gate — Cinematic Foundation
    - shared MoveTo / FaceActor / PlayAnimation / LookAt / CameraTrack / CameraFocus / WorldEvent / Wait / Dialogue / Expression actions;
    - Chapter 1 story text/order preserved;
    - exact 0.75s Legacy idle untouched;
    - dedicated combat_retreat untouched;
    - control returns without a chapter/menu break.

    ## U7B gate — Ability / Rrvvfo Presentation
    - Object Swap is literal object-position exchange, never a dash;
    - free swap is 225 range / 4.5s cooldown / sparse safe props;
    - required swaps ignore the free-use cooldown;
    - Object Swap gold, Lens purple + short recoverable blindness;
    - Shots of Agony remains unavailable in Chapter 1.

    ## U7 Golden Regression Gate — HARD BLOCK BEFORE ACCEPTING U8
    Verify full Chapter 1: tutorial full/quick/skip, river, all three routes, relay, transport, cart, optional roadside story, collapse, checkpoint, Lens/detour, sign story, outskirts and seamless Tournament handoff. Verify save/load and Mac/Linux/real Old 3DS. Source staging may be batched ahead, but acceptance is NOT green until these checks pass.

    ## U8 gate — Continuous Tournament Hub
    - no Chapter 1 -> Chapter 2 menu;
    - arrival walks into the hub and Sage rejoins in motion;
    - one shared tournament_grounds map with authored districts;
    - Lost Bracket teaches navigation without mission-select presentation.

    ## U9 gate — Minimum Complete Tournament Loop
    - same px_core combat/runtime;
    - practice brawl, Hamual, Daniel, Bark/Pouki, Wade, Plouke final;
    - first-to-three official stocks and ring-outs;
    - Tournament Card progression/save;
    - Cracked Ring has clues only, no Chapter-2 saboteur reveal;
    - story always resolves with Plouke winning.

    ## U10 gate — Tournament Golden
    - tournament phases/world life/intermission dressing;
    - Tournament Card readable on Mac and 3DS;
    - Chapter-1 flags remain stable in Chapter-2 saves;
    - accessibility and 3DS reductions preserve behavior;
    - final Mac/Linux/Old 3DS smoke + save/load + suspend/resume + performance pass before declaring Tournament Golden.
    '''))

    mark(root, "U10")


def write_verifier(root: pathlib.Path):
    verifier = dedent('''\
    #!/usr/bin/env python3
    from __future__ import annotations
    import pathlib
    import re
    import sys

    root = pathlib.Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else pathlib.Path.cwd().resolve()
    failures = []
    passes = []

    def text(rel):
        p = root / rel
        if not p.exists():
            failures.append(f"missing {rel}")
            return ""
        return p.read_text(encoding="utf-8")

    def require(rel, needle, label):
        value = text(rel)
        if needle in value: passes.append(label)
        else: failures.append(label)

    # Architecture / milestone locks.
    require("src/content/cutscene_registry.hpp", "enum class CutsceneActionKind", "U7A shared cinematic action language")
    require("src/core/runtime.cpp", "applyCutsceneActions", "U7A shared RuntimeSession staging")
    require("src/core/runtime.cpp", "objectSwapCooldownTime_ = 4.5f", "U7B long free Object Swap cooldown")
    require("src/core/runtime.cpp", "kFreeSwapRange = 225.0f", "U7B short free Object Swap range")
    require("src/core/runtime.cpp", "lensBlindnessAmount", "U7B Lens blindness presentation")
    require("src/content/chapter_registry.cpp", 'chapters_.emplace("rrvvfo_ch2"', "U8 continuous Chapter 2 content")
    require("src/content/map_registry.cpp", 'tournament.id = "tournament_grounds"', "U8 shared tournament hub map")
    require("src/content/arena_encounter_registry.cpp", '"ch2_vs_hamual"', "U9 Hamual match")
    require("src/content/arena_encounter_registry.cpp", '"ch2_vs_daniel"', "U9 Daniel match")
    require("src/content/arena_encounter_registry.cpp", '"ch2_vs_wade"', "U9 Wade match")
    require("src/content/arena_encounter_registry.cpp", '"ch2_vs_plouke"', "U9 Plouke final")
    require("src/core/save.hpp", "kSchemaVersion = 6", "U9 Tournament Card save schema")
    require("src/platform/3ds/legacy_ui_3ds.cpp", "TOURNAMENT CARD", "U10 3DS Tournament Card")
    require("src/platform/macos/main.mm", "TOURNAMENT CARD", "U10 macOS Tournament Card")
    require("src/core/runtime.cpp", 'game_.story().chapterId == "rrvvfo_ch1"', "U10 chapter-safe save scoping")
    require("docs/U7_U10_GOLDEN_GATES.md", "U7 Golden Regression Gate", "milestone acceptance gates")

    # One-engine rule.
    cmake = text("CMakeLists.txt")
    if "src/content/arena_encounter_registry.cpp" in cmake and cmake.count("add_library(px_core") == 1:
        passes.append("one shared px_core architecture")
    else:
        failures.append("one shared px_core architecture")
    for forbidden in ["chapter2_engine", "tournament_engine", "TournamentEngine", "Chapter2Engine"]:
        if forbidden in "\\n".join([text("src/core/runtime.cpp"), text("src/content/chapter_registry.cpp"), cmake]):
            failures.append(f"forbidden separate engine token: {forbidden}")

    # Object Swap can never regress into a dash mechanic/name in gameplay/content.
    gameplay_sources = "\\n".join([
        text("src/core/runtime.cpp"), text("src/core/runtime.hpp"),
        text("src/content/exploration_registry.cpp"), text("src/content/chapter_registry.cpp")
    ]).lower()
    if "object_swap_dash" in gameplay_sources or "object swap dash" in gameplay_sources:
        failures.append("Object Swap incorrectly named/implemented as a dash")
    else:
        passes.append("Object Swap literal-swap naming lock")

    # Chapter 1 never exposes Shots of Agony.
    hotbar = text("src/core/ability_hotbar.cpp")
    start = hotbar.find("AbilityHotbarCatalog::rrvvfoChapter1")
    end = hotbar.find("AbilityHotbarCatalog::rrvvfoWithShotsOfAgony")
    ch1_hotbar = hotbar[start:end] if start >= 0 and end > start else hotbar
    if "shotsOfAgony" in ch1_hotbar or "SHOTS OF AGONY" in ch1_hotbar:
        failures.append("Shots of Agony exposed in Chapter 1 hotbar")
    else:
        passes.append("Shots of Agony remains unavailable in Chapter 1")

    # Color language.
    require("src/platform/macos/main.mm", "{1.0f,.78f,.12f,.56f}", "Object Swap gold visual language")
    require("src/platform/macos/main.mm", "{.63f,.22f,.92f,.68f}", "Lens purple visual language")

    # Exact legacy idle protection: authored source must still state 0.75s idle.
    anim = text("assets/characters/rrvvfo/rrvvfo-core.animation.json")
    if re.search(r'"name"\\s*:\\s*"idle"', anim) and (re.search(r'"duration"\\s*:\\s*0\\.75(?:0+)?', anim) or '"durationSeconds": 0.75' in anim):
        passes.append("Legacy idle remains 0.75 seconds")
    else:
        failures.append("Legacy idle 0.75-second lock not found")
    require("src/core/runtime.cpp", "Combat retreat is now its own authored clip", "dedicated combat_retreat lock")

    # Story order / outcome locks.
    chapter = text("src/content/chapter_registry.cpp")
    order = ["ch2_vs_hamual","ch2_vs_daniel","ch2_bark_pouki","ch2_vs_wade","ch2_vs_plouke"]
    positions = [chapter.find(x) for x in order]
    if all(p >= 0 for p in positions) and positions == sorted(positions):
        passes.append("Tournament required match order")
    else:
        failures.append("Tournament required match order")
    require("src/core/runtime.cpp", "PloukeStoryFinal", "Plouke final story resolution")
    if "saboteur" in text("src/content/dialogue_registry.cpp").lower() and "ch2_cracked_ring" in text("src/content/dialogue_registry.cpp"):
        # The word may exist elsewhere; fail only if an explicit reveal/fight token was authored.
        if "ch2_cracked_ring_saboteur" in text("src/content/dialogue_registry.cpp") or "ch2_saboteur_fight" in chapter:
            failures.append("Chapter 2 Cracked Ring reveals/fights a saboteur")
        else:
            passes.append("Cracked Ring stays investigation-only")
    else:
        passes.append("Cracked Ring stays investigation-only")

    print("PARALLELS X U7-U10 SOURCE VERIFIER")
    for item in passes: print("PASS:", item)
    if failures:
        print("\\nFAILED INVARIANTS:")
        for item in failures: print("FAIL:", item)
        raise SystemExit(1)
    print(f"\\nPASS: {len(passes)} locked invariants verified")
    print("NOTE: source verification is not a substitute for Mac/Linux/real Old 3DS acceptance.")
    ''')
    write(root / "scripts/verify-update7-10.py", verifier)


def verify_milestone(root: pathlib.Path, name: str):
    checks = {
        "7a": [("src/content/cutscene_registry.hpp", "CutsceneActionKind"), ("src/core/runtime.cpp", "applyCutsceneActions")],
        "7b": [("src/core/runtime.cpp", "objectSwapCooldownTime_ = 4.5f"), ("src/core/runtime.cpp", "lensBlindnessAmount")],
        "8": [("src/content/chapter_registry.cpp", 'chapters_.emplace("rrvvfo_ch2"'), ("src/content/map_registry.cpp", 'tournament.id = "tournament_grounds"')],
        "9": [("src/content/arena_encounter_registry.cpp", '"ch2_vs_plouke"'), ("src/core/save.hpp", "kSchemaVersion = 6")],
        "10": [("src/platform/3ds/legacy_ui_3ds.cpp", "TOURNAMENT CARD"), ("docs/U10_TOURNAMENT_GOLDEN.md", "Chapter-1 transient progression")],
    }
    missing = []
    for rel, needle in checks[name]:
        p = root / rel
        if not p.exists() or needle not in read(p): missing.append(f"{rel}: {needle}")
    if missing:
        raise RuntimeError(f"Milestone U{name.upper()} source gate failed:\n  " + "\n  ".join(missing))
    print(f"Source gate PASS — U{name.upper()} implementation anchors present.")


def main():
    parser = argparse.ArgumentParser(description="Apply Parallels X Updates U7A-U10 to the U6 Golden Gate source tree.")
    parser.add_argument("root", nargs="?", default=".", help="Path to the Parallels X repository")
    parser.add_argument("--milestone", choices=["7a","7b","8","9","10","all"], default="all")
    parser.add_argument("--check-only", action="store_true", help="Run preflight only; do not modify files")
    args = parser.parse_args()
    root = pathlib.Path(args.root).expanduser().resolve()
    preflight(root)
    if args.check_only:
        print("Check-only PASS — no files changed.")
        return 0

    order = ["7a","7b","8","9","10"]
    target = args.milestone
    if target == "all": selected = order
    else:
        # Milestones depend on every earlier stage. Applying through the requested target is idempotent.
        selected = order[:order.index(target)+1]

    actions = {"7a":apply_u7a,"7b":apply_u7b,"8":apply_u8,"9":apply_u9,"10":apply_u10}
    for name in selected:
        actions[name](root)
        verify_milestone(root, name)

    write_verifier(root)
    verifier = run([sys.executable, "scripts/verify-update7-10.py", str(root)], root)
    sys.stdout.write(verifier.stdout)
    if verifier.returncode != 0:
        sys.stderr.write(verifier.stderr)
        raise RuntimeError("Final U7-U10 source invariant verification failed")

    print("\nU7A-U10 batch authored into source tree.")
    print("Build/test acceptance is intentionally separate: Mac/Linux/real Old 3DS remain the Golden Gate authorities.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(1)
