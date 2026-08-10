#pragma once
#include "core/world.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace px {

class MapRegistry {
public:
    MapRegistry();
    const MapDefinition& get(const std::string& id) const;
    std::vector<std::string> ids() const;

private:
    std::unordered_map<std::string, MapDefinition> maps_;
};

} // namespace px
