#pragma once
#include "core/math.hpp"
#include <string>
#include <vector>

namespace px {

struct MapZone {
    std::string id;
    std::string name;
    Vec2 center;
    std::string gameplayIdentity;
};

struct MapLink {
    std::string from;
    std::string to;
    std::string routeTag;
};

struct WorldBlocker {
    std::string id;
    Rect bounds;
    bool enabledByDefault{true};
};

struct Landmark {
    std::string id;
    std::string name;
    Vec2 position;
};

struct MapDefinition {
    std::string id;
    std::string name;
    Rect bounds;
    Vec2 playerStart;
    std::vector<MapZone> zones;
    std::vector<MapLink> links;
    std::vector<WorldBlocker> blockers;
    std::vector<Landmark> landmarks;
};

class WorldCollision {
public:
    static Vec2 move(const MapDefinition& map, Vec2 from, Vec2 requested, const std::vector<std::string>& disabledBlockers = {});
};

} // namespace px
