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
    fieldAnchors.detail = "Use Object Swap at each anchor. No walking between them.";
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
        "SWAP WITH THE FAR-BANK ROCK", "Stand at the river edge and use Object Swap on the far-bank rock. The river remains solid.");
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
        {"main", "MAIN ROAD • CONTROL THE FIRE", {535.0f, 105.0f}, 65.0f,
            "fireBlast", {270.0f, 0.0f}, 115.0f, "fallen_tree_center"},
        {"forest", "FOREST SHORTCUT • FOLLOW THE BELLS", {535.0f, -300.0f}, 95.0f, "", {}, 100.0f, ""},
        {"cliff", "CLIFF ROUTE • STAY ON THE SOUTH LEDGE", {540.0f, 255.0f}, 95.0f, "", {}, 100.0f, ""}
    };
    // 7.2R parity counts: Main = four work-lane beats, Forest = four sequential bells, Cliff = five ledges.
    // These native 3D positions preserve the existing route geography; exact browser pixel coordinates are not fabricated.
    routeAdventure.mainWorkMarkers = {{305.0f, 265.0f}, {365.0f, 185.0f}, {438.0f, 285.0f}, {520.0f, 105.0f}};
    routeAdventure.forestBellMarkers = {{285.0f, -250.0f}, {345.0f, -430.0f}, {430.0f, -540.0f}, {520.0f, -300.0f}};
    routeAdventure.jumpMarkers = {{285.0f, 255.0f}, {340.0f, 420.0f}, {410.0f, 550.0f}, {480.0f, 430.0f}, {535.0f, 255.0f}};
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
        "INTERCEPT THE RUNAWAY CART", "Cut across the road using the shown four-step movement sequence.");
    runaway.qteId = "runaway_tournament_cart";
    runaway.openingDialogueId = "runaway_cart_intro";
    runaway.completionDialogueId = "runaway_cart_result";
    scenes_.emplace(runaway.sceneId, runaway);

    auto encounter = roadScene(
        "roadside_encounter", ExplorationRuleKind::RoadsideEncounter,
        "CONTINUE THROUGH THE PRACTICE CLEARING", "The supply cart is safe. Continue through the practice clearing.");
    encounter.encounterId = "roadside_challenger";
    encounter.target = {780.0f, 0.0f};
    encounter.radius = 42.0f;
    scenes_.emplace(encounter.sceneId, encounter);

    auto checkpoint = roadScene(
        "reach_tournament_checkpoint", ExplorationRuleKind::ReachPoint,
        "PASS THE TOURNAMENT CHECKPOINT", "Continue east and speak with the checkpoint worker.");
    auto collapse = roadScene(
        "collapsed_tournament_road_detour", ExplorationRuleKind::TerrainDetour,
        "FIND A WAY AROUND THE COLLAPSE", "The direct road is gone. Take the high north-side trail and reconnect ahead.");
    collapse.openingDialogueId = "terrain_collapse_intro";
    collapse.completionDialogueId = "terrain_collapse_complete";
    collapse.jumpMarkers = {{900.0f, -300.0f}, {970.0f, -430.0f}, {1045.0f, -515.0f}};
    collapse.jumpMarkerRadius = 74.0f;
    collapse.detourDashMarker = {1120.0f, -520.0f};
    collapse.detourDashRadius = 88.0f;
    collapse.requiredAbilityId = "objectSwap";
    collapse.shortcutPoint = {1205.0f, -455.0f};
    collapse.shortcutRadius = 125.0f;
    collapse.swapDestination = {1285.0f, -285.0f};
    collapse.target = {1340.0f, -70.0f};
    collapse.radius = 85.0f;
    collapse.blockersToDisable = {"detour_jump_gate_1", "detour_jump_gate_2", "detour_jump_gate_3", "detour_dash_gate", "detour_swap_gate"};
    scenes_.emplace(collapse.sceneId, collapse);

    checkpoint.target = {1420.0f, 0.0f};
    checkpoint.radius = 45.0f;
    scenes_.emplace(checkpoint.sceneId, checkpoint);

    auto lensApproach = roadScene(
        "reach_lens_roadblock", ExplorationRuleKind::ReachPoint,
        "CONTINUE TO THE OUTSKIRTS", "The final manual check is near the stadium roadblock.");
    lensApproach.target = {1650.0f, 0.0f};
    lensApproach.radius = 42.0f;
    scenes_.emplace(lensApproach.sceneId, lensApproach);

    auto lens = roadScene(
        "lens_roadblock_reveal", ExplorationRuleKind::MandatoryAbilityReveal,
        "CHECK THE SUSPICIOUS ROADBLOCK", "Use Lens of Truth to reveal the real route.");
    lens.target = {1700.0f, 0.0f};
    lens.radius = 160.0f;
    lens.requiredAbilityId = "lensOfTruth";
    lens.blockersToDisable = {"lens_roadblock", "lens_roadblock_north", "lens_roadblock_south"};
    scenes_.emplace(lens.sceneId, lens);

    auto outskirts = roadScene(
        "reach_tournament_outskirts", ExplorationRuleKind::ReachPoint,
        "REACH THE TOURNAMENT ENTRANCE", "The stadium is directly ahead.");
    outskirts.target = {1960.0f, -10.0f};
    outskirts.radius = 35.0f;
    scenes_.emplace(outskirts.sceneId, outskirts);

ExplorationDefinition bracket;
bracket.sceneId="ch2_lost_bracket"; bracket.rule=ExplorationRuleKind::InteractionSequence;
bracket.objective="THE LOST BRACKET • FIND THREE CONTESTANT CARDS";
bracket.detail="Search the grounds while registration is delayed.";
bracket.hasPlayerStart=true; bracket.playerStart={-1190.0f,80.0f};
bracket.sequenceMarkers={{-850.0f,-790.0f},{-600.0f,-615.0f},{-45.0f,-220.0f}};
bracket.sequenceLabels={"WADE'S CARD","BARK'S CARD","QUALIFIER CARD"};
bracket.sequenceDialogueIds={"ch2_bracket_wade","ch2_bracket_bark","ch2_bracket_qualifier"};
bracket.sequenceRadius=92.0f; bracket.sequenceRequiresInteract=true;
bracket.completionDialogueId="ch2_bracket_return";
scenes_.emplace(bracket.sceneId,bracket);

ExplorationDefinition wadeRace;
wadeRace.sceneId="ch2_wade_shortcut"; wadeRace.rule=ExplorationRuleKind::TimedCheckpointSequence;
wadeRace.objective="WADE'S SHORTCUT • FIVE DISTRICTS"; wadeRace.detail="Finish the route. Beat 32 seconds for Wade's target.";
wadeRace.hasPlayerStart=true; wadeRace.playerStart={-1120.0f,160.0f};
wadeRace.sequenceMarkers={{-860,-650},{-460,-850},{40,-500},{-440,700},{420,760}};
wadeRace.sequenceRadius=105; wadeRace.sequenceTargetSeconds=32.0f; wadeRace.sequenceRequiresInteract=false;
wadeRace.completionDialogueId="ch2_wade_shortcut_result"; scenes_.emplace(wadeRace.sceneId,wadeRace);

ExplorationDefinition cracked;
cracked.sceneId="ch2_cracked_ring"; cracked.rule=ExplorationRuleKind::InteractionSequence;
cracked.objective="THE CRACKED RING • INSPECT THREE SUPPORTS"; cracked.detail="Bark and Wade are watching the practice ring.";
cracked.hasPlayerStart=true; cracked.playerStart={-760,760};
cracked.sequenceMarkers={{-750,790},{-540,980},{-330,790}}; cracked.sequenceLabels={"WEST SUPPORT","SOUTH SUPPORT","EAST SUPPORT"};
cracked.sequenceDialogueIds={"ch2_crack_west","ch2_crack_south","ch2_crack_east"}; cracked.sequenceRadius=82;
cracked.completionDialogueId="ch2_cracked_ring_result"; scenes_.emplace(cracked.sceneId,cracked);

const auto addClue=[&](std::string id,std::string objective,Vec2 point,std::string dialogue){
    ExplorationDefinition clue; clue.sceneId=id; clue.rule=ExplorationRuleKind::InteractionSequence; clue.objective=objective;
    clue.detail="Learn the pattern from a person or visible arena behavior, then continue."; clue.hasPlayerStart=true; clue.playerStart={900,80};
    clue.sequenceMarkers={point}; clue.sequenceLabels={"OBSERVE"}; clue.sequenceDialogueIds={dialogue}; clue.sequenceRadius=95;
    scenes_.emplace(clue.sceneId,clue);
};
addClue("ch2_intermission_stillness","ASK THE OLD COMPETITOR ABOUT PLOUKE",{260,-820},"ch2_clue_stillness");
addClue("ch2_intermission_positioning","CHECK THE PRACTICE RING WITH THE WORKER",{-540,760},"ch2_clue_positioning");
addClue("ch2_intermission_timing","FIND BARK AT THE WAITING TENT",{410,780},"ch2_clue_timing");
addClue("ch2_intermission_edge","MEET WADE AT THE ARENA EDGE",{1110,220},"ch2_clue_edge");
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
