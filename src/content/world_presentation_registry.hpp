#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace px {

enum class PresentationPrimitiveKind : std::uint8_t {
    Box,
    Cylinder,
    Cone
};

enum class PresentationDetailTier : std::uint8_t {
    Essential,
    Full
};

struct PresentationColor {
    float r{1.0f};
    float g{1.0f};
    float b{1.0f};
    float a{1.0f};
};

struct PresentationTransform {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};
    float scaleX{1.0f};
    float scaleY{1.0f};
    float scaleZ{1.0f};
    float yawDegrees{0.0f};
};

struct WorldPrimitiveDefinition {
    std::string id;
    PresentationPrimitiveKind kind{PresentationPrimitiveKind::Box};
    PresentationTransform transform{};
    PresentationColor color{};
    PresentationDetailTier detail{PresentationDetailTier::Essential};
    // When set, the renderer hides this authored obstacle after the shared
    // gameplay blocker is disabled. The content remains platform-neutral.
    std::string visibleWhileBlockerEnabled;
};

struct AmbientActorDefinition {
    std::string id;
    std::string characterId;
    PresentationTransform transform{};
    PresentationDetailTier detail{PresentationDetailTier::Full};
};

struct PerspectiveCameraDefinition {
    float yawDegrees{38.0f};
    float fovDegrees{45.0f};
    float baseDistance{980.0f};
    float minDistance{930.0f};
    float maxDistance{1160.0f};
    float height{430.0f};
    float targetHeight{44.0f};
    float nearPlane{8.0f};
    float farPlane{4200.0f};
    float focusCenterX{0.0f};
    float focusCenterZ{0.0f};
    float focusClampX{1380.0f};
    float focusClampZ{820.0f};
    bool followPlayer{true};
};

struct WorldPresentationDefinition {
    std::string id;
    std::string displayName;
    PerspectiveCameraDefinition camera{};
    PresentationColor clearColor{};
    PresentationColor fogColor{};
    float fogNear{1000.0f};
    float fogFar{2900.0f};
    std::vector<WorldPrimitiveDefinition> primitives;
    std::vector<AmbientActorDefinition> ambientActors;
};

// Reusable authored world presentation. Renderers choose their quality tier, but
// the camera, landmark transforms and scenery layout do not belong to macOS.
class WorldPresentationRegistry {
public:
    WorldPresentationRegistry();
    const WorldPresentationDefinition& get(const std::string& id) const;
    bool has(const std::string& id) const;
    std::vector<std::string> ids() const;

private:
    std::unordered_map<std::string, WorldPresentationDefinition> stages_;
};

} // namespace px
