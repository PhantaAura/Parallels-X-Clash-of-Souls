#include "content/faction_registry.hpp"
#include <stdexcept>

namespace px {

FactionRegistry::FactionRegistry() {
    factions_ = {
        {"organization_red", "ORGANIZATION OF THE RED",
         "Surviving cells from the damaged Season 1 Clone incident quietly regroup during the Lost Year. They continue cloning, replicated-combatant, Echo, and unstable-teleportation experiments behind the tournament sabotage and hidden facilities. Rrvvfo breaks this branch badly enough to reasonably believe the Organization scattered and disbanded, while some survivors remain for its later return.",
         {"project_hollow"}},
    };
}

std::string FactionRegistry::canonicalId(const std::string& id) const {
    for (const auto& faction : factions_) {
        if (faction.id == id) return faction.id;
        for (const auto& alias : faction.legacyDevelopmentAliases) if (alias == id) return faction.id;
    }
    return id;
}

const FactionDefinition& FactionRegistry::get(const std::string& id) const {
    const auto canonical = canonicalId(id);
    for (const auto& faction : factions_) if (faction.id == canonical) return faction;
    throw std::out_of_range("Unknown faction: " + id);
}

} // namespace px
