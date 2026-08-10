#pragma once
#include "core/math.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace px {

enum class GameMode : std::uint8_t {
    Title,
    MainMenu,
    Cutscene,
    Exploration,
    ArenaCombat,
    Pause
};

enum class Action : std::uint8_t {
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    Jump,
    Light,
    Heavy,
    Launcher,
    Grab,
    Block,
    Counter,
    Breaker,
    Charge,
    Dash,
    Interact,
    Ability1,
    Ability2,
    Ability3,
    Ability4,
    Ability5,
    AbilityUse,
    Pause,
    Confirm,
    Cancel
};

enum class SceneKind : std::uint8_t {
    Cutscene,
    Exploration,
    Arena
};

struct SceneStep {
    SceneKind kind{};
    std::string id;
    std::string checkpointId;
    std::string presentationStageId;
};

struct ChapterDefinition {
    std::string id;
    std::string title;
    std::string primaryMap;
    std::string playableCharacter;
    std::vector<SceneStep> openingFlow;
    // Empty until the next chapter is registered. When set, Story Mode crosses
    // this boundary directly; it never returns to a menu or mission selector.
    std::string nextChapterId;
};

struct StoryState {
    std::string routeId{"rrvvfo"};
    std::string chapterId;
    std::size_t sceneIndex{0};
    std::string checkpointId;
    // Set at an audited boundary when the next chapter content is not registered yet.
    // Story remains in its completion state; no player-facing menu is introduced.
    std::string pendingChapterId;
    std::vector<std::string> flags;
};

struct WorldObjectState {
    std::string id;
    Vec2 position{};
    bool active{true};
};

struct PlayerWorldState {
    std::string mapId;
    Vec2 position{};
    std::string routeChoice;
    float hp{100.0f};
    float energy{100.0f};
    float guard{100.0f};
    std::vector<WorldObjectState> objects;
};

} // namespace px
