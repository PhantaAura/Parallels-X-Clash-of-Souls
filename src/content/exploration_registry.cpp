#include "content/exploration_registry.hpp"
#include <stdexcept>

namespace px {
namespace {

constexpr const char* kRoadAdventure = "rrvvfo_ch1_road";

ExplorationDefinition roadScene(std::string id, ExplorationRuleKind rule, std::string objective, std::string detail) {
    ExplorationDefinition scene;
    scene.sceneId = std::move(id);
    scene.rule = rule;
    scene.objective = std::move(objective);
    scene.detail = std::move(detail);
    scene.adventureId = kRoadAdventure;
    return scene;
}

} // namespace

ExplorationRegistry::ExplorationRegistry() {
    ExplorationDefinition fieldAnchors;
    fieldAnchors.sceneId = "sage_object_swap_field_trial";
    fieldAnchors.rule = ExplorationRuleKind::SwapRelay;
    fieldAnchors.objective = "SAGE FIELD ANCHORS • 0 / 3";
    fieldAnchors.detail = "Use Object Swap [2] at each anchor. No walking between them.";
    fieldAnchors.radius = 900.0f;
    fieldAnchors.requiredAbilityId = "objectSwap";
    fieldAnchors.hasPlayerStart = true;
    fieldAnchors.playerStart = {-1260.0f, 90.0f};
    fieldAnchors.companionVisible = true;
    fieldAnchors.companionId = "sage";
    fieldAnchors.companionPosition = {-1080.0f, 110.0f};
    fieldAnchors.relayMarkers = {{-1410.0f, -105.0f}, {-1015.0f, 225.0f}, {-1225.0f, -255.0f}};
    scenes_.emplace(fieldAnchors.sceneId, fieldAnchors);

    auto departure = roadScene(
        "tournament_road_departure", ExplorationRuleKind::ReachPoint,
        "LEAVE THE TRAINING GROUNDS", "Follow the tan road east toward the tournament banners.");
    departure.target = {-785.0f, 0.0f};
    departure.radius = 55.0f;
    scenes_.emplace(departure.sceneId, departure);

    auto river = roadScene(
        "river_object_swap_problem", ExplorationRuleKind::UseAbilityPoint,
        "SWAP WITH THE FAR-BANK ROCK", "Stand at the river edge and use [2] Object Swap. The river remains solid.");
    river.target = {-75.0f, 0.0f};
    river.radius = 120.0f;
    river.requiredAbilityId = "objectSwap";
    river.swapDestination = {230.0f, 0.0f};
    river.swapPhysicalObject = true;
    scenes_.emplace(river.sceneId, river);

    auto routeChoice = roadScene(
        "legacy_route_choice", ExplorationRuleKind::ChooseRoute,
        "CHOOSE A ROUTE", "Main Road, Forest Shortcut, or Cliff Route.");
    routeChoice.routes = {
        {"main", "MAIN ROAD"},
        {"forest", "FOREST SHORTCUT"},
        {"cliff", "CLIFF ROUTE"}
    };
    scenes_.emplace(routeChoice.sceneId, routeChoice);

    auto routeAdventure = roadScene(
        "selected_route_adventure", ExplorationRuleKind::RouteChallenge,
        "FOLLOW YOUR ROUTE TO THE SWAP RELAY", "All three authored routes reconnect at the same gate.");
    routeAdventure.routeChallenges = {
        {"main", "MAIN ROAD • CONTROL THE FIRE", {430.0f, 0.0f}, 55.0f,
            "fireBlast", {270.0f, 0.0f}, 115.0f, "fallen_tree_center"},
        {"forest", "FOREST SHORTCUT • FOLLOW THE BELLS", {430.0f, -390.0f}, 90.0f, "", {}, 100.0f, ""},
        {"cliff", "CLIFF ROUTE • STAY ON THE SOUTH LEDGE", {430.0f, 390.0f}, 90.0f, "", {}, 100.0f, ""}
    };
    // 7.2R parity counts: Main = four work-lane beats, Forest = four sequential bells, Cliff = five ledges.
    // These native 3D positions preserve the existing route geography; exact browser pixel coordinates are not fabricated.
    routeAdventure.mainWorkMarkers = {{305.0f, 42.0f}, {340.0f, -34.0f}, {378.0f, 38.0f}, {414.0f, 0.0f}};
    routeAdventure.forestBellMarkers = {{286.0f, -242.0f}, {327.0f, -322.0f}, {371.0f, -398.0f}, {418.0f, -350.0f}};
    routeAdventure.jumpMarkers = {{286.0f, 305.0f}, {325.0f, 365.0f}, {360.0f, 425.0f}, {397.0f, 388.0f}, {425.0f, 342.0f}};
    routeAdventure.jumpMarkerRadius = 72.0f;
    routeAdventure.routeHintFirstSeconds = 18.0f;
    routeAdventure.routeHintSecondSeconds = 36.0f;
    routeAdventure.openingDialogueId = "main_route_worker_intro";
    routeAdventure.completionDialogueId = "main_route_worker_result";
    scenes_.emplace(routeAdventure.sceneId, routeAdventure);

    auto relay = roadScene(
        "swap_relay_trial", ExplorationRuleKind::SwapRelay,
        "OBJECT SWAP RELAY • 0 / 3", "Use Object Swap on each marker. Every swap changes the next angle.");
    relay.requiredAbilityId = "objectSwap";
    relay.radius = 155.0f;
    relay.relayMarkers = {{468.0f, -96.0f}, {522.0f, 96.0f}, {570.0f, 0.0f}};
    relay.blockersToDisable = {"swap_gate", "swap_gate_forest", "swap_gate_cliff"};
    scenes_.emplace(relay.sceneId, relay);

    auto transport = roadScene(
        "transport_wheel_recovery", ExplorationRuleKind::SwapRelay,
        "RECOVER THE TRANSPORT WHEEL", "Swap to the stranded wheel, then use the return anchor to get back.");
    transport.radius = 250.0f;
    transport.requiredAbilityId = "objectSwap";
    transport.relayMarkers = {{770.0f, -210.0f}, {0.0f, 0.0f}};
    transport.completionDialogueId = "transport_wheel_result";
    scenes_.emplace(transport.sceneId, transport);

    auto runaway = roadScene(
        "runaway_tournament_cart", ExplorationRuleKind::QteSequence,
        "INTERCEPT THE RUNAWAY CART", "Clear the road, then swap the wheel block into place.");
    runaway.qteId = "runaway_tournament_cart";
    runaway.openingDialogueId = "runaway_cart_intro";
    runaway.completionDialogueId = "runaway_cart_result";
    scenes_.emplace(runaway.sceneId, runaway);

    auto encounter = roadScene(
        "roadside_encounter", ExplorationRuleKind::RoadsideEncounter,
        "CONTINUE THROUGH THE PRACTICE CLEARING", "The supply cart is safe. A roaming fighter is ahead.");
    encounter.encounterId = "roadside_challenger";
    encounter.target = {780.0f, 0.0f};
    encounter.radius = 42.0f;
    scenes_.emplace(encounter.sceneId, encounter);

    auto checkpoint = roadScene(
        "reach_tournament_checkpoint", ExplorationRuleKind::ReachPoint,
        "PASS THE TOURNAMENT CHECKPOINT", "Continue east and speak with the checkpoint worker.");
    checkpoint.target = {915.0f, 0.0f};
    checkpoint.radius = 45.0f;
    scenes_.emplace(checkpoint.sceneId, checkpoint);

    auto lensApproach = roadScene(
        "reach_lens_roadblock", ExplorationRuleKind::ReachPoint,
        "CONTINUE TO THE OUTSKIRTS", "The final manual check is near the stadium roadblock.");
    lensApproach.target = {995.0f, 0.0f};
    lensApproach.radius = 42.0f;
    scenes_.emplace(lensApproach.sceneId, lensApproach);

    auto lens = roadScene(
        "lens_roadblock_reveal", ExplorationRuleKind::MandatoryAbilityReveal,
        "CHECK THE SUSPICIOUS ROADBLOCK", "Use [3] Lens of Truth to reveal the real route.");
    lens.target = {1000.0f, 0.0f};
    lens.radius = 160.0f;
    lens.requiredAbilityId = "lensOfTruth";
    lens.blockersToDisable = {"lens_roadblock", "lens_roadblock_north", "lens_roadblock_south"};
    scenes_.emplace(lens.sceneId, lens);

    auto outskirts = roadScene(
        "reach_tournament_outskirts", ExplorationRuleKind::ReachPoint,
        "REACH THE TOURNAMENT ENTRANCE", "The stadium is directly ahead.");
    outskirts.target = {1280.0f, -10.0f};
    outskirts.radius = 35.0f;
    scenes_.emplace(outskirts.sceneId, outskirts);
}

const ExplorationDefinition& ExplorationRegistry::get(const std::string& sceneId) const {
    const auto it = scenes_.find(sceneId);
    if (it == scenes_.end()) throw std::out_of_range("No exploration definition for scene: " + sceneId);
    return it->second;
}

bool ExplorationRegistry::has(const std::string& sceneId) const {
    return scenes_.find(sceneId) != scenes_.end();
}

} // namespace px
