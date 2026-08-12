#pragma once
#include "core/math.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace px {

enum class ExplorationRuleKind {
    ReachPoint,
    InteractPoint,
    UseAbilityPoint,
    ChooseRoute,
    RouteChallenge,
    TerrainDetour,
    SwapRelay,
    CliffJumpRoute,
    QteSequence,
    RoadsideEncounter,
    MandatoryAbilityReveal,
    InteractionSequence,
    TimedCheckpointSequence
};

struct RouteChoiceDefinition {
    std::string id;
    std::string label;
};

struct RouteChallenge {
    std::string routeId;
    std::string objective;
    Vec2 finish;
    float finishRadius{80.0f};
    std::string requiredAbilityId;
    Vec2 abilityTarget{};
    float abilityRadius{100.0f};
    std::string blockerToDisable;
};

struct ExplorationDefinition {
    std::string sceneId;
    ExplorationRuleKind rule{ExplorationRuleKind::ReachPoint};
    std::string objective;
    std::string detail;
    Vec2 target{};
    float radius{90.0f};
    std::string requiredAbilityId;
    std::vector<RouteChoiceDefinition> routes;
    std::vector<RouteChallenge> routeChallenges;
    std::vector<Vec2> relayMarkers;
    std::vector<std::string> blockersToDisable;
    std::string optionalAbilityId;
    std::string optionalBlockerToDisable;
    Vec2 shortcutPoint{};
    float shortcutRadius{120.0f};
    bool hasPlayerStart{false};
    Vec2 playerStart{};
    bool companionVisible{false};
    std::string companionId;
    Vec2 companionPosition{};
    std::string adventureId;
    std::string openingDialogueId;
    std::string completionDialogueId;
    Vec2 swapDestination{};
    bool swapPhysicalObject{false};
    std::vector<Vec2> jumpMarkers;
    float jumpMarkerRadius{72.0f};
    // Omega keeps the browser-authoritative route counts/order while mapping them into the native 3D road.
    std::vector<Vec2> mainWorkMarkers;
    std::vector<Vec2> forestBellMarkers;
    float routeHintFirstSeconds{18.0f};
    float routeHintSecondSeconds{36.0f};
    Vec2 detourDashMarker{};
    float detourDashRadius{90.0f};
    std::string qteId;
// Shared authored sequence data. Any chapter may use these; they are not tournament-specific engine code.
std::vector<Vec2> sequenceMarkers;
std::vector<std::string> sequenceLabels;
std::vector<std::string> sequenceDialogueIds;
float sequenceRadius{82.0f};
float sequenceTargetSeconds{0.0f};
bool sequenceRequiresInteract{true};
    std::string encounterId;
};

class ExplorationRegistry {
public:
    ExplorationRegistry();
    const ExplorationDefinition& get(const std::string& sceneId) const;
    bool has(const std::string& sceneId) const;
private:
    std::unordered_map<std::string, ExplorationDefinition> scenes_;
};

} // namespace px
