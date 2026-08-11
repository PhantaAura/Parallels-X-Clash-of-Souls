#include "content/chapter_registry.hpp"
#include <stdexcept>

namespace px {

ChapterRegistry::ChapterRegistry() {
    chapters_.emplace("rrvvfo_ch1", ChapterDefinition{
        "rrvvfo_ch1",
        "Back to Normal",
        "training_region",
        "rrvvfo",
        {
            {SceneKind::Cutscene, "ch1_object_swap_setup", "rrvvfo-ch1-swap-setup", "training-field"},
            {SceneKind::Exploration, "sage_object_swap_field_trial", "rrvvfo-ch1-field-anchors", "training-field"},
            {SceneKind::Cutscene, "ch1_object_swap_result", "rrvvfo-ch1-swap-result", "training-field"},
            {SceneKind::Cutscene, "ch1_opening_sage_setup", "rrvvfo-ch1-opening", "training-field"},
            {SceneKind::Arena, "sage_tutorial_spar", "rrvvfo-ch1-sage-spar", "training-field"},
            {SceneKind::Cutscene, "ch1_post_spar_banter", "rrvvfo-ch1-post-spar", "training-field"},
            {SceneKind::Cutscene, "tournament_road_departure_dialogue", "rrvvfo-ch1-road-talk", "training-road"},
            {SceneKind::Exploration, "tournament_road_departure", "rrvvfo-ch1-road-start", "training-road"},
            {SceneKind::Exploration, "river_object_swap_problem", "rrvvfo-ch1-river", "training-road"},
            {SceneKind::Exploration, "legacy_route_choice", "rrvvfo-ch1-route-fork", "training-road"},
            {SceneKind::Exploration, "selected_route_adventure", "rrvvfo-ch1-route-travel", "training-road"},
            {SceneKind::Exploration, "swap_relay_trial", "rrvvfo-ch1-swap-relay", "training-road"},
            {SceneKind::Exploration, "transport_wheel_recovery", "rrvvfo-ch1-transport", "training-road"},
            {SceneKind::Exploration, "runaway_tournament_cart", "rrvvfo-ch1-runaway-cart", "training-road"},
            {SceneKind::Exploration, "roadside_encounter", "rrvvfo-ch1-roadside-encounter", "training-road"},
            {SceneKind::Exploration, "collapsed_tournament_road_detour", "rrvvfo-ch1-collapse-detour", "training-road"},
            {SceneKind::Exploration, "reach_tournament_checkpoint", "rrvvfo-ch1-checkpoint-approach", "training-road"},
            {SceneKind::Cutscene, "tournament_checkpoint_dialogue", "rrvvfo-ch1-checkpoint", "training-road"},
            {SceneKind::Exploration, "reach_lens_roadblock", "rrvvfo-ch1-lens-approach", "training-road"},
            {SceneKind::Cutscene, "lens_manual_reaction", "rrvvfo-ch1-lens-manual", "training-road"},
            {SceneKind::Exploration, "lens_roadblock_reveal", "rrvvfo-ch1-lens-roadblock", "training-road"},
            {SceneKind::Exploration, "reach_tournament_outskirts", "rrvvfo-ch1-outskirts-approach", "training-road"},
            {SceneKind::Cutscene, "tournament_outskirts_arrival", "rrvvfo-ch1-complete", "training-road"}
        },
        "rrvvfo_ch2"
    });

}

const ChapterDefinition& ChapterRegistry::get(const std::string& id) const {
    const auto it = chapters_.find(id);
    if (it == chapters_.end()) throw std::out_of_range("Unknown chapter: " + id);
    return it->second;
}

bool ChapterRegistry::has(const std::string& id) const {
    return chapters_.find(id) != chapters_.end();
}

std::vector<std::string> ChapterRegistry::ids() const {
    std::vector<std::string> result;
    result.reserve(chapters_.size());
    for (const auto& pair : chapters_) result.push_back(pair.first);
    return result;
}

} // namespace px
