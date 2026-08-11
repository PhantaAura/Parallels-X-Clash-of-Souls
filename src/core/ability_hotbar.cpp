#include "core/ability_hotbar.hpp"

namespace px {

const AbilityHotbarLayout& AbilityHotbarCatalog::rrvvfoChapter1() {
    // Shots of Agony and Solar Weave are both absent. At this story point only
    // techniques Rrvvfo can actually use are allowed to occupy visible UI.
    static const AbilityHotbarLayout layout{
        {1, 1, "fireBlast", "FIRE BLAST", "FIRE", 22.0f, 0.0f,
            AbilityState::Ready, "READY", "Legacy core ranged fire technique."},
        {2, 3, "objectSwap", "OBJECT SWAP", "SWAP", 20.0f, 0.0f,
            AbilityState::Ready, "READY", "Traversal and combat position exchange."},
        {3, 4, "lensOfTruth", "LENS OF TRUTH", "LENS", 60.0f, 25.0f,
            AbilityState::Ready, "EARLY / UNSTABLE", "Costs Energy and HP; Chapter 1 framing stays unstable."}
    };
    return layout;
}

const AbilityHotbarLayout& AbilityHotbarCatalog::rrvvfoWithShotsOfAgony() {
    static const AbilityHotbarLayout layout{
        {1, 1, "fireBlast", "FIRE BLAST", "FIRE", 22.0f, 0.0f,
            AbilityState::Ready, "READY", "Legacy core ranged fire technique."},
        {2, 2, "shotsOfAgony", "SHOTS OF AGONY", "SHOTS", 42.0f, 0.0f,
            AbilityState::Ready, "READY", "Only appears after Rrvvfo actually invents the technique."},
        {3, 3, "objectSwap", "OBJECT SWAP", "SWAP", 20.0f, 0.0f,
            AbilityState::Ready, "READY", "Moves to display slot 3 after Shots of Agony is inserted."},
        {4, 4, "lensOfTruth", "LENS OF TRUTH", "LENS", 60.0f, 25.0f,
            AbilityState::Ready, "READY", "Later hotbar position."},
        {5, 5, "solarWeave", "SOLAR WEAVE", "SOLAR", 90.0f, 0.0f,
            AbilityState::Ready, "READY", "Later hotbar position."}
    };
    return layout;
}

const AbilitySlotDefinition* AbilityHotbarCatalog::abilityAtDisplaySlot(const AbilityHotbarLayout& layout, int displaySlot) {
    for (const auto& ability : layout) {
        if (ability.displaySlot == displaySlot) return &ability;
    }
    return nullptr;
}

const AbilitySlotDefinition* AbilityHotbarCatalog::abilityForAction(const AbilityHotbarLayout& layout, Action action) {
    return abilityAtDisplaySlot(layout, displaySlotForAction(action));
}

int displaySlotForAction(Action action) {
    switch (action) {
        case Action::Ability1: return 1;
        case Action::Ability2: return 2;
        case Action::Ability3: return 3;
        case Action::Ability4: return 4;
        case Action::Ability5: return 5;
        default: return 0;
    }
}

const char* abilityStateName(AbilityState state) {
    switch (state) {
        case AbilityState::Ready: return "READY";
        case AbilityState::Locked: return "LOCKED";
        case AbilityState::StoryRestricted: return "STORY RESTRICTED";
    }
    return "UNKNOWN";
}

} // namespace px
