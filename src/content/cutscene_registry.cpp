#include "content/cutscene_registry.hpp"
#include <stdexcept>

namespace px {

CutsceneRegistry::CutsceneRegistry() {
    cutscenes_.emplace("ch1_object_swap_setup", CutsceneDefinition{
        "ch1_object_swap_setup", CutsceneTier::Directed,
        {
            {"field_establish", "", "Establish Sage's compact sanctuary and the three separated anchors.", "legacy_38_degree_wide", "field_idle"},
            {"anchor_lesson", "SAGE", "Sage forbids walking between the anchors and hands control over.", "close_two_shot", "point_to_anchors"}
        },
        {{"rrvvfo", {-1325.0f, 65.0f}, 25.0f, true}, {"sage", {-1080.0f, 95.0f}, -145.0f, true}}
    });

    cutscenes_.emplace("ch1_object_swap_result", CutsceneDefinition{
        "ch1_object_swap_result", CutsceneTier::Directed,
        {{"lesson_result", "RRVVFO", "Rrvvfo understands the traversal implication while keeping the old back-and-forth.", "close_two_shot", "annoyed_reply"}},
        {{"rrvvfo", {-1225.0f, -255.0f}, 20.0f, true}, {"sage", {-1080.0f, 90.0f}, -150.0f, true}}
    });

    cutscenes_.emplace("ch1_opening_sage_setup", CutsceneDefinition{
        "ch1_opening_sage_setup", CutsceneTier::Directed,
        {
            {"field_establish", "", "Re-establish Sage's Training Field and Rrvvfo's recovered physical confidence.", "legacy_38_degree_wide", "idle_variation"},
            {"sage_needles", "SAGE", "Sage irritates Rrvvfo and challenges his pride.", "sage_medium", "lazy_gesture"},
            {"rrvvfo_pushback", "RRVVFO", "Rrvvfo answers in his established voice.", "rrvvfo_medium", "annoyed_reply"},
            {"manual", "SAGE", "Hand over the Fighting Manual and move directly into active training.", "two_shot", "hand_over_manual"}
        },
        // Legacy training-field composition: Rrvvfo occupies the upper-left
        // half while Sage reads lower-right across the practice circle.
        {{"rrvvfo", {-1450.0f, 42.0f}, 15.0f, true}, {"sage", {-1030.0f, -42.0f}, -165.0f, true}}
    });

    cutscenes_.emplace("ch1_post_spar_banter", CutsceneDefinition{
        "ch1_post_spar_banter", CutsceneTier::Directed,
        {
            {"spar_reaction", "SAGE", "Restore Sage's warning about Rrvvfo's pride.", "close_two_shot", "recover_idle"},
            {"old_history", "SAGE", "Preserve Sage's teasing hint that his experience predates Rrvvfo's assumptions.", "sage_medium", "amused_reply"}
        },
        {{"rrvvfo", {-1240.0f, 120.0f}, 15.0f, true}, {"sage", {-1075.0f, 120.0f}, -165.0f, true}}
    });

    cutscenes_.emplace("tournament_road_departure_dialogue", CutsceneDefinition{
        "tournament_road_departure_dialogue", CutsceneTier::Directed,
        {
            {"road_direction", "SAGE", "Sage points out Tournament Road and the trainers' markers.", "pan_to_torii", "point_to_road"},
            {"sage_exit", "SAGE", "The camera-and-binoculars argument releases Rrvvfo onto the road.", "close_two_shot", "comic_argument"}
        },
        {{"rrvvfo", {-1085.0f, 95.0f}, 10.0f, true}, {"sage", {-955.0f, 130.0f}, -150.0f, true}}
    });

    cutscenes_.emplace("tournament_checkpoint_dialogue", CutsceneDefinition{
        "tournament_checkpoint_dialogue", CutsceneTier::FieldDialogue,
        {{"checkpoint", "TOURNAMENT CHECKPOINT", "Stop Rrvvfo at the authored road checkpoint.", "road_two_shot", "checkpoint_idle"}},
        {{"rrvvfo", {1420.0f, 0.0f}, 90.0f, true}, {"checkpoint_worker", {1475.0f, 0.0f}, -90.0f, true}}
    });

    cutscenes_.emplace("lens_manual_reaction", CutsceneDefinition{
        "lens_manual_reaction", CutsceneTier::FieldDialogue,
        {{"lens_page", "RRVVFO", "React to Sage hiding the Lens explanation.", "rrvvfo_medium", "annoyed_reply"}},
        {{"rrvvfo", {1650.0f, 0.0f}, 90.0f, true}}
    });

    cutscenes_.emplace("tournament_outskirts_arrival", CutsceneDefinition{
        "tournament_outskirts_arrival", CutsceneTier::Directed,
        {
            {"fan_recognizes", "TOURNAMENT FAN", "Recognize Rrvvfo from the transport rescue.", "outskirts_three_shot", "excited"},
            {"knockoff", "RRVVFO", "Preserve the tournament-company joke at the Chapter 1 boundary.", "rrvvfo_medium", "dry_reply"}
        },
        {{"rrvvfo", {1960.0f, -10.0f}, 90.0f, true}, {"tournament_fan", {2000.0f, 70.0f}, -110.0f, true},
         {"sign_painter", {1900.0f, -100.0f}, -70.0f, true}}
    });
}

const CutsceneDefinition& CutsceneRegistry::get(const std::string& id) const {
    const auto it = cutscenes_.find(id);
    if (it == cutscenes_.end()) throw std::out_of_range("Unknown cutscene: " + id);
    return it->second;
}

bool CutsceneRegistry::has(const std::string& id) const {
    return cutscenes_.find(id) != cutscenes_.end();
}

} // namespace px
