#include "content/character_face.hpp"

#include <algorithm>
#include <cmath>

namespace px {
namespace {

using Color = std::array<float, 4>;
using Point = std::array<float, 3>;
using Mesh = std::vector<CharacterFaceTriangle>;

constexpr float kCenterX = .016f;
constexpr float kEyeY = 1.681f;
// Seat the lightweight expression plane against the modeled face.  The former
// depth was visibly separated at the 3DS route/dialogue camera angle.
constexpr float kFaceZ = 1.748f;
constexpr Color kOutline{.075f, .008f, .006f, 1.0f};
constexpr Color kBrow{.245f, .025f, .014f, 1.0f};
constexpr Color kEyeWhite{.965f, .955f, .905f, 1.0f};
constexpr Color kIris{1.0f, .245f, .018f, 1.0f};
constexpr Color kPupil{.055f, .006f, .004f, 1.0f};
constexpr Color kTeeth{.98f, .975f, .925f, 1.0f};
constexpr Color kTongue{.72f, .055f, .075f, 1.0f};

void triangle(Mesh& mesh, Point a, Point b, Point c, Color color) {
    mesh.push_back({{{a, b, c}}, color});
}

void quad(Mesh& mesh, Point a, Point b, Point c, Point d, Color color) {
    triangle(mesh, a, b, c, color);
    triangle(mesh, a, c, d, color);
}

void rect(Mesh& mesh, float left, float bottom, float right, float top,
          float z, Color color) {
    quad(mesh, {left, bottom, z}, {right, bottom, z},
         {right, top, z}, {left, top, z}, color);
}

void polygon(Mesh& mesh, const std::vector<std::array<float, 2>>& points,
             float z, Color color) {
    if (points.size() < 3) return;
    for (std::size_t index = 1; index + 1 < points.size(); ++index) {
        triangle(mesh,
                 {points[0][0], points[0][1], z},
                 {points[index][0], points[index][1], z},
                 {points[index + 1][0], points[index + 1][1], z}, color);
    }
}

void brow(Mesh& mesh, float centerX, float centerY, float tilt, bool raised = false) {
    const float half = .048f;
    const float lift = raised ? .010f : 0.0f;
    const float leftY = centerY + lift + tilt;
    const float rightY = centerY + lift - tilt;
    quad(mesh,
         {centerX - half, leftY - .004f, kFaceZ + .004f},
         {centerX + half, rightY - .004f, kFaceZ + .004f},
         {centerX + half * .90f, rightY + .007f, kFaceZ + .004f},
         {centerX - half * .88f, leftY + .007f, kFaceZ + .004f}, kBrow);
}

void closedEye(Mesh& mesh, float centerX, float centerY, float tilt) {
    const float half = .044f;
    quad(mesh,
         {centerX - half, centerY + tilt - .0035f, kFaceZ + .005f},
         {centerX + half, centerY - tilt - .0035f, kFaceZ + .005f},
         {centerX + half, centerY - tilt + .0035f, kFaceZ + .005f},
         {centerX - half, centerY + tilt + .0035f, kFaceZ + .005f}, kOutline);
}

void eye(Mesh& mesh, float centerX, float centerY, float openness,
         float tilt, float irisShift = 0.0f) {
    const float half = .050f;
    const std::vector<std::array<float, 2>> outline{
        {centerX - half, centerY + openness * .32f + tilt},
        {centerX + half, centerY + openness * .22f - tilt},
        {centerX + half * .83f, centerY - openness * .62f - tilt},
        {centerX - half * .83f, centerY - openness * .62f + tilt},
    };
    polygon(mesh, outline, kFaceZ + .002f, kOutline);
    const float inset = .006f;
    const std::vector<std::array<float, 2>> white{
        {centerX - half + inset, centerY + openness * .19f + tilt * .74f},
        {centerX + half - inset, centerY + openness * .12f - tilt * .74f},
        {centerX + half * .72f, centerY - openness * .48f - tilt * .72f},
        {centerX - half * .72f, centerY - openness * .48f + tilt * .72f},
    };
    polygon(mesh, white, kFaceZ + .004f, kEyeWhite);

    const float irisX = centerX + irisShift;
    const float irisHalfW = .0125f;
    const float irisHalfH = std::max(.007f, openness * .33f);
    polygon(mesh, {{irisX, centerY + irisHalfH},
                   {irisX + irisHalfW, centerY - .002f},
                   {irisX, centerY - irisHalfH},
                   {irisX - irisHalfW, centerY - .002f}},
            kFaceZ + .006f, kIris);
    const float pupilHalfW = .0045f;
    const float pupilHalfH = std::max(.004f, irisHalfH * .62f);
    polygon(mesh, {{irisX, centerY + pupilHalfH},
                   {irisX + pupilHalfW, centerY - .002f},
                   {irisX, centerY - pupilHalfH},
                   {irisX - pupilHalfW, centerY - .002f}},
            kFaceZ + .008f, kPupil);
}

void neutralMouth(Mesh& mesh, float rise = 0.0f) {
    const float y = 1.592f + rise;
    polygon(mesh, {{kCenterX - .039f, y + .002f},
                   {kCenterX + .039f, y + .003f},
                   {kCenterX + .031f, y - .004f},
                   {kCenterX - .033f, y - .004f}}, kFaceZ + .006f, kOutline);
}

void smirkMouth(Mesh& mesh) {
    polygon(mesh, {{kCenterX - .040f, 1.590f},
                   {kCenterX + .043f, 1.599f},
                   {kCenterX + .034f, 1.591f},
                   {kCenterX - .032f, 1.585f}}, kFaceZ + .006f, kOutline);
}

void gritMouth(Mesh& mesh, bool hurt) {
    const float halfW = hurt ? .052f : .048f;
    const float bottom = hurt ? 1.570f : 1.574f;
    const float top = hurt ? 1.609f : 1.606f;
    polygon(mesh, {{kCenterX - halfW, bottom + .006f},
                   {kCenterX - halfW * .80f, top},
                   {kCenterX + halfW * .80f, top - .002f},
                   {kCenterX + halfW, bottom + .004f},
                   {kCenterX + halfW * .70f, bottom},
                   {kCenterX - halfW * .72f, bottom}}, kFaceZ + .003f, kOutline);
    polygon(mesh, {{kCenterX - halfW * .76f, bottom + .008f},
                   {kCenterX - halfW * .62f, top - .006f},
                   {kCenterX + halfW * .62f, top - .007f},
                   {kCenterX + halfW * .76f, bottom + .007f}}, kFaceZ + .006f, kTeeth);
    rect(mesh, kCenterX - halfW * .64f, (bottom + top) * .5f - .0015f,
         kCenterX + halfW * .64f, (bottom + top) * .5f + .0015f,
         kFaceZ + .008f, kOutline);
}

void shoutMouth(Mesh& mesh) {
    polygon(mesh, {{kCenterX - .044f, 1.602f},
                   {kCenterX - .035f, 1.620f},
                   {kCenterX + .034f, 1.618f},
                   {kCenterX + .046f, 1.600f},
                   {kCenterX + .036f, 1.548f},
                   {kCenterX, 1.536f},
                   {kCenterX - .036f, 1.550f}}, kFaceZ + .003f, kOutline);
    polygon(mesh, {{kCenterX - .032f, 1.602f},
                   {kCenterX - .025f, 1.612f},
                   {kCenterX + .025f, 1.611f},
                   {kCenterX + .033f, 1.600f},
                   {kCenterX + .021f, 1.589f},
                   {kCenterX - .022f, 1.590f}}, kFaceZ + .006f, kTeeth);
    polygon(mesh, {{kCenterX - .026f, 1.555f},
                   {kCenterX, 1.545f},
                   {kCenterX + .027f, 1.556f},
                   {kCenterX + .020f, 1.570f},
                   {kCenterX - .020f, 1.570f}}, kFaceZ + .006f, kTongue);
}

Mesh build(RrvvfoFaceExpression expression) {
    Mesh mesh;
    mesh.reserve(kRrvvfoFaceTriangleBudget);
    const float left = kCenterX - .061f;
    const float right = kCenterX + .061f;

    switch (expression) {
        case RrvvfoFaceExpression::HalfBlink:
            eye(mesh, left, kEyeY, .012f, .002f, .005f);
            eye(mesh, right, kEyeY, .012f, -.002f, -.005f);
            brow(mesh, left, 1.724f, -.002f);
            brow(mesh, right, 1.724f, .002f);
            neutralMouth(mesh);
            break;
        case RrvvfoFaceExpression::Blink:
            closedEye(mesh, left, kEyeY - .003f, .001f);
            closedEye(mesh, right, kEyeY - .003f, -.001f);
            brow(mesh, left, 1.722f, -.002f);
            brow(mesh, right, 1.722f, .002f);
            neutralMouth(mesh);
            break;
        case RrvvfoFaceExpression::Confident:
            eye(mesh, left, kEyeY, .025f, .002f, .006f);
            eye(mesh, right, kEyeY, .021f, -.001f, -.005f);
            brow(mesh, left, 1.727f, -.001f, true);
            brow(mesh, right, 1.724f, .003f);
            smirkMouth(mesh);
            break;
        case RrvvfoFaceExpression::Focused:
            eye(mesh, left, kEyeY, .022f, .004f, .006f);
            eye(mesh, right, kEyeY, .022f, -.004f, -.006f);
            brow(mesh, left, 1.716f, .008f);
            brow(mesh, right, 1.716f, -.008f);
            neutralMouth(mesh, -.002f);
            break;
        case RrvvfoFaceExpression::Grit:
            eye(mesh, left, kEyeY, .020f, .005f, .006f);
            eye(mesh, right, kEyeY, .020f, -.005f, -.006f);
            brow(mesh, left, 1.713f, .009f);
            brow(mesh, right, 1.713f, -.009f);
            gritMouth(mesh, false);
            break;
        case RrvvfoFaceExpression::Hurt:
            closedEye(mesh, left, kEyeY - .005f, -.004f);
            eye(mesh, right, kEyeY - .004f, .016f, .006f, -.008f);
            brow(mesh, left, 1.715f, .007f);
            brow(mesh, right, 1.708f, .010f);
            gritMouth(mesh, true);
            break;
        case RrvvfoFaceExpression::Shout:
            eye(mesh, left, kEyeY + .002f, .032f, .004f, .006f);
            eye(mesh, right, kEyeY + .002f, .032f, -.004f, -.006f);
            brow(mesh, left, 1.716f, .010f);
            brow(mesh, right, 1.716f, -.010f);
            shoutMouth(mesh);
            break;
        case RrvvfoFaceExpression::Neutral:
        default:
            eye(mesh, left, kEyeY, .028f, .0015f, .005f);
            eye(mesh, right, kEyeY, .028f, -.0015f, -.005f);
            brow(mesh, left, 1.724f, -.003f);
            brow(mesh, right, 1.724f, .003f);
            neutralMouth(mesh);
            break;
    }
    return mesh;
}

bool oneOf(const std::string& value, std::initializer_list<const char*> names) {
    return std::any_of(names.begin(), names.end(), [&](const char* name) { return value == name; });
}

} // namespace

const char* rrvvfoFaceExpressionName(RrvvfoFaceExpression expression) {
    switch (expression) {
        case RrvvfoFaceExpression::HalfBlink: return "half-blink";
        case RrvvfoFaceExpression::Blink: return "blink";
        case RrvvfoFaceExpression::Confident: return "confident";
        case RrvvfoFaceExpression::Focused: return "focused";
        case RrvvfoFaceExpression::Grit: return "grit";
        case RrvvfoFaceExpression::Hurt: return "hurt";
        case RrvvfoFaceExpression::Shout: return "shout";
        case RrvvfoFaceExpression::Neutral:
        default: return "neutral";
    }
}

RrvvfoFaceExpression resolveRrvvfoFaceExpression(
    const std::string& animationState, float clipSeconds, float faceSeconds,
    const std::string& dialogueExpression, bool rrvvfoSpeaking) {
    if (rrvvfoSpeaking) {
        if (dialogueExpression == "annoyed" || dialogueExpression == "angry")
            return RrvvfoFaceExpression::Focused;
        if (dialogueExpression == "realizing" || dialogueExpression == "amused" ||
            dialogueExpression == "confident")
            return RrvvfoFaceExpression::Confident;
        if (dialogueExpression == "hurt") return RrvvfoFaceExpression::Hurt;
        if (dialogueExpression == "shout" || dialogueExpression == "shouting")
            return RrvvfoFaceExpression::Shout;
        return RrvvfoFaceExpression::Neutral;
    }
    if (animationState == "hurt") return RrvvfoFaceExpression::Hurt;
    if (oneOf(animationState, {"perfect_block", "counter", "lens_activate"}))
        return RrvvfoFaceExpression::Confident;
    if (oneOf(animationState, {"run", "dash", "jump_start", "fall", "land",
                               "fighting_stance", "block", "object_swap"}))
        return RrvvfoFaceExpression::Focused;
    if (animationState == "charge") return clipSeconds > .16f
        ? RrvvfoFaceExpression::Grit : RrvvfoFaceExpression::Focused;
    if (oneOf(animationState, {"heavy", "launcher", "air_heavy", "pursuit_heavy",
                               "breaker", "fire_blast", "grab"}))
        return clipSeconds >= .12f && clipSeconds <= .44f
            ? RrvvfoFaceExpression::Shout : RrvvfoFaceExpression::Grit;
    if (oneOf(animationState, {"light_1", "light_2", "light_3", "air_light",
                               "pursuit_light"}))
        return clipSeconds >= .07f && clipSeconds <= .22f
            ? RrvvfoFaceExpression::Shout : RrvvfoFaceExpression::Focused;

    if (animationState == "idle") {
        float phase = std::fmod(std::max(0.0f, faceSeconds), 3.6f);
        if (phase >= 3.30f && phase < 3.38f) return RrvvfoFaceExpression::Blink;
        if ((phase >= 3.25f && phase < 3.30f) || (phase >= 3.38f && phase < 3.43f))
            return RrvvfoFaceExpression::HalfBlink;
    }
    return RrvvfoFaceExpression::Neutral;
}

const std::vector<CharacterFaceTriangle>& rrvvfoFaceTriangles(RrvvfoFaceExpression expression) {
    static const Mesh neutral = build(RrvvfoFaceExpression::Neutral);
    static const Mesh halfBlink = build(RrvvfoFaceExpression::HalfBlink);
    static const Mesh blink = build(RrvvfoFaceExpression::Blink);
    static const Mesh confident = build(RrvvfoFaceExpression::Confident);
    static const Mesh focused = build(RrvvfoFaceExpression::Focused);
    static const Mesh grit = build(RrvvfoFaceExpression::Grit);
    static const Mesh hurt = build(RrvvfoFaceExpression::Hurt);
    static const Mesh shout = build(RrvvfoFaceExpression::Shout);
    switch (expression) {
        case RrvvfoFaceExpression::HalfBlink: return halfBlink;
        case RrvvfoFaceExpression::Blink: return blink;
        case RrvvfoFaceExpression::Confident: return confident;
        case RrvvfoFaceExpression::Focused: return focused;
        case RrvvfoFaceExpression::Grit: return grit;
        case RrvvfoFaceExpression::Hurt: return hurt;
        case RrvvfoFaceExpression::Shout: return shout;
        case RrvvfoFaceExpression::Neutral:
        default: return neutral;
    }
}

CharacterModelVertex rrvvfoFaceVertex(const std::array<float, 3>& position) {
    CharacterModelVertex vertex;
    vertex.position = position;
    vertex.normal = {0.0f, 0.0f, 1.0f};
    vertex.joints = {kRrvvfoFaceJointIndex, 0, 0, 0};
    vertex.weights = {1.0f, 0.0f, 0.0f, 0.0f};
    return vertex;
}

} // namespace px
