#include "content/adventure_registry.hpp"
#include <stdexcept>

namespace px {

AdventureRegistry::AdventureRegistry() {
    AdventureDefinition road;
    road.id = "rrvvfo_ch1_road";
    road.presentationStageId = "training-road";
    road.npcs = {
        {"dojo_student", "DOJO STUDENT", "ambient_student", {-900.0f, 300.0f}, 0.0f, 115.0f, "road_npc_dojo_student", "", "", false},
        {"traveler", "TRAVELER", "ambient_traveler", {-520.0f, -300.0f}, 1.7f, 115.0f, "road_npc_traveler", "", "", false},
        {"road_worker", "ROAD WORKER", "ambient_worker", {365.0f, 300.0f}, 3.1f, 115.0f, "road_npc_worker", "", "", false},
        {"lost_competitor", "LOST COMPETITOR", "ambient_competitor", {730.0f, -285.0f}, 4.1f, 115.0f,
            "road_npc_lost_competitor", "road_npc_lost_competitor_helped", "road_npc_lost_competitor_declined", true},
        {"tournament_fan", "TOURNAMENT FAN", "ambient_fan", {1480.0f, 285.0f}, 5.2f, 115.0f, "road_npc_fan", "", "", false},
        {"vendor", "VENDOR", "ambient_vendor", {1750.0f, 260.0f}, 4.6f, 115.0f, "road_npc_vendor", "", "", false},
        {"sign_painter", "SIGN PAINTER", "ambient_painter", {1840.0f, -300.0f}, 2.4f, 115.0f, "road_npc_sign_painter", "", "", false}
    };
    road.cliffJumpMarkers = {{300.0f, 330.0f}, {355.0f, 400.0f}, {412.0f, 350.0f}};
    road.cliffJumpRadius = 72.0f;
    road.farBankRock = {230.0f, 0.0f};
    road.gateRelayMarkers = {{468.0f, -96.0f}, {522.0f, 96.0f}, {570.0f, 0.0f}};
    road.gateReleaseDelaySeconds = 0.52f;
    road.transportWheel = {770.0f, -210.0f};
    road.runawayCart = {
        "runaway_tournament_cart", "INTERCEPT THE RUNAWAY CART",
        {Action::MoveRight, Action::Jump, Action::MoveLeft, Action::MoveRight}, 6.2f, 1, true
    };
    road.roadsideFight = {
        "roadside_challenger", "road_fighter", "Roadside Fighter", {790.0f, 70.0f}, {940.0f, -70.0f}, 45.0f, 1
    };
    road.sidePuzzle = {
        "sign_that_points_back", "THE SIGN THAT POINTS BACK", "sign_painter",
        {1760.0f, -245.0f}, {1870.0f, -285.0f}, 105.0f,
        "sign_quest_intro", "sign_quest_lens_reveal", "sign_quest_complete", "sign_quest_repeat"
    };
    road.ambientLife = {5, -1300.0f, 2900.0f, 95.0f, -520.0f, 245.0f,
                        {-620.0f, 520.0f}, 1900.0f, 42.0f, {1080.0f, 420.0f}};
    adventures_.emplace(road.id, road);
}

const AdventureDefinition& AdventureRegistry::get(const std::string& id) const {
    const auto it = adventures_.find(id);
    if (it == adventures_.end()) throw std::out_of_range("Unknown adventure definition: " + id);
    return it->second;
}

bool AdventureRegistry::has(const std::string& id) const {
    return adventures_.find(id) != adventures_.end();
}

} // namespace px
