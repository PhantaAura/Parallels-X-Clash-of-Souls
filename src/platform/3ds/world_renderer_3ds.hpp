#pragma once

#include <3ds.h>
#include <citro3d.h>

#include "content/character_face.hpp"
#include "content/character_model_asset.hpp"
#include "content/character_presentation_registry.hpp"
#include "content/skeletal_animation.hpp"
#include "content/world_presentation_registry.hpp"
#include "core/runtime.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace px::platform3ds {

enum class CharacterPreview3ds {
    FullBody,
    DialogueBust,
};

// Native perspective renderer for the shared authored Chapter 1 world. The
// Old 3DS selects a cheaper detail tier, but camera, geography, actors,
// animation, blocker visibility and gameplay feedback all come from the same
// definitions and RuntimeView used by the desktop versions.
class WorldRenderer3ds {
public:
    WorldRenderer3ds();
    ~WorldRenderer3ds();

    WorldRenderer3ds(const WorldRenderer3ds&) = delete;
    WorldRenderer3ds& operator=(const WorldRenderer3ds&) = delete;

    bool init(std::string* error = nullptr);
    void shutdown();

    void render(const WorldPresentationDefinition& stage,
                const RuntimeView& view,
                const std::vector<std::string>& disabledBlockers,
                const CharacterPresentationRegistry& characters,
                const CharacterModelAsset* playerModel,
                const SkeletalAnimationPlayer* playerAnimation,
                RrvvfoFaceExpression faceExpression);

    // Draws the exact same cooked/skinned Rrvvfo mesh used in gameplay. The
    // dialogue bust is a live close-up, so the lightweight eyes/mouth remain
    // visibly tied to the current expression instead of becoming a fake icon.
    void renderPlayerPreview(C3D_RenderTarget* target,
                             const CharacterModelAsset& playerModel,
                             const SkeletalAnimationPlayer& playerAnimation,
                             RrvvfoFaceExpression faceExpression,
                             const CharacterPresentationDefinition& binding,
                             CharacterPreview3ds kind);

    std::size_t submittedVertexCount() const { return submittedVertexCount_; }
    bool ready() const { return ready_; }

private:
    struct Vertex;
    struct Point3;

    void rebuildStaticWorld(const WorldPresentationDefinition& stage,
                            const std::vector<std::string>& disabledBlockers);
    bool blockerDisabled(const std::vector<std::string>& disabledBlockers,
                         const std::string& id) const;
    void configureCamera(const WorldPresentationDefinition& stage,
                         const RuntimeView& view);
    void appendRuntimeActors(const WorldPresentationDefinition& stage,
                             const RuntimeView& view,
                             const CharacterPresentationRegistry& characters);
    void appendRuntimeMarkers(const RuntimeView& view);
    void appendPlayer(const RuntimeView& view,
                      const CharacterPresentationDefinition& binding,
                      const CharacterModelAsset* model,
                      const SkeletalAnimationPlayer* animation,
                      RrvvfoFaceExpression faceExpression);
    void appendOpponent(const RuntimeView& view,
                        const CharacterPresentationRegistry& characters);

    void appendPrimitive(std::vector<Vertex>& destination,
                         const WorldPrimitiveDefinition& primitive);
    Point3 transformPoint(Point3 local, const PresentationTransform& transform) const;
    void appendTriangle(std::vector<Vertex>& destination,
                        Point3 a, Point3 b, Point3 c,
                        PresentationColor color);
    void appendQuad(std::vector<Vertex>& destination,
                    Point3 a, Point3 b, Point3 c, Point3 d,
                    PresentationColor color);
    void appendBox(std::vector<Vertex>& destination,
                   const PresentationTransform& transform,
                   PresentationColor color);
    void appendCylinder(std::vector<Vertex>& destination,
                        const PresentationTransform& transform,
                        PresentationColor color,
                        int segments = 8);
    void appendCone(std::vector<Vertex>& destination,
                    const PresentationTransform& transform,
                    PresentationColor color,
                    int segments = 8);
    void appendCharacterFallback(std::vector<Vertex>& destination,
                                 const CharacterPresentationDefinition& binding,
                                 Vec2 position,
                                 float worldY,
                                 float yawDegrees,
                                 bool focused);
    bool appendCharacterModel(std::vector<Vertex>& destination,
                              const CharacterModelAsset* asset,
                              const SkeletalAnimationPlayer* animation,
                              RrvvfoFaceExpression faceExpression,
                              const CharacterPresentationDefinition& binding,
                              Vec2 position,
                              float worldY,
                              float yawDegrees,
                              bool focused);
    void submitVertices();

    static std::string cacheKey(const WorldPresentationDefinition& stage,
                                const std::vector<std::string>& disabledBlockers);

    bool ready_{false};
    DVLB_s* shaderBinary_{nullptr};
    shaderProgram_s shaderProgram_{};
    int projectionUniform_{-1};
    int viewUniform_{-1};
    C3D_Mtx projection_{};
    C3D_Mtx view_{};
    Vertex* gpuVertices_{nullptr};
    std::vector<Vertex> staticWorld_;
    std::vector<Vertex> frameVertices_;
    std::vector<Point3> skinnedPositions_;
    std::string staticWorldKey_;
    std::size_t submittedVertexCount_{0};
};

} // namespace px::platform3ds
