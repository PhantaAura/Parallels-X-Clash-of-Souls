#include "core/rpg_progress.hpp"
#include <algorithm>

namespace px {
namespace {

int bitCount(std::uint32_t value) {
    int result = 0;
    while (value != 0) { result += static_cast<int>(value & 1u); value >>= 1u; }
    return result;
}

} // namespace

BattleRankResult RpgProgressSystem::evaluate(const BattlePerformance& performance) {
    int score = performance.playerWon ? 35 : 12;
    score += std::clamp(20 - static_cast<int>(performance.damageTaken * .20f), 0, 20);
    score += std::clamp(performance.bestCombo * 2, 0, 15);
    score += std::min(12, performance.perfectBlocks * 4);
    score += std::min(10, performance.guardBreaks * 5);
    score += std::min(12, performance.pursuitFinishers * 6);
    score += std::min(10, bitCount(performance.actionVarietyMask) * 2);
    score += performance.stocksLost == 0 ? 10 : performance.stocksLost == 1 ? 5 : 0;
    score = std::clamp(score, 0, 100);
    const BattleRank rank = score >= 85 ? BattleRank::S : score >= 70 ? BattleRank::A :
                            score >= 55 ? BattleRank::B : score >= 40 ? BattleRank::C :
                            score >= 25 ? BattleRank::D : BattleRank::E;
    return {rank, score};
}

const char* RpgProgressSystem::rankLabel(BattleRank rank) {
    switch (rank) {
        case BattleRank::S: return "S";
        case BattleRank::A: return "A";
        case BattleRank::B: return "B";
        case BattleRank::C: return "C";
        case BattleRank::D: return "D";
        case BattleRank::E: return "E";
    }
    return "E";
}

BattleRank RpgProgressSystem::rankFromLabel(const std::string& label) {
    if (label == "S") return BattleRank::S;
    if (label == "A") return BattleRank::A;
    if (label == "B") return BattleRank::B;
    if (label == "C") return BattleRank::C;
    if (label == "D") return BattleRank::D;
    return BattleRank::E;
}

int RpgProgressSystem::rankOrder(BattleRank rank) { return static_cast<int>(rank); }

void RpgProgressSystem::commitBattle(AdventureRecordsState& records,
                                     const BattlePerformance& performance,
                                     BattleRankResult result) {
    if (performance.playerWon) ++records.wins; else ++records.losses;
    records.bestCombo = std::max(records.bestCombo, performance.bestCombo);
    records.perfectBlocks += performance.perfectBlocks;
    records.guardBreaks += performance.guardBreaks;
    records.pursuitFinishers += performance.pursuitFinishers;
    if (!records.hasRank || rankOrder(result.rank) > rankOrder(records.bestRank)) {
        records.bestRank = result.rank;
        records.hasRank = true;
    }
}

float RpgProgressSystem::necklaceMasteryRatio(const RpgProgressState& state) {
    return std::clamp(state.weightedNecklaceMastery / 100.0f, 0.0f, 1.0f);
}

float RpgProgressSystem::necklaceMovementMultiplier(const RpgProgressState& state) {
    if (state.equippedAccessoryId != "alts_weighted_necklace") return 1.0f;
    return .95f + .05f * necklaceMasteryRatio(state);
}

float RpgProgressSystem::necklaceDashMultiplier(const RpgProgressState& state) {
    return necklaceMovementMultiplier(state);
}

float RpgProgressSystem::necklaceChargeMultiplier(const RpgProgressState& state) {
    return necklaceMovementMultiplier(state);
}

float RpgProgressSystem::necklacePowerMultiplier(const RpgProgressState& state) {
    return state.equippedAccessoryId == "alts_weighted_necklace" ? 1.04f : 1.0f;
}

bool RpgProgressSystem::addNecklaceMastery(RpgProgressState& state, float amount) {
    if (!state.weightedNecklaceAcquired || state.equippedAccessoryId != "alts_weighted_necklace" || amount <= 0.0f)
        return false;
    const bool wasMastered = state.weightedNecklaceMastery >= 100.0f;
    state.weightedNecklaceMastery = std::clamp(state.weightedNecklaceMastery + amount, 0.0f, 100.0f);
    return !wasMastered && state.weightedNecklaceMastery >= 100.0f;
}

} // namespace px
