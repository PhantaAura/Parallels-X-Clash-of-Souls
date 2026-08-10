#pragma once
#include "content/chapter_registry.hpp"
#include "core/save.hpp"
#include "core/types.hpp"
#include <string>

namespace px {

class Game {
public:
    explicit Game(const ChapterRegistry& chapters);

    void startChapter(const std::string& chapterId);
    void loadSave(const SaveData& data);
    void advanceScene();
    void pause();
    void resume();

    GameMode mode() const { return mode_; }
    const StoryState& story() const { return save_.story; }
    const ChapterDefinition& chapter() const;
    const SceneStep& scene() const;
    const SaveData& saveData() const { return save_; }
    bool hasPendingChapter() const { return !save_.story.pendingChapterId.empty(); }
    GameMode modeBeforePause() const { return modeBeforePause_; }

private:
    void syncModeToScene();

    const ChapterRegistry& chapters_;
    SaveData save_;
    GameMode mode_{GameMode::Title};
    GameMode modeBeforePause_{GameMode::Title};
};

} // namespace px
