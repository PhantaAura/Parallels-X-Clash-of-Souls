#pragma once

#include "content/character_model_asset.hpp"

#include <array>
#include <string>
#include <vector>

namespace px {

using CharacterMatrix = std::array<float, 16>;

// One portable player for every imported character. It samples engine-owned
// clip data and exposes CPU skin matrices to either platform renderer.
class SkeletalAnimationPlayer {
public:
    void bind(const CharacterModelAsset* asset);
    bool setState(const std::string& clipName, bool restart = false);
    void update(float deltaSeconds);
    void seek(float timeSeconds);
    void setPlaybackSpeed(float speed);

    const CharacterModelAsset* asset() const { return asset_; }
    const std::string& state() const { return state_; }
    float time() const { return time_; }
    float playbackSpeed() const { return playbackSpeed_; }
    bool playing() const { return clip_ != nullptr && !finished_; }
    bool finished() const { return finished_; }
    const std::vector<CharacterMatrix>& skinMatrices() const { return skinMatrices_; }
    std::array<float, 3> skinPosition(const CharacterModelVertex& vertex) const;

private:
    void rebuildBindCache();
    void sample();

    const CharacterModelAsset* asset_{nullptr};
    const CharacterAnimationClip* clip_{nullptr};
    std::string state_;
    float time_{0.0f};
    float playbackSpeed_{1.0f};
    bool finished_{false};

    // Update 4: animation sampling owns reusable 39-joint work buffers.
    // `sample()` never allocates or decomposes unchanged bind transforms per frame.
    std::vector<CharacterMatrix> bindLocalMatrices_;
    std::vector<CharacterMatrix> localMatrices_;
    std::vector<CharacterMatrix> worldMatrices_;
    std::vector<CharacterMatrix> skinMatrices_;
    std::vector<std::array<float, 3>> bindTranslations_;
    std::vector<std::array<float, 4>> bindRotations_;
    std::vector<std::array<float, 3>> bindScales_;
};

} // namespace px
