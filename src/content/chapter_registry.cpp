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

chapters_.emplace("rrvvfo_ch2", ChapterDefinition{
    "rrvvfo_ch2", "Tournament Grounds", "tournament_grounds", "rrvvfo",
    {
        {SceneKind::Cutscene, "tournament_gate_walk_in", "rrvvfo-ch2-arrival", "tournament-hub"},
        {SceneKind::Cutscene, "ch2_arrival_delay", "rrvvfo-ch2-delay", "tournament-hub"},
        {SceneKind::Exploration, "ch2_lost_bracket", "rrvvfo-ch2-bracket", "tournament-hub"},
        {SceneKind::Cutscene, "ch2_practice_brawl_intro", "rrvvfo-ch2-practice-intro", "tournament-hub"},
        {SceneKind::Arena, "ch2_practice_brawl", "rrvvfo-ch2-practice", "tournament-arena"},
        {SceneKind::Cutscene, "ch2_ninja_reunion", "rrvvfo-ch2-reunion", "tournament-hub"},
        {SceneKind::Exploration, "ch2_wade_shortcut", "rrvvfo-ch2-wade-race", "tournament-hub"},
        {SceneKind::Exploration, "ch2_cracked_ring", "rrvvfo-ch2-cracked-ring", "tournament-hub"},
        {SceneKind::Cutscene, "ch2_registration_card", "rrvvfo-ch2-card", "tournament-hub"},
        {SceneKind::Cutscene, "ch2_opening_ceremony", "rrvvfo-ch2-ceremony", "tournament-hub"},
        {SceneKind::Cutscene, "ch2_hailey_plouke", "rrvvfo-ch2-hailey-plouke", "tournament-arena"},
        {SceneKind::Arena, "ch2_vs_hamual", "rrvvfo-ch2-hamual", "tournament-arena"},
        {SceneKind::Exploration, "ch2_intermission_stillness", "rrvvfo-ch2-clue-stillness", "tournament-hub"},
        {SceneKind::Arena, "ch2_vs_daniel", "rrvvfo-ch2-daniel", "tournament-arena"},
        {SceneKind::Exploration, "ch2_intermission_positioning", "rrvvfo-ch2-clue-position", "tournament-hub"},
        {SceneKind::Cutscene, "ch2_bark_pouki", "rrvvfo-ch2-bark-pouki", "tournament-arena"},
        {SceneKind::Exploration, "ch2_intermission_timing", "rrvvfo-ch2-clue-timing", "tournament-hub"},
        {SceneKind::Arena, "ch2_vs_wade", "rrvvfo-ch2-wade", "tournament-arena"},
        {SceneKind::Exploration, "ch2_intermission_edge", "rrvvfo-ch2-clue-edge", "tournament-hub"},
        {SceneKind::Cutscene, "ch2_pre_plouke", "rrvvfo-ch2-pre-final", "tournament-hub"},
        {SceneKind::Arena, "ch2_vs_plouke", "rrvvfo-ch2-plouke", "tournament-arena"},
        {SceneKind::Cutscene, "ch2_plouke_reveal", "rrvvfo-ch2-reveal", "tournament-hub"},
        {SceneKind::Cutscene, "ch2_tournament_aftermath", "rrvvfo-ch2-aftermath", "tournament-hub"}
    }, "rrvvfo_ch3"
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
