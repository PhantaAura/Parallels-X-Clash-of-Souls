#pragma once

#include "content/character_model_asset.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace px {

// Rrvvfo's face is a deliberately small cel layer attached to the existing
// final head joint. It changes no source-model vertices, weights, materials or
// skeleton data and therefore has no additional skin-matrix cost.
enum class RrvvfoFaceExpression : std::uint8_t {
    Neutral,
    HalfBlink,
    Blink,
    Confident,
    Focused,
    Grit,
    Hurt,
    Shout,
};

struct CharacterFaceTriangle {
    std::array<std::array<float, 3>, 3> positions{};
    std::array<float, 4> color{1.0f, 1.0f, 1.0f, 1.0f};
};

constexpr std::uint16_t kRrvvfoFaceJointIndex = 28; // spine.006 / head
constexpr std::size_t kRrvvfoFaceTriangleBudget = 48;

const char* rrvvfoFaceExpressionName(RrvvfoFaceExpression expression);

// One shared decision table keeps dialogue, idles and combat expressions the
// same on Mac, Linux and 3DS. faceSeconds supplies the slow natural blink clock;
// clipSeconds controls readable startup/active/recovery attack faces.
RrvvfoFaceExpression resolveRrvvfoFaceExpression(
    const std::string& animationState,
    float clipSeconds,
    float faceSeconds,
    const std::string& dialogueExpression = {},
    bool rrvvfoSpeaking = false);

// The returned meshes are immutable, prebuilt once and allocation-free during
// normal frames. Only the selected expression is submitted by the renderer.
const std::vector<CharacterFaceTriangle>& rrvvfoFaceTriangles(
    RrvvfoFaceExpression expression);

// Converts one bind-space face point into a vertex fully weighted to the
// existing head joint so it follows all 26 current body clips automatically.
CharacterModelVertex rrvvfoFaceVertex(const std::array<float, 3>& position);

} // namespace px
