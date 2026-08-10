#include "content/chapter_registry.hpp"
#include "content/adventure_registry.hpp"
#include "content/cutscene_registry.hpp"
#include "content/dialogue_registry.hpp"
#include "content/exploration_registry.hpp"
#include "content/map_registry.hpp"
#include "content/training_registry.hpp"
#include "core/input.hpp"
#include "core/runtime.hpp"
#include <iostream>

int main() {
    px::ChapterRegistry chapters;
    px::MapRegistry maps;
    px::CutsceneRegistry cutscenes;
    px::DialogueRegistry dialogue;
    px::ExplorationRegistry exploration;
    px::TrainingRegistry training;
    px::AdventureRegistry adventures;
    px::RuntimeSession runtime(chapters, maps, cutscenes, dialogue, exploration, training, adventures);
    px::InputState input;

    runtime.startChapter("rrvvfo_ch1");
    std::cout << "Parallels X 3.0R shared runtime smoke\n";
    std::cout << "scene=" << runtime.view().sceneId << "\n";
    std::cout << "speaker=" << runtime.view().dialogueSpeaker << "\n";

    // Walk the first directed scene into the restored shared Object Swap field trial.
    while (runtime.view().mode == px::GameMode::Cutscene) runtime.confirm();
    std::cout << "scene=" << runtime.view().sceneId << "\n";
    std::cout << "objective=" << runtime.view().objective << "\n";

    return runtime.view().sceneId == "sage_object_swap_field_trial" ? 0 : 1;
}
