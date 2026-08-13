#pragma once
#include "core/types.hpp"
#include "core/story_progression.hpp"
#include "core/rpg_progress.hpp"
#include <string>

namespace px {

struct FrontendProgress {
    std::vector<std::string> discoveredStoryRoutes{"rrvvfo"};
    std::string selectedStoryRoute{"rrvvfo"};
    std::size_t storySoFarSection{0};
    std::vector<std::string> pendingStoryUnlocks;
    std::vector<std::string> objectiveHistory;
    std::string lastMenuMode{"story"};
    std::size_t lastBattleSelection{0};
    std::size_t lastExtrasSelection{0};
    std::size_t lastOptionsSelection{0};
    std::string currentArea;
    std::string currentObjective;
    int storyProgressPercent{0};
    float playtimeSeconds{0.0f};
};

// Small, portable QoL preferences live beside the save so every platform uses
// the same behavior. Renderers may present these options differently, but they
// must not invent different gameplay rules.
struct QolSettings {
    bool holdToAdvanceDialogue{true};
    bool firstTimeHints{true};
    bool reducedMotion{false};
    bool reducedCameraShake{false};
    bool reducedFlashes{false};
    bool highContrastHud{false};
    bool largerText{false};
    std::string combatMessages{"full"};
    std::string objectiveDisplay{"full"};
    float cameraSensitivity{1.0f};
    bool invertCameraX{false};
    bool invertCameraY{false};
    bool gentleCameraRecenter{true};
    float hudScale{1.0f};
    float dialogueScale{1.0f};
    std::string dialogueSpeed{"normal"};
    bool dialogueAutoAdvance{false};
};

struct SaveData {
    static constexpr int kSchemaVersion = 7;
    int schemaVersion{kSchemaVersion};
    StoryState story;
    PlayerWorldState world;
    FrontendProgress frontend;
    QolSettings qol;
    TournamentCardState tournamentCard{"rrvvfo"};
    AdventureRecordsState records;
    RpgProgressState rpg;
    std::string inputPreset{"modern"};
};

class SaveCodec {
public:
    static std::string serialize(const SaveData& data);
    static SaveData deserialize(const std::string& text);
};

} // namespace px
