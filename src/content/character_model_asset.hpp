#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace px {

struct CharacterModelMaterial {
    std::string name;
    std::array<float, 4> color{1.0f, 1.0f, 1.0f, 1.0f};
    bool unlit{false};
    bool doubleSided{false};
};

struct CharacterModelJoint {
    std::string name;
    std::int32_t parentIndex{-1};
    std::array<float, 16> localBindMatrix{};
    std::array<float, 16> inverseBindMatrix{};
};

struct CharacterModelVertex {
    std::array<float, 3> position{};
    std::array<float, 3> normal{};
    std::array<std::uint16_t, 4> joints{};
    std::array<float, 4> weights{};
};

struct CharacterModelSubmesh {
    std::uint32_t firstIndex{0};
    std::uint32_t indexCount{0};
    std::uint32_t materialIndex{0};
};

enum class CharacterAnimationInterpolation : std::uint8_t {
    Linear = 0,
    Step = 1,
};

struct CharacterAnimationVectorKey {
    float time{0.0f};
    std::array<float, 3> value{};
};

struct CharacterAnimationQuaternionKey {
    float time{0.0f};
    std::array<float, 4> value{0.0f, 0.0f, 0.0f, 1.0f};
};

struct CharacterAnimationTrack {
    std::uint16_t jointIndex{0};
    CharacterAnimationInterpolation translationInterpolation{CharacterAnimationInterpolation::Linear};
    CharacterAnimationInterpolation rotationInterpolation{CharacterAnimationInterpolation::Linear};
    CharacterAnimationInterpolation scaleInterpolation{CharacterAnimationInterpolation::Linear};
    std::vector<CharacterAnimationVectorKey> translations;
    std::vector<CharacterAnimationQuaternionKey> rotations;
    std::vector<CharacterAnimationVectorKey> scales;
};

struct CharacterAnimationClip {
    std::string name;
    float duration{0.0f};
    bool looping{false};
    std::vector<CharacterAnimationTrack> tracks;
};

// Portable, renderer-independent character data. Phase 1 renders its authored
// bind pose; the retained joint hierarchy/weights are the Phase 2 animation
// input and are intentionally not interpreted as clips yet.
class CharacterModelAsset {
public:
    bool loadCooked(const std::string& path, std::string* error = nullptr);
    bool valid() const { return valid_; }
    bool hasSkinning() const { return hasSkinning_; }
    float height() const { return boundsMax_[1] - boundsMin_[1]; }

    const std::array<float, 3>& boundsMin() const { return boundsMin_; }
    const std::array<float, 3>& boundsMax() const { return boundsMax_; }
    const std::vector<CharacterModelMaterial>& materials() const { return materials_; }
    const std::vector<CharacterModelJoint>& joints() const { return joints_; }
    const std::vector<CharacterModelVertex>& vertices() const { return vertices_; }
    const std::vector<std::uint32_t>& indices() const { return indices_; }
    const std::vector<CharacterModelSubmesh>& submeshes() const { return submeshes_; }
    const std::vector<CharacterAnimationClip>& animations() const { return animations_; }
    const CharacterAnimationClip* findAnimation(const std::string& name) const;

private:
    void clear();

    bool valid_{false};
    bool hasSkinning_{false};
    std::array<float, 3> boundsMin_{};
    std::array<float, 3> boundsMax_{};
    std::vector<CharacterModelMaterial> materials_;
    std::vector<CharacterModelJoint> joints_;
    std::vector<CharacterModelVertex> vertices_;
    std::vector<std::uint32_t> indices_;
    std::vector<CharacterModelSubmesh> submeshes_;
    std::vector<CharacterAnimationClip> animations_;
};

// Assets are loaded once by a platform shell and then shared by every frame.
// The repository is keyed by gameplay-neutral character ids so later fighters
// can use the same path without creating a Rrvvfo-specific renderer.
class CharacterModelRepository {
public:
    bool load(const std::string& characterId, const std::string& cookedPath,
              std::string* error = nullptr);
    const CharacterModelAsset* find(const std::string& characterId) const;
    bool has(const std::string& characterId) const { return find(characterId) != nullptr; }

private:
    std::unordered_map<std::string, CharacterModelAsset> assets_;
};

} // namespace px
