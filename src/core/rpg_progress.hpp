#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace px {

enum class BattleRank : std::uint8_t { E, D, C, B, A, S };

struct BattlePerformance {
    bool playerWon{false};
    float damageTaken{0.0f};
    int bestCombo{0};
    int perfectBlocks{0};
    int guardBreaks{0};
    int pursuitFinishers{0};
    std::uint32_t actionVarietyMask{0};
    int stocksLost{0};
};

struct BattleRankResult {
    BattleRank rank{BattleRank::E};
    int score{0};
};

struct AdventureRecordsState {
    int wins{0};
    int losses{0};
    int bestCombo{0};
    int perfectBlocks{0};
    int guardBreaks{0};
    int pursuitFinishers{0};
    int energyBeamUses{0};
    int objectSwaps{0};
    BattleRank bestRank{BattleRank::E};
    bool hasRank{false};
};

struct RpgProgressState {
    int coins{0};
    int vendorDiscountPercent{0};
    bool nextOfficialMealBoost{false};
    std::vector<std::string> titles;
    std::vector<std::string> mementos;
    std::vector<std::string> profileUnlocks;
    // One gameplay accessory/training-item slot. Do not turn this into an
    // inventory/equipment framework until the game actually needs one.
    std::string equippedAccessoryId;
    bool weightedNecklaceAcquired{false};
    float weightedNecklaceMastery{0.0f};
    bool weightedNecklaceMasteryRewardGranted{false};
};

class RpgProgressSystem {
public:
    static BattleRankResult evaluate(const BattlePerformance& performance);
    static const char* rankLabel(BattleRank rank);
    static BattleRank rankFromLabel(const std::string& label);
    static int rankOrder(BattleRank rank);
    static void commitBattle(AdventureRecordsState& records,
                             const BattlePerformance& performance,
                             BattleRankResult result);
    static float necklaceMasteryRatio(const RpgProgressState& state);
    static float necklaceMovementMultiplier(const RpgProgressState& state);
    static float necklaceDashMultiplier(const RpgProgressState& state);
    static float necklaceChargeMultiplier(const RpgProgressState& state);
    static float necklacePowerMultiplier(const RpgProgressState& state);
    static bool addNecklaceMastery(RpgProgressState& state, float amount);
};

} // namespace px
