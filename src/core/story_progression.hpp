#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace px {

enum class StoryStat : std::uint8_t { Hp, Power, Defense, Speed, Focus };

struct StoryStatBonuses {
    int hp{0};
    int power{0};
    int defense{0};
    int speed{0};
    int focus{0};
};

struct StoryStats {
    int hp{100};
    int power{10};
    int defense{10};
    int speed{10};
    int focus{10};
};

struct TournamentCardState {
    std::string ownerId;
    int level{1};
    int xp{0};
    StoryStatBonuses bonuses{};
    int pendingBonusChoices{0};
    bool acquired{false};
};

struct XpGrantResult {
    int oldLevel{1};
    int newLevel{1};
    int levelsGained{0};
};

// Shared progression math for every route and platform. Chapter 2 content decides
// when the card becomes visible; this system never exposes it by itself.
class StoryProgressionSystem {
public:
    static const std::vector<int>& thresholds();
    static int levelForXp(int xp);
    static StoryStats statsFor(const TournamentCardState& card);
    static XpGrantResult grantXp(TournamentCardState& card, int amount);
    static void applyBonusWheel(TournamentCardState& card, StoryStat stat, int rolledBonus);
    static int xpToNextLevel(const TournamentCardState& card);
};

} // namespace px

