#include "core/game.hpp"
#include <stdexcept>

namespace px {

Game::Game(const ChapterRegistry& chapters) : chapters_(chapters) {}

void Game::startChapter(const std::string& chapterId) {
    const auto& def = chapters_.get(chapterId);
    if (def.openingFlow.empty()) throw std::runtime_error("Chapter has no opening flow");
    save_.story.chapterId = chapterId;
    save_.story.pendingChapterId.clear();
    save_.story.sceneIndex = 0;
    save_.story.checkpointId = def.openingFlow.front().checkpointId;
    save_.world.mapId = def.primaryMap;
    syncModeToScene();
}

void Game::loadSave(const SaveData& data) {
    if (!chapters_.has(data.story.chapterId)) throw std::runtime_error("Save references unknown chapter");
    const auto& def = chapters_.get(data.story.chapterId);
    if (data.story.sceneIndex >= def.openingFlow.size()) throw std::runtime_error("Save scene index is out of range");
    save_ = data;
    syncModeToScene();
}

void Game::advanceScene() {
    const auto& def = chapter();
    if (save_.story.sceneIndex + 1 >= def.openingFlow.size()) {
        if (!def.nextChapterId.empty()) {
            if (chapters_.has(def.nextChapterId)) startChapter(def.nextChapterId);
            else save_.story.pendingChapterId = def.nextChapterId;
        }
        return;
    }
    ++save_.story.sceneIndex;
    save_.story.checkpointId = def.openingFlow[save_.story.sceneIndex].checkpointId;
    syncModeToScene();
}

void Game::pause() {
    if (mode_ == GameMode::Pause) return;
    modeBeforePause_ = mode_;
    mode_ = GameMode::Pause;
}

void Game::resume() {
    if (mode_ != GameMode::Pause) return;
    mode_ = modeBeforePause_;
}

const ChapterDefinition& Game::chapter() const {
    if (save_.story.chapterId.empty()) throw std::runtime_error("No chapter active");
    return chapters_.get(save_.story.chapterId);
}

const SceneStep& Game::scene() const {
    const auto& def = chapter();
    if (save_.story.sceneIndex >= def.openingFlow.size()) throw std::runtime_error("Scene index out of range");
    return def.openingFlow[save_.story.sceneIndex];
}

void Game::syncModeToScene() {
    switch (scene().kind) {
        case SceneKind::Cutscene: mode_ = GameMode::Cutscene; break;
        case SceneKind::Exploration: mode_ = GameMode::Exploration; break;
        case SceneKind::Arena: mode_ = GameMode::ArenaCombat; break;
    }
}

} // namespace px
