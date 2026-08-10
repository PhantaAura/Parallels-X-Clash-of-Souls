#include "content/training_registry.hpp"
#include <stdexcept>

namespace px {

TrainingRegistry::TrainingRegistry() {
    TrainingDefinition sage;
    sage.sceneId = "sage_tutorial_spar";
    sage.opponentId = "sage";
    sage.opponentName = "THE SAGE";
    sage.playerStart = {-1240.0f, 120.0f};
    sage.opponentStart = {-1075.0f, 120.0f};
    sage.steps = {
        {"movement", TrainingTaskKind::Movement,
         "SAGE'S CHALLENGE • CATCH ME",
         "Move, jump, and dash while holding a direction.",
         "SAGE • IF YOU CAN'T REACH ME, YOU CAN'T HIT ME.", 3},
        {"basic_attacks", TrainingTaskKind::BasicAttacks,
         "SAGE'S CHALLENGE • HIT BACK",
         "Use one light, heavy, launcher, and close-range grab. Any order works.",
         "SAGE • FOUR WAYS IN. SHOW ME ALL FOUR.", 4},
        {"perfect_block", TrainingTaskKind::PerfectBlock,
         "SAGE'S CHALLENGE • READ THE HIT",
         "Wait for BLOCK NOW, then tap guard as the Sage strikes.",
         "SAGE • DON'T GUARD EARLY. READ ME.", 1},
        {"charge", TrainingTaskKind::ChargeEnergy,
         "SAGE'S CHALLENGE • CONTROL POWER",
         "Stand still and hold CHARGE until the energy meter reaches 75.",
         "SAGE • POWER IS USELESS IF YOU CAN'T CONTROL IT.", 75},
        {"core_abilities", TrainingTaskKind::CoreAbilities,
         "SAGE'S CHALLENGE • CHANGE THE FIELD",
         "Use Fire Blast and Object Swap. Only current techniques are visible.",
         "SAGE • FIRE MOVES THE PROBLEM. SWAP MOVES YOU.", 2},
        {"lens_read", TrainingTaskKind::LensRead,
         "SAGE'S CHALLENGE • TRUST THE LENS",
         "Charge to 60 energy, activate Lens, then follow its prediction.",
         "SAGE • THE LENS WARNS YOU. YOUR FEET STILL HAVE TO LISTEN.", 3},
        {"clean_hits", TrainingTaskKind::CleanHits,
         "SAGE'S CHALLENGE • PROVE IT",
         "Land three clean hits on the Sage using anything you relearned.",
         "SAGE • THREE CLEAN HITS. MAKE ME BELIEVE YOU'RE AWAKE.", 3}
    };
    scenes_.emplace(sage.sceneId, std::move(sage));
}

const TrainingDefinition& TrainingRegistry::get(const std::string& sceneId) const {
    const auto it = scenes_.find(sceneId);
    if (it == scenes_.end()) throw std::out_of_range("No training definition for scene: " + sceneId);
    return it->second;
}

bool TrainingRegistry::has(const std::string& sceneId) const {
    return scenes_.find(sceneId) != scenes_.end();
}

} // namespace px
