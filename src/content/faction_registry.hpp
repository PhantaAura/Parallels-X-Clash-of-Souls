#pragma once
#include <string>
#include <vector>

namespace px {

struct FactionDefinition {
    std::string id;
    std::string displayName;
    std::string lostYearRole;
    std::vector<std::string> legacyDevelopmentAliases;
};

class FactionRegistry {
public:
    FactionRegistry();
    const FactionDefinition& get(const std::string& id) const;
    std::string canonicalId(const std::string& id) const;

private:
    std::vector<FactionDefinition> factions_;
};

} // namespace px
