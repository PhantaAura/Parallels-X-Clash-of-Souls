#!/usr/bin/env python3
"""Static checks for platform boundaries that runtime unit tests cannot observe."""

from pathlib import Path
import hashlib


ROOT = Path(__file__).resolve().parents[1]


def read(relative: str) -> str:
    return (ROOT / relative).read_text(encoding="utf-8")


shared_text = "\n".join(
    path.read_text(encoding="utf-8")
    for directory in (ROOT / "src/core", ROOT / "src/content")
    for path in sorted(directory.glob("*"))
    if path.suffix in {".cpp", ".hpp"}
)

for forbidden in (
    "<emscripten",
    "Emscripten",
    "window.document",
    "window.localStorage",
    "document.getElementById",
    "navigator.userAgent",
    "WebKit",
    "touchstart",
    "touchend",
    "<SDL",
    "SDL_",
    "<X11/",
    "<wayland-",
    "<Cocoa/",
    "<Metal/",
):
    assert forbidden not in shared_text, f"web/mobile dependency leaked into shared C++: {forbidden}"

world_content = read("src/content/world_presentation_registry.cpp")
mac_renderer = read("src/platform/macos/main.mm")
linux_shell = read("src/platform/linux/main.cpp")
linux_renderer = read("src/platform/linux/presentation_renderer.cpp")
for landmark in (
    "sage_bell",
    "focus_pillar_center",
    "broken_bridge_rail_",
    "river",
    "tournament_gate_red",
    "outskirts_shop_fire",
):
    assert landmark in world_content, f"shared world content is missing {landmark}"
    assert landmark not in mac_renderer, f"world content was hardcoded into the Mac renderer: {landmark}"

for required_renderer_feature in (
    "perspective(",
    "lookAt(",
    "MTLPixelFormatDepth32Float",
    "setDepthStencilState",
    "WorldPresentationRegistry",
):
    assert required_renderer_feature in mac_renderer, f"Mac 3D foundation is missing {required_renderer_feature}"

for shared_mac_contract in (
    "MenuState",
    "MenuSnapshot",
    "MenuRegistry",
    "StoryRouteRegistry",
    "StoryRecapRegistry",
    "CombatManualRegistry",
    "UiPresentationRegistry",
    "PXFrontEndView",
    "PXManualView",
    "GameController",
):
    assert shared_mac_contract in mac_renderer, f"Mac shell is not consuming shared {shared_mac_contract}"
assert 'AppState() { session.startChapter("rrvvfo_ch1"); }' not in mac_renderer, "Mac still boots directly into Chapter 1"
assert "_state->menu.snapshot()" in mac_renderer, "Mac front end is not rendered from shared menu state"
assert "macSavePath" in mac_renderer and "ApplicationSupportDirectory" in mac_renderer, "Mac save location is not platform isolated"

for obsolete_renderer_path in ("project(", "pushHumanoid(", "pushPine("):
    assert obsolete_renderer_path not in mac_renderer, f"obsolete screen-space renderer path remains: {obsolete_renderer_path}"

for shared_linux_contract in (
    "RuntimeSession",
    "MenuState",
    "MenuRegistry",
    "StoryRouteRegistry",
    "StoryRecapRegistry",
    "CombatManualRegistry",
    "UiPresentationRegistry",
):
    assert shared_linux_contract in linux_shell, f"Linux shell is not consuming shared {shared_linux_contract}"
for review_state in (
    "press-start",
    "mode-story",
    "mode-arena",
    "mode-online",
    "story-character-rrvvfo",
    "story-so-far",
    "combat-manual",
    "chapter1-exploration",
    "chapter1-combat",
    "chapter1-dialogue",
    "rrvvfo-model",
    "rrvvfo-legacy-comparison",
    "rrvvfo-idle-bind",
    "rrvvfo-idle-sampled",
    "rrvvfo-idle-pose-1",
    "rrvvfo-idle-pose-3",
    "rrvvfo-idle-pose-5",
    "rrvvfo-idle-pose-6",
    "rrvvfo-run",
    "rrvvfo-dash",
    "rrvvfo-charge",
    "rrvvfo-heavy",
    "rrvvfo-fire-blast",
    "rrvvfo-object-swap",
    "rrvvfo-lens",
):
    assert review_state in linux_shell, f"Linux deterministic review state is missing: {review_state}"
assert not (ROOT / "src/platform/linux/game.cpp").exists(), "Linux-specific gameplay fork exists"
assert "UiPresentationRegistry" in linux_renderer and "MenuSnapshot" in linux_renderer
assert "dev-saves/linux" in linux_shell, "Linux development saves are not isolated"

character_binding = read("src/content/character_presentation_registry.cpp")
model_asset = read("src/content/character_model_asset.cpp")
assert "assets/characters/rrvvfo/rrvvfo-dev.glb" in character_binding
assert "assets/characters/rrvvfo/rrvvfo-dev.pxskel" in character_binding
assert "CharacterModelRepository" in linux_shell and "CharacterModelRepository" in mac_renderer
assert "playerPosition" in linux_renderer and 'characterModels_.find("rrvvfo")' in linux_renderer
assert "SoftwareWorldCanvas" in linux_renderer and "stage.camera" in linux_renderer
assert "playerPosition" in mac_renderer and "pushCharacterModel" in mac_renderer
assert "Required Rrvvfo model failed to load" in linux_shell and "using procedural fallback" not in linux_shell
assert "actorCanvas.fallback(rrvvfo" not in linux_renderer, "Linux gameplay may substitute a fake playable Rrvvfo"
assert "pushCharacterFallback(vertices,rrvvfoBinding" not in mac_renderer, "Mac gameplay may substitute a fake playable Rrvvfo"
assert "drawMenuRrvvfo" in mac_renderer and 'characterModel("rrvvfo"' in linux_renderer, \
    "desktop route/menu presentation is not using the real cooked Rrvvfo model"
animation_source = read("src/content/skeletal_animation.cpp")
face_source = read("src/content/character_face.cpp")
assert "SkeletalAnimationPlayer" in animation_source
assert "findAnimation" in model_asset
for renderer in (linux_renderer, mac_renderer):
    assert "playerAnimation" in renderer, "platform renderer is not consuming the shared animation player"
    assert "resolveRrvvfoFaceExpression" in renderer, "desktop renderer is not consuming the shared face state"
assert "kRrvvfoFaceTriangleBudget = 48" in read("src/content/character_face.hpp")
assert "spine.006" not in face_source, "face implementation must use the validated joint index instead of a renderer lookup"
for forbidden in ("light_attack", "object_swap_clip", "lens_clip"):
    assert forbidden not in shared_text, f"out-of-scope animation behavior entered shared code: {forbidden}"
field_movement = read("src/core/field_movement.cpp")
assert "hasDashDirection" in field_movement, "directionless zero-distance dash regression returned"

source_paths = [path.name.lower() for path in (ROOT / "src").rglob("*") if path.is_file()]
assert not any("mission" in name for name in source_paths), "Legacy mission architecture filename leaked into 3.0R"

chapter_content = read("src/content/chapter_registry.cpp")
for obsolete_scene in ("sage_energy_signature_training", "bark_wade_reunion"):
    assert obsolete_scene not in chapter_content, f"invented 0.4D Chapter 1 scene remains: {obsolete_scene}"
assert '"legacy_route_choice"' in chapter_content
assert '"rrvvfo_ch2"' in chapter_content
assert "ChoosePhysicalRoute" not in shared_text
assert "ChooseRoute" in shared_text
for fabricated_route_chapter in ("bark_ch1", "wade_ch1", "virek_ch1", "oddballs_ch1"):
    assert fabricated_route_chapter not in chapter_content, f"unfinished route has fabricated gameplay: {fabricated_route_chapter}"

runtime = read("src/core/runtime.cpp")
for shared_feature in ("AdventureRegistry", "CombatSystem::advanceAttack", "runawayCart", "chapterComplete_",
                       "trainingCheckpointStep_", "loadSnapshot", "pendingChapterId", "startCpuFight",
                       "startStandaloneTraining"):
    assert shared_feature in runtime, f"shared Chapter 1 runtime behavior is missing: {shared_feature}"

for shared_foundation in ("StoryProgressionSystem", "TournamentCardState", "QuestSystem", "PartyState"):
    assert shared_foundation in shared_text, f"Chapter 2–4 shared foundation is missing: {shared_foundation}"

story_rules = read("docs/STORY_SOURCE_RULES.md")
assert story_rules.index("Latest playable browser content") < story_rules.index("Explicit approved 3.0R") < \
       story_rules.index("Exact cumulative Legacy build") < story_rules.index("Current route documents")
for chapter in (2, 3, 4):
    audit = read(f"docs/LEGACY_CHAPTER{chapter}_PARITY_AUDIT.md")
    for classification in ("MATCHING", "DIFFERENT", "MISSING", "APPROVED CURRENT-CANON OVERRIDE", "OBSOLETE TECHNICAL ARCHITECTURE"):
        assert classification in audit, f"Chapter {chapter} audit omits classification {classification}"

for ui_contract in ("showEnergy", "showGuard", "hotbarVisible", "pauseVisible", "nearbyInteractionLabel"):
    assert ui_contract in runtime and ui_contract in mac_renderer, f"shared/Mac UI contract missing {ui_contract}"
assert "0.4E" not in mac_renderer, "Mac presentation still labels itself as 0.4E"

three_ds = read("src/platform/3ds/main.cpp")
three_ds_renderer = read("src/platform/3ds/world_renderer_3ds.cpp")
three_ds_ui = read("src/platform/3ds/legacy_ui_3ds.cpp")
for platform_shell in (mac_renderer, linux_shell, three_ds):
    for direct_mode_contract in ("MenuOutcome::LaunchMode", "startCpuFight", "startStandaloneTraining", "standaloneMode"):
        assert direct_mode_contract in platform_shell, \
            f"platform shell is missing playable Fight/Training contract: {direct_mode_contract}"
for shared_3ds_contract in (
    "RuntimeSession", "ChapterRegistry", "ExplorationRegistry", "TrainingRegistry", "AdventureRegistry",
    "SaveCodec", "CharacterModelAsset", "SkeletalAnimationPlayer", "WorldPresentationRegistry",
    "CharacterPresentationRegistry", "WorldRenderer3ds",
):
    assert shared_3ds_contract in three_ds, f"3DS Chapter 1 port is not consuming shared {shared_3ds_contract}"
assert "resolveRrvvfoFaceExpression" in three_ds, "3DS port is not consuming the shared face state"
for parity_contract in (
    "stage.camera", "Mtx_PerspTilt", "Mtx_LookAt", "view.playerPosition", "view.playerYawDegrees",
    "view.opponentPosition", "view.opponentYawDegrees", "disabledBlockers", "playerAnimation",
    "rrvvfoFaceTriangles", "PresentationDetailTier::Essential",
):
    assert parity_contract in three_ds_renderer, f"3DS perspective renderer is missing shared parity contract: {parity_contract}"
for obsolete_diagnostic in (
    "drawModelInset", "mapX(", "mapY(", "Gate diagnostic presentation", "COMPATIBILITY GATE",
    "kModelSampleStride", "kFaceInsetDrawBudget",
):
    assert obsolete_diagnostic not in three_ds + three_ds_renderer, f"3DS diagnostic gameplay path survived: {obsolete_diagnostic}"
assert "C3D_RenderTargetCreate" in three_ds and "GPU_RB_DEPTH24_STENCIL8" in three_ds
assert "C3D_DrawArrays(GPU_TRIANGLES" in three_ds_renderer
assert "C2D_DrawCircleSolid(pxp" not in three_ds, "3DS player is still a top-down diagnostic dot"
assert "presentationStageId" in three_ds and "worldPresentation.get" in three_ds
assert "session.disabledBlockers()" in three_ds
three_ds_makefile = read("src/platform/3ds/Makefile")
assert "ROMFS       :=" in three_ds_makefile and "--romfs=$(ROMFS)" in three_ds_makefile, \
    "3DS port is not embedding its ROMFS assets in the 3DSX"
assert "--smdh=$(OUTPUT).smdh" in three_ds_makefile and "$(OUTPUT).smdh" in three_ds_makefile, \
    "3DS port is not embedding its Homebrew Menu metadata"
assert "PICAFILES" in three_ds_makefile, "3DS port is not compiling its native PICA shader"
assert "projection[4]" in read("src/platform/3ds/world.v.pica"), "3DS perspective shader lost its camera transform"
romfs_model = ROOT / "src/platform/3ds/romfs/assets/characters/rrvvfo/rrvvfo-dev.pxskel"
desktop_model = ROOT / "assets/characters/rrvvfo/rrvvfo-dev.pxskel"
assert romfs_model.is_file(), "3DS ROMFS Rrvvfo model is missing"
assert hashlib.sha256(romfs_model.read_bytes()).digest() == hashlib.sha256(desktop_model.read_bytes()).digest(), "3DS ROMFS model diverged from the shared cooked desktop asset"
assert "CombatSystem::light" not in three_ds, "3DS port reintroduced platform-specific combat"
assert "FieldMovementSystem::tick" not in three_ds, "3DS port reintroduced platform-specific movement"
assert "LegacyUi3ds" in three_ds and "MenuState" in three_ds, \
    "3DS port bypasses the shared front end or native Legacy UI"
assert "consoleInit" not in three_ds and "std::printf" not in three_ds, \
    "3DS release presentation fell back to a debug console"
assert "!modelReady" in three_ds and "RRVVFO MODEL DID NOT LOAD" in three_ds, \
    "3DS release no longer blocks instead of substituting a fake playable Rrvvfo"
assert "renderPlayerPreview" in three_ds and "CharacterPreview3ds::FullBody" in three_ds, \
    "3DS Legacy route screen is not presenting the real cooked Rrvvfo model"
assert "kMinimumReadableScale = .36f" in three_ds_ui, \
    "3DS UI lost its physical-screen readability floor"
for restored_story_control in ("snapshot.storySoFarSelected", "STORY MODE", "STORY SO FAR", "SWITCH CHOICE"):
    assert restored_story_control in three_ds_ui, \
        f"3DS Mode Select hid the restored Story So Far path: {restored_story_control}"
for remake_touch_ui in ("remakePanel", "remakeButton", "hintPill", "roundedRect"):
    assert remake_touch_ui in three_ds_ui, \
        f"3DS touch screen lost its remake-style readable command layout: {remake_touch_ui}"
for tutorial_control in ("CHOOSE TRAINING", "trainingManualOptions", "trainingManualSelection", "A START"):
    assert tutorial_control in three_ds_ui, \
        f"3DS tutorial hid the shared training-start control: {tutorial_control}"
assert "bust?131.0f:110.0f" in three_ds_renderer and "bust?185.0f:520.0f" in three_ds_renderer, \
    "3DS route preview camera can crop Rrvvfo's head/face again"
assert "C2D_Prepare();" in three_ds and "C2D_Flush();" in three_ds, \
    "3DS raw-3D/Citro2D handoff guard is missing"

allowed_deprecated_faction_paths = {
    "src/content/faction_registry.cpp",
    "src/core/save.cpp",
    "tests/menu_tests.cpp",
    "tests/source_architecture_tests.py",
    "docs/ORGANIZATION_OF_THE_RED_LOST_YEAR_CONTINUITY.md",
}
deprecated_faction_display = "Project" + " Hollow"
for path in ROOT.rglob("*"):
    if not path.is_file() or ".git" in path.parts or "build-linux" in path.parts:
        continue
    try:
        text = path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        continue
    relative = path.relative_to(ROOT).as_posix()
    if "project_hollow" in text:
        assert relative in allowed_deprecated_faction_paths, f"deprecated faction ID leaked into new content: {relative}"
    assert deprecated_faction_display not in text, f"deprecated player/planning faction name remains: {relative}"

continuity = read("docs/ORGANIZATION_OF_THE_RED_LOST_YEAR_CONTINUITY.md")
for required in ("organization_red", "Season 1 Clone incident", "Lost Year", "Strange Man", "unstable", "Echo", "disbanded"):
    assert required in continuity, f"Organization continuity lock is missing {required}"

def assert_u4_u6_invariants():
    skeletal = read("src/content/skeletal_animation.cpp")
    renderer3ds = read("src/platform/3ds/world_renderer_3ds.cpp")
    runtime = read("src/core/runtime.cpp")
    animation_author = read("scripts/author-rrvvfo-core-animations.py")

    assert "bindLocalMatrices_" in skeletal
    assert "std::vector<CharacterMatrix> local;" not in skeletal
    assert "desiredHotbarSize" in runtime
    assert "view_.hotbar = AbilityHotbarCatalog::rrvvfoChapter1();" not in runtime
    assert "staticGpuDirty_" in renderer3ds
    assert "frameVertices_=staticWorld_" not in renderer3ds
    mac = read("src/platform/macos/main.mm")
    assert "_worldVertices.reserve(32000)" in mac
    assert "std::vector<Vertex> vertices;vertices.reserve(32000);" not in mac
    assert '"flow_cancel"' in animation_author
    assert '"pursuit-lock"' in runtime
    assert runtime.count("tryFlowCancel()") >= 3  # definition + Arena + roadside calls
    assert runtime.count("CombatSystem::flowCancel(player_)") == 1
    assert "recordObjective(objectiveBeforePause_)" not in runtime
    assert 'kExplorationTurnDegreesPerSecond = 1320.0f' in runtime
    assert '"dash-dust"' in runtime and '"landing-dust"' in runtime
    assert "SHOTS OF AGONY" not in runtime


assert_u4_u6_invariants()

print("PASS: portable source boundary, shared Mac/Linux presentation ownership, no platform gameplay fork, no mission architecture, and perspective RuntimeSession 3DS parity boundary")
