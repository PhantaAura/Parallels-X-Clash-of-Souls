#include "content/combat_manual_registry.hpp"
#include <algorithm>
#include <stdexcept>

namespace px {

CombatManualRegistry::CombatManualRegistry() {
    pages_ = {
        {"welcome", "START HERE", "HOW THE REFRESHER WORKS", "ONE TASK AT A TIME",
         "The current task is taught, demonstrated, and checked before the refresher advances. Guided begins at Movement, Resume uses the latest checkpoint, and Quick begins at Core Techniques.",
         {
             {"Guided", "ENTER", "A", "Play all seven refresher steps in Legacy order.", "manual_guided"},
             {"Resume", "ENTER", "A", "Continue from the latest saved refresher checkpoint.", "manual_resume"},
             {"Quick", "ENTER", "A", "Begin at Core Techniques, then complete Lens and the final spar.", "manual_quick"},
             {"Confirmation", "", "", "Light, Heavy, and Launcher count on attack start. Grab must connect. Other steps use their stated success rules.", "manual_confirmation"},
         }},
        {"movement", "FOUNDATIONS", "MOVEMENT", "GET INTO POSITION",
         "Movement is the first combat tool and the first refresher page.",
         {
             {"Move", "W A S D", "LEFT STICK", "Travel a meaningful total distance through the field.", "manual_move"},
             {"Jump", "SPACE", "SOUTH BUTTON", "Leave the ground once.", "manual_jump"},
             {"Dash", "SHIFT", "SHOULDER", "Perform the committed burst Dash. Dash is not a hold-to-run modifier.", "manual_dash"},
         }},
        {"basic_combat", "FOUNDATIONS", "BASIC COMBAT", "DO NOT JUST MASH",
         "Light, Heavy, Launcher, Timed Guard, and Grab form the readable base of every fight.",
         {
             {"Light", "J / MOUSE 1", "WEST BUTTON", "Fast pressure and combo starts.", "manual_light"},
             {"Heavy", "K", "NORTH BUTTON", "Slower impact, knockback, and pursuit opportunity.", "manual_heavy"},
             {"Launcher", "I", "EAST BUTTON", "Starts an air route and opens pursuit.", "manual_launcher"},
             {"Timed Guard / Perfect Block", "L / MOUSE 2", "RIGHT TRIGGER", "Tap at impact during the opening perfect-block timing.", "manual_guard"},
             {"Grab", "U", "LEFT SHOULDER", "Beats guarding. The refresher counts it only when it connects.", "manual_grab"},
         }},
        {"kinetic_combat", "FOUNDATIONS", "KINETIC COMBAT", "LAUNCH, CHASE, FINISH",
         "A launch is only the beginning. Pursuit turns knockback into a controlled chase with real offensive and defensive choices.",
         {
             {"Launcher to Pursuit", "I → SHIFT", "EAST → SHOULDER", "Land Launcher, then Dash during the Pursuit timing.", "manual_pursuit"},
             {"Pursuit Light", "J / MOUSE 1", "WEST BUTTON", "Buffer Light during the chase so it starts on arrival.", "manual_pursuit_light"},
             {"Pursuit Heavy", "K", "NORTH BUTTON", "Buffer Heavy for the stronger committed arrival option.", "manual_pursuit_heavy"},
             {"Buffering", "PRESS DURING CHASE", "PRESS DURING CHASE", "Store the follow-up while closing the distance.", "manual_buffering"},
             {"Pursuit Tech", "SHIFT + 15 ENERGY", "DASH + 15 ENERGY", "The defender can Dash during the incoming chase to escape and punish a predictable pursuit.", "manual_pursuit_tech"},
         }},
        {"resource_control", "FOUNDATIONS", "RESOURCE CONTROL", "STANDING STILL IS A DECISION",
         "Energy and Guard determine when Rrvvfo can commit to techniques and defense.",
         {
             {"Charge", "HOLD C", "HOLD D-PAD DOWN", "Stand still and charge Energy. Movement interrupts the lesson.", "manual_charge"},
             {"Attack Gain", "LAND CLEAN HITS", "LAND CLEAN HITS", "Successful offense restores Energy.", "manual_attack_gain"},
             {"Guard Recovery", "STOP BLOCKING", "STOP BLOCKING", "Guard recovers faster while standing still.", "manual_guard_recovery"},
             {"Lesson Target", "75 ENERGY", "75 ENERGY", "The refresher advances only after stationary charging reaches 75.", "manual_charge_target"},
         }},
        {"advanced_defense", "FOUNDATIONS", "ADVANCED DEFENSE", "READ THE ATTACK",
         "Guard cannot last forever. A correct read creates a punish window; a predictable guard can be grabbed.",
         {
             {"Perfect Block", "TAP L / MOUSE 2", "TAP RIGHT TRIGGER", "Tap during BLOCK NOW instead of holding early.", "manual_perfect_block"},
             {"Guard Pressure", "HEAVY / GRAB", "NORTH / SHOULDER", "Heavy attacks pressure Guard and Grab punishes passive defense.", "manual_guard_pressure"},
             {"Counter", "Q", "RIGHT SHOULDER", "Commit to a read and punish an incoming normal.", "manual_counter"},
             {"Combo Breaker", "R", "LEFT TRIGGER", "Spend the defensive resource to break a valid combo state.", "manual_breaker"},
         }},
        {"rrvvfo_techniques", "TECHNIQUES", "RRVVFO'S CURRENT TECHNIQUES", "CHAPTER 1 CHRONOLOGY",
         "Only techniques Rrvvfo actually has at this point appear. Future abilities do not occupy empty or question-mark slots.",
         {
             {"Fire Blast", "1", "ABILITY SLOT 1", "Ranged fire pressure.", "manual_fire_blast"},
             {"Object Swap", "2", "ABILITY SLOT 2", "Swap positions with a valid target or marked field object.", "manual_object_swap"},
             {"Lens of Truth", "3", "ABILITY SLOT 3", "Spend Energy and health to read the probable next attack.", "manual_lens"},
         }},
        {"input_devices", "REFERENCE", "INPUT DEVICES", "PROMPTS FOLLOW THE PLAYER",
         "The same semantic actions work on keyboard, controller, and future physical 3DS buttons.",
         {
             {"Keyboard", "CURRENT KEY LABELS", "", "Prompts use the active keyboard profile.", "manual_keyboard"},
             {"Controller", "", "CURRENT BUTTON GLYPHS", "Prompts switch to the active controller family.", "manual_controller"},
             {"Old 3DS", "", "PHYSICAL BUTTONS", "All actions remain available without touch-only controls.", "manual_3ds"},
         }},
    };
}

const CombatManualPage& CombatManualRegistry::get(const std::string& id) const {
    for (const auto& page : pages_) if (page.id == id) return page;
    throw std::out_of_range("Unknown combat manual page: " + id);
}

std::vector<std::string> CombatManualRegistry::categories() const {
    std::vector<std::string> result;
    for (const auto& page : pages_) {
        if (std::find(result.begin(), result.end(), page.category) == result.end()) result.push_back(page.category);
    }
    return result;
}

} // namespace px
