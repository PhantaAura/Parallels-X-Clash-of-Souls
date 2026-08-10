#include "core/world.hpp"
#include <algorithm>

namespace px {

static bool disabled(const std::vector<std::string>& ids, const std::string& id) {
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

Vec2 WorldCollision::move(const MapDefinition& map, Vec2 from, Vec2 requested, const std::vector<std::string>& disabledBlockers) {
    Vec2 result = clampToRect(requested, map.bounds);
    for (const auto& blocker : map.blockers) {
        if (!blocker.enabledByDefault || disabled(disabledBlockers, blocker.id)) continue;
        if (!blocker.bounds.contains(result)) continue;

        // Axis fallback makes authored hub collision predictable and cheap enough for old hardware.
        Vec2 xOnly{result.x, from.z};
        Vec2 zOnly{from.x, result.z};
        if (!blocker.bounds.contains(xOnly)) result = xOnly;
        else if (!blocker.bounds.contains(zOnly)) result = zOnly;
        else result = from;
    }
    return result;
}

} // namespace px
