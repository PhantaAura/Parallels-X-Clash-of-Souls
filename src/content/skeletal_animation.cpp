#include "content/skeletal_animation.hpp"

#include <algorithm>
#include <cmath>

namespace px {
namespace {

struct Transform {
    std::array<float, 3> translation{};
    std::array<float, 4> rotation{0.0f, 0.0f, 0.0f, 1.0f};
    std::array<float, 3> scale{1.0f, 1.0f, 1.0f};
};

CharacterMatrix identity() {
    return {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
}

CharacterMatrix multiply(const CharacterMatrix& a, const CharacterMatrix& b) {
    CharacterMatrix result{};
    for (std::size_t column = 0; column < 4; ++column)
        for (std::size_t row = 0; row < 4; ++row)
            for (std::size_t inner = 0; inner < 4; ++inner)
                result[column * 4 + row] += a[inner * 4 + row] * b[column * 4 + inner];
    return result;
}

Transform decompose(const CharacterMatrix& matrix) {
    Transform result;
    result.translation = {matrix[12], matrix[13], matrix[14]};
    for (std::size_t column = 0; column < 3; ++column) {
        const float x = matrix[column * 4];
        const float y = matrix[column * 4 + 1];
        const float z = matrix[column * 4 + 2];
        result.scale[column] = std::sqrt(x*x + y*y + z*z);
        if (result.scale[column] < 1.0e-7f) result.scale[column] = 1.0f;
    }
    const float r00 = matrix[0] / result.scale[0];
    const float r10 = matrix[1] / result.scale[0];
    const float r20 = matrix[2] / result.scale[0];
    const float r01 = matrix[4] / result.scale[1];
    const float r11 = matrix[5] / result.scale[1];
    const float r21 = matrix[6] / result.scale[1];
    const float r02 = matrix[8] / result.scale[2];
    const float r12 = matrix[9] / result.scale[2];
    const float r22 = matrix[10] / result.scale[2];
    const float trace = r00 + r11 + r22;
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f;
    if (trace > 0.0f) {
        const float s = std::sqrt(trace + 1.0f) * 2.0f;
        w = .25f * s; x = (r21-r12)/s; y = (r02-r20)/s; z = (r10-r01)/s;
    } else if (r00 > r11 && r00 > r22) {
        const float s = std::sqrt(1.0f+r00-r11-r22) * 2.0f;
        w = (r21-r12)/s; x = .25f*s; y = (r01+r10)/s; z = (r02+r20)/s;
    } else if (r11 > r22) {
        const float s = std::sqrt(1.0f+r11-r00-r22) * 2.0f;
        w = (r02-r20)/s; x = (r01+r10)/s; y = .25f*s; z = (r12+r21)/s;
    } else {
        const float s = std::sqrt(1.0f+r22-r00-r11) * 2.0f;
        w = (r10-r01)/s; x = (r02+r20)/s; y = (r12+r21)/s; z = .25f*s;
    }
    const float length = std::sqrt(x*x+y*y+z*z+w*w);
    result.rotation = {x/length, y/length, z/length, w/length};
    return result;
}

CharacterMatrix compose(const Transform& transform) {
    const auto& q = transform.rotation;
    const float x=q[0], y=q[1], z=q[2], w=q[3];
    CharacterMatrix result = identity();
    result[0]=(1-2*(y*y+z*z))*transform.scale[0];
    result[1]=(2*(x*y+z*w))*transform.scale[0];
    result[2]=(2*(x*z-y*w))*transform.scale[0];
    result[4]=(2*(x*y-z*w))*transform.scale[1];
    result[5]=(1-2*(x*x+z*z))*transform.scale[1];
    result[6]=(2*(y*z+x*w))*transform.scale[1];
    result[8]=(2*(x*z+y*w))*transform.scale[2];
    result[9]=(2*(y*z-x*w))*transform.scale[2];
    result[10]=(1-2*(x*x+y*y))*transform.scale[2];
    result[12]=transform.translation[0];
    result[13]=transform.translation[1];
    result[14]=transform.translation[2];
    return result;
}

template <std::size_t Size, typename Key>
std::array<float, Size> sampleLinear(const std::vector<Key>& keys, float time,
                                     CharacterAnimationInterpolation interpolation) {
    if (keys.empty()) return {};
    if (time <= keys.front().time) return keys.front().value;
    if (time >= keys.back().time) return keys.back().value;
    const auto upper = std::upper_bound(keys.begin(), keys.end(), time,
        [](float value, const Key& key) { return value < key.time; });
    const auto& right = *upper;
    const auto& left = *(upper - 1);
    if (interpolation == CharacterAnimationInterpolation::Step) return left.value;
    const float amount = (time-left.time)/(right.time-left.time);
    std::array<float, Size> result{};
    for (std::size_t i=0;i<Size;++i) result[i]=left.value[i]+(right.value[i]-left.value[i])*amount;
    return result;
}

std::array<float, 4> sampleRotation(const std::vector<CharacterAnimationQuaternionKey>& keys,
                                    float time, CharacterAnimationInterpolation interpolation) {
    if (keys.empty()) return {0,0,0,1};
    if (time <= keys.front().time) return keys.front().value;
    if (time >= keys.back().time) return keys.back().value;
    const auto upper = std::upper_bound(keys.begin(), keys.end(), time,
        [](float value, const auto& key) { return value < key.time; });
    const auto& right = *upper;
    const auto& left = *(upper - 1);
    if (interpolation == CharacterAnimationInterpolation::Step) return left.value;
    const float amount = (time-left.time)/(right.time-left.time);
    float dot = 0.0f;
    for (std::size_t i=0;i<4;++i) dot += left.value[i]*right.value[i];
    const float sign = dot < 0.0f ? -1.0f : 1.0f;
    dot = std::clamp(dot * sign, 0.0f, 1.0f);
    std::array<float,4> result{};
    if (dot < .9995f) {
        const float angle=std::acos(dot);
        const float inverseSine=1.0f/std::sin(angle);
        const float leftAmount=std::sin((1.0f-amount)*angle)*inverseSine;
        const float rightAmount=std::sin(amount*angle)*inverseSine;
        for (std::size_t i=0;i<4;++i)
            result[i]=left.value[i]*leftAmount+right.value[i]*sign*rightAmount;
        return result;
    }
    float lengthSquared=0.0f;
    for (std::size_t i=0;i<4;++i) {
        result[i]=left.value[i]+(right.value[i]*sign-left.value[i])*amount;
        lengthSquared+=result[i]*result[i];
    }
    const float inverseLength=1.0f/std::sqrt(lengthSquared);
    for (auto& value:result)value*=inverseLength;
    return result;
}

} // namespace

void SkeletalAnimationPlayer::bind(const CharacterModelAsset* asset) {
    asset_ = asset;
    clip_ = nullptr;
    state_.clear();
    time_ = 0.0f;
    finished_ = false;
    rebuildBindCache();
    sample();
}

void SkeletalAnimationPlayer::rebuildBindCache() {
    bindLocalMatrices_.clear();
    localMatrices_.clear();
    worldMatrices_.clear();
    skinMatrices_.clear();
    bindTranslations_.clear();
    bindRotations_.clear();
    bindScales_.clear();
    if (!asset_ || !asset_->valid()) return;

    const auto& joints = asset_->joints();
    const auto count = joints.size();
    bindLocalMatrices_.resize(count);
    localMatrices_.resize(count);
    worldMatrices_.resize(count);
    skinMatrices_.resize(count);
    bindTranslations_.resize(count);
    bindRotations_.resize(count);
    bindScales_.resize(count);
    for (std::size_t index = 0; index < count; ++index) {
        bindLocalMatrices_[index] = joints[index].localBindMatrix;
        const Transform bind = decompose(joints[index].localBindMatrix);
        bindTranslations_[index] = bind.translation;
        bindRotations_[index] = bind.rotation;
        bindScales_[index] = bind.scale;
    }
}

bool SkeletalAnimationPlayer::setState(const std::string& clipName, bool restart) {
    if (!restart && state_ == clipName) return clip_ != nullptr;
    state_ = clipName;
    clip_ = asset_ ? asset_->findAnimation(clipName) : nullptr;
    time_ = 0.0f;
    finished_ = false;
    sample();
    return clip_ != nullptr;
}

void SkeletalAnimationPlayer::setPlaybackSpeed(float speed) {
    playbackSpeed_ = std::isfinite(speed) ? speed : 1.0f;
}

void SkeletalAnimationPlayer::update(float deltaSeconds) {
    if (!clip_ || finished_ || deltaSeconds <= 0.0f || std::abs(playbackSpeed_) <= 1.0e-7f) return;
    seek(time_ + deltaSeconds * playbackSpeed_);
}

void SkeletalAnimationPlayer::seek(float timeSeconds) {
    if (!clip_) {
        time_ = 0.0f;
        sample();
        return;
    }
    if (clip_->looping) {
        time_ = std::fmod(timeSeconds, clip_->duration);
        if (time_ < 0.0f) time_ += clip_->duration;
        finished_ = false;
    } else {
        time_ = std::clamp(timeSeconds, 0.0f, clip_->duration);
        finished_ = playbackSpeed_ < 0.0f ? timeSeconds <= 0.0f : timeSeconds >= clip_->duration;
    }
    sample();
}

void SkeletalAnimationPlayer::sample() {
    if (!asset_ || !asset_->valid()) {
        skinMatrices_.clear();
        return;
    }
    const auto& joints = asset_->joints();
    const auto count = joints.size();
    if (bindLocalMatrices_.size() != count || localMatrices_.size() != count ||
        worldMatrices_.size() != count || skinMatrices_.size() != count ||
        bindTranslations_.size() != count || bindRotations_.size() != count ||
        bindScales_.size() != count) {
        // Defensive only: normal frame sampling never enters this path.
        // Preserve the current clip/state while refreshing an externally changed asset.
        rebuildBindCache();
    }

    std::copy(bindLocalMatrices_.begin(), bindLocalMatrices_.end(), localMatrices_.begin());

    if (clip_) {
        for (const auto& track : clip_->tracks) {
            if (track.jointIndex >= count) continue;
            const auto index = static_cast<std::size_t>(track.jointIndex);
            Transform transform;
            transform.translation = bindTranslations_[index];
            transform.rotation = bindRotations_[index];
            transform.scale = bindScales_[index];
            if (!track.translations.empty())
                transform.translation = sampleLinear<3>(track.translations, time_, track.translationInterpolation);
            if (!track.rotations.empty())
                transform.rotation = sampleRotation(track.rotations, time_, track.rotationInterpolation);
            if (!track.scales.empty())
                transform.scale = sampleLinear<3>(track.scales, time_, track.scaleInterpolation);
            localMatrices_[index] = compose(transform);
        }
    }

    for (std::size_t index = 0; index < count; ++index) {
        const auto parent = joints[index].parentIndex;
        worldMatrices_[index] = parent >= 0
            ? multiply(worldMatrices_[static_cast<std::size_t>(parent)], localMatrices_[index])
            : localMatrices_[index];
        skinMatrices_[index] = multiply(worldMatrices_[index], joints[index].inverseBindMatrix);
    }
}

std::array<float, 3> SkeletalAnimationPlayer::skinPosition(const CharacterModelVertex& vertex) const {
    if (skinMatrices_.empty()) return vertex.position;
    std::array<float,3> result{};
    float total=0.0f;
    for (std::size_t influence=0;influence<4;++influence) {
        const float weight=vertex.weights[influence];
        if (weight<=0.0f)continue;
        const auto joint=vertex.joints[influence];
        if (joint>=skinMatrices_.size())continue;
        const auto& matrix=skinMatrices_[joint];
        result[0]+=weight*(matrix[0]*vertex.position[0]+matrix[4]*vertex.position[1]+matrix[8]*vertex.position[2]+matrix[12]);
        result[1]+=weight*(matrix[1]*vertex.position[0]+matrix[5]*vertex.position[1]+matrix[9]*vertex.position[2]+matrix[13]);
        result[2]+=weight*(matrix[2]*vertex.position[0]+matrix[6]*vertex.position[1]+matrix[10]*vertex.position[2]+matrix[14]);
        total+=weight;
    }
    return total>0.0f?result:vertex.position;
}

} // namespace px
