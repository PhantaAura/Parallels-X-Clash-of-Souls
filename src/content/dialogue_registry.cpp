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

scenes_.emplace("tournament_gate_walk_in", std::vector<DialogueLine>{
    {"THE SAGE", "Hey. How was the road? I had some important business to take care of.", "sage", "casual", "sage"},
    {"RRVVFO", "We both know what that was, Mr. Sage the Great.", "rrvvfo", "sarcastic", "rrvvfo"}
});
scenes_.emplace("ch2_arrival_delay", std::vector<DialogueLine>{
    {"ANNOUNCER", "Registration delay! Nobody panic unless you’re holding part of the bracket!"},
    {"RRVVFO", "Not my problem."},
    {"THE SAGE", "You’re bored, right? Go do that."},
    {"RRVVFO", "Whatever. I’ve got nothing better to be doing."},
    {"ANNOUNCER", "Disaster! Three contestant cards escaped the bracket board!"},
    {"RRVVFO", "Aren’t you the announcer I used to watch in those World Tournaments on TV when I was younger? So I guess your clumsiness wasn’t a character."},
    {"ANNOUNCER", "HEY! HURTFUL!"},
    {"RRVVFO", "I’m helping you. I can be as rude as I want."}
});
scenes_.emplace("ch2_bracket_wade", std::vector<DialogueLine>{
    {"TOURNAMENT FAN", "A contestant card blew into my souvenir bag. It says Wade."},
    {"RRVVFO", "Of course his card traveled faster than everybody else’s."}
});
scenes_.emplace("ch2_bracket_bark", std::vector<DialogueLine>{
    {"RRVVFO", "Bark’s card landed on the upper market walkway."},
    {"FOOD VENDOR", "I said it went up. I did not say the wind respected stairs."}
});
scenes_.emplace("ch2_bracket_qualifier", std::vector<DialogueLine>{
    {"OLD COMPETITOR", "There it is—caught on the maintenance cart!"},
    {"RRVVFO", "The bracket paperwork is officially faster than the staff."}
});
scenes_.emplace("ch2_bracket_return", std::vector<DialogueLine>{
    {"ANNOUNCER", "Wade, Bark, and the unreadable qualifier! The bracket lives!"},
    {"RRVVFO", "I’m about to win."}
});
scenes_.emplace("ch2_u8_gate", std::vector<DialogueLine>{
    {"ANNOUNCER", "Registration is moving again. Competitors, stay near the practice grounds."}
});

scenes_.emplace("ch2_practice_brawl_intro", std::vector<DialogueLine>{
    {"THE SAGE", "Stay near the practice ring. I need to check something before your first brawl."},
    {"RRVVFO", "He’s probably spying on ladies again. Perv."},
    {"PRACTICE RING FIGHTER", "Alright, let’s fight. A little training’s good for ya, boy."},
    {"RRVVFO", "Alright. Let’s do it."}
});
scenes_.emplace("ch2_practice_brawl_result", std::vector<DialogueLine>{{"PRACTICE RING FIGHTER","Not bad. That card system will make more sense once registration catches up."}});
scenes_.emplace("ch2_ninja_reunion", std::vector<DialogueLine>{
    {"WADE", "There you are! We’ve been looking all over—"},
    {"BARK", "We just got here, Wade. We didn’t even start looking yet."},
    {"RRVVFO", "Wade, you really are dense, huh?"},
    {"WADE", "Looks like a big ninja reunion. The three of us."},
    {"RRVVFO", "But only one of us can win."},
    {"BARK", "It’s me."},
    {"WADE", "It’s me. I’m the most tactical."},
    {"BARK", "In what world?"},
    {"RRVVFO", "In what world?"}
});
scenes_.emplace("ch2_wade_shortcut_win", std::vector<DialogueLine>{
    {"WADE","You beat that time? I was showing you the route, not racing at full speed."},
    {"RRVVFO","A shortcut only matters if I reach the end first."},
    {"BARK","The east support is cracked. Look."}
});
scenes_.emplace("ch2_wade_shortcut_loss", std::vector<DialogueLine>{
    {"WADE","You made it. The route is faster when you stop arguing with every corner."},
    {"RRVVFO","Uh, well—I WAS HOLDING BACK! YOU'LL SEE IN THE TOURNAMENT!"},
    {"BARK","The east support is cracked. Look."}
});
// Kept for save/content compatibility with U10 while runtime now chooses the
// authored win or loss response.
scenes_.emplace("ch2_wade_shortcut_result", std::vector<DialogueLine>{{"WADE","That shortcut connects the whole grounds."}});
scenes_.emplace("ch2_cracked_ring_intro", std::vector<DialogueLine>{
    {"BARK","You're the sneakiest one here. Inspect the cracked supports before an official sees us."},
    {"RRVVFO","Fine, but you and Wade guard me. I'm not getting banned because your ring broke."},
    {"WADE","The shoe marks on the east side look different."},
    {"RRVVFO","It's Wade. We'll check the useful clues first."},
    {"BARK","Check all three."}
});
scenes_.emplace("ch2_crack_west", std::vector<DialogueLine>{{"RRVVFO","The damage is pushing outward. Something hit this from inside the ring."}});
scenes_.emplace("ch2_crack_south", std::vector<DialogueLine>{{"RRVVFO","Footprints stop halfway. That’s not normal."}});
scenes_.emplace("ch2_crack_east", std::vector<DialogueLine>{{"WADE","These shoe marks are different from the other ones."},{"RRVVFO","Yeah. More than one pattern."}});
scenes_.emplace("ch2_cracked_ring_result", std::vector<DialogueLine>{
    {"RRVVFO","We fixed them, but it has to be a fighter. All the spectators are kept away from the rings until the tournament starts."},
    {"RRVVFO","We shouldn’t make a big deal out of it yet and worry the guests."},
    {"RRVVFO","After the tournament, I’ll figure out who did it. Don’t stress about it, Bark."},
    {"BARK","Alright. You find the culprit. I’m gonna practice."}
});
scenes_.emplace("ch2_registration_card", std::vector<DialogueLine>{
    {"REGISTRATION STAFF","Cards, please."},
    {"RRVVFO","What card?"},
    {"WADE","I thought you had ours."},
    {"BARK","Why would he have ours?"},
    {"REGISTRATION STAFF","Spare Tournament Cards. One each. Keep them with you."}
});
scenes_.emplace("ch2_opening_ceremony", std::vector<DialogueLine>{
    {"ANNOUNCER","Welcome to the local tournament! Hamual, Daniel, Hailey, Bark, Wade, Pouki, Plouke—and first-time entrant Rrvvfo!"},
    {"CROWD","The former champion Hamual towers over the entrance line. Daniel looks ordinary enough to be tournament staff."},
    {"RRVVFO","And Sage is missing. Shocking."},
    {"ANNOUNCER","First preliminary: Hailey versus Plouke!"}
});
scenes_.emplace("ch2_hailey_plouke", std::vector<DialogueLine>{
    {"HAILEY","Stop staring and defend yourself!"},
    {"RRVVFO","This guy reminds me of the Sage. I hope they never meet."},
    {"PLOUKE","I might not have won if it weren’t for that pebble I placed in front of me, just in case I got distracted."},
    {"ANNOUNCER","Official matches are first to three. A knockout or crossing the ring boundary removes a stock."},
    {"RRVVFO","So ring-outs matter. I should stay away from the edge."}
});
scenes_.emplace("ch2_hamual_intro", std::vector<DialogueLine>{
    {"ANNOUNCER","Opening round: Rrvvfo versus former champion Hamual!"},
    {"HAMUAL","Don't confuse size with slowness. I won this bracket before you entered one."},
    {"RRVVFO","You couldn't win even if you were ten times bigger."},
    {"RRVVFO","And no, I'm not wasting Fire Awakening on an opening round."}
});
scenes_.emplace("ch2_hamual_result", std::vector<DialogueLine>{
    {"ANNOUNCER","Rrvvfo advances!"},{"HAMUAL","You found a route through my reach. Most fighters only run from it."},
    {"RRVVFO","I don't run. I reposition."}
});
scenes_.emplace("ch2_daniel_intro", std::vector<DialogueLine>{
    {"ANNOUNCER","Next match: Rrvvfo versus Daniel!"},
    {"DANIEL","You expected another giant. That's useful."},
    {"RRVVFO","You look a little casual for a tournament."},
    {"DANIEL","Watch my feet, not my clothes."}
});
scenes_.emplace("ch2_daniel_result", std::vector<DialogueLine>{
    {"ANNOUNCER","Rrvvfo advances again!"},
    {"DANIEL","You stopped watching the jacket and adapted. That's why you caught my step."},
    {"RRVVFO","I notice things. Eventually."}
});
scenes_.emplace("ch2_bark_pouki", std::vector<DialogueLine>{
    {"ANNOUNCER","Pouki wins! Bark held the center, survived the guard break, and nearly landed one final counter!"},
    {"BARK","He changed rhythm every time I settled. My last counter was the first opening he gave me."},
    {"RRVVFO","If he got Bark that easily, he must be really strong."},
    {"WADE","You still have to beat me first."}
});
scenes_.emplace("ch2_wade_intro", std::vector<DialogueLine>{{"WADE","Guess the bracket really wanted this."},{"RRVVFO","You may be fast, but you’re slow in the brain."},{"WADE","I’m fast, not slow."}});
scenes_.emplace("ch2_wade_result", std::vector<DialogueLine>{{"WADE","You won! Yay!"},{"RRVVFO","I don’t think that’s supposed to be your reaction."},{"ANNOUNCER","Plouke has defeated Pouki in the opposite semifinal. The final is set!"}});
scenes_.emplace("ch2_clue_stillness", std::vector<DialogueLine>{{"OLD COMPETITOR","Plouke barely moves until the other fighter commits first."},{"RRVVFO","So he waits for people to slip up."}});
scenes_.emplace("ch2_clue_positioning", std::vector<DialogueLine>{{"WORKER","Every fighter who faces Plouke ends up standing exactly where he wants."},{"RRVVFO","So I have to be a little more cautious."}});
scenes_.emplace("ch2_clue_timing", std::vector<DialogueLine>{{"BARK","Plouke doesn’t overpower people immediately. He waits until their strongest option becomes predictable."},{"RRVVFO","If he has Bark on edge, I should take him a little more seriously."}});
scenes_.emplace("ch2_clue_edge", std::vector<DialogueLine>{{"WADE","Plouke always looks at the edge. Maybe he’s in love with it."},{"RRVVFO","Maybe that gave me some clues. He probably rings people out a lot."},{"RRVVFO","I should observe the matches."}});
scenes_.emplace("ch2_pre_plouke", std::vector<DialogueLine>{
    {"BARK","Stillness, positioning, timing, and the ring edge. We verified every pattern Plouke uses."},
    {"WADE","Don’t chase his retreat. Cut through the center and make him choose first."},
    {"RRVVFO","And Sage is still gone. Great timing."},
    {"PLOUKE","You used too much energy reaching this round."},
    {"RRVVFO","I can win this with my hands tied up."},
    {"PLOUKE","That confidence is exactly why you’re tired."},
    {"RRVVFO","Keep talking. It’ll make losing more embarrassing."}
});
scenes_.emplace("ch2_plouke_awakening_failure", std::vector<DialogueLine>{
    {"RRVVFO","Fine. I'll end this with Fire Awakening.","rrvvfo","focused","rrvvfo"},
    {"PLOUKE","Try it.","plouke","still","plouke"},
    {"RRVVFO","Come on... ignite!","rrvvfo","strained","rrvvfo"},
    {"RRVVFO","Why isn't it holding?","rrvvfo","surprised","rrvvfo"},
    {"PLOUKE","Because power doesn't erase exhaustion.","plouke","calm","plouke"},
    {"PLOUKE","Show me what you have left.","plouke","focused","plouke"}
});
scenes_.emplace("ch2_plouke_final_ringout", std::vector<DialogueLine>{
    {"RRVVFO","I beat you in the beam! Haha—"},{"RRVVFO","WAIT, I’M ON THE GRASS! AHH!"},{"PLOUKE","I beat you."},
    {"ANNOUNCER","Rrvvfo wins the beam clash—but Plouke wins by ring-out!"}
});
scenes_.emplace("ch2_plouke_final_exhausted", std::vector<DialogueLine>{
    {"RRVVFO","No! I lost the clash... I’m out of energy."},{"PLOUKE","The match is over."},{"ANNOUNCER","Plouke wins the tournament!"}
});
scenes_.emplace("ch2_plouke_reveal", std::vector<DialogueLine>{
    {"RRVVFO","Who are you?"},{"PLOUKE","You really did skim the disguise section."},{"RRVVFO","...No."},{"THE SAGE","Plouke was me."},
    {"RRVVFO","Now that explains what you were doing during your fight with Hailey."},{"RRVVFO","I hate how planned ahead you are."}
});
scenes_.emplace("ch2_tournament_aftermath", std::vector<DialogueLine>{{"ANNOUNCER","The local tournament is complete. Grounds remain open while staff begins cleanup."}});

scenes_.emplace("ch2_wade_lost_fan_start", std::vector<DialogueLine>{
    {"WADE","A fan followed my shortcut and vanished near the market. That's proof it was advanced."},
    {"RRVVFO","That's proof you gave directions like Wade. I'll find them before they enter the service lane."}
});
scenes_.emplace("ch2_wade_lost_fan_market", std::vector<DialogueLine>{
    {"LOST WADE FAN","Wade said to turn where the blue banner looks fastest."},
    {"RRVVFO","Banners don't have speed. Follow me back before Wade explains another corner."}
});
scenes_.emplace("ch2_wade_lost_fan_done", std::vector<DialogueLine>{
    {"WADE","You found them! My directions worked."},
    {"RRVVFO","I found them because your directions didn't."}
});
scenes_.emplace("ch2_fake_champion_start", std::vector<DialogueLine>{
    {"FAKE CHAMPION","Train with the undefeated champion! One lesson and you'll never lose."},
    {"RRVVFO","Then why are you hiding next to a food awning? Hold still."}
});
scenes_.emplace("ch2_fake_champion_reveal", std::vector<DialogueLine>{
    {"RRVVFO","Lens says your medal is painted wood and your record is zero fights."},
    {"FAKE CHAMPION","The Lens can't measure fighting spirit!"},
    {"RRVVFO","Good. Fight me for free."}
});
scenes_.emplace("ch2_fake_champion_done", std::vector<DialogueLine>{
    {"FAKE CHAMPION","Fine! The lessons are canceled."},
    {"RRVVFO","Keep the wooden medal. It matches the record."}
});
scenes_.emplace("ch2_runaway_dummy_start", std::vector<DialogueLine>{
    {"ARENA WORKER","That training dummy's wheel lock snapped! Stop it before it reaches registration!"},
    {"RRVVFO","Everything here runs away the second I arrive."}
});
scenes_.emplace("ch2_runaway_dummy_done", std::vector<DialogueLine>{
    {"ARENA WORKER","You knocked it back onto the display base."},
    {"RRVVFO","Leave it there. Now it looks like an exhibit about losing."}
});
scenes_.emplace("ch2_bark_practice_start", std::vector<DialogueLine>{
    {"BARK","Quiet spar? No score. We stop when one of us has enough."},
    {"RRVVFO","You mean when you have enough."}
});
scenes_.emplace("ch2_bark_practice_done", std::vector<DialogueLine>{{"BARK","That's enough."},{"RRVVFO","For you."}});
scenes_.emplace("ch2_festival_food", std::vector<DialogueLine>{
    {"FOOD VENDOR","Champion's skewers! Free sample for a contestant."},
    {"RRVVFO","Calling food 'champion' doesn't make it good."},
    {"FOOD VENDOR","You took two."},{"RRVVFO","I needed a second opinion."}
});
scenes_.emplace("ch2_festival_photo", std::vector<DialogueLine>{
    {"PHOTO WORKER","Champion pose on three!"},{"RRVVFO","You only need one."},{"PHOTO WORKER","...That was actually good."}
});
scenes_.emplace("ch2_bracket_board_status", std::vector<DialogueLine>{
    {"BRACKET BOARD","Current round and next opponent updated. Another match is already in the ring."}
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
