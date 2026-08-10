#include "core/story_progression.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace px {

const std::vector<int>& StoryProgressionSystem::thresholds() {
    static const std::vector<int> values{0, 100, 250, 450, 700, 1000, 1360, 1780, 2260, 2810};
    return values;
}

int StoryProgressionSystem::levelForXp(int xp) {
    const auto& values = thresholds();
    const auto it = std::upper_bound(values.begin(), values.end(), std::max(0, xp));
    return std::max(1, static_cast<int>(it - values.begin()));
}

StoryStats StoryProgressionSystem::statsFor(const TournamentCardState& card) {
    const int level = std::max(1, card.level);
    const int growth = level - 1;
    StoryStats stats;
    stats.hp = 100 + 4 * growth + card.bonuses.hp;
    stats.power = 10 + static_cast<int>(std::lround(1.25 * growth)) + card.bonuses.power;
    stats.defense = 10 + static_cast<int>(std::lround(0.75 * growth)) + card.bonuses.defense;
    stats.speed = 10 + static_cast<int>(std::lround(0.75 * growth)) + card.bonuses.speed;
    stats.focus = 10 + growth + card.bonuses.focus;
    return stats;
}

XpGrantResult StoryProgressionSystem::grantXp(TournamentCardState& card, int amount) {
    XpGrantResult result;
    result.oldLevel = card.level;
    card.xp = std::max(0, card.xp + std::max(0, amount));
    card.level = levelForXp(card.xp);
    result.newLevel = card.level;
    result.levelsGained = std::max(0, result.newLevel - result.oldLevel);
    card.pendingBonusChoices += result.levelsGained;
    return result;
}

void StoryProgressionSystem::applyBonusWheel(TournamentCardState& card, StoryStat stat, int rolledBonus) {
    if (card.pendingBonusChoices <= 0) throw std::logic_error("No pending level-up bonus");
    if (rolledBonus < 1 || rolledBonus > 3) throw std::out_of_range("Bonus wheel must roll +1, +2, or +3");
    switch (stat) {
        case StoryStat::Hp: card.bonuses.hp += rolledBonus; break;
        case StoryStat::Power: card.bonuses.power += rolledBonus; break;
        case StoryStat::Defense: card.bonuses.defense += rolledBonus; break;
        case StoryStat::Speed: card.bonuses.speed += rolledBonus; break;
        case StoryStat::Focus: card.bonuses.focus += rolledBonus; break;
    }
    --card.pendingBonusChoices;
}

int StoryProgressionSystem::xpToNextLevel(const TournamentCardState& card) {
    const auto& values = thresholds();
    if (card.level >= static_cast<int>(values.size())) return 0;
    return std::max(0, values[card.level] - card.xp);
}

} // namespace px

