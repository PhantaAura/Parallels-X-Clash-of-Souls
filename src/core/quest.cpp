#include "core/quest.hpp"
#include <algorithm>

namespace px {

namespace {
bool contains(const std::vector<std::string>& values, const std::string& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}
}

bool QuestSystem::prerequisitesMet(const QuestDefinition& definition, const std::vector<std::string>& storyFlags) {
    return std::all_of(definition.requiredFlags.begin(), definition.requiredFlags.end(),
        [&](const std::string& flag){ return contains(storyFlags, flag); });
}

bool QuestSystem::completionMet(const QuestDefinition& definition, const QuestState& state) {
    return std::all_of(definition.completionFlags.begin(), definition.completionFlags.end(),
        [&](const std::string& flag){ return contains(state.flags, flag); });
}

void QuestSystem::addFlag(QuestState& state, const std::string& flag) {
    if (!contains(state.flags, flag)) state.flags.push_back(flag);
}

} // namespace px

