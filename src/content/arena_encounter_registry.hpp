#pragma once
#include "core/combat.hpp"
#include "core/world.hpp"
#include <string>
#include <unordered_map>

namespace px {

enum class StoryArenaResolution { PlayerVictory, PloukeStoryFinal };

struct ArenaEncounterDefinition {
    std::string sceneId;
    std::string mapId;
    std::string opponentId;
    std::string opponentLabel;
    AiArchetype ai{AiArchetype::Balanced};
    Vec2 playerStart{};
    Vec2 opponentStart{};
    Rect ringBounds{};
    int stockTarget{3};
    float startingEnergy{45.0f};
    float opponentMoveSpeed{154.0f};
    int recommendedLevel{1};
    int xpReward{0};
    bool official{true};
    StoryArenaResolution resolution{StoryArenaResolution::PlayerVictory};
    std::string postDialogueId;
};

class ArenaEncounterRegistry {
public:
    ArenaEncounterRegistry();
    const ArenaEncounterDefinition& get(const std::string& sceneId) const;
    bool has(const std::string& sceneId) const;
private:
    std::unordered_map<std::string, ArenaEncounterDefinition> encounters_;
};

} // namespace px
