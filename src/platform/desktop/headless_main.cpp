#include "content/chapter_registry.hpp"
#include "content/cutscene_registry.hpp"
#include "content/map_registry.hpp"
#include "core/game.hpp"
#include "core/save.hpp"
#include <iostream>

static const char* modeName(px::GameMode mode) {
    switch(mode) {
        case px::GameMode::Cutscene: return "Cutscene";
        case px::GameMode::Exploration: return "Exploration";
        case px::GameMode::ArenaCombat: return "ArenaCombat";
        case px::GameMode::Pause: return "Pause";
        case px::GameMode::Title: return "Title";
        case px::GameMode::MainMenu: return "MainMenu";
    }
    return "Unknown";
}

int main() {
    px::ChapterRegistry chapters;
    px::MapRegistry maps;
    px::CutsceneRegistry cutscenes;
    px::Game game(chapters);
    game.startChapter("rrvvfo_ch1");

    std::cout << "Parallels X 3.0R 0.4H — Golden Gate QoL\n";
    std::cout << "ONE SHARED ENGINE • Legacy 2.9A.40.7.1.1 reconstruction\n\n";
    const auto& ch = game.chapter();
    std::cout << "Chapter: " << ch.title << "\n";
    std::cout << "Playable: " << ch.playableCharacter << "\n";
    std::cout << "Map: " << maps.get(ch.primaryMap).name << "\n\n";

    for (std::size_t i = 0; i < ch.openingFlow.size(); ++i) {
        std::cout << '[' << modeName(game.mode()) << "] " << game.scene().id
                  << " • checkpoint=" << game.scene().checkpointId << '\n';
        if (i + 1 < ch.openingFlow.size()) game.advanceScene();
    }

    const auto save = px::SaveCodec::serialize(game.saveData());
    std::cout << "\nSave schema preview:\n" << save;
    return 0;
}
