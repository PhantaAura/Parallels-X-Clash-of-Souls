#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace px {

enum class QuestKind : std::uint8_t { Required, Optional };
enum class QuestStatus : std::uint8_t { Locked, Available, Active, Complete, Skipped };

struct QuestDefinition {
    std::string id;
    QuestKind kind{QuestKind::Required};
    std::string title;
    std::vector<std::string> requiredFlags;
    std::vector<std::string> completionFlags;
};

struct QuestState {
    QuestStatus status{QuestStatus::Locked};
    std::vector<std::string> flags;
};

class QuestSystem {
public:
    static bool prerequisitesMet(const QuestDefinition& definition, const std::vector<std::string>& storyFlags);
    static bool completionMet(const QuestDefinition& definition, const QuestState& state);
    static void addFlag(QuestState& state, const std::string& flag);
};

struct PartyMemberDefinition {
    std::string characterId;
    std::string fieldRoleId;
    std::string combatRoleId;
};

struct PartyState {
    std::vector<PartyMemberDefinition> members;
    std::string command{"focus"};
};

} // namespace px

