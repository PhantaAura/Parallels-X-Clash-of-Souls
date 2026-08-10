#include "content/map_registry.hpp"
#include <stdexcept>

namespace px {

MapRegistry::MapRegistry() {
    // Legacy 2.9A.40.7.1.1 geometry used a long authored road from roughly X=-1550 to X=1450,
    // with the river, three-route fork, swap relay/gate, Lens roadblock and tournament outskirts
    // all occupying recognizable positions. 3.0R keeps that spatial memory but is free to improve art/elevation.
    MapDefinition training;
    training.id = "training_region";
    training.name = "Sage Training Field / Tournament Road";
    training.bounds = {-1550.0f, 1450.0f, -720.0f, 720.0f};
    training.playerStart = {-1260.0f, 180.0f};
    training.zones = {
        {"sage_field", "Sage's Training Field", {-1260.0f, 0.0f}, "opening training and personality"},
        {"road_junction", "Tournament Road Junction", {-650.0f, 0.0f}, "breathing room and route setup"},
        {"river", "Broken River Crossing", {70.0f, 0.0f}, "Object Swap story solution"},
        {"route_fork", "Three-Way Route Fork", {250.0f, 0.0f}, "physical choice, no popup required"},
        {"main_road", "Main Road", {405.0f, 0.0f}, "damaged road and practical problem solving"},
        {"forest", "Forest Route", {405.0f, -350.0f}, "navigation and environmental reading"},
        {"cliff", "Cliff Route", {405.0f, 350.0f}, "jumping, risk and optional reward"},
        {"swap_relay", "Swap Relay", {600.0f, 0.0f}, "routes reconnect and precision Object Swap"},
        {"riverside", "Riverside Road", {820.0f, 0.0f}, "transport, cart and roadside encounter"},
        {"lens_roadblock", "Outskirts Roadblock", {1080.0f, 0.0f}, "mandatory Lens route reveal"},
        {"tournament_outskirts", "Tournament Outskirts", {1320.0f, 0.0f}, "Chapter 1 arrival boundary"}
    };
    training.links = {
        {"sage_field", "road_junction", "main"},
        {"road_junction", "river", "main"},
        {"river", "route_fork", "main"},
        {"route_fork", "main_road", "main"},
        {"route_fork", "forest", "forest"},
        {"route_fork", "cliff", "cliff"},
        {"main_road", "swap_relay", "main"},
        {"forest", "swap_relay", "forest"},
        {"cliff", "swap_relay", "cliff"},
        {"swap_relay", "riverside", "main"},
        {"riverside", "lens_roadblock", "main"},
        {"lens_roadblock", "tournament_outskirts", "main"}
    };
    training.blockers = {
        {"dojo", {-1395.0f, -940.0f, -405.0f, -125.0f}, true},
        {"river", {-25.0f, 175.0f, -700.0f, 700.0f}, true},
        {"fallen_tree_center", {285.0f, 395.0f, -190.0f, 190.0f}, true},
        {"fallen_tree_north", {285.0f, 395.0f, -720.0f, -190.0f}, true},
        {"fallen_tree_south", {285.0f, 395.0f, 190.0f, 720.0f}, true},
        {"swap_gate", {555.0f, 645.0f, -175.0f, 175.0f}, true},
        {"swap_gate_forest", {555.0f, 645.0f, -720.0f, -175.0f}, true},
        {"swap_gate_cliff", {555.0f, 645.0f, 175.0f, 720.0f}, true},
        {"lens_roadblock", {1045.0f, 1120.0f, -175.0f, 175.0f}, true},
        {"lens_roadblock_north", {1045.0f, 1120.0f, -720.0f, -175.0f}, true},
        {"lens_roadblock_south", {1045.0f, 1120.0f, 175.0f, 720.0f}, true}
    };
    training.landmarks = {
        {"sage_sanctuary", "Sage Sanctuary", {-1340.0f, -300.0f}},
        {"sage_bell", "Hanging Sage Bell", {-1285.0f, -215.0f}},
        {"focus_pillars", "Training Focus Pillars", {-1130.0f, -210.0f}},
        {"field_anchor_west", "Field Anchor • West", {-1410.0f, -105.0f}},
        {"field_anchor_east", "Field Anchor • East", {-1015.0f, 225.0f}},
        {"field_anchor_south", "Field Anchor • South", {-1225.0f, -255.0f}},
        {"tournament_gate", "Tournament Road Gate", {-830.0f, 0.0f}},
        {"manual_post", "Sage Manual Post", {-1010.0f, 250.0f}},
        {"far_bank_rock", "Far-Bank Swap Rock", {230.0f, 0.0f}},
        {"relay_low", "Swap Relay • Low Marker", {468.0f, -96.0f}},
        {"relay_high", "Swap Relay • High Marker", {522.0f, 96.0f}},
        {"relay_center", "Swap Relay • Center Lock", {570.0f, 0.0f}},
        {"road_worker", "Road Worker", {365.0f, 300.0f}},
        {"dojo_student", "Dojo Student", {-900.0f, 300.0f}},
        {"traveler", "Traveler", {-520.0f, -300.0f}},
        {"lost_competitor", "Lost Competitor", {730.0f, -285.0f}},
        {"tournament_fan", "Tournament Fan", {940.0f, 285.0f}},
        {"vendor", "Road Vendor", {1130.0f, 260.0f}},
        {"sign_painter", "Sign Painter", {1225.0f, -300.0f}}
    };
    maps_.emplace(training.id, training);

    // Legacy treated combat spaces as authored arenas, not pieces of the road.
    // Keeping separate collision bounds prevents road gates, rivers, or scenery
    // from affecting a spar while preserving the presentation-stage identity.
    MapDefinition sageArena;
    sageArena.id = "sage_training_arena";
    sageArena.name = "Sage Training Arena";
    sageArena.bounds = {-1475.0f, -850.0f, -240.0f, 460.0f};
    sageArena.playerStart = {-1240.0f, 120.0f};
    sageArena.zones = {
        {"sparring_floor", "Sage Training Arena", {-1157.5f, 120.0f}, "Legacy tutorial spar and movement refresher"}
    };
    maps_.emplace(sageArena.id, sageArena);

    MapDefinition roadsideArena;
    roadsideArena.id = "roadside_arena";
    roadsideArena.name = "Tournament Road Challenge Ring";
    roadsideArena.bounds = {700.0f, 1030.0f, -260.0f, 260.0f};
    roadsideArena.playerStart = {790.0f, 70.0f};
    roadsideArena.zones = {
        {"challenge_ring", "Roadside Challenge Ring", {865.0f, 0.0f}, "optional Legacy roadside fight"}
    };
    maps_.emplace(roadsideArena.id, roadsideArena);

    maps_.emplace("tangai_dojo", MapDefinition{"tangai_dojo", "Tangai's Dojo", {-500,500,-400,400}, {0,0}, {}, {}, {}, {}});
    maps_.emplace("paper_world", MapDefinition{"paper_world", "Paper World", {-500,500,-400,400}, {0,0}, {}, {}, {}, {}});
    maps_.emplace("virek_island", MapDefinition{"virek_island", "Virek's Island", {-500,500,-400,400}, {0,0}, {}, {}, {}, {}});
}

const MapDefinition& MapRegistry::get(const std::string& id) const {
    const auto it = maps_.find(id);
    if (it == maps_.end()) throw std::out_of_range("Unknown map: " + id);
    return it->second;
}

std::vector<std::string> MapRegistry::ids() const {
    std::vector<std::string> result;
    result.reserve(maps_.size());
    for (const auto& pair : maps_) result.push_back(pair.first);
    return result;
}

} // namespace px
