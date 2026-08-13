#include "core/camera_policy.hpp"

#include <algorithm>
#include <cmath>

namespace px {
namespace {

float mix(float from, float to, float amount) {
    return from + (to - from) * amount;
}

} // namespace

ResolvedCamera resolveRuntimeCamera(const WorldPresentationDefinition& stage,
                                    const RuntimeView& view,
                                    float gameplayDistanceScale,
                                    float gameplayFovOffset) {
    float requestedX = stage.camera.focusCenterX;
    float requestedZ = stage.camera.focusCenterZ;
    if (stage.camera.followPlayer) {
        requestedX = view.playerPosition.x;
        requestedZ = view.playerPosition.z;
        if (view.opponentVisible) {
            requestedX = (view.playerPosition.x + view.opponentPosition.x) * .5f;
            requestedZ = (view.playerPosition.z + view.opponentPosition.z) * .5f;
        }
    }

    const float shakeSign = std::sin((view.playerPosition.x + view.playerPosition.z) * .017f) >= 0.0f ? 1.0f : -1.0f;
    const float shake = view.reducedCameraShake ? 0.0f : std::min(8.0f, view.cameraImpulse * 1.15f) * shakeSign;
    const Vec2 gameplayFocus{
        std::clamp(requestedX + shake,
                   stage.camera.focusCenterX - stage.camera.focusClampX,
                   stage.camera.focusCenterX + stage.camera.focusClampX),
        std::clamp(requestedZ - shake * .45f,
                   stage.camera.focusCenterZ - stage.camera.focusClampZ,
                   stage.camera.focusCenterZ + stage.camera.focusClampZ)
    };

    const float blend = std::clamp(view.cinematicCameraBlend, 0.0f, 1.0f);
    ResolvedCamera result;
    result.focus = {mix(gameplayFocus.x, view.cinematicCameraFocus.x, blend),
                    mix(gameplayFocus.z, view.cinematicCameraFocus.z, blend)};
    result.yawDegrees = mix(stage.camera.yawDegrees + view.cameraYawOffsetDegrees,
                            view.cinematicCameraYawDegrees, blend);
    result.distance = mix(stage.camera.baseDistance * gameplayDistanceScale,
                          view.cinematicCameraDistance, blend);
    result.height = mix(stage.camera.height + view.cameraHeightOffset, view.cinematicCameraHeight, blend);
    result.targetHeight = stage.camera.targetHeight;
    result.fovDegrees = std::clamp(mix(stage.camera.fovDegrees + gameplayFovOffset,
                                       view.cinematicCameraFovDegrees, blend), 26.0f, 70.0f);
    result.nearPlane = stage.camera.nearPlane;
    result.farPlane = stage.camera.farPlane;
    result.cinematic = blend > .001f;
    if (view.cinematicCameraOcclusionRescue && !result.cinematic) {
        // Shared close-in rescue for dense hub geometry. It changes no authored
        // staging and is intentionally cheap enough for the Old 3DS renderer.
        result.distance *= .88f;
        result.height *= .94f;
    }
    return result;
}

} // namespace px
