#include "content/menu_registry.hpp"
#include <stdexcept>

namespace px {

MenuRegistry::MenuRegistry() {
    modes_ = {
        {MenuModeId::Story, "story", "STORY MODE", "THE LOST YEAR",
         "Follow Rrvvfo through the recovery year after Season 1.", "AVAILABLE",
         "mode_story_rrvvfo_sage", "crimson", true},
        {MenuModeId::ArenaBattle, "arena", "FIGHT", "RRVVFO VS CPU",
         "Jump straight into a playable one-on-one fight using the shared Story combat.", "PLAYABLE",
         "mode_arena_ring", "ember", true},
        {MenuModeId::OnlinePlay, "online", "ONLINE PLAY", "NETWORK BATTLE",
         "Online play is reserved for a later networking milestone.", "COMING LATER",
         "mode_online_signal", "warm_gold", false},
        {MenuModeId::VsCpu, "cpu", "VS CPU", "SINGLE BATTLE",
         "Choose a fighter and face a computer-controlled opponent.", "COMING LATER",
         "mode_cpu_versus", "deep_red", false},
        {MenuModeId::TwoPlayer, "local", "2 PLAYER", "LOCAL VS",
         "Local two-player battles on one system.", "COMING LATER",
         "mode_local_versus", "off_white", false},
        {MenuModeId::Training, "training", "TRAINING", "SAGE'S CHALLENGE",
         "Practice movement, defense, Pursuit, Flow Cancel, and Chapter 1 techniques.", "PLAYABLE",
         "mode_training_field", "ember", true},
        {MenuModeId::Extras, "extras", "EXTRAS", "SAGE ARCHIVES",
         "Combat Manual, records, profiles, and other reference material.", "COMING LATER",
         "mode_extras_manual", "warm_gold", false},
        {MenuModeId::Options, "options", "OPTIONS", "SETTINGS",
         "Gameplay, controls, audio, video, accessibility, and save options.", "COMING LATER",
         "mode_options_x", "off_white", false},
        {MenuModeId::Credits, "credits", "CREDITS", "PARALLELS X",
         "Project and development credits.", "COMING LATER",
         "mode_credits_x", "off_white", false},
        {MenuModeId::Arcade, "arcade", "ARCADE", "BATTLE ROAD",
         "A future combat run built around the completed roster.", "COMING LATER",
         "mode_arcade_road", "muted_red", false},
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
