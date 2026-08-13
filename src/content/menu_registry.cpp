#include "content/menu_registry.hpp"
#include <stdexcept>

namespace px {

MenuRegistry::MenuRegistry() {
    // Finished-game hierarchy. Unreleased versus modes live inside Battle and
    // no longer occupy most of the top-level carousel.
    modes_ = {
        {MenuModeId::Continue, "continue", "CONTINUE", "RETURN TO THE ROAD",
         "Resume the current Story save from its exact area and objective.", "NO STORY SAVE",
         "mode_continue_road", "warm_gold", true},
        {MenuModeId::Story, "story", "STORY", "THE LOST YEAR",
         "Begin a route or continue through its character-driven Story.", "AVAILABLE",
         "mode_story_rrvvfo_sage", "crimson", true},
        {MenuModeId::ArenaBattle, "battle", "BATTLE", "FIGHTING GROUNDS",
         "Enter a CPU fight now. Future versus modes remain grouped inside Battle.", "AVAILABLE",
         "mode_arena_ring", "ember", true},
        {MenuModeId::Training, "training", "TRAINING", "SAGE'S CHALLENGE",
         "Practice movement, defense, Pursuit, Flow Cancel, and current techniques.", "PLAYABLE",
         "mode_training_field", "ember", true},
        {MenuModeId::Extras, "extras", "EXTRAS", "ARCHIVES & RECORDS",
         "Adventure Records, Combat Manual, Story So Far, profiles, gallery, and credits.", "AVAILABLE",
         "mode_extras_manual", "warm_gold", true},
        {MenuModeId::Options, "options", "OPTIONS", "SETTINGS",
         "Controls, camera, dialogue, HUD, objectives, and accessibility.", "AVAILABLE",
         "mode_options_x", "off_white", true},
    };
}

const MenuModeDefinition& MenuRegistry::get(MenuModeId id) const {
    for (const auto& mode : modes_) if (mode.id == id) return mode;
    throw std::out_of_range("Unknown menu mode");
}

const MenuModeDefinition& MenuRegistry::get(const std::string& stableId) const {
    for (const auto& mode : modes_) if (mode.stableId == stableId) return mode;
    throw std::out_of_range("Unknown menu mode id: " + stableId);
}

} // namespace px
