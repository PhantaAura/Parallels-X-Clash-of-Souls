#include "core/combat.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace px {
namespace {

constexpr AttackDefinition kDefaultAttacks[] = {
    {},
    {AttackKind::Light1, .27f, .06f, .14f, 94, 40, 78, 5.1f, 7, 23, .28f, 4, 25},
    {AttackKind::Light2, .29f, .07f, .16f, 100, 44, 80, 5.5f, 8, 28, .30f, 4, 28},
    {AttackKind::Light3, .39f, .085f, .195f, 108, 49, 84, 8.1f, 12, 66, .34f, 6, 32},
    {AttackKind::Heavy, .62f, .19f, .34f, 124, 64, 94, 13.5f, 21, 104, .40f, 8, 40, 175, 0, false, false, true},
    {AttackKind::Launcher, .55f, .16f, .285f, 101, 50, 108, 9.8f, 15, 31, .31f, 10, 30, 430},
    {AttackKind::AirLight, .36f, .07f, .18f, 94, 48, 88, 6.1f, 7, 31, .23f, 5, 18, 0, 0, true},
    {AttackKind::AirHeavy, .52f, .14f, .29f, 118, 60, 102, 10.8f, 16, 76, .36f, 8, 24, 0, 285, true, false, true},
    {AttackKind::PursuitLight, .32f, .07f, .17f, 110, 56, 98, 7.4f, 9, 54, .28f, 2, 20, 105, 0, true, true},
    {AttackKind::PursuitHeavy, .46f, .12f, .26f, 122, 64, 112, 11.7f, 18, 118, .42f, 4, 24, 0, 390, true, true, true, true},
    {AttackKind::Grab, .38f, .08f, .18f, 72, 52, 80, 6.0f, 0, 66, .38f, 5, 0, 0, 0, false, false, false},
    {AttackKind::Projectile, .42f, .18f, .18f, 999, 24, 72, 11.5f, 6, 46, .27f, 6, 0},
    {AttackKind::Beam, .80f, .32f, .58f, 999, 58, 96, 28.0f, 22, 118, .48f, 11, 0}
};

constexpr std::size_t indexOf(AttackKind kind) { return static_cast<std::size_t>(kind); }

AttackDefinition sageAttack(AttackKind kind) {
    auto value = kDefaultAttacks[indexOf(kind)];
    switch (kind) {
        case AttackKind::Light1: value.duration=.29f;value.activeStart=.05f;value.activeEnd=.13f;value.range=104;value.width=48;value.damage=5.8f;value.lunge=20;break;
        case AttackKind::Light2: value.duration=.31f;value.activeStart=.11f;value.activeEnd=.19f;value.range=116;value.width=52;value.damage=6.4f;value.lunge=16;break;
        case AttackKind::Light3: value.duration=.44f;value.activeStart=.07f;value.activeEnd=.22f;value.range=130;value.width=59;value.damage=8.6f;value.knockback=74;value.lunge=12;break;
        case AttackKind::Heavy: value.duration=.67f;value.activeStart=.16f;value.activeEnd=.35f;value.range=151;value.width=73;value.damage=14.2f;value.guardDamage=24;value.knockback=118;value.lunge=15;value.launch=180;break;
        case AttackKind::Launcher: value.duration=.58f;value.activeStart=.18f;value.activeEnd=.31f;value.range=123;value.width=60;value.damage=10.2f;value.launch=435;value.lunge=14;break;
        case AttackKind::AirLight: value.range=112;value.width=57;value.damage=6.8f;value.lunge=12;break;
        case AttackKind::AirHeavy: value.range=138;value.width=68;value.damage=12;value.knockback=94;value.lunge=10;break;
        case AttackKind::PursuitLight: value.range=136;value.width=65;value.damage=8.7f;value.lunge=12;break;
        case AttackKind::PursuitHeavy: value.range=154;value.width=76;value.damage=13.2f;value.knockback=136;value.spike=410;value.lunge=10;break;
        default: break;
    }
    return value;
}

bool isMelee(AttackKind kind) {
    return kind >= AttackKind::Light1 && kind <= AttackKind::Grab;
}

bool isStrong(AttackKind kind) {
    return kind == AttackKind::Heavy || kind == AttackKind::Launcher || kind == AttackKind::AirHeavy || kind == AttackKind::PursuitHeavy;
}

float energyGain(AttackKind kind) {
    if (kind == AttackKind::Heavy || kind == AttackKind::AirHeavy || kind == AttackKind::PursuitHeavy) return 8.0f;
    if (kind == AttackKind::Launcher) return 7.0f;
    if (kind == AttackKind::Projectile || kind == AttackKind::Beam) return 3.0f;
    if (kind == AttackKind::Grab) return 4.0f;
    return 5.0f;
}

} // namespace

const AttackDefinition& CombatSystem::attackFor(const std::string& fighterId, AttackKind kind) {
    const auto index = indexOf(kind);
    if (index >= std::size(kDefaultAttacks)) throw std::out_of_range("Unknown attack kind");
    // Browser 2.9A.40.7.2R Chapter-1 Rrvvfo Fire Blast combat profile.
    // Projectile travel speed/radius are presentation/simulation fields still pending the projectile-object port;
    // do not fake those values by changing unrelated melee geometry.
    if (fighterId == "rrvvfo" && kind == AttackKind::Projectile) {
        static const AttackDefinition rrvvfoFireBlast = [] {
            auto value = kDefaultAttacks[indexOf(AttackKind::Projectile)];
            value.damage = 15.0f;
            value.guardDamage = 9.0f;
            return value;
        }();
        return rrvvfoFireBlast;
    }
    if (fighterId != "sage" && fighterId != "plouke") return kDefaultAttacks[index];
    static const std::array<AttackDefinition, std::size(kDefaultAttacks)> sage = [] {
        std::array<AttackDefinition, std::size(kDefaultAttacks)> result{};
        for (std::size_t i = 0; i < result.size(); ++i) result[i] = sageAttack(static_cast<AttackKind>(i));
        return result;
    }();
    return sage[index];
}

void CombatSystem::tick(FighterState& fighter, float dt) {
    const auto decay = [dt](float value){ return std::max(0.0f, value - dt); };
    fighter.comboTimer=decay(fighter.comboTimer);fighter.flowCancelWindow=decay(fighter.flowCancelWindow);
    fighter.perfectBlockWindow=decay(fighter.perfectBlockWindow);fighter.guardDelay=decay(fighter.guardDelay);
    fighter.guardBreakTimer=decay(fighter.guardBreakTimer);fighter.stunTimer=decay(fighter.stunTimer);
    fighter.knockdownTimer=decay(fighter.knockdownTimer);fighter.invulnerabilityTimer=decay(fighter.invulnerabilityTimer);
    fighter.counterWindow=decay(fighter.counterWindow);fighter.counterRecovery=decay(fighter.counterRecovery);
    fighter.counterCooldown=decay(fighter.counterCooldown);fighter.breakerCooldown=decay(fighter.breakerCooldown);
    fighter.dashTime=decay(fighter.dashTime);fighter.dashCooldown=decay(fighter.dashCooldown);
    fighter.pursuitWindow=decay(fighter.pursuitWindow);fighter.pursuitTime=decay(fighter.pursuitTime);
    fighter.pursuitFollowupWindow=decay(fighter.pursuitFollowupWindow);fighter.pursuitFinishWindow=decay(fighter.pursuitFinishWindow);
    fighter.wallSplatTimer=decay(fighter.wallSplatTimer);fighter.groundBounceTimer=decay(fighter.groundBounceTimer);
    fighter.guardBroken = fighter.guardBreakTimer > 0.0f;
    if (fighter.comboTimer <= 0.0f && fighter.activeAttack == AttackKind::None) {
        fighter.lightStep=0;fighter.comboHits=0;fighter.pursuitUsed=false;fighter.wallSplatUsed=false;fighter.groundBounceUsed=false;
    }
    if (fighter.stunTimer <= 0.0f && fighter.knockdownTimer <= 0.0f) fighter.breakerLocked=false;
    if (!fighter.blocking && !fighter.guardBroken && fighter.guardDelay <= 0.0f)
        fighter.guard=std::min(100.0f,fighter.guard+dt*10.0f);
}

void CombatSystem::startBlock(FighterState& fighter) {
    if (fighter.guardBroken || fighter.stunTimer > 0.0f || fighter.counterRecovery > 0.0f) return;
    fighter.blocking=true;fighter.perfectBlockWindow=kPerfectBlockSeconds;
}

void CombatSystem::stopBlock(FighterState& fighter) { fighter.blocking=false;fighter.perfectBlockWindow=0.0f; }

bool CombatSystem::startAttack(FighterState& fighter, AttackKind kind) {
    const auto& attack=attackFor(fighter.id,kind);
    if (kind==AttackKind::None || fighter.activeAttack!=AttackKind::None || fighter.stunTimer>0 || fighter.guardBroken || fighter.knockdownTimer>0) return false;
    if (attack.airborne && !fighter.airborne && !attack.pursuit) return false;
    if (!attack.airborne && fighter.airborne) return false;
    if (attack.pursuit && fighter.pursuitTime<=0 && fighter.pursuitFollowupWindow<=0 && fighter.pursuitFinishWindow<=0) return false;
    fighter.activeAttack=kind;fighter.attackElapsed=0;fighter.attackConnected=false;fighter.blocking=false;
    return true;
}

HitResult CombatSystem::advanceAttack(FighterState& attacker,FighterState& defender,float dt,bool inRange,HitContext context) {
    if (attacker.activeAttack==AttackKind::None) return {};
    const auto attack=attackFor(attacker.id,attacker.activeAttack);
    attacker.attackElapsed+=std::max(0.0f,dt);
    HitResult result;
    if (!attacker.attackConnected && inRange && attacker.attackElapsed>=attack.activeStart && attacker.attackElapsed<=attack.activeEnd) {
        attacker.attackConnected=true;result=apply(attacker,defender,attack,context);
    }
    if (attacker.attackElapsed>=attack.duration) { attacker.activeAttack=AttackKind::None;attacker.attackElapsed=0; }
    return result;
}

HitResult CombatSystem::apply(FighterState& attacker,FighterState& defender,const AttackDefinition& attack,HitContext context) {
    HitResult hit;hit.connected=true;hit.knockback=attack.knockback;hit.stun=attack.stun;hit.hitstopFrames=attack.hitstopFrames;
    const bool counterable=attack.kind!=AttackKind::Projectile&&attack.kind!=AttackKind::Beam&&attack.kind!=AttackKind::Grab;
    if (defender.counterWindow>0&&counterable) {
        defender.counterWindow=0;defender.counterRecovery=.62f;attacker.hp=std::max(0.0f,attacker.hp-8.0f);attacker.stunTimer=.38f;
        hit.countered=true;hit.connected=false;return hit;
    }
    if (defender.invulnerabilityTimer>0) { hit.connected=false;return hit; }
    if (defender.blocking&&!defender.guardBroken&&attack.kind!=AttackKind::Grab) {
        hit.blocked=true;hit.perfectBlocked=defender.perfectBlockWindow>0;
        hit.guardDamage=attack.guardDamage*(hit.perfectBlocked?.18f:1.0f);
        defender.guard=std::max(0.0f,defender.guard-hit.guardDamage);defender.guardDelay=1.25f;
        defender.stunTimer=hit.perfectBlocked?.04f:.115f;
        if (hit.perfectBlocked) { attacker.stunTimer=std::max(attacker.stunTimer,.24f);defender.energy=std::min(100.0f,defender.energy+8.0f); }
        if (defender.guard<=0) {
            defender.guard=28;defender.guardBreakTimer=.88f;defender.stunTimer=.88f;defender.blocking=false;
            defender.hp=std::max(0.0f,defender.hp-2.0f);hit.damage=2.0f;hit.guardBroken=true;
        }
        return hit;
    }
    const float scale=attack.kind==AttackKind::Grab?1.0f:std::max(.55f,1.0f-static_cast<float>(attacker.comboHits)*.08f);
    hit.damage=attack.damage*scale;defender.hp=std::max(0.0f,defender.hp-hit.damage);
    defender.stunTimer=std::max(defender.stunTimer,attack.stun);defender.knockbackVelocity=attack.knockback;
    hit.launched=attack.launch>0;hit.knockdown=attack.knockdown;
    if (hit.launched) { defender.airborne=true;defender.verticalVelocity=std::max(defender.verticalVelocity,attack.launch); }
    if (attack.spike>0&&defender.airborne) defender.verticalVelocity=-attack.spike;
    if (attack.knockdown) defender.knockdownTimer=std::max(defender.knockdownTimer,.72f);
    if (context.arenaUsesWalls&&context.defenderNearWall&&!attacker.wallSplatUsed&&isStrong(attack.kind)) {
        attacker.wallSplatUsed=true;defender.wallSplatTimer=.58f;defender.stunTimer=std::max(defender.stunTimer,.58f);hit.wallSplat=true;
    } else if (defender.airborne&&!attacker.groundBounceUsed&&(attack.kind==AttackKind::AirHeavy||attack.kind==AttackKind::PursuitHeavy)) {
        attacker.groundBounceUsed=true;defender.groundBounceTimer=1.25f;hit.groundBounce=true;
    }
    if (isMelee(attack.kind)&&attack.kind!=AttackKind::Grab) attacker.flowCancelWindow=kFlowCancelSeconds;
    attacker.energy=std::min(100.0f,attacker.energy+energyGain(attack.kind));defender.energy=std::min(100.0f,defender.energy+2.0f);
    attacker.comboHits=attacker.comboTimer>0?attacker.comboHits+1:1;attacker.comboTimer=.94f;
    if ((attack.kind==AttackKind::Heavy||attack.kind==AttackKind::Launcher)&&!attacker.pursuitUsed&&!hit.wallSplat) {
        attacker.pursuitWindow=attack.kind==AttackKind::Launcher?.72f:.58f;hit.pursuitOpened=true;
    }
    return hit;
}

HitResult CombatSystem::light(FighterState& attacker,FighterState& defender,HitContext context) {
    if (attacker.comboTimer<=0) attacker.lightStep=0;
    attacker.lightStep=(attacker.lightStep%3)+1;
    const auto kind=attacker.lightStep==1?AttackKind::Light1:attacker.lightStep==2?AttackKind::Light2:AttackKind::Light3;
    return apply(attacker,defender,attackFor(attacker.id,kind),context);
}
HitResult CombatSystem::heavy(FighterState& a,FighterState& d,HitContext c){return apply(a,d,attackFor(a.id,AttackKind::Heavy),c);}
HitResult CombatSystem::launcher(FighterState& a,FighterState& d,HitContext c){return apply(a,d,attackFor(a.id,AttackKind::Launcher),c);}
HitResult CombatSystem::airLight(FighterState& a,FighterState& d,HitContext c){return apply(a,d,attackFor(a.id,AttackKind::AirLight),c);}
HitResult CombatSystem::airHeavy(FighterState& a,FighterState& d,HitContext c){return apply(a,d,attackFor(a.id,AttackKind::AirHeavy),c);}
HitResult CombatSystem::pursuitLight(FighterState& a,FighterState& d,HitContext c){a.pursuitFollowupWindow=0;a.pursuitFinishWindow=.36f;return apply(a,d,attackFor(a.id,AttackKind::PursuitLight),c);}
HitResult CombatSystem::pursuitHeavy(FighterState& a,FighterState& d,HitContext c){a.pursuitFinishWindow=0;return apply(a,d,attackFor(a.id,AttackKind::PursuitHeavy),c);}
HitResult CombatSystem::grab(FighterState& a,FighterState& d,HitContext c){(void)c;d.blocking=false;return apply(a,d,attackFor(a.id,AttackKind::Grab));}

bool CombatSystem::startCounter(FighterState& f){if(f.energy<kCounterCost||f.counterCooldown>0||f.stunTimer>0)return false;f.energy-=kCounterCost;f.counterWindow=.14f;f.counterCooldown=kCounterCooldown;return true;}
bool CombatSystem::comboBreaker(FighterState& f){if(f.stunTimer<=0||f.energy<kBreakerCost||f.breakerCooldown>0||f.breakerLocked)return false;f.energy-=kBreakerCost;f.breakerCooldown=kBreakerCooldown;f.breakerLocked=true;f.stunTimer=0;f.knockdownTimer=0;f.invulnerabilityTimer=.28f;return true;}
bool CombatSystem::flowCancel(FighterState& f){if(f.flowCancelWindow<=0||f.energy<kFlowCancelCost)return false;f.energy-=kFlowCancelCost;f.flowCancelWindow=0;f.activeAttack=AttackKind::None;return true;}
bool CombatSystem::startDash(FighterState& f){if(f.dashCooldown>0||f.stunTimer>0||f.activeAttack!=AttackKind::None)return false;f.dashTime=.22f;f.dashCooldown=.30f;f.invulnerabilityTimer=std::max(f.invulnerabilityTimer,.11f);return true;}
bool CombatSystem::startAirDash(FighterState& f){if(!f.airborne||f.airDashUsed||!startDash(f))return false;f.airDashUsed=true;return true;}
bool CombatSystem::startPursuit(FighterState& f){if(f.pursuitWindow<=0||f.pursuitUsed)return false;f.pursuitUsed=true;f.pursuitWindow=0;f.pursuitTime=.38f;f.pursuitFollowupWindow=.70f;f.airborne=true;return true;}
float CombatSystem::charge(FighterState& f,float dt,bool still){if(still&&f.stunTimer<=0)f.energy=std::min(100.0f,f.energy+f.chargeRate*std::max(0.0f,dt));return f.energy;}

ClashResult CombatSystem::resolveMeleeClash(const FighterState& a,const FighterState& b,float distance,float heightDifference){
    if(distance>118||std::abs(heightDifference)>85)return ClashResult::None;
    const auto active=[](const FighterState& f){if(f.activeAttack==AttackKind::None)return false;const auto& d=CombatSystem::attackFor(f.id,f.activeAttack);return (f.activeAttack==AttackKind::Heavy||f.activeAttack==AttackKind::AirHeavy||f.activeAttack==AttackKind::Launcher)&&f.attackElapsed>=d.activeStart&&f.attackElapsed<=d.activeEnd;};
    return active(a)&&active(b)?ClashResult::Draw:ClashResult::None;
}
ClashResult CombatSystem::resolveProjectileClash(float a,float b){const float delta=a-b;if(std::abs(delta)<.35f)return ClashResult::Draw;return delta>0?ClashResult::FirstWins:ClashResult::SecondWins;}

AiDecision CombatSystem::chooseAiAction(AiArchetype archetype,const FighterState& self,const FighterState& opponent,float distance,unsigned index){
    AiDecision result;
    if(opponent.activeAttack!=AttackKind::None&&distance<150){if(archetype==AiArchetype::Defensive||index%3==0)result.block=true;else result.dash=true;return result;}
    if(distance>170){result.approach=true;if(archetype==AiArchetype::Aggressive&&self.dashCooldown<=0)result.dash=true;return result;}
    if(opponent.blocking){result.attack=AttackKind::Grab;return result;}
    if(archetype==AiArchetype::Defensive&&index%4==0){result.retreat=true;result.block=true;return result;}
    result.attack=index%5==0?AttackKind::Launcher:index%3==0?AttackKind::Heavy:AttackKind::Light1;
    return result;
}

} // namespace px
