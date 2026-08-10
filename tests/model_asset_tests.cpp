#include "content/character_model_asset.hpp"
#include "content/character_face.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>

int main(int argc, char** argv) {
    assert(argc == 2);
    px::CharacterModelAsset model;
    std::string error;
    assert(model.loadCooked(argv[1], &error));
    assert(error.empty());
    assert(model.valid() && model.hasSkinning());
    assert(model.materials().size() == 6);
    assert(model.joints().size() == 39);
    // Artist-authored DEV exports can change topology slightly while preserving the
    // same gameplay rig. Keep this test focused on portable model integrity instead
    // of pinning a brittle historical vertex count.
    assert(model.vertices().size() > 2400 && model.vertices().size() < 2700);
    assert(model.indices().size() % 3 == 0);
    assert(model.indices().size() / 3 > 3000 && model.indices().size() / 3 < 3400);
    assert(model.submeshes().size() == 7);
    assert(model.animations().size() == 26);
    const auto* idle = model.findAnimation("idle");
    assert(idle);
    assert(idle->looping);
    assert(idle->duration == .75f);
    assert(idle->tracks.size() == 35);
    const std::array<float, 7> expectedIdleTimes{0.0f, .125f, .25f, .375f, .5f, .625f, .75f};
    for (const auto& track : idle->tracks) {
        assert(track.rotations.size() == expectedIdleTimes.size());
        for (std::size_t key = 0; key < expectedIdleTimes.size(); ++key)
            assert(track.rotations[key].time == expectedIdleTimes[key]);
    }
    assert(model.findAnimation("run"));
    assert(model.findAnimation("dash"));
    assert(model.findAnimation("charge"));
    assert(model.findAnimation("counter"));
    assert(model.findAnimation("breaker"));
    assert(model.findAnimation("object_swap"));
    assert(model.findAnimation("lens_activate"));
    assert(model.joints().front().name == "spine");
    assert(model.joints().front().parentIndex == -1);
    assert(model.height() > 2.0f && model.height() < 2.3f);

    const auto materialNamed = [&](const std::string& name) {
        return std::find_if(model.materials().begin(), model.materials().end(),
                            [&](const auto& material) { return material.name == name; });
    };
    for (const auto* name : {"Red", "Maroon", "Skin", "Hair", "Head", "Material.001"})
        assert(materialNamed(name) != model.materials().end());
    assert(materialNamed("Red")->color[0] > materialNamed("Red")->color[1] * 8.0f);
    assert(materialNamed("Skin")->color[0] > materialNamed("Skin")->color[2]);

    std::size_t weightedVertices = 0;
    for (const auto& vertex : model.vertices()) {
        const float total = vertex.weights[0] + vertex.weights[1] + vertex.weights[2] + vertex.weights[3];
        if (total > 0.0f) {
            ++weightedVertices;
            assert(std::abs(total - 1.0f) < 0.002f);
        }
    }
    // Every cooked gameplay vertex must be skinned. This specifically catches
    // accidental unskinned clothing/shoes while the unskinned Training Suit
    // reference remains excluded by the cooker.
    assert(weightedVertices == model.vertices().size());

    px::CharacterModelRepository repository;
    assert(repository.load("rrvvfo", argv[1], &error));
    assert(repository.has("rrvvfo"));
    assert(!repository.has("sage"));
    assert(repository.find("rrvvfo")->joints().size() == 39);

    // The cel face reuses spine.006 and stays outside the cooked GLB, so the
    // portable model/skeleton counts above remain unchanged on Old 3DS.
    assert(model.joints()[px::kRrvvfoFaceJointIndex].name == "spine.006");
    float modeledHeadFront = -std::numeric_limits<float>::infinity();
    for (const auto& vertex : model.vertices()) {
        for (std::size_t influence = 0; influence < vertex.joints.size(); ++influence) {
            if (vertex.joints[influence] == px::kRrvvfoFaceJointIndex &&
                vertex.weights[influence] > .5f) {
                modeledHeadFront = std::max(modeledHeadFront, vertex.position[2]);
            }
        }
    }
    assert(std::isfinite(modeledHeadFront));
    const std::array<px::RrvvfoFaceExpression, 8> expressions{
        px::RrvvfoFaceExpression::Neutral, px::RrvvfoFaceExpression::HalfBlink,
        px::RrvvfoFaceExpression::Blink, px::RrvvfoFaceExpression::Confident,
        px::RrvvfoFaceExpression::Focused, px::RrvvfoFaceExpression::Grit,
        px::RrvvfoFaceExpression::Hurt, px::RrvvfoFaceExpression::Shout};
    for (const auto expression : expressions) {
        const auto& face = px::rrvvfoFaceTriangles(expression);
        assert(!face.empty() && face.size() <= px::kRrvvfoFaceTriangleBudget);
        for (const auto& triangle : face) {
            for (const auto& point : triangle.positions) {
                for (const float value : point) assert(std::isfinite(value));
                // The expression surface may clear the skin by a few
                // millimeters for z-fighting, but must not read as a floating
                // mask in the 3DS route and dialogue cameras.
                assert(point[2] <= modeledHeadFront + .006f);
                assert(point[2] >= modeledHeadFront - .020f);
            }
            for (const float value : triangle.color)
                assert(std::isfinite(value) && value >= 0.0f && value <= 1.0f);
            const auto attached = px::rrvvfoFaceVertex(triangle.positions[0]);
            assert(attached.joints[0] == px::kRrvvfoFaceJointIndex);
            assert(attached.weights[0] == 1.0f);
        }
    }
    assert(px::resolveRrvvfoFaceExpression("idle", 0.0f, 0.0f) == px::RrvvfoFaceExpression::Neutral);
    assert(px::resolveRrvvfoFaceExpression("idle", 0.0f, 3.27f) == px::RrvvfoFaceExpression::HalfBlink);
    assert(px::resolveRrvvfoFaceExpression("idle", 0.0f, 3.33f) == px::RrvvfoFaceExpression::Blink);
    assert(px::resolveRrvvfoFaceExpression("heavy", .20f, 0.0f) == px::RrvvfoFaceExpression::Shout);
    assert(px::resolveRrvvfoFaceExpression("heavy", .55f, 0.0f) == px::RrvvfoFaceExpression::Grit);
    assert(px::resolveRrvvfoFaceExpression("hurt", 0.0f, 0.0f) == px::RrvvfoFaceExpression::Hurt);
    assert(px::resolveRrvvfoFaceExpression("idle", 0.0f, 0.0f, "annoyed", true) ==
           px::RrvvfoFaceExpression::Focused);
    assert(px::resolveRrvvfoFaceExpression("idle", 0.0f, 0.0f, "realizing", true) ==
           px::RrvvfoFaceExpression::Confident);

    std::cout << "PASS: unchanged Rrvvfo model/39-joint skin, lightweight cel face, and 26 Chapter-1 clips loaded\n";
    return 0;
}
