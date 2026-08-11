#pragma once
#include "core/types.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace px {

enum class AbilityState : std::uint8_t {
    Ready,
    Locked,
    StoryRestricted
};

struct AbilitySlotDefinition {
    // displaySlot is what the player sees and presses right now.
    // canonicalSlot is the long-term Rrvvfo identity used once the full five-technique bar exists.
    int displaySlot{0};
    int canonicalSlot{0};
    std::string id;
    std::string label;
    std::string icon;
    float energyCost{0.0f};
    float hpCost{0.0f};
    AbilityState state{AbilityState::Locked};
    std::string stateLabel;
    std::string note;
};

using AbilityHotbarLayout = std::vector<AbilitySlotDefinition>;

class AbilityHotbarCatalog {
public:
    // Early Story: Shots of Agony does not exist, so there is no fake/??? slot.
    // Existing techniques compress left and display as a three-slot bar.
    static const AbilityHotbarLayout& rrvvfoChapter1();

    // Later Story: when Shots of Agony is actually invented it is inserted at display slot 2.
    // Object Swap / Lens / Solar move to display slots 3 / 4 / 5 without changing ability ids.
    static const AbilityHotbarLayout& rrvvfoWithShotsOfAgony();

    static const AbilitySlotDefinition* abilityAtDisplaySlot(const AbilityHotbarLayout& layout, int displaySlot);
    static const AbilitySlotDefinition* abilityForAction(const AbilityHotbarLayout& layout, Action action);
};

const char* abilityStateName(AbilityState state);
int displaySlotForAction(Action action);

} // namespace px
