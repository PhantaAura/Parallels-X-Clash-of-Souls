#pragma once
#include "core/math.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace px {

enum class TrainingTaskKind {
    Movement,
    BasicAttacks,
    PerfectBlock,
    ChargeEnergy,
    CoreAbilities,
    LensRead,
    CleanHits
};

struct TrainingStepDefinition {
    std::string id;
    TrainingTaskKind task{TrainingTaskKind::Movement};
    std::string objective;
    std::string prompt;
    std::string startBark;
    int requiredCount{1};
};

struct TrainingDefinition {
    std::string sceneId;
    std::string opponentId;
    std::string opponentName;
    Vec2 playerStart{};
    Vec2 opponentStart{};
    float meaningfulMovementDistance{72.0f};
    float chargeStartingEnergy{20.0f};
    float chargeTargetEnergy{75.0f};
    float lensStartingEnergy{35.0f};
    float lensRequiredEnergy{60.0f};
    // Legacy counts down to each Sage attack. The first attempt and retries use
    // deliberately different delays; BLOCK NOW / REACT NOW is only exposed
    // during the final warning window.
    float parryFirstAttackSeconds{1.35f};
    float parryRetryAttackSeconds{1.45f};
    float parryWarningSeconds{0.32f};
    float lensFirstAttackSeconds{0.90f};
    float lensRetryAttackSeconds{1.10f};
    float lensWarningSeconds{0.32f};
    float lensEvadeConfirmationSeconds{1.05f};
    std::vector<TrainingStepDefinition> steps;
};

class TrainingRegistry {
public:
    TrainingRegistry();
    const TrainingDefinition& get(const std::string& sceneId) const;
    bool has(const std::string& sceneId) const;
private:
    std::unordered_map<std::string, TrainingDefinition> scenes_;
};

} // namespace px
