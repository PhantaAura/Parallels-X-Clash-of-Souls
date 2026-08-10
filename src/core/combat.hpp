#pragma once
#include <cstdint>
#include <string>

namespace px {

enum class AttackKind : std::uint8_t {
    None,
    Light1,
    Light2,
    Light3,
    Heavy,
    Launcher,
    AirLight,
    AirHeavy,
    PursuitLight,
    PursuitHeavy,
    Grab,
    Projectile,
    Beam
};

struct AttackDefinition {
    AttackKind kind{AttackKind::None};
    float duration{0.0f};
    float activeStart{0.0f};
    float activeEnd{0.0f};
    float range{0.0f};
    float width{0.0f};
    float height{0.0f};
    float damage{0.0f};
    float guardDamage{0.0f};
    float knockback{0.0f};
    float stun{0.0f};
    int hitstopFrames{0};
    float lunge{0.0f};
    float launch{0.0f};
    float spike{0.0f};
    bool airborne{false};
    bool pursuit{false};
    bool knockdown{false};
    bool wallBounce{false};
};

struct FighterState {
    std::string id;
    float hp{100.0f};
    float maxHp{100.0f};
    float energy{100.0f};
    float guard{100.0f};
    int lightStep{0};
    int comboHits{0};
    float comboTimer{0.0f};
    float flowCancelWindow{0.0f};
    float perfectBlockWindow{0.0f};
    float guardDelay{0.0f};
    float guardBreakTimer{0.0f};
    float stunTimer{0.0f};
    float knockdownTimer{0.0f};
    float invulnerabilityTimer{0.0f};
    float counterWindow{0.0f};
    float counterRecovery{0.0f};
    float counterCooldown{0.0f};
    float breakerCooldown{0.0f};
    float dashTime{0.0f};
    float dashCooldown{0.0f};
    float pursuitWindow{0.0f};
    float pursuitTime{0.0f};
    float pursuitFollowupWindow{0.0f};
    float pursuitFinishWindow{0.0f};
    float wallSplatTimer{0.0f};
    float groundBounceTimer{0.0f};
    float knockbackVelocity{0.0f};
    float verticalVelocity{0.0f};
    float chargeRate{42.0f};
    AttackKind activeAttack{AttackKind::None};
    float attackElapsed{0.0f};
    bool attackConnected{false};
    bool blocking{false};
    bool airborne{false};
    bool guardBroken{false};
    bool breakerLocked{false};
    bool airDashUsed{false};
    bool pursuitUsed{false};
    bool wallSplatUsed{false};
    bool groundBounceUsed{false};
};

struct HitContext {
    bool defenderNearWall{false};
    bool arenaUsesWalls{true};
};

struct HitResult {
    float damage{0.0f};
    float guardDamage{0.0f};
    float knockback{0.0f};
    float stun{0.0f};
    int hitstopFrames{0};
    bool connected{false};
    bool blocked{false};
    bool perfectBlocked{false};
    bool countered{false};
    bool guardBroken{false};
    bool launched{false};
    bool knockdown{false};
    bool wallSplat{false};
    bool groundBounce{false};
    bool pursuitOpened{false};
};

enum class ClashResult : std::uint8_t { None, Draw, FirstWins, SecondWins };
enum class AiArchetype : std::uint8_t { Balanced, Aggressive, Defensive, Adaptive };

struct AiDecision {
    AttackKind attack{AttackKind::None};
    bool approach{false};
    bool retreat{false};
    bool block{false};
    bool dash{false};
    bool counter{false};
};

class CombatSystem {
public:
    static constexpr float kStep = 1.0f / 60.0f;
    static constexpr float kFlowCancelCost = 8.0f;
    static constexpr float kFlowCancelSeconds = 0.30f;
    static constexpr float kPerfectBlockSeconds = 0.11f;
    static constexpr float kCounterCost = 18.0f;
    static constexpr float kCounterCooldown = 2.4f;
    static constexpr float kBreakerCost = 60.0f;
    static constexpr float kBreakerCooldown = 6.5f;
    static constexpr int kJuggleLimit = 6;

    static const AttackDefinition& attackFor(const std::string& fighterId, AttackKind kind);
    static void tick(FighterState& fighter, float dt);
    static void startBlock(FighterState& fighter);
    static void stopBlock(FighterState& fighter);
    static bool startAttack(FighterState& fighter, AttackKind kind);
    static HitResult advanceAttack(FighterState& attacker, FighterState& defender, float dt, bool inRange,
                                   HitContext context = {});
    static HitResult light(FighterState& attacker, FighterState& defender, HitContext context = {});
    static HitResult heavy(FighterState& attacker, FighterState& defender, HitContext context = {});
    static HitResult launcher(FighterState& attacker, FighterState& defender, HitContext context = {});
    static HitResult airLight(FighterState& attacker, FighterState& defender, HitContext context = {});
    static HitResult airHeavy(FighterState& attacker, FighterState& defender, HitContext context = {});
    static HitResult pursuitLight(FighterState& attacker, FighterState& defender, HitContext context = {});
    static HitResult pursuitHeavy(FighterState& attacker, FighterState& defender, HitContext context = {});
    static HitResult grab(FighterState& attacker, FighterState& defender, HitContext context = {});
    static bool startCounter(FighterState& fighter);
    static bool comboBreaker(FighterState& fighter);
    static bool flowCancel(FighterState& fighter);
    static bool startDash(FighterState& fighter);
    static bool startAirDash(FighterState& fighter);
    static bool startPursuit(FighterState& fighter);
    static float charge(FighterState& fighter, float dt, bool standingStill);
    static ClashResult resolveMeleeClash(const FighterState& first, const FighterState& second, float distance, float heightDifference);
    static ClashResult resolveProjectileClash(float firstPower, float secondPower);
    static AiDecision chooseAiAction(AiArchetype archetype, const FighterState& self, const FighterState& opponent,
                                     float distance, unsigned decisionIndex);

private:
    static HitResult apply(FighterState& attacker, FighterState& defender, const AttackDefinition& attack,
                           HitContext context = {});
};

} // namespace px
