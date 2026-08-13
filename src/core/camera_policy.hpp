#pragma once

#include "content/world_presentation_registry.hpp"
#include "core/runtime.hpp"

namespace px {

// Final platform-neutral camera values. Every renderer consumes this policy so
// authored cutscene shots keep the same focus and direction on desktop and 3DS.
struct ResolvedCamera {
    Vec2 focus{};
    float yawDegrees{38.0f};
    float distance{980.0f};
    float height{430.0f};
    float targetHeight{44.0f};
    float fovDegrees{45.0f};
    float nearPlane{8.0f};
    float farPlane{4200.0f};
    bool cinematic{false};
};

ResolvedCamera resolveRuntimeCamera(const WorldPresentationDefinition& stage,
                                    const RuntimeView& view,
                                    float gameplayDistanceScale = 1.0f,
                                    float gameplayFovOffset = 0.0f);

} // namespace px
