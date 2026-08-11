#include "content/dialogue_registry.hpp"
#include <stdexcept>

namespace px {

DialogueRegistry::DialogueRegistry() {
    // Restored from the exact Legacy opening. Dialogue remains content data so presentation
    // and platform shells never own character voice or story order.
    scenes_.emplace("ch1_object_swap_setup", std::vector<DialogueLine>{
        {"THE SAGE", "Before you start throwing attacks around, prove you can move without a path.", "sage", "instructing", "sage"},
        {"RRVVFO", "There is literally a path right there.", "rrvvfo", "annoyed", "rrvvfo"},
        {"THE SAGE", "Then ignore it. See the glowing anchor? Swap to it. Three anchors. No walking between them.", "sage", "pointing", "sage"},
        {"THE SAGE", "If an object can get somewhere, you can turn that into a route. Figure out the rest.", "sage", "amused", "sage"}
    });

    scenes_.emplace("ch1_object_swap_result", std::vector<DialogueLine>{
        {"RRVVFO", "So if I can get something over there, I don’t need the path.", "rrvvfo", "realizing", "rrvvfo"},
        {"THE SAGE", "Exactly. Try remembering that when the road stops being convenient.", "sage", "instructing", "sage"},
        {"RRVVFO", "You could’ve just said that.", "rrvvfo", "annoyed", "rrvvfo"}
    });

    scenes_.emplace("ch1_opening_sage_setup", std::vector<DialogueLine>{
        {"THE SAGE", "I signed you up for this tournament. I heard there’d be some nice ladies there."},
        {"RRVVFO", "Oh, I was about to praise you for once. Now I know your motive, perv."},
        {"THE SAGE", "HEY! DON’T CALL ME THAT!"},
        {"RRVVFO", "I’ll be winning. I’m the one who defeated Perfected Revvfo."},
        {"THE SAGE", "You’ve gotten cocky. You’ll lose for sure if you keep that up."},
        {"RRVVFO", "So what?"},
        {"THE SAGE", "I’d rather not teach you with words. It’d go in one ear and out the other. So here’s a manual."},
        {"RRVVFO", "As if I’d need it."}
    });

    scenes_.emplace("ch1_post_spar_banter", std::vector<DialogueLine>{
        {"THE SAGE", "You’re too prideful. You’ll probably lose. Maybe it’ll be a reality check."},
        {"RRVVFO", "As if. You’re just trying to put me down."},
        {"THE SAGE", "I’d be on my guard if I were you."},
        {"RRVVFO", "I defeated Revvfo. No one has a feat anywhere near that."},
        {"THE SAGE", "Heh. I have—many times. If only you’d care to listen to my stories."},
        {"RRVVFO", "Yeah, no."}
    });

    scenes_.emplace("tournament_road_departure_dialogue", std::vector<DialogueLine>{
        {"THE SAGE", "The tournament’s that way. There are a lot of trainers around here. They might’ve left some markers."},
        {"THE SAGE", "I’ve gotta go do some important training. Don’t mind this camera or these binoculars."},
        {"RRVVFO", "Lemme guess. Your ‘important training’ is going to the spa and spying on women, perv."},
        {"THE SAGE", "I’VE HAD IT WITH YOU CALLING ME THAT!"}
    });

    scenes_.emplace("main_route_worker_intro", std::vector<DialogueLine>{
        {"ROAD WORKER", "Help! This wood is too big for me to push. Hey, you’re the hero who beat Revvfo! Please, please burn this wood for me!"},
        {"RRVVFO", "No problemo."}
    });
    scenes_.emplace("main_route_worker_result", std::vector<DialogueLine>{
        {"ROAD WORKER", "Thank you—but you should be more careful! You almost caused a forest fire!"},
        {"RRVVFO", "My bad. I’m in a rush."}
    });
    scenes_.emplace("transport_wheel_result", std::vector<DialogueLine>{
        {"TRANSPORT DRIVER", "You saved the whole transport—and the tournament supplies."},
        {"RRVVFO", "Remember that when the crowd starts cheering for me."}
    });
    scenes_.emplace("runaway_cart_intro", std::vector<DialogueLine>{
        {"TRANSPORT DRIVER", "WAIT—THE SUPPLY CART BRAKE!"},
        {"RRVVFO", "I just fixed one moving problem."},
        {"ROAD WORKER", "Catch it before the hill! Cut right, jump the debris, cut left, then right again to get in front of it!"}
    });
    scenes_.emplace("runaway_cart_result", std::vector<DialogueLine>{
        {"ROAD WORKER", "The supplies are safe!"},
        {"RRVVFO", "Tell the tournament I expect a good seat for the cart."}
    });

    scenes_.emplace("road_npc_dojo_student", std::vector<DialogueLine>{
        {"DOJO STUDENT", "There was a weird old man who said he had to look at some people swimming. Do you know why?"},
        {"RRVVFO", "Yeah. You’re too young to understand."}
    });
    scenes_.emplace("road_npc_traveler", std::vector<DialogueLine>{
        {"TRAVELER", "The decorations look so similar."},
        {"RRVVFO", "I heard they’re owned by the same company. I’m not sure."}
    });
    scenes_.emplace("road_npc_worker", std::vector<DialogueLine>{
        {"WORRIED WORKER", "Don’t go to the tournament. Something seems fishy about it, kid."},
        {"RRVVFO", "Don’t worry. I beat Revvfo. I’m capable of anything."}
    });
    scenes_.emplace("road_npc_fan", std::vector<DialogueLine>{
        {"TOURNAMENT FAN", "I heard someone with red hair summoned a black hole, and someone with brown hair outran it!"},
        {"RRVVFO", "You’re looking at him."},
        {"RRVVFO", "Look at the news. The name’s Rrvvfo. I’m in a rush, kid."}
    });
    scenes_.emplace("road_npc_sign_painter", std::vector<DialogueLine>{
        {"SIGN PAINTER", "Don’t enter. You’re not cut out for it, kid."},
        {"RRVVFO", "As if."}
    });
    scenes_.emplace("road_npc_vendor", std::vector<DialogueLine>{
        {"VENDOR", "Ugh, my boss is making me sell this horrible food."},
        {"RRVVFO", "Food is food, but I don’t got cash on me right now."}
    });
    scenes_.emplace("road_npc_lost_competitor", std::vector<DialogueLine>{
        {"LOST COMPETITOR", "Ugh, I forgot to sign up. I can’t enter, and I wanted to spectate."},
        {"LOST COMPETITOR", "There’s some guy up ahead who isn’t even gonna spectate. He said he’d give me his pass only if I beat him."}
    });
    scenes_.emplace("road_npc_lost_competitor_help", std::vector<DialogueLine>{
        {"RRVVFO", "Alright. I’ll fight him and pry it out of his hands."}
    });
    scenes_.emplace("road_npc_lost_competitor_decline", std::vector<DialogueLine>{
        {"RRVVFO", "Eh, I’m in a rush. Sorry."}
    });
    scenes_.emplace("road_npc_lost_competitor_helped", std::vector<DialogueLine>{
        {"LOST COMPETITOR", "The fighter with the spectator pass is up ahead. Please make sure he keeps his word."}
    });
    scenes_.emplace("road_npc_lost_competitor_pass_delivered", std::vector<DialogueLine>{
        {"LOST COMPETITOR", "You actually got it?!"},
        {"RRVVFO", "Obviously. He challenged the way I walk and brought paperwork as a prize."},
        {"LOST COMPETITOR", "Thank you! I'll cheer for you when you fight!"},
        {"RRVVFO", "Make it loud. I don't do favors quietly."}
    });
    scenes_.emplace("road_npc_lost_competitor_pass_repeat", std::vector<DialogueLine>{
        {"LOST COMPETITOR", "I'll be the loudest person in the stands!"},
        {"RRVVFO", "You better be. I fought for that seat."}
    });
    scenes_.emplace("road_npc_lost_competitor_declined", std::vector<DialogueLine>{
        {"LOST COMPETITOR", "I understand. Good luck in the tournament."}
    });

    scenes_.emplace("roadside_challenger_intro", std::vector<DialogueLine>{
        {"ROADSIDE FIGHTER", "You walked past like you'd already won the tournament. I hated that."},
        {"RRVVFO", "You challenged the way I walk?"},
        {"ROADSIDE FIGHTER", "Beat me and the spectator pass is yours."},
        {"RRVVFO", "Fine. You better keep your word, or I'll make you."}
    });
    scenes_.emplace("roadside_challenger_leave", std::vector<DialogueLine>{
        {"RRVVFO", "I’m busy, and I can’t miss the tournament. Sorry."}
    });

    scenes_.emplace("sign_quest_intro", std::vector<DialogueLine>{
        {"SIGN PAINTER", "Hey, red kid. Don't touch that sign."},
        {"RRVVFO", "Wasn't planning to."},
        {"SIGN PAINTER", "Good. It keeps turning around to point at you."},
        {"RRVVFO", "Then the sign has good taste."},
        {"SIGN PAINTER", "Signs have paint, not taste. Use that weird eye thing and tell me what's wrong before the checkpoint guy notices."}
    });
    scenes_.emplace("sign_quest_lens_reveal", std::vector<DialogueLine>{
        {"RRVVFO", "It's not painted backward. It's lying."},
        {"SIGN", "CHAMPION."},
        {"RRVVFO", "Yeah, I know. You're still facing the wrong road."},
        {"SIGN PAINTER", "Please stop encouraging it. Put it on the empty post."}
    });
    scenes_.emplace("sign_quest_complete", std::vector<DialogueLine>{
        {"SIGN", "CHAMPION →"},
        {"RRVVFO", "There. Now it's right twice."},
        {"SIGN PAINTER", "How can a sign be right twice?"},
        {"RRVVFO", "It points to the tournament and me."},
        {"SIGN PAINTER", "I liked you better when you weren't touching the sign."}
    });
    scenes_.emplace("sign_quest_repeat", std::vector<DialogueLine>{
        {"SIGN PAINTER", "It hasn't moved since you fixed it."},
        {"RRVVFO", "It knows better."}
    });
    scenes_.emplace("sign_quest_reminder_lens", std::vector<DialogueLine>{
        {"SIGN PAINTER", "Use that eye thing on the sign. I want proof before I repaint the arrow again."}
    });
    scenes_.emplace("sign_quest_reminder_swap", std::vector<DialogueLine>{
        {"SIGN PAINTER", "The empty post is right there. Swap the lying sign onto it."}
    });

    scenes_.emplace("cliff_overlook_reaction", std::vector<DialogueLine>{
        {"RRVVFO", "Okay. That view almost makes the climb worth it.", "rrvvfo", "confident", "rrvvfo"}
    });

    scenes_.emplace("terrain_collapse_intro", std::vector<DialogueLine>{
        {"RRVVFO", "Seriously? The road waited until I got here to collapse?", "rrvvfo", "annoyed", "rrvvfo"},
        {"RRVVFO", "Fine. Side path. I'll be back on the road before this dust settles.", "rrvvfo", "confident", "rrvvfo"}
    });
    scenes_.emplace("terrain_collapse_complete", std::vector<DialogueLine>{
        {"RRVVFO", "There. Same road. Problem solved.", "rrvvfo", "confident", "rrvvfo"}
    });

    scenes_.emplace("tournament_checkpoint_dialogue", std::vector<DialogueLine>{
        {"TOURNAMENT CHECKPOINT", "Name and reason. Spectator or challenger?"},
        {"RRVVFO", "My name’s Rrvvfo. I’m a challenger—and I’m gonna win."},
        {"TOURNAMENT CHECKPOINT", "Proceed."}
    });
    scenes_.emplace("lens_manual_reaction", std::vector<DialogueLine>{
        {"RRVVFO", "Why would he hide the page explaining how to find hidden things?"}
    });
    scenes_.emplace("tournament_outskirts_arrival", std::vector<DialogueLine>{
        {"TOURNAMENT FAN", "The arena’s right there! Wait—aren’t you the guy who helped the transport?"},
        {"RRVVFO", "Yeah, I beat Revvfo. No big deal."},
        {"TOURNAMENT FAN", "WAIT, REALLY?! HOLY—BIG DEAL!"},
        {"RRVVFO", "This seems like a knockoff of the World Martial Arts Tournament."},
        {"SIGN PAINTER", "They’re owned by the same CEO."}
    });
}

const std::vector<DialogueLine>& DialogueRegistry::get(const std::string& sceneId) const {
    const auto it = scenes_.find(sceneId);
    if (it == scenes_.end()) throw std::out_of_range("No dialogue for scene: " + sceneId);
    return it->second;
}

bool DialogueRegistry::has(const std::string& sceneId) const {
    return scenes_.find(sceneId) != scenes_.end();
}

} // namespace px
