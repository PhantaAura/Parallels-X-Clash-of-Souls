#pragma once
#include <algorithm>
#include <cmath>

namespace px {

struct Vec2 {
    float x{0.0f};
    float z{0.0f};
};

inline float distance(Vec2 a, Vec2 b) {
    const float dx = a.x - b.x;
    const float dz = a.z - b.z;
    return std::sqrt(dx * dx + dz * dz);
}

struct Rect {
    float minX{0.0f};
    float maxX{0.0f};
    float minZ{0.0f};
    float maxZ{0.0f};

    bool contains(Vec2 p) const {
        return p.x >= minX && p.x <= maxX && p.z >= minZ && p.z <= maxZ;
    }
};

inline Vec2 clampToRect(Vec2 p, const Rect& r) {
    p.x = std::clamp(p.x, r.minX, r.maxX);
    p.z = std::clamp(p.z, r.minZ, r.maxZ);
    return p;
}

} // namespace px
