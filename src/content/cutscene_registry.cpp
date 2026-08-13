#include "content/cutscene_registry.hpp"
#include <stdexcept>
#include <utility>

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


cutscenes_.emplace("tournament_gate_walk_in", CutsceneDefinition{
    "tournament_gate_walk_in", CutsceneTier::Major,
    {{"threshold","","Walk through the tournament entrance instead of cutting to a menu.","long_track","walk"},
     {"sage_rejoins","SAGE","Sage approaches from another direction while both keep moving.","walking_two_shot","walk"}},
    {{"rrvvfo",{-1810.0f,40.0f},90.0f,true},{"sage",{-1570.0f,-290.0f},0.0f,true}},
    {
        {0,CutsceneActionKind::CameraTrack,"rrvvfo","",{},0,.20f,90.0f,560.0f,260.0f,38.0f,"front_walk_backward"},
        {0,CutsceneActionKind::MoveTo,"rrvvfo","",{-1510.0f,30.0f},142.0f,1.10f},
        {0,CutsceneActionKind::MoveTo,"sage","",{-1515.0f,-55.0f},175.0f,.70f,38.0f,900.0f,410.0f,43.0f,"",.48f},
        {0,CutsceneActionKind::TriggerWorldEvent,"sage","",{},0,.10f,38.0f,900.0f,410.0f,43.0f,"sage_side_entrance",.48f},
        {0,CutsceneActionKind::Expression,"rrvvfo","sage",{},0,.10f,38.0f,900.0f,410.0f,43.0f,"side_eye",.62f},
        {0,CutsceneActionKind::Wait,"","",{},0,1.10f},
        {1,CutsceneActionKind::CameraTrack,"rrvvfo","sage",{},0,.20f,78.0f,660.0f,300.0f,40.0f,"walking_two_shot"},
        {1,CutsceneActionKind::MoveTo,"rrvvfo","",{-1250.0f,55.0f},158.0f,1.25f},
        {1,CutsceneActionKind::MoveTo,"sage","",{-1300.0f,-20.0f},148.0f,1.15f},
        {1,CutsceneActionKind::PlayAnimation,"rrvvfo","",{},0,.28f,38.0f,900.0f,410.0f,43.0f,"dismissive_shrug",.12f},
        {1,CutsceneActionKind::Expression,"rrvvfo","",{},0,.10f,38.0f,900.0f,410.0f,43.0f,"unimpressed",.12f},
        {1,CutsceneActionKind::CameraFocus,"","",{650.0f,0.0f},0,.20f,60.0f,1380.0f,540.0f,47.0f,"arena_landmark_reveal",.88f},
        {1,CutsceneActionKind::Wait,"","",{},0,1.30f}
    }
});
cutscenes_.emplace("ch2_arrival_delay", CutsceneDefinition{
    "ch2_arrival_delay", CutsceneTier::Directed,
    {{"announcer_delay","ANNOUNCER","Registration trouble becomes an active hub problem.","registration_wide","crowd_react"}},
    {{"rrvvfo",{-1260.0f,40.0f},90.0f,true},{"sage",{-1310.0f,-40.0f},30.0f,true}},
    {{0,CutsceneActionKind::CameraFocus,"","",{-1190.0f,80.0f},0,.15f,35.0f,900.0f,360.0f,42.0f,"registration"},
     {1,CutsceneActionKind::FaceActor,"rrvvfo","sage"},
     {2,CutsceneActionKind::CameraTrack,"sage","rrvvfo",{},0,.15f,42.0f,620.0f,290.0f,40.0f,"sage_volunteers_rrvvfo"}}
});
cutscenes_.emplace("ch2_u8_gate", CutsceneDefinition{
    "ch2_u8_gate", CutsceneTier::FieldDialogue,
    {{"practice_call","ANNOUNCER","Send the player toward the practice grounds without a menu break.","practice_pan","point"}},
    {{"rrvvfo",{-1030.0f,80.0f},80.0f,true}},
    {{0,CutsceneActionKind::CameraFocus,"","",{-540.0f,760.0f},0,.15f,35.0f,960.0f,390.0f,43.0f,"practice_grounds"}}
});

cutscenes_.emplace("ch2_practice_brawl_intro", CutsceneDefinition{"ch2_practice_brawl_intro",CutsceneTier::Directed,
    {{"sage_exit","SAGE","Sage slips behind the waiting tent.","practice_wide","walk"},{"fighter_steps","PRACTICE RING FIGHTER","The practice fighter steps into the ring.","fighter_medium","ready"}},
    {{"rrvvfo",{-620,690},15,true},{"sage",{-430,820},90,true},{"practice_fighter",{-525,870},180,true}},
    {{0,CutsceneActionKind::CameraTrack,"sage","rrvvfo",{},0,.15f,34,720,310,41,"sage_slips_away"},
     {0,CutsceneActionKind::MoveTo,"sage","",{-170,910},145,1.25f},
     {1,CutsceneActionKind::CameraFocus,"","",{-540,760},0,.15f,32,760,320,41,"practice"},
     {2,CutsceneActionKind::MoveTo,"practice_fighter","",{-540,790},115,.65f},
     {2,CutsceneActionKind::PlayAnimation,"practice_fighter","",{},0,.25f,38,900,410,43,"ready",.35f},
     {3,CutsceneActionKind::FaceActor,"rrvvfo","practice_fighter"}}});
cutscenes_.emplace("ch2_ninja_reunion", CutsceneDefinition{"ch2_ninja_reunion",CutsceneTier::Directed,
    {{"reunion","WADE","Wade and Bark enter from the plaza while Rrvvfo keeps moving.","walking_three_shot","walk"}},
    {{"rrvvfo",{-720,620},0,true},{"wade",{-1040,360},70,true},{"bark",{-1100,430},65,true}},
    {{0,CutsceneActionKind::CameraTrack,"rrvvfo","",{},0,.15f,35,860,350,42,"reunion"},
     {0,CutsceneActionKind::MoveTo,"wade","",{-810,540},170,1.15f},
     {0,CutsceneActionKind::MoveTo,"bark","",{-865,610},148,1.20f},
     {1,CutsceneActionKind::CameraTrack,"bark","rrvvfo",{},0,.15f,48,650,300,40,"bark_deadpan"},
     {2,CutsceneActionKind::Expression,"rrvvfo","wade",{},0,.10f,38,900,410,43,"amused"}}});
cutscenes_.emplace("ch2_registration_card", CutsceneDefinition{"ch2_registration_card",CutsceneTier::Directed,
    {{"cards","REGISTRATION STAFF","All three ninjas receive spare Tournament Cards.","counter_three_shot","card_handoff"}},
    {{"rrvvfo",{-1210,80},90,true},{"wade",{-1280,-20},80,true},{"bark",{-1280,170},95,true}},
    {{0,CutsceneActionKind::CameraFocus,"","",{-1190,80},0,.15f,35,760,310,40,"registration_counter"},
     {4,CutsceneActionKind::TriggerWorldEvent,"","",{},0,.10f,0,0,0,0,"tournament_card_handoff"},
     {4,CutsceneActionKind::CameraTrack,"rrvvfo","",{},0,.15f,26,520,255,37,"card_close"},
     {4,CutsceneActionKind::PlayAnimation,"rrvvfo","",{},0,.35f,38,900,410,43,"card_receive",.18f}}});
cutscenes_.emplace("ch2_opening_ceremony", CutsceneDefinition{"ch2_opening_ceremony",CutsceneTier::Major,
    {{"lineup","ANNOUNCER","Introduce the bracket and first preliminary.","arena_establish","lineup"}},
    {{"rrvvfo",{940,-580},90,true}},{{0,CutsceneActionKind::CameraFocus,"","",{1290,0},0,.25f,30,1380,520,46,"arena_establish"},
     {1,CutsceneActionKind::CameraTrack,"rrvvfo","",{},0,.15f,32,650,290,40,"contestant_reaction"},
     {3,CutsceneActionKind::CameraFocus,"","",{1290,0},0,.18f,18,1080,400,43,"first_preliminary"}}});
cutscenes_.emplace("ch2_hailey_plouke", CutsceneDefinition{"ch2_hailey_plouke",CutsceneTier::Major,
    {{"prelim","","Show Plouke's stillness and pebble ring-out visually.","spectator_track","fight_observe"}},
    {{"rrvvfo",{520,-430},140,true},{"hailey",{-150,0},90,true},{"plouke",{150,0},-90,true}},
    {{0,CutsceneActionKind::CameraTrack,"hailey","plouke",{},0,.15f,34,790,315,41,"ring_two_shot"},
     {0,CutsceneActionKind::MoveTo,"hailey","",{20,0},185,.75f},
     {0,CutsceneActionKind::PlayAnimation,"hailey","",{},0,.28f,38,900,410,43,"heavy_attack",.25f},
     {1,CutsceneActionKind::CameraTrack,"rrvvfo","",{},0,.15f,30,570,265,38,"rrvvfo_observes"},
     {2,CutsceneActionKind::TriggerWorldEvent,"plouke","",{},0,.10f,0,0,0,0,"plouke_pebble_ringout"},
     {2,CutsceneActionKind::CameraTrack,"plouke","hailey",{},0,.12f,22,590,270,38,"pebble_reveal"}}});
cutscenes_.emplace("ch2_bark_pouki", CutsceneDefinition{"ch2_bark_pouki",CutsceneTier::Major,
    {{"bark_center","","Bark controls center.","ring_wide","block"},{"pouki_break","","Pouki changes rhythm and breaks defense.","ring_track","heavy"},{"last_counter","","Bark nearly lands the final counter.","reaction_close","counter"}},
    {{"rrvvfo",{520,-430},140,true},{"bark",{-180,0},90,true},{"pouki",{180,0},-90,true},{"wade",{600,-380},150,true}},
    {{0,CutsceneActionKind::CameraTrack,"bark","pouki",{},0,.15f,30,760,305,40,"ring_center"},
     {0,CutsceneActionKind::Wait,"","",{},0,.35f},
     {1,CutsceneActionKind::TriggerWorldEvent,"pouki","bark",{},0,.10f,0,0,0,0,"bark_guard_break"},
     {1,CutsceneActionKind::CameraTrack,"pouki","bark",{},0,.12f,18,560,255,38,"guard_break_close"},
     {2,CutsceneActionKind::TriggerWorldEvent,"bark","pouki",{},0,.10f,0,0,0,0,"bark_last_counter"},
     {2,CutsceneActionKind::CameraTrack,"rrvvfo","bark",{},0,.12f,42,610,280,39,"spectator_reaction"}}});
cutscenes_.emplace("ch2_pre_plouke", CutsceneDefinition{"ch2_pre_plouke",CutsceneTier::Directed,
    {{"quiet_prep","BARK","Quiet contestant-lane preparation before the final.","bench_three_shot","idle"}},
    {{"rrvvfo",{510,760},20,true},{"bark",{420,840},-20,true},{"wade",{610,845},20,true}},
    {{0,CutsceneActionKind::CameraTrack,"rrvvfo","",{},0,.15f,28,780,315,40,"quiet_prep"},
     {1,CutsceneActionKind::FaceActor,"rrvvfo","wade"},
     {2,CutsceneActionKind::Expression,"rrvvfo","",{},0,.10f,0,0,0,0,"focused"},
     {3,CutsceneActionKind::CameraFocus,"","",{1290,0},0,.20f,24,1280,480,45,"final_entrance"}}});
cutscenes_.emplace("ch2_plouke_reveal", CutsceneDefinition{"ch2_plouke_reveal",CutsceneTier::Major,
    {{"reveal","SAGE","Plouke reveals himself as Sage after the final.","reveal_orbit","reveal"}},
    {{"rrvvfo",{1120,70},90,true},{"sage",{1320,70},-90,true}},
    {{0,CutsceneActionKind::CameraTrack,"rrvvfo","sage",{},0,.15f,26,620,275,39,"post_match_two_shot"},
     {3,CutsceneActionKind::TriggerWorldEvent,"sage","",{},0,.10f,0,0,0,0,"plouke_to_sage"},
     {3,CutsceneActionKind::CameraTrack,"sage","rrvvfo",{},0,.12f,18,520,245,37,"reveal_close"},
     {4,CutsceneActionKind::CameraFocus,"","rrvvfo",{},0,.15f,24,660,280,39,"reaction"}}});
cutscenes_.emplace("ch2_tournament_aftermath", CutsceneDefinition{"ch2_tournament_aftermath",CutsceneTier::Directed,
    {{"cleanup","ANNOUNCER","Tournament transitions to cleanup and later investigation state.","hub_wide","cleanup"}},
    {{"rrvvfo",{920,120},90,true}},{{0,CutsceneActionKind::CameraFocus,"","",{410,0},0,.20f,35,1320,500,46,"cleanup"}}});
// U7A: retrofit existing Chapter 1 beats with movement/camera intent while
// preserving every existing story line and scene identity.
const auto setActions = [&](const std::string& id, std::vector<CutsceneAction> actions) {
    cutscenes_.at(id).actions = std::move(actions);
};

setActions("ch1_object_swap_setup", {
    {0, CutsceneActionKind::CameraTrack, "rrvvfo", "", {-1325.0f, 65.0f}, 0.0f, .20f, 28.0f, 720.0f, 285.0f, 39.0f, "low_walk_in"},
    {0, CutsceneActionKind::MoveTo, "rrvvfo", "", {-1325.0f, 65.0f}, 175.0f, .30f},
    {0, CutsceneActionKind::PlayAnimation, "sage", "", {}, 0.0f, .30f, 0,0,0,0, "set_training_post"},
    {1, CutsceneActionKind::Expression, "rrvvfo", "", {}, 0.0f, .20f, 0,0,0,0, "unimpressed"},
    {2, CutsceneActionKind::FaceActor, "sage", "rrvvfo"},
    {2, CutsceneActionKind::CameraFocus, "", "sage", {}, 0.0f, .15f, 34.0f, 760.0f, 310.0f, 41.0f, "anchor_explain"},
    {3, CutsceneActionKind::CameraFocus, "", "rrvvfo", {}, 0.0f, .15f, 30.0f, 700.0f, 290.0f, 40.0f, "reaction"}
});

setActions("ch1_opening_sage_setup", {
    {0, CutsceneActionKind::CameraTrack, "rrvvfo", "sage", {}, 0.0f, .20f, 38.0f, 850.0f, 360.0f, 42.0f, "walking_two_shot"},
    {1, CutsceneActionKind::FaceActor, "sage", "rrvvfo"},
    {2, CutsceneActionKind::Expression, "rrvvfo", "", {}, 0.0f, .10f, 0,0,0,0, "annoyed"},
    {4, CutsceneActionKind::CameraFocus, "", "rrvvfo", {}, 0.0f, .15f, 24.0f, 660.0f, 275.0f, 39.0f, "pride_close"},
    {6, CutsceneActionKind::TriggerWorldEvent, "sage", "", {}, 0.0f, .10f, 0,0,0,0, "manual_handoff"}
});

setActions("tournament_road_departure_dialogue", {
    {0, CutsceneActionKind::CameraFocus, "", "", {-785.0f, 0.0f}, 0.0f, .25f, 38.0f, 960.0f, 420.0f, 44.0f, "show_road"},
    {1, CutsceneActionKind::MoveTo, "rrvvfo", "", {-900.0f, 90.0f}, 155.0f, .40f},
    {2, CutsceneActionKind::CameraTrack, "rrvvfo", "", {}, 0.0f, .15f, 35.0f, 820.0f, 340.0f, 42.0f, "walk_past_sage"},
    {3, CutsceneActionKind::MoveTo, "rrvvfo", "", {-835.0f, 55.0f}, 180.0f, .35f},
    {3, CutsceneActionKind::Wait, "", "", {}, 0.0f, .18f}
});

setActions("tournament_checkpoint_dialogue", {
    {0, CutsceneActionKind::CameraTrack, "rrvvfo", "checkpoint_worker", {}, 0.0f, .12f, 38.0f, 760.0f, 320.0f, 41.0f, "checkpoint_two_shot"},
    {1, CutsceneActionKind::Expression, "rrvvfo", "", {}, 0.0f, .10f, 0,0,0,0, "confident"}
});

setActions("lens_manual_reaction", {
    {0, CutsceneActionKind::CameraFocus, "", "rrvvfo", {}, 0.0f, .12f, 25.0f, 650.0f, 280.0f, 39.0f, "lens_reaction"},
    {0, CutsceneActionKind::Expression, "rrvvfo", "", {}, 0.0f, .12f, 0,0,0,0, "annoyed"}
});

setActions("tournament_outskirts_arrival", {
    {0, CutsceneActionKind::MoveTo, "rrvvfo", "", {1990.0f, -10.0f}, 115.0f, .65f},
    {0, CutsceneActionKind::CameraTrack, "rrvvfo", "", {}, 0.0f, .20f, 32.0f, 980.0f, 390.0f, 43.0f, "long_approach"},
    {1, CutsceneActionKind::Expression, "rrvvfo", "", {}, 0.0f, .10f, 0,0,0,0, "dry"},
    {3, CutsceneActionKind::CameraFocus, "", "", {2050.0f, 0.0f}, 0.0f, .18f, 30.0f, 900.0f, 360.0f, 42.0f, "entrance_reveal"},
    {4, CutsceneActionKind::MoveTo, "rrvvfo", "", {2075.0f, 0.0f}, 165.0f, .45f},
    {4, CutsceneActionKind::TriggerWorldEvent, "", "", {}, 0.0f, .10f, 0,0,0,0, "cross_tournament_threshold"}
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
