#pragma once
#include "core/math.hpp"
#include "core/types.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace px {

struct AdventureNpcDefinition {
    std::string id;
    std::string label;
    std::string characterPresentationId;
    Vec2 basePosition{};
    float motionPhase{0.0f};
    float interactionRadius{115.0f};
    std::string dialogueId;
    std::string helpedDialogueId;
    std::string declinedDialogueId;
    bool offersChoice{false};
};

struct AdventureQteDefinition {
    std::string id;
    std::string title;
    std::vector<Action> sequence;
    float durationSeconds{0.0f};
    int safeRetries{0};
    bool failForward{false};
};

struct AdventureEncounterDefinition {
    std::string id;
    std::string opponentId;
    std::string opponentName;
    Vec2 playerStart{};
    Vec2 opponentStart{};
    float startingEnergy{45.0f};
    int koTarget{1};
};

struct AdventureAmbientLifeDefinition {
    int birdCount{0};
    float birdStartX{0.0f};
    float birdTravelDistance{0.0f};
    float birdSpeed{0.0f};
    float birdStartZ{0.0f};
    float birdZSpacing{0.0f};
    Vec2 deliveryCartStart{};
    float deliveryCartTravelDistance{0.0f};
    float deliveryCartSpeed{0.0f};
    Vec2 savedCartPosition{};
};

struct AdventureAbilityPuzzleDefinition {
    std::string id;
    std::string title;
    std::string npcId;
    Vec2 lensTarget{};
    Vec2 swapTarget{};
    float radius{100.0f};
    std::string introDialogueId;
    std::string lensDialogueId;
    std::string completionDialogueId;
    std::string repeatDialogueId;
};

struct AdventureDefinition {
    std::string id;
    std::string presentationStageId;
    std::vector<AdventureNpcDefinition> npcs;
    std::vector<Vec2> cliffJumpMarkers;
    float cliffJumpRadius{72.0f};
    Vec2 farBankRock{};
    std::vector<Vec2> gateRelayMarkers;
    float gateReleaseDelaySeconds{0.52f};
    Vec2 transportWheel{};
    AdventureQteDefinition runawayCart;
    AdventureEncounterDefinition roadsideFight;
    AdventureAbilityPuzzleDefinition sidePuzzle;
    AdventureAmbientLifeDefinition ambientLife;
};

class AdventureRegistry {
public:
    AdventureRegistry();
    const AdventureDefinition& get(const std::string& id) const;
    bool has(const std::string& id) const;

private:
    std::unordered_map<std::string, AdventureDefinition> adventures_;
};

} // namespace px
