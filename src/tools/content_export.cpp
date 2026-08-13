#include "content/adventure_registry.hpp"
#include "content/chapter_registry.hpp"
#include "content/character_presentation_registry.hpp"
#include "content/cutscene_registry.hpp"
#include "content/exploration_registry.hpp"
#include "content/map_registry.hpp"
#include "content/training_registry.hpp"
#include "content/world_presentation_registry.hpp"
#include "core/ability_hotbar.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>

static std::string escape(const std::string& value) {
    std::string out;
    for (char c : value) {
        if (c == '\\' || c == '"') out.push_back('\\');
        out.push_back(c);
    }
    return out;
}

int main(int argc, char** argv) {
    const std::string path = argc > 1 ? argv[1] : "content.json";
    px::ChapterRegistry chapters;
    px::MapRegistry maps;
    px::ExplorationRegistry exploration;
    px::TrainingRegistry training;
    px::CutsceneRegistry cutscenes;
    px::WorldPresentationRegistry worlds;
    px::CharacterPresentationRegistry characters;
    px::AdventureRegistry adventures;
    const auto& chapter = chapters.get("rrvvfo_ch1");
    const auto& map = maps.get("training_region");
    const auto hotbar = px::AbilityHotbarCatalog::rrvvfoChapter1();

    std::ofstream out(path);
    if (!out) return 2;
    const bool asJs = path.size() >= 3 && path.substr(path.size()-3) == ".js";
    if (asJs) out << "window.PX_CONTENT = ";
    out << "{\n  \"build\": \"Parallels X: Clash of Souls 3.0R\",";
    out << "\n  \"chapter\": {\"id\": \"" << escape(chapter.id) << "\", \"title\": \"" << escape(chapter.title) << "\", \"flow\": [";
    for (std::size_t i = 0; i < chapter.openingFlow.size(); ++i) {
        if (i) out << ',';
        out << "\n    {\"id\": \"" << escape(chapter.openingFlow[i].id)
            << "\", \"checkpoint\": \"" << escape(chapter.openingFlow[i].checkpointId)
            << "\", \"presentationStage\": \"" << escape(chapter.openingFlow[i].presentationStageId) << "\"}";
    }
    out << "\n  ]},\n  \"hotbar\": [";
    for (std::size_t i = 0; i < hotbar.size(); ++i) {
        if (i) out << ',';
        const auto& a = hotbar[i];
        out << "\n    {\"displaySlot\": " << a.displaySlot << ", \"canonicalSlot\": " << a.canonicalSlot
            << ", \"id\": \"" << escape(a.id) << "\", \"label\": \"" << escape(a.label)
            << "\", \"state\": \"" << px::abilityStateName(a.state) << "\"}";
    }
    out << "\n  ],\n  \"map\": {\"id\": \"" << escape(map.id) << "\", \"name\": \"" << escape(map.name) << "\", \"bounds\": [" << map.bounds.minX << ',' << map.bounds.maxX << ',' << map.bounds.minZ << ',' << map.bounds.maxZ << "], \"zones\": [";
    for (std::size_t i = 0; i < map.zones.size(); ++i) {
        if (i) out << ',';
        const auto& z = map.zones[i];
        out << "\n    {\"id\": \"" << escape(z.id) << "\", \"name\": \"" << escape(z.name) << "\", \"x\": " << z.center.x << ", \"z\": " << z.center.z << ", \"identity\": \"" << escape(z.gameplayIdentity) << "\"}";
    }
    out << "\n  ], \"links\": [";
    for (std::size_t i = 0; i < map.links.size(); ++i) {
        if (i) out << ',';
        const auto& l = map.links[i];
        out << "\n    {\"from\": \"" << escape(l.from) << "\", \"to\": \"" << escape(l.to) << "\", \"route\": \"" << escape(l.routeTag) << "\"}";
    }
    out << "\n  ]},\n  \"grandAdventure\": {\n";
    const char* scenes[] = {"sage_object_swap_field_trial","tournament_road_departure","river_object_swap_problem","legacy_route_choice","selected_route_adventure","swap_relay_trial","transport_wheel_recovery","runaway_tournament_cart","roadside_encounter","reach_tournament_checkpoint","reach_lens_roadblock","lens_roadblock_reveal","reach_tournament_outskirts"};
    out << "    \"explorationScenes\": [";
    bool first = true;
    for (const char* id : scenes) {
        if (!exploration.has(id)) continue;
        const auto& e = exploration.get(id);
        if (!first) out << ',';
        first = false;
        out << "\n      {\"id\": \"" << escape(e.sceneId) << "\", \"objective\": \"" << escape(e.objective) << "\", \"detail\": \"" << escape(e.detail) << "\"}";
    }
    out << "\n    ],\n    \"sageTraining\": {";
    const auto& trainingDefinition = training.get("sage_tutorial_spar");
    out << "\n      \"meaningfulMovementDistance\": " << trainingDefinition.meaningfulMovementDistance
        << ", \"chargeStartingEnergy\": " << trainingDefinition.chargeStartingEnergy
        << ", \"chargeTargetEnergy\": " << trainingDefinition.chargeTargetEnergy
        << ", \"lensStartingEnergy\": " << trainingDefinition.lensStartingEnergy
        << ", \"lensRequiredEnergy\": " << trainingDefinition.lensRequiredEnergy
        << ", \"steps\": [";
    const auto& steps = trainingDefinition.steps;
    for (std::size_t i = 0; i < steps.size(); ++i) {
        if (i) out << ',';
        out << "\n      {\"id\": \"" << escape(steps[i].id) << "\", \"objective\": \"" << escape(steps[i].objective) << "\"}";
    }
    const auto& adventure = adventures.get("rrvvfo_ch1_road");
    out << "\n      ]\n    },\n    \"road\": {\"npcCount\": " << adventure.npcs.size()
        << ", \"relayCount\": " << adventure.gateRelayMarkers.size()
        << ", \"gateDelaySeconds\": " << adventure.gateReleaseDelaySeconds
        << ", \"runawayCartSeconds\": " << adventure.runawayCart.durationSeconds
        << ", \"runawayCartFailForward\": " << (adventure.runawayCart.failForward ? "true" : "false")
        << ", \"roadsideKoTarget\": " << adventure.roadsideFight.koTarget
        << "}\n  },\n  \"worldPresentation\": [";
    auto stageIds = worlds.ids();
    std::sort(stageIds.begin(), stageIds.end());
    for (std::size_t i = 0; i < stageIds.size(); ++i) {
        if (i) out << ',';
        const auto& stage = worlds.get(stageIds[i]);
        out << "\n    {\"id\": \"" << escape(stage.id) << "\", \"camera\": {\"yawDeg\": "
            << stage.camera.yawDegrees << ", \"fov\": " << stage.camera.fovDegrees
            << ", \"distance\": " << stage.camera.baseDistance << ", \"height\": "
            << stage.camera.height << ", \"focusX\": " << stage.camera.focusCenterX
            << ", \"focusZ\": " << stage.camera.focusCenterZ << "}, \"primitiveCount\": " << stage.primitives.size()
            << ", \"ambientActorCount\": " << stage.ambientActors.size() << '}';
    }
    const auto& openingCutscene = cutscenes.get("ch1_object_swap_setup");
    out << "\n  ],\n  \"openingStaging\": [";
    for (std::size_t i = 0; i < openingCutscene.staging.size(); ++i) {
        if (i) out << ',';
        const auto& actor = openingCutscene.staging[i];
        out << "\n    {\"actorId\": \"" << escape(actor.actorId) << "\", \"x\": "
            << actor.position.x << ", \"z\": " << actor.position.z
            << ", \"yawDeg\": " << actor.yawDegrees << '}';
    }
    const auto& rrvvfoPresentation = characters.get("rrvvfo");
    out << "\n  ],\n  \"characterPresentation\": {\"rrvvfo\": {\"sourceModel\": \""
        << escape(rrvvfoPresentation.sourceModelPath) << "\", \"cookedAssetReady\": "
        << (rrvvfoPresentation.cookedAssetReady ? "true" : "false")
        << ", \"fallback\": \"procedural-humanoid\"}}\n}";
    if (asJs) out << ";";
    out << '\n';
    std::cout << "Exported " << path << '\n';
    return 0;
}
