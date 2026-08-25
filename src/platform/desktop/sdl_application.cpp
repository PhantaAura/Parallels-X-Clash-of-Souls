#include "content/adventure_registry.hpp"
#include "content/chapter_registry.hpp"
#include "content/character_model_asset.hpp"
#include "content/skeletal_animation.hpp"
#include "content/character_presentation_registry.hpp"
#include "content/combat_manual_registry.hpp"
#include "content/cutscene_registry.hpp"
#include "content/dialogue_registry.hpp"
#include "content/exploration_registry.hpp"
#include "content/map_registry.hpp"
#include "content/menu_registry.hpp"
#include "content/story_recap_registry.hpp"
#include "content/story_route_registry.hpp"
#include "content/training_registry.hpp"
#include "content/ui_presentation_registry.hpp"
#include "content/world_presentation_registry.hpp"
#include "core/input.hpp"
#include "core/menu_state.hpp"
#include "core/runtime.hpp"
#include "core/save.hpp"
#include "core/story_unlocks.hpp"
#include "platform/desktop/presentation_renderer.hpp"
#include "platform/desktop/sdl_application.hpp"
#include "platform/desktop/sdl_compat.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

struct Arguments {
    std::string review;
    std::string screenshot;
    std::string smoke;
    std::string saveDirectory;
    std::string assetRoot;
    bool headless{false};
    int exitAfterFrames{0};
};

Arguments parseArguments(int argc, char** argv, const px::desktop::SdlPlatformConfig& platform) {
    Arguments args;
    args.saveDirectory = platform.defaultSaveDirectory;
    const auto executableDir = std::filesystem::absolute(argv[0]).parent_path();
    args.assetRoot = std::filesystem::exists(executableDir / "assets")
        ? executableDir.string() : executableDir.parent_path().string();
    if (!platform.saveEnvironmentVariable.empty())
        if (const char* configured = std::getenv(platform.saveEnvironmentVariable.c_str()))
            args.saveDirectory = configured;
    if (const char* configured = std::getenv("PX_ASSET_ROOT")) args.assetRoot = configured;
    for (int i = 1; i < argc; ++i) {
        const std::string value = argv[i];
        if (value == "--review" && i + 1 < argc) args.review = argv[++i];
        else if (value == "--screenshot" && i + 1 < argc) args.screenshot = argv[++i];
        else if (value == "--smoke" && i + 1 < argc) {
            args.smoke = argv[++i];
            args.headless = true;
            args.exitAfterFrames = 3;
        }
        else if (value == "--save-dir" && i + 1 < argc) args.saveDirectory = argv[++i];
        else if (value == "--asset-root" && i + 1 < argc) args.assetRoot = argv[++i];
        else if (value == "--headless") args.headless = true;
        else if (value == "--help") {
            std::cout
                << "Parallels X " << platform.platformName << " desktop shell\n"
                << "  --review NAME       deterministic presentation state\n"
                << "  --screenshot FILE   write the current frame as PPM and exit\n"
                << "  --smoke MODE        launch story, fight, or training for three frames\n"
                << "  --headless          create a hidden software-rendered window\n"
                << "  --save-dir DIR      isolated development save directory\n"
                << "  --asset-root DIR    project root containing cooked assets\n";
            std::exit(0);
        }
    }
    if (!args.screenshot.empty()) args.headless = true;
    return args;
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

bool readValidatedSave(const std::filesystem::path& path, px::SaveData& output) {
    std::ifstream input(path);
    if (!input) return false;
    const std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    try {
        output = px::SaveCodec::deserialize(text);
        return validSaveLocation(output);
    } catch (const std::exception& error) {
        std::cerr << "Desktop save ignored: " << error.what() << '\n';
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
        std::cerr << "Could not write desktop save: " << error.what() << '\n';
        return false;
    }
}

struct ApplicationState {
    px::ChapterRegistry chapters;
    px::MapRegistry maps;
    px::CutsceneRegistry cutscenes;
    px::DialogueRegistry dialogue;
    px::ExplorationRegistry exploration;
    px::TrainingRegistry training;
    px::AdventureRegistry adventures;
    px::WorldPresentationRegistry worlds;
    px::CharacterPresentationRegistry characters;
    px::CharacterModelRepository characterModels;
    px::SkeletalAnimationPlayer playerAnimation;
    px::MenuRegistry menus;
    px::StoryRouteRegistry routes;
    px::StoryRecapRegistry recap;
    px::CombatManualRegistry manual;
    px::UiPresentationRegistry ui;
    px::SaveData save;
    px::RuntimeSession runtime;
    px::MenuState menu;
    px::InputState input;
    bool gameplay{false};
    bool manualReview{false};
    std::size_t manualPage{3};

    explicit ApplicationState(px::SaveData initial, const std::filesystem::path& assetRoot)
        : save(std::move(initial)),
          runtime(chapters, maps, cutscenes, dialogue, exploration, training, adventures),
          menu(menus, routes, recap, save) {
        menu.setReducedMotion(save.qol.reducedMotion);
        const auto& rrvvfo = characters.get("rrvvfo");
        std::string error;
        if (!characterModels.load(rrvvfo.characterId, (assetRoot / rrvvfo.desktopCookedAsset).string(), &error))
            throw std::runtime_error("Required Rrvvfo model failed to load: " + error);
        playerAnimation.bind(characterModels.find(rrvvfo.characterId));
        playerAnimation.setState("idle");
    }
};

std::size_t sceneIndex(const px::ChapterDefinition& chapter, const std::string& sceneId) {
    for (std::size_t i = 0; i < chapter.openingFlow.size(); ++i)
        if (chapter.openingFlow[i].id == sceneId) return i;
    throw std::runtime_error("Review scene is not registered: " + sceneId);
}

void loadRuntimeReview(ApplicationState& state, const std::string& sceneId, bool beginTraining = true) {
    const auto& chapter = state.chapters.get("rrvvfo_ch1");
    px::SaveData review;
    review.story.chapterId = chapter.id;
    review.story.sceneIndex = sceneIndex(chapter, sceneId);
    review.story.checkpointId = chapter.openingFlow[review.story.sceneIndex].checkpointId;
    review.world.mapId = chapter.primaryMap;
    review.world.hp = 82.0f;
    review.world.energy = 68.0f;
    review.world.guard = 74.0f;
    if (sceneId == "selected_route_adventure") {
        review.world.routeChoice = "main";
        review.world.position = {470.0f, 115.0f};
        review.story.flags = {"ch1_tutorial_checkpoint=6"};
    } else if (sceneId == "sage_tutorial_spar" || sceneId == "ch1_opening_sage_setup") {
        review.world.position = sceneId == "ch1_opening_sage_setup"
            ? px::Vec2{-1450.0f, 42.0f}
            : px::Vec2{-1240.0f, 120.0f};
        if (sceneId == "sage_tutorial_spar" && beginTraining)
            review.story.flags = {"ch1_tutorial_checkpoint=6"};
    } else if (sceneId == "tournament_outskirts_arrival") {
        review.world.routeChoice = "main";
        review.world.position = {1090.0f, 185.0f};
        review.story.flags = {"ch1_tutorial_checkpoint=6", "ch1_transport_rescued",
                              "ch1_precision_swap_mastered", "ch1_roadside_encounter_resolved"};
    }
    state.runtime.loadSnapshot(review);
    if (sceneId == "sage_tutorial_spar" && beginTraining) state.runtime.confirm();
    state.gameplay = true;
}

void prepareReview(const std::string& review, ApplicationState& state) {
    if (review.empty() || review == "press-start") {
        state.menu.openTitle();
    } else if (review == "mode-story") {
        state.menu.openModeSelect();
        state.menu.selectMode("story");
    } else if (review == "mode-arena") {
        state.menu.openModeSelect();
        state.menu.selectMode("battle");
    } else if (review == "mode-online") {
        state.menu.openModeSelect();
        state.menu.selectMode("battle");
    } else if (review == "story-character-rrvvfo") {
        state.menu.openStoryCharacterSelect();
    } else if (review == "story-character-bark") {
        px::StoryUnlockSystem::discover(state.save, px::StoryDiscoveryEvent::BarkReunion);
        state.menu.openStoryCharacterSelect();
        state.menu.handle(px::Action::MoveRight);
    } else if (review == "story-character-wade") {
        px::StoryUnlockSystem::discover(state.save, px::StoryDiscoveryEvent::BarkReunion);
        px::StoryUnlockSystem::discover(state.save, px::StoryDiscoveryEvent::WadeReunion);
        state.menu.openStoryCharacterSelect();
        state.menu.handle(px::Action::MoveRight);
        state.menu.handle(px::Action::MoveRight);
    } else if (review == "story-character-virek") {
        px::StoryUnlockSystem::discover(state.save, px::StoryDiscoveryEvent::BarkReunion);
        px::StoryUnlockSystem::discover(state.save, px::StoryDiscoveryEvent::WadeReunion);
        px::StoryUnlockSystem::discover(state.save, px::StoryDiscoveryEvent::VirekEmeraldMissingNoticed);
        state.menu.openStoryCharacterSelect();
        state.menu.handle(px::Action::MoveLeft);
    } else if (review == "story-so-far") {
        state.menu.openStorySoFar(8, 3);
    } else if (review == "combat-manual") {
        loadRuntimeReview(state, "sage_tutorial_spar", false);
    } else if (review == "chapter1-exploration") {
        loadRuntimeReview(state, "selected_route_adventure");
    } else if (review == "rrvvfo-model") {
        loadRuntimeReview(state, "ch1_opening_sage_setup", false);
    } else if (review == "rrvvfo-legacy-comparison") {
        loadRuntimeReview(state, "ch1_opening_sage_setup", false);
        state.playerAnimation.setState("bind_pose_review");
    } else if (review == "rrvvfo-idle-bind") {
        loadRuntimeReview(state, "ch1_opening_sage_setup", false);
        state.playerAnimation.setState("bind_pose_review");
    } else if (review == "rrvvfo-idle-sampled") {
        loadRuntimeReview(state, "ch1_opening_sage_setup", false);
        state.playerAnimation.setState("idle", true);
        state.playerAnimation.seek(.375f);
    } else if (review == "rrvvfo-idle-pose-1" || review == "rrvvfo-idle-pose-3" ||
               review == "rrvvfo-idle-pose-5" || review == "rrvvfo-idle-pose-6") {
        loadRuntimeReview(state, "ch1_opening_sage_setup", false);
        state.playerAnimation.setState("idle", true);
    } else if (review == "rrvvfo-run" || review == "rrvvfo-dash" || review == "rrvvfo-charge" ||
               review == "rrvvfo-heavy" || review == "rrvvfo-fire-blast" ||
               review == "rrvvfo-object-swap" || review == "rrvvfo-lens") {
        loadRuntimeReview(state, "ch1_opening_sage_setup", false);
        const std::string clip = review == "rrvvfo-run" ? "run" :
            review == "rrvvfo-dash" ? "dash" : review == "rrvvfo-charge" ? "charge" :
            review == "rrvvfo-heavy" ? "heavy" : review == "rrvvfo-fire-blast" ? "fire_blast" :
            review == "rrvvfo-object-swap" ? "object_swap" : "lens_activate";
        state.playerAnimation.setState(clip, true);
    } else if (review == "chapter1-combat") {
        loadRuntimeReview(state, "sage_tutorial_spar");
    } else if (review == "chapter1-dialogue") {
        loadRuntimeReview(state, "tournament_outskirts_arrival");
    } else {
        throw std::runtime_error("Unknown review state: " + review);
    }
}

std::optional<px::Action> actionForKey(Sint32 key) {
    switch (key) {
        case SDLK_LEFT: case 'a': return px::Action::MoveLeft;
        case SDLK_RIGHT: case 'd': return px::Action::MoveRight;
        case SDLK_UP: case 'w': return px::Action::MoveUp;
        case SDLK_DOWN: case 's': return px::Action::MoveDown;
        case SDLK_RETURN: return px::Action::Confirm;
        case SDLK_ESCAPE: case SDLK_BACKSPACE: return px::Action::Cancel;
        case SDLK_SPACE: return px::Action::Jump;
        case SDLK_LSHIFT: case SDLK_RSHIFT: return px::Action::Dash;
        case 'j': return px::Action::Light;
        case 'k': return px::Action::Heavy;
        case 'i': return px::Action::Launcher;
        case 'u': return px::Action::Grab;
        case 'l': return px::Action::Block;
        case 'q': return px::Action::Counter;
        case 'r': return px::Action::Breaker;
        case 'c': return px::Action::Charge;
        case 'e': return px::Action::Interact;
        case '1': return px::Action::Ability1;
        case '2': return px::Action::Ability2;
        case '3': return px::Action::Ability3;
        case '4': return px::Action::Ability4;
        case '5': return px::Action::Ability5;
        default: return std::nullopt;
    }
}

std::optional<px::Action> actionForControllerMenu(Uint8 button) {
    switch (button) {
        case SDL_CONTROLLER_BUTTON_A: return px::Action::Confirm;
        case SDL_CONTROLLER_BUTTON_B: return px::Action::Cancel;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: return px::Action::MoveUp;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return px::Action::MoveDown;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return px::Action::MoveLeft;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return px::Action::MoveRight;
        case SDL_CONTROLLER_BUTTON_START: return px::Action::Pause;
        default: return std::nullopt;
    }
}

void setGameplayControllerButton(px::InputState& input, Uint8 button, bool held, bool abilityLayer) {
    switch (button) {
        case SDL_CONTROLLER_BUTTON_A:
            input.set(px::Action::Jump, !abilityLayer && held);
            input.set(px::Action::Confirm, !abilityLayer && held);
            input.set(px::Action::Ability3, abilityLayer && held);
            break;
        case SDL_CONTROLLER_BUTTON_B:
            input.set(px::Action::Grab, !abilityLayer && held);
            input.set(px::Action::Interact, !abilityLayer && held);
            input.set(px::Action::Cancel, !abilityLayer && held);
            input.set(px::Action::Ability4, abilityLayer && held);
            break;
        case SDL_CONTROLLER_BUTTON_X:
            input.set(px::Action::Light, !abilityLayer && held);
            input.set(px::Action::Ability1, abilityLayer && held);
            break;
        case SDL_CONTROLLER_BUTTON_Y:
            input.set(px::Action::Heavy, !abilityLayer && held);
            input.set(px::Action::Ability2, abilityLayer && held);
            break;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: input.set(px::Action::Block, held); break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
            input.set(px::Action::Dash, !abilityLayer && held);
            input.set(px::Action::Ability5, abilityLayer && held);
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: input.set(px::Action::Charge, !abilityLayer && held); break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: input.set(px::Action::Counter, !abilityLayer && held); break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: input.set(px::Action::Breaker, !abilityLayer && held); break;
        case SDL_CONTROLLER_BUTTON_START: input.set(px::Action::Pause, held); break;
        default: break;
    }
}

Uint8 controllerButtonState(SDL_GameController* controller, Uint8 button) {
#if defined(_WIN32)
    return SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(button));
#else
    return SDL_GameControllerGetButton(controller, button);
#endif
}

void remapGameplayControllerLayer(px::InputState& input, SDL_GameController* controller, bool abilityLayer) {
    constexpr Uint8 buttons[] = {
        SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_B, SDL_CONTROLLER_BUTTON_X, SDL_CONTROLLER_BUTTON_Y,
        SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN,
        SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT
    };
    for (const auto button : buttons)
        setGameplayControllerButton(input, button,
            controller && controllerButtonState(controller, button) != 0, abilityLayer);
}


void processMenuOutcome(ApplicationState& state) {
    const auto outcome = state.menu.outcome();
    if (outcome == px::MenuOutcome::None) return;
    if (outcome == px::MenuOutcome::BeginStory) {
        state.runtime.startChapter("rrvvfo_ch1");
        state.runtime.setQolSettings(state.save.qol);
        state.gameplay = true;
    } else if (outcome == px::MenuOutcome::ReplayChapter) {
        state.runtime.startReplayChapter(state.save);
        state.gameplay = true;
    } else if (outcome == px::MenuOutcome::ContinueStory) {
        if (state.save.story.chapterId.empty()) state.runtime.startChapter("rrvvfo_ch1");
        else state.runtime.loadSnapshot(state.save);
        state.gameplay = true;
    } else if (outcome == px::MenuOutcome::LaunchMode) {
        const auto mode = state.menu.snapshot().selectedMode.id;
        if (mode == px::MenuModeId::ArenaBattle) state.runtime.startCpuFight();
        else if (mode == px::MenuModeId::Training) state.runtime.startStandaloneTraining();
        state.runtime.setQolSettings(state.save.qol);
        state.gameplay = true;
    }
    state.menu.clearOutcome();
}

void mergeRuntimeSave(ApplicationState& state) {
    if (!state.gameplay) return;
    if (state.runtime.standaloneMode()) { state.save.qol = state.runtime.qolSettings(); return; }
    auto runtimeSave = state.runtime.saveSnapshot();
    runtimeSave.frontend.discoveredStoryRoutes = state.save.frontend.discoveredStoryRoutes;
    runtimeSave.frontend.selectedStoryRoute = state.save.frontend.selectedStoryRoute;
    runtimeSave.frontend.storySoFarSection = state.save.frontend.storySoFarSection;
    runtimeSave.frontend.pendingStoryUnlocks = state.save.frontend.pendingStoryUnlocks;
    runtimeSave.frontend.lastMenuMode = state.save.frontend.lastMenuMode;
    runtimeSave.frontend.lastBattleSelection = state.save.frontend.lastBattleSelection;
    runtimeSave.frontend.lastExtrasSelection = state.save.frontend.lastExtrasSelection;
    runtimeSave.frontend.lastOptionsSelection = state.save.frontend.lastOptionsSelection;
    state.save = std::move(runtimeSave);
}

} // namespace

int px::desktop::runSdlApplication(int argc, char** argv, const SdlPlatformConfig& platform) {
    const auto arguments = parseArguments(argc, argv, platform);
    const bool reviewRun = !arguments.review.empty() || !arguments.screenshot.empty();
    const std::filesystem::path savePath =
        std::filesystem::path(arguments.saveDirectory) / "ParallelsX-Omega-save-v5.txt";
    const bool currentSaveExists = std::filesystem::exists(savePath) || std::filesystem::exists(savePath.string() + ".bak");
    auto selectedSavePath = savePath;
    if (!currentSaveExists) {
        for (const auto& fileName : platform.legacySaveFileNames) {
            const auto candidate = std::filesystem::path(arguments.saveDirectory) / fileName;
            if (std::filesystem::exists(candidate) || std::filesystem::exists(candidate.string() + ".bak")) {
                selectedSavePath = candidate;
                break;
            }
        }
    }
    ApplicationState state(reviewRun ? px::SaveData{} : loadDevelopmentSave(selectedSavePath), arguments.assetRoot);

    try {
        prepareReview(arguments.review, state);
        if (!arguments.smoke.empty()) {
            if (arguments.smoke == "story") state.runtime.startChapter("rrvvfo_ch1");
            else if (arguments.smoke == "fight") state.runtime.startCpuFight();
            else if (arguments.smoke == "training") state.runtime.startStandaloneTraining();
            else throw std::runtime_error("Unknown smoke mode: " + arguments.smoke);
            state.runtime.setQolSettings(state.save.qol);
            state.gameplay = true;
        }
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 2;
    }

    SDL_SetHint("SDL_RENDER_DRIVER", "software");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << '\n';
        return 3;
    }
    const Uint32 windowFlags = arguments.headless ? SDL_WINDOW_HIDDEN : SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
    SDL_Window* window = SDL_CreateWindow(
        "Parallels X: Clash of Souls 3.0R - U16 Final Prototype",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, windowFlags);
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 4;
    }
    SDL_Renderer* sdlRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!sdlRenderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 5;
    }
    SDL_RenderSetLogicalSize(sdlRenderer, 1280, 720);
    SDL_SetRenderDrawBlendMode(sdlRenderer, SDL_BLENDMODE_BLEND);
    px::desktop::PresentationRenderer renderer(
        sdlRenderer, state.ui, state.menus, state.manual, state.worlds,
        state.characters, state.characterModels, state.playerAnimation);

    SDL_GameController* controller = nullptr;
    for (int i = 0; i < SDL_NumJoysticks() && !controller; ++i)
        if (SDL_IsGameController(i)) controller = SDL_GameControllerOpen(i);
    bool controllerAbilityLayer = false;
    bool releaseDisconnectPause = false;

    bool running = true;
    int renderedFrames = 0;
    Uint32 previousTicks = SDL_GetTicks();
    bool firstFrame = true;
    while (running) {
        state.input.beginFrame();
        if (releaseDisconnectPause) {
            state.input.set(px::Action::Pause, false);
            releaseDisconnectPause = false;
        }
        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) { running = false; continue; }
            if (event.type == SDL_CONTROLLERDEVICEADDED && !controller) {
                if (SDL_IsGameController(event.cdevice.which))
                    controller = SDL_GameControllerOpen(event.cdevice.which);
                continue;
            }
            if (event.type == SDL_CONTROLLERDEVICEREMOVED && controller) {
                SDL_GameControllerClose(controller);
                controller = nullptr;
                controllerAbilityLayer = false;
                if (state.gameplay && !state.runtime.view().pauseVisible) {
                    state.input.set(px::Action::Pause, true);
                    releaseDisconnectPause = true;
                }
                continue;
            }
            if (state.gameplay && (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)) {
                const bool held = event.type == SDL_MOUSEBUTTONDOWN;
                if (event.button.button == SDL_BUTTON_LEFT) state.input.set(px::Action::Light, held);
                else if (event.button.button == SDL_BUTTON_RIGHT) state.input.set(px::Action::Block, held);
                continue;
            }
            const bool down = event.type == SDL_KEYDOWN || event.type == SDL_CONTROLLERBUTTONDOWN;
            const bool up = event.type == SDL_KEYUP || event.type == SDL_CONTROLLERBUTTONUP;
            if (!down && !up) continue;

            std::optional<px::Action> action;
            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                action = actionForKey(event.key.keysym.sym);
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    action = state.gameplay ? px::Action::Pause : px::Action::Cancel;
            } else if (state.gameplay) {
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER) {
                    controllerAbilityLayer = down;
                    remapGameplayControllerLayer(state.input, controller, controllerAbilityLayer);
                } else {
                    setGameplayControllerButton(state.input, event.cbutton.button, down, controllerAbilityLayer);
                }
            } else {
                action = actionForControllerMenu(event.cbutton.button);
            }

            if (!state.gameplay && !state.manualReview && down && event.type == SDL_KEYDOWN &&
                state.menu.screen() == px::MenuScreen::StorySoFar &&
                (event.key.keysym.sym == 's' || event.key.keysym.sym == 'x')) {
                state.menu.skipStorySoFar();
                continue;
            }
            if (state.manualReview && down && action &&
                (*action == px::Action::MoveLeft || *action == px::Action::MoveUp)) {
                state.manualPage = state.manualPage == 0 ? state.manual.pages().size() - 1 : state.manualPage - 1;
                continue;
            }
            if (state.manualReview && down && action &&
                (*action == px::Action::MoveRight || *action == px::Action::MoveDown)) {
                state.manualPage = (state.manualPage + 1) % state.manual.pages().size();
                continue;
            }
            if (state.manualReview && down && action &&
                (*action == px::Action::Cancel || *action == px::Action::Pause)) {
                state.manualReview = false;
                continue;
            }
            if (!state.gameplay && !state.manualReview && down) {
                if (action) state.menu.handle(*action);
                processMenuOutcome(state);
            } else if (state.gameplay && action) {
                state.input.set(*action, down);
            }
        }

        if (state.gameplay && controller) {
            constexpr Sint16 stickDeadzone = 9000;
            constexpr Sint16 triggerThreshold = 16000;
            const Sint16 lx = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
            const Sint16 ly = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);
            const Sint16 rx = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX);
            const Sint16 ry = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY);
            const auto normalizedAxis = [&](Sint16 value) {
                if (std::abs(static_cast<int>(value)) <= stickDeadzone) return 0.0f;
                return std::clamp(static_cast<float>(value) / 32767.0f, -1.0f, 1.0f);
            };
            state.input.setMovementAxes(normalizedAxis(lx), normalizedAxis(ly));
            state.input.setCameraAxes(normalizedAxis(rx), normalizedAxis(ry));
            state.input.set(px::Action::MoveLeft, lx < -stickDeadzone);
            state.input.set(px::Action::MoveRight, lx > stickDeadzone);
            state.input.set(px::Action::MoveUp, ly < -stickDeadzone);
            state.input.set(px::Action::MoveDown, ly > stickDeadzone);
            state.input.set(px::Action::Launcher, SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > triggerThreshold);
            const bool triggerDash = !controllerAbilityLayer && SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > triggerThreshold;
            const bool dpadDash = !controllerAbilityLayer && SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP) != 0;
            state.input.set(px::Action::Dash, triggerDash || dpadDash);
        }

        const Uint32 now = SDL_GetTicks();
        const float dt = std::min(0.1f, static_cast<float>(now - previousTicks) / 1000.0f);
        previousTicks = now;
        if (state.gameplay) {
            const float frameDt = dt > 0.0f ? dt : 1.0f / 60.0f;
            state.runtime.tick(state.input, frameDt);
            if (state.runtime.consumeManualSaveRequest()) {
                mergeRuntimeSave(state);
                state.runtime.notifyManualSaveResult(writeDevelopmentSave(savePath, state.save));
            }
            if (state.runtime.consumeReturnToTitleRequest()) {
                mergeRuntimeSave(state);
                writeDevelopmentSave(savePath, state.save);
                state.gameplay = false;
                state.menu.openTitle();
            }
            const bool fixedAnimationReview = arguments.review == "rrvvfo-idle-sampled" ||
                arguments.review == "rrvvfo-idle-pose-1" || arguments.review == "rrvvfo-idle-pose-3" ||
                arguments.review == "rrvvfo-idle-pose-5" || arguments.review == "rrvvfo-idle-pose-6" ||
                arguments.review == "rrvvfo-idle-bind" || arguments.review == "rrvvfo-legacy-comparison" ||
                arguments.review == "rrvvfo-run" || arguments.review == "rrvvfo-dash" ||
                arguments.review == "rrvvfo-charge" || arguments.review == "rrvvfo-heavy" ||
                arguments.review == "rrvvfo-fire-blast" || arguments.review == "rrvvfo-object-swap" ||
                arguments.review == "rrvvfo-lens";
            if (arguments.review == "rrvvfo-idle-sampled") state.playerAnimation.seek(.375f);
            else if (arguments.review == "rrvvfo-idle-pose-1") state.playerAnimation.seek(0.0f);
            else if (arguments.review == "rrvvfo-idle-pose-3") state.playerAnimation.seek(.25f);
            else if (arguments.review == "rrvvfo-idle-pose-5") state.playerAnimation.seek(.5f);
            else if (arguments.review == "rrvvfo-idle-pose-6") state.playerAnimation.seek(.625f);
            else if (arguments.review == "rrvvfo-run") state.playerAnimation.seek(.082f);
            else if (arguments.review == "rrvvfo-dash") state.playerAnimation.seek(.120f);
            else if (arguments.review == "rrvvfo-charge") state.playerAnimation.seek(.184f);
            else if (arguments.review == "rrvvfo-heavy") state.playerAnimation.seek(.310f);
            else if (arguments.review == "rrvvfo-fire-blast") state.playerAnimation.seek(.240f);
            else if (arguments.review == "rrvvfo-object-swap") state.playerAnimation.seek(.160f);
            else if (arguments.review == "rrvvfo-lens") state.playerAnimation.seek(.160f);
            else if (!fixedAnimationReview) {
                const auto& view = state.runtime.view();
                if (state.playerAnimation.state() != view.playerAnimation)
                    state.playerAnimation.setState(view.playerAnimation);
                state.playerAnimation.setPlaybackSpeed(view.playerAnimationSpeed);
                if (view.hitFreezeSeconds <= 0.0f) state.playerAnimation.update(frameDt);
            }
        }
        else {
            const float menuDt = dt > 0.0f ? dt : 1.0f / 60.0f;
            state.menu.tick(menuDt);
            if (state.playerAnimation.state() != "idle") state.playerAnimation.setState("idle");
            state.playerAnimation.setPlaybackSpeed(1.0f);
            state.playerAnimation.update(menuDt);
        }

        if (arguments.review == "rrvvfo-model" || arguments.review == "rrvvfo-idle-bind" ||
            arguments.review == "rrvvfo-idle-sampled") renderer.renderModelReview(state.runtime.view());
        else if (arguments.review == "rrvvfo-idle-pose-1") renderer.renderIdlePoseReview(1, 0.0f);
        else if (arguments.review == "rrvvfo-idle-pose-3") renderer.renderIdlePoseReview(3, .25f);
        else if (arguments.review == "rrvvfo-idle-pose-5") renderer.renderIdlePoseReview(5, .5f);
        else if (arguments.review == "rrvvfo-idle-pose-6") renderer.renderIdlePoseReview(6, .625f);
        else if (arguments.review == "rrvvfo-legacy-comparison") renderer.renderLegacyComparison(state.runtime.view());
        else if (arguments.review == "rrvvfo-run") renderer.renderAnimationReview("RUN • LEGACY run_01..04 @82ms", .082f, .328f);
        else if (arguments.review == "rrvvfo-dash") renderer.renderAnimationReview("DASH • LEGACY dash_01..04 @60ms", .120f, .240f);
        else if (arguments.review == "rrvvfo-charge") renderer.renderAnimationReview("CHARGE • LEGACY ultimate_01..03 @92ms", .184f, .276f);
        else if (arguments.review == "rrvvfo-heavy") renderer.renderAnimationReview("HEAVY • LEGACY STARTUP / ACTIVE / RECOVERY", .310f, .620f);
        else if (arguments.review == "rrvvfo-fire-blast") renderer.renderAnimationReview("FIRE BLAST • LEGACY special_01..04", .240f, .420f);
        else if (arguments.review == "rrvvfo-object-swap") renderer.renderAnimationReview("OBJECT SWAP • LEGACY beam_01..04", .160f, .320f);
        else if (arguments.review == "rrvvfo-lens") renderer.renderAnimationReview("LENS OF TRUTH • LEGACY power_01..03", .160f, .300f);
        else if (state.manualReview) renderer.renderManual(state.manualPage);
        else if (state.gameplay && state.runtime.view().trainingManualVisible)
            renderer.renderManual(state.runtime.view().trainingManualPageIndex, &state.runtime.view());
        else if (state.gameplay) renderer.renderRuntime(state.runtime.view());
        else renderer.renderMenu(state.menu.snapshot());

        if (firstFrame && !arguments.screenshot.empty()) {
            if (!renderer.writePpm(arguments.screenshot)) {
                std::cerr << "Screenshot capture failed: " << SDL_GetError() << '\n';
                running = false;
            }
            SDL_RenderPresent(sdlRenderer);
            break;
        }
        SDL_RenderPresent(sdlRenderer);
        firstFrame = false;
        ++renderedFrames;
        if (arguments.exitAfterFrames > 0 && renderedFrames >= arguments.exitAfterFrames)
            running = false;
        SDL_Delay(16);
    }

    if (!reviewRun) {
        mergeRuntimeSave(state);
        writeDevelopmentSave(savePath, state.save);
    }
    if (controller) SDL_GameControllerClose(controller);
    SDL_DestroyRenderer(sdlRenderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    if (!arguments.smoke.empty()) {
        std::cout << "SMOKE PASS: " << arguments.smoke
                  << " | asset=" << arguments.assetRoot
                  << " | save=" << savePath.string() << '\n';
    }
    return 0;
}
