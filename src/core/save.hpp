#pragma once
#include "core/types.hpp"
#include <string>

namespace px {

struct FrontendProgress {
    std::vector<std::string> discoveredStoryRoutes{"rrvvfo"};
    std::string selectedStoryRoute{"rrvvfo"};
    std::size_t storySoFarSection{0};
    std::vector<std::string> pendingStoryUnlocks;
    std::vector<std::string> objectiveHistory;
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
};

struct SaveData {
    static constexpr int kSchemaVersion = 4;
    int schemaVersion{kSchemaVersion};
    StoryState story;
    PlayerWorldState world;
    FrontendProgress frontend;
    QolSettings qol;
    std::string inputPreset{"modern"};
};

class SaveCodec {
public:
    static std::string serialize(const SaveData& data);
    static SaveData deserialize(const std::string& text);
};

} // namespace px
