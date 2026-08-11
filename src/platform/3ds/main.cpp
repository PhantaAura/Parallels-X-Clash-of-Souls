#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>

#include <algorithm>
#include <cstdio>
#include <string>
#include <sys/stat.h>

#include "content/adventure_registry.hpp"
#include "content/chapter_registry.hpp"
#include "content/character_face.hpp"
#include "content/character_model_asset.hpp"
#include "content/character_presentation_registry.hpp"
#include "content/cutscene_registry.hpp"
#include "content/dialogue_registry.hpp"
#include "content/exploration_registry.hpp"
#include "content/map_registry.hpp"
#include "content/menu_registry.hpp"
#include "content/skeletal_animation.hpp"
#include "content/story_recap_registry.hpp"
#include "content/story_route_registry.hpp"
#include "content/training_registry.hpp"
#include "content/world_presentation_registry.hpp"
#include "core/input.hpp"
#include "core/menu_state.hpp"
#include "core/runtime.hpp"
#include "core/save.hpp"
#include "platform/3ds/input_3ds.hpp"
#include "platform/3ds/legacy_ui_3ds.hpp"
#include "platform/3ds/world_renderer_3ds.hpp"

namespace {

#define DISPLAY_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
     GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
     GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

constexpr const char* kSavePath = "sdmc:/3ds/ParallelsX/save-v5.txt";
constexpr const char* kBackupSavePath = "sdmc:/3ds/ParallelsX/save-v5.txt.bak";
constexpr const char* kTemporarySavePath = "sdmc:/3ds/ParallelsX/save-v5.txt.tmp";
constexpr const char* kLegacyQolSavePath = "sdmc:/3ds/ParallelsX/save-v4.txt";
constexpr const char* kLegacyGateSavePath = "sdmc:/3ds/ParallelsX/chapter1_gate_save.txt";
constexpr const char* kRrvvfoModelPath = "romfs:/assets/characters/rrvvfo/rrvvfo-dev.pxskel";

bool readText(const char* path, std::string& text) {
    FILE* file = std::fopen(path, "rb");
    if (!file) return false;
    text.clear();
    char buffer[1024];
    size_t count = 0;
    while ((count = std::fread(buffer, 1, sizeof(buffer), file)) > 0) text.append(buffer, count);
    std::fclose(file);
    return true;
}

bool writeText(const char* path, const std::string& text) {
    FILE* file = std::fopen(path, "wb");
    if (!file) return false;
    const bool ok = std::fwrite(text.data(), 1, text.size(), file) == text.size() &&
                    std::fflush(file) == 0;
    std::fclose(file);
    return ok;
}

bool validSaveLocation(const px::SaveData& save) {
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

bool loadSave(px::SaveData& output) {
    if (loadSaveAt(kSavePath, output)) return true;
    if (loadSaveAt(kBackupSavePath, output)) return true;
    if (loadSaveAt(kLegacyQolSavePath, output)) return true;
    return loadSaveAt(kLegacyGateSavePath, output);
}

bool writeSave(const px::SaveData& save) {
    mkdir("sdmc:/3ds", 0777);
    mkdir("sdmc:/3ds/ParallelsX", 0777);
    const std::string text = px::SaveCodec::serialize(save);
    std::string previous;
    px::SaveData validPrevious;
    if (loadSaveAt(kSavePath, validPrevious) && readText(kSavePath, previous)) writeText(kBackupSavePath, previous);
    if (!writeText(kTemporarySavePath, text)) return false;
    std::remove(kSavePath);
    return std::rename(kTemporarySavePath, kSavePath) == 0;
}

bool persist(px::RuntimeSession& session, px::SaveData& save, bool gameplay) {
    if (gameplay && !session.standaloneMode()) {
        px::SaveData runtimeSave = session.saveSnapshot("old-3ds-xl");
        runtimeSave.frontend = save.frontend;
        save = std::move(runtimeSave);
    } else if (gameplay) {
        save.qol = session.qolSettings();
    }
    return writeSave(save);
}

px::Action menuAction(const px::InputState& input, u32 down) {
    if ((down & KEY_DLEFT) || input.pressed(px::Action::MoveLeft)) return px::Action::MoveLeft;
    if ((down & KEY_DRIGHT) || input.pressed(px::Action::MoveRight)) return px::Action::MoveRight;
    if ((down & KEY_DUP) || input.pressed(px::Action::MoveUp)) return px::Action::MoveUp;
    if ((down & KEY_DDOWN) || input.pressed(px::Action::MoveDown)) return px::Action::MoveDown;
    // Front-end presses should use the hardware edge as well as the shared
    // semantic input state. Very short taps (notably from emulators) can begin
    // and end between held-state samples while still appearing in hidKeysDown.
    if ((down & KEY_A) || input.pressed(px::Action::Confirm)) return px::Action::Confirm;
    if ((down & KEY_B) || input.pressed(px::Action::Cancel)) return px::Action::Cancel;
    return px::Action::AbilityUse;
}

bool isMenuAction(px::Action action) { return action != px::Action::AbilityUse; }

void processMenuOutcome(px::MenuState& menu, px::RuntimeSession& session,
                        px::SaveData& save, bool& gameplay) {
    const auto outcome = menu.outcome();
    if (outcome == px::MenuOutcome::None) return;
    if (outcome == px::MenuOutcome::BeginStory) {
        session.startChapter("rrvvfo_ch1");
        session.setQolSettings(save.qol);
    } else if (outcome == px::MenuOutcome::ReplayChapter)
        session.startReplayChapter(save);
    else if (outcome == px::MenuOutcome::ContinueStory) {
        if (save.story.chapterId.empty()) session.startChapter("rrvvfo_ch1");
        else session.loadSnapshot(save);
    } else if (outcome == px::MenuOutcome::LaunchMode) {
        const auto mode = menu.snapshot().selectedMode.id;
        if (mode == px::MenuModeId::ArenaBattle) session.startCpuFight();
        else if (mode == px::MenuModeId::Training) session.startStandaloneTraining();
        session.setQolSettings(save.qol);
    }
    if (outcome == px::MenuOutcome::BeginStory || outcome == px::MenuOutcome::ContinueStory ||
        outcome == px::MenuOutcome::ReplayChapter || outcome == px::MenuOutcome::LaunchMode)
        gameplay = true;
    menu.clearOutcome();
    persist(session, save, gameplay);
}

u32 clearColor(const px::PresentationColor& color) {
    return C2D_Color32(static_cast<u8>(std::clamp(color.r, 0.0f, 1.0f) * 255.0f),
                       static_cast<u8>(std::clamp(color.g, 0.0f, 1.0f) * 255.0f),
                       static_cast<u8>(std::clamp(color.b, 0.0f, 1.0f) * 255.0f), 255);
}

} // namespace

int main() {
    gfxInitDefault();
    gfxSet3D(false);
    const Result romfsResult = romfsInit();
    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) {
        if (R_SUCCEEDED(romfsResult)) romfsExit();
        gfxExit();
        return 1;
    }
    if (!C2D_Init(C2D_DEFAULT_MAX_OBJECTS)) {
        C3D_Fini();
        if (R_SUCCEEDED(romfsResult)) romfsExit();
        gfxExit();
        return 1;
    }
    C2D_Prepare();

    C3D_RenderTarget* top = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTarget* bottom = C3D_RenderTargetCreate(240, 320, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    if (!top || !bottom) {
        C2D_Fini();
        C3D_Fini();
        if (R_SUCCEEDED(romfsResult)) romfsExit();
        gfxExit();
        return 1;
    }
    C3D_RenderTargetSetOutput(top, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
    C3D_RenderTargetSetOutput(bottom, GFX_BOTTOM, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

    px::ChapterRegistry chapters;
    px::MapRegistry maps;
    px::CutsceneRegistry cutscenes;
    px::DialogueRegistry dialogue;
    px::ExplorationRegistry exploration;
    px::TrainingRegistry training;
    px::AdventureRegistry adventures;
    px::WorldPresentationRegistry worldPresentation;
    px::CharacterPresentationRegistry characterPresentation;
    px::MenuRegistry menuRegistry;
    px::StoryRouteRegistry storyRoutes;
    px::StoryRecapRegistry storyRecap;
    px::SaveData save;
    loadSave(save);
    px::RuntimeSession session(chapters, maps, cutscenes, dialogue, exploration, training, adventures);
    px::MenuState menu(menuRegistry, storyRoutes, storyRecap, save);
    menu.setReducedMotion(save.qol.reducedMotion);

    px::CharacterModelAsset rrvvfoModel;
    std::string modelError;
    const bool modelReady = R_SUCCEEDED(romfsResult) &&
                            rrvvfoModel.loadCooked(kRrvvfoModelPath, &modelError);
    if (R_FAILED(romfsResult)) modelError = "The packaged game assets could not be mounted.";
    px::SkeletalAnimationPlayer modelAnimation;
    if (modelReady) {
        modelAnimation.bind(&rrvvfoModel);
        modelAnimation.setState("idle", true);
    }

    px::platform3ds::WorldRenderer3ds renderer;
    std::string rendererError;
    const bool rendererReady = renderer.init(&rendererError);
    px::platform3ds::LegacyUi3ds ui;
    std::string uiError;
    const bool uiReady = ui.init(&uiError);
    if (!uiReady) {
        renderer.shutdown();
        C2D_Fini();
        C3D_Fini();
        if (R_SUCCEEDED(romfsResult)) romfsExit();
        gfxExit();
        return 1;
    }

    px::InputState input;
    bool gameplay = false;
    TickCounter tick;
    osTickCounterStart(&tick);

    while (aptMainLoop()) {
        hidScanInput();
        const u32 held = hidKeysHeld();
        const u32 down = hidKeysDown();
        circlePosition circle{};
        hidCircleRead(&circle);
        touchPosition touch{};
        if (held & KEY_TOUCH) hidTouchRead(&touch);
        px::platform3ds::updateInput(input, held, circle);

        osTickCounterUpdate(&tick);
        float deltaSeconds = static_cast<float>(osTickCounterRead(&tick)) / 1000.0f;
        if (deltaSeconds <= 0.0f || deltaSeconds > .1f) deltaSeconds = 1.0f / 30.0f;

        const bool buildBlocked = !rendererReady || !modelReady;
        if (!buildBlocked) {
            if (!gameplay) {
                menu.tick(deltaSeconds);
                if (menu.screen() == px::MenuScreen::Title && down != 0)
                    menu.handle(px::Action::Confirm);
                else {
                    const px::Action action = menuAction(input, down);
                    if (isMenuAction(action)) menu.handle(action);
                }
                processMenuOutcome(menu, session, save, gameplay);
            } else {
                const bool touchSave = (down & KEY_TOUCH) && touch.py >= 210 && touch.px < 160;
                const bool touchPause = (down & KEY_TOUCH) && touch.py >= 210 && touch.px >= 160;
                if (touchPause) input.set(px::Action::Pause, true);
                session.tick(input, deltaSeconds);
                if (touchSave) {
                    if (session.canManualSave()) session.notifyManualSaveResult(persist(session, save, true));
                    else session.notifyManualSaveUnavailable();
                }
                if (session.consumeManualSaveRequest())
                    session.notifyManualSaveResult(persist(session, save, true));
                if (session.consumeReturnToTitleRequest()) {
                    persist(session, save, true);
                    gameplay = false;
                    menu.openTitle();
                }
                if ((down & KEY_SELECT) && session.canManualSave())
                    session.notifyManualSaveResult(persist(session, save, true));
            }
        }

        if (modelReady) {
            const std::string desired = gameplay ? session.view().playerAnimation : "idle";
            if (modelAnimation.state() != desired) modelAnimation.setState(desired, true);
            modelAnimation.setPlaybackSpeed(gameplay ? session.view().playerAnimationSpeed : 1.0f);
            if (!gameplay || session.view().hitFreezeSeconds <= 0.0f) modelAnimation.update(deltaSeconds);
        }
        if ((held & KEY_L) && (down & KEY_START)) {
            if (!buildBlocked) persist(session, save, gameplay);
            break;
        }

        ui.beginFrame();
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        const u32 legacyBlue = C2D_Color32(13, 91, 180, 255);
        C3D_RenderTargetClear(top, C3D_CLEAR_ALL, legacyBlue, 0);
        C3D_RenderTargetClear(bottom, C3D_CLEAR_ALL, C2D_Color32(7, 24, 52, 255), 0);

        if (buildBlocked) {
            const std::string titleText = !modelReady ? "RRVVFO MODEL DID NOT LOAD" : "3D RENDERER DID NOT START";
            const std::string detail = !modelReady ? modelError : rendererError;
            C2D_SceneBegin(top);
            ui.drawFatal(titleText, detail, false);
            C2D_SceneBegin(bottom);
            ui.drawFatal("NOT A PLAYABLE BUILD", "Fix the packaged asset/render path, then rebuild.", true);
        } else if (!gameplay) {
            const auto snapshot = menu.snapshot();
            const bool showRrvvfoRouteModel =
                snapshot.screen == px::MenuScreen::StoryCharacterSelect &&
                snapshot.selectedRoute.id == "rrvvfo";
            if (showRrvvfoRouteModel) {
                renderer.renderPlayerPreview(top, rrvvfoModel, modelAnimation,
                                             px::RrvvfoFaceExpression::Confident,
                                             characterPresentation.get("rrvvfo"),
                                             px::platform3ds::CharacterPreview3ds::FullBody);
                C3D_RenderTargetClear(top, C3D_CLEAR_DEPTH, 0, 0);
                C2D_Prepare();
            }
            C2D_SceneBegin(top);
            ui.drawTopMenu(snapshot, true);
            C2D_SceneBegin(bottom);
            ui.drawBottomMenu(snapshot, true);
        } else {
            const auto& view = session.view();
            const px::WorldPresentationDefinition* stage = nullptr;
            if (worldPresentation.has(view.presentationStageId)) stage = &worldPresentation.get(view.presentationStageId);
            if (stage) {
                C3D_RenderTargetClear(top, C3D_CLEAR_ALL, clearColor(stage->clearColor), 0);
                C3D_FrameDrawOn(top);
                const bool playerSpeaking = view.dialogueVisible &&
                    (view.dialogueFocusActorId == "rrvvfo" || view.dialoguePortraitId == "rrvvfo" ||
                     view.dialogueSpeaker == "RRVVFO");
                const auto faceExpression = px::resolveRrvvfoFaceExpression(
                    view.playerAnimation, modelAnimation.time(), view.playerFaceSeconds,
                    view.dialogueExpression, playerSpeaking);
                renderer.render(*stage, view, session.disabledBlockers(), characterPresentation,
                                &rrvvfoModel, &modelAnimation, faceExpression);
                // Rrvvfo's lightweight expression geometry is attached to the
                // real gameplay mesh above. Keep dialogue text on the second
                // screen and reserve the one-pass live model showcase for the
                // route screen; two raw skinned draws per frame are not a safe
                // Old-3DS performance trade.
                // The raw world shader replaced Citro2D's GPU bindings, so
                // restore them before submitting either UI target.
                C2D_Prepare();
                C2D_SceneBegin(top);
                ui.drawTopGameplay(view);
                C2D_SceneBegin(bottom);
                ui.drawBottomGameplay(view, faceExpression, true);
            } else {
                C2D_SceneBegin(top);
                ui.drawFatal("STAGE PRESENTATION MISSING", view.presentationStageId, false);
                C2D_SceneBegin(bottom);
                ui.drawFatal("CHAPTER CANNOT CONTINUE", "The shared map has no presentation definition.", true);
            }
        }
        C2D_Flush();
        C3D_FrameEnd(0);
    }

    if (rendererReady && modelReady) persist(session, save, gameplay);
    ui.shutdown();
    renderer.shutdown();
    C2D_Fini();
    C3D_Fini();
    if (R_SUCCEEDED(romfsResult)) romfsExit();
    gfxExit();
    return 0;
}
