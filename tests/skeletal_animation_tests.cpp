#include "content/character_model_asset.hpp"
#include "content/skeletal_animation.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    assert(argc == 2);
    px::CharacterModelAsset model;
    std::string error;
    if (!model.loadCooked(argv[1], &error)) {
        std::cerr << "Could not load model fixture: " << error << "\n";
        return 1;
    }

    px::SkeletalAnimationPlayer player;
    player.bind(&model);
    assert(player.skinMatrices().size() == 39);
    const bool acceptedMissingClip = player.setState("missing_clip");
    assert(!acceptedMissingClip);
    assert(player.skinMatrices().size() == 39);

    float bindError = 0.0f;
    for (const auto& vertex : model.vertices()) {
        const auto skinned = player.skinPosition(vertex);
        for (std::size_t axis=0;axis<3;++axis)
            bindError = std::max(bindError, std::abs(skinned[axis]-vertex.position[axis]));
    }
    assert(bindError < .001f);


    struct ExpectedClip { const char* name; float duration; };
    constexpr ExpectedClip expected[] = {
        {"idle", .750f}, {"fighting_stance", .420f}, {"run", .328f}, {"dash", .240f},
        {"jump_start", .150f}, {"fall", .220f}, {"land", .130f},
        {"light_1", .270f}, {"light_2", .290f}, {"light_3", .390f},
        {"heavy", .620f}, {"launcher", .550f}, {"air_light", .360f}, {"air_heavy", .520f},
        {"pursuit_light", .320f}, {"pursuit_heavy", .460f}, {"grab", .380f},
        {"block", .240f}, {"perfect_block", .140f}, {"hurt", .210f},
        {"charge", .276f}, {"counter", .216f}, {"breaker", .150f},
        {"fire_blast", .420f}, {"object_swap", .320f}, {"lens_activate", .300f}
    };
    for (const auto& expectedClip : expected) {
        const auto* clip = model.findAnimation(expectedClip.name);
        assert(clip);
        assert(std::abs(clip->duration - expectedClip.duration) < .001f);
        const bool selectedExpectedClip = player.setState(expectedClip.name, true);
        assert(selectedExpectedClip);
        player.seek(expectedClip.duration * .55f);
        for (const auto& vertex : model.vertices()) {
            const auto skinned = player.skinPosition(vertex);
            for (const auto value : skinned) assert(std::isfinite(value) && std::abs(value) < 6.0f);
        }
    }

    // Garments previously inherited forearm/pelvis weights across their torso
    // panels.  Exercise every clip at several phases and reject the resulting
    // triangle explosions or collapses, while allowing deliberate squash and
    // stretch in the stylized poses.  Cook order keeps jacket at submesh 0 and
    // shirt at submesh 5.
    assert(model.submeshes().size() == 7);
    float minimumGarmentEdgeRatio = 1.0f;
    float maximumGarmentEdgeRatio = 1.0f;
    std::string minimumGarmentCase;
    std::string maximumGarmentCase;
    for (const auto& expectedClip : expected) {
        const bool selectedGarmentClip = player.setState(expectedClip.name, true);
        assert(selectedGarmentClip);
        for (const float phase : {.20f, .50f, .80f}) {
            player.seek(expectedClip.duration * phase);
            for (const std::size_t submeshIndex : {std::size_t{0}, std::size_t{5}}) {
                const auto& submesh = model.submeshes()[submeshIndex];
                for (std::size_t offset = 0; offset < submesh.indexCount; offset += 3) {
                    const auto firstIndex = submesh.firstIndex + static_cast<std::uint32_t>(offset);
                    for (const auto edge : {std::array<int,2>{0,1}, std::array<int,2>{1,2}, std::array<int,2>{2,0}}) {
                        const auto aIndex = model.indices()[firstIndex + static_cast<std::uint32_t>(edge[0])];
                        const auto bIndex = model.indices()[firstIndex + static_cast<std::uint32_t>(edge[1])];
                        const auto& aBind = model.vertices()[aIndex].position;
                        const auto& bBind = model.vertices()[bIndex].position;
                        const auto aSkinned = player.skinPosition(model.vertices()[aIndex]);
                        const auto bSkinned = player.skinPosition(model.vertices()[bIndex]);
                        float bindLengthSquared = 0.0f;
                        float skinnedLengthSquared = 0.0f;
                        for (std::size_t axis = 0; axis < 3; ++axis) {
                            const float bindDelta = aBind[axis] - bBind[axis];
                            const float skinnedDelta = aSkinned[axis] - bSkinned[axis];
                            bindLengthSquared += bindDelta * bindDelta;
                            skinnedLengthSquared += skinnedDelta * skinnedDelta;
                        }
                        if (bindLengthSquared <= 1.0e-8f) continue;
                        const float ratio = std::sqrt(skinnedLengthSquared / bindLengthSquared);
                        const std::string detail = std::string(expectedClip.name) + " phase=" +
                            std::to_string(phase) + " submesh=" + std::to_string(submeshIndex) +
                            " edge=" + std::to_string(aIndex) + "-" + std::to_string(bIndex);
                        if (ratio < minimumGarmentEdgeRatio) {
                            minimumGarmentEdgeRatio = ratio;
                            minimumGarmentCase = detail;
                        }
                        if (ratio > maximumGarmentEdgeRatio) {
                            maximumGarmentEdgeRatio = ratio;
                            maximumGarmentCase = detail;
                        }
                    }
                }
            }
        }
    }
    if (minimumGarmentEdgeRatio <= .20f || maximumGarmentEdgeRatio >= 3.0f)
        std::cerr << "Garment edge coherence failed: " << minimumGarmentEdgeRatio
                  << " (" << minimumGarmentCase << ").." << maximumGarmentEdgeRatio
                  << " (" << maximumGarmentCase << ")\n";
    assert(minimumGarmentEdgeRatio > .20f);
    assert(maximumGarmentEdgeRatio < 3.0f);

    const bool selectedIdle = player.setState("idle");
    assert(selectedIdle);
    assert(player.playing());
    player.seek(.5f);
    float maximumMotion = 0.0f;
    float maximumMagnitude = 0.0f;
    for (const auto& vertex : model.vertices()) {
        const auto skinned = player.skinPosition(vertex);
        for (std::size_t axis=0;axis<3;++axis) {
            assert(std::isfinite(skinned[axis]));
            maximumMotion = std::max(maximumMotion, std::abs(skinned[axis]-vertex.position[axis]));
            maximumMagnitude = std::max(maximumMagnitude, std::abs(skinned[axis]));
        }
    }
    assert(maximumMotion > .05f);
    assert(maximumMagnitude < 5.0f);

    player.setPlaybackSpeed(2.0f);
    player.seek(.70f);
    player.update(.05f);
    assert(std::abs(player.time()-.05f) < .0001f);
    assert(player.playing() && !player.finished());

    // Combat backpedaling reuses the authored run without moonwalking: the
    // shared sampler supports signed playback and wraps looping clips backward.
    player.setPlaybackSpeed(-1.0f);
    const bool selectedReverseRun = player.setState("run", true);
    assert(selectedReverseRun);
    player.seek(.05f);
    player.update(.10f);
    assert(std::abs(player.time() - .278f) < .001f);
    assert(player.playing() && !player.finished());

    std::cout << "PASS: 26 Legacy-based Chapter-1 clips, bind fallback, sampling, looping, speed, stable 39-joint deformation, and garment edge coherence "
              << minimumGarmentEdgeRatio << ".." << maximumGarmentEdgeRatio << "\n";
    return 0;
}
