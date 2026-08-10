#include "content/character_model_asset.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <sstream>

namespace px {
namespace {

constexpr char kMagic[8] = {'P','X','S','K','E','L','1','\0'};
constexpr std::uint32_t kCurrentVersion = 2;
constexpr std::uint32_t kMaximumMaterials = 256;
constexpr std::uint32_t kMaximumJoints = 1024;
constexpr std::uint32_t kMaximumVertices = 4'000'000;
constexpr std::uint32_t kMaximumIndices = 12'000'000;
constexpr std::uint32_t kMaximumClips = 1024;
constexpr std::uint32_t kMaximumTracksPerClip = 4096;
constexpr std::uint32_t kMaximumKeysPerChannel = 1'000'000;

class BinaryReader {
public:
    explicit BinaryReader(const std::string& path) : stream_(path, std::ios::binary) {}
    bool open() const { return static_cast<bool>(stream_); }

    template <typename T>
    bool scalar(T& value) {
        stream_.read(reinterpret_cast<char*>(&value), sizeof(T));
        return static_cast<bool>(stream_);
    }

    template <typename T, std::size_t Count>
    bool array(std::array<T, Count>& value) {
        stream_.read(reinterpret_cast<char*>(value.data()), sizeof(T) * Count);
        return static_cast<bool>(stream_);
    }

    bool bytes(char* destination, std::size_t size) {
        stream_.read(destination, static_cast<std::streamsize>(size));
        return static_cast<bool>(stream_);
    }

    bool string(std::string& value) {
        std::uint16_t size = 0;
        if (!scalar(size)) return false;
        value.resize(size);
        return size == 0 || bytes(value.data(), size);
    }

    bool finished() {
        char byte = 0;
        stream_.read(&byte, 1);
        return stream_.eof();
    }

private:
    std::ifstream stream_;
};

void setError(std::string* destination, const std::string& value) {
    if (destination) *destination = value;
}

bool finite(float value) { return std::isfinite(value); }

} // namespace

void CharacterModelAsset::clear() {
    valid_ = false;
    hasSkinning_ = false;
    boundsMin_ = {};
    boundsMax_ = {};
    materials_.clear();
    joints_.clear();
    vertices_.clear();
    indices_.clear();
    submeshes_.clear();
    animations_.clear();
}

bool CharacterModelAsset::loadCooked(const std::string& path, std::string* error) {
    clear();
    BinaryReader input(path);
    if (!input.open()) {
        setError(error, "Could not open cooked character asset: " + path);
        return false;
    }

    char magic[8]{};
    if (!input.bytes(magic, sizeof(magic)) || std::memcmp(magic, kMagic, sizeof(kMagic)) != 0) {
        setError(error, "Cooked character asset has an invalid header: " + path);
        return false;
    }

    std::uint32_t version = 0;
    std::uint32_t materialCount = 0;
    std::uint32_t jointCount = 0;
    std::uint32_t vertexCount = 0;
    std::uint32_t indexCount = 0;
    std::uint32_t submeshCount = 0;
    if (!input.scalar(version) || !input.scalar(materialCount) || !input.scalar(jointCount) ||
        !input.scalar(vertexCount) || !input.scalar(indexCount) || !input.scalar(submeshCount) ||
        !input.array(boundsMin_) || !input.array(boundsMax_)) {
        setError(error, "Cooked character header is truncated: " + path);
        clear();
        return false;
    }
    if ((version != 1 && version != kCurrentVersion) || materialCount == 0 || materialCount > kMaximumMaterials ||
        jointCount == 0 || jointCount > kMaximumJoints || vertexCount == 0 || vertexCount > kMaximumVertices ||
        indexCount == 0 || indexCount > kMaximumIndices || indexCount % 3 != 0 || submeshCount == 0) {
        setError(error, "Cooked character header contains unsupported counts or version");
        clear();
        return false;
    }
    for (std::size_t axis = 0; axis < 3; ++axis) {
        if (!finite(boundsMin_[axis]) || !finite(boundsMax_[axis]) || boundsMax_[axis] <= boundsMin_[axis]) {
            setError(error, "Cooked character bounds are invalid");
            clear();
            return false;
        }
    }

    materials_.resize(materialCount);
    for (auto& material : materials_) {
        std::uint8_t unlit = 0;
        std::uint8_t doubleSided = 0;
        if (!input.string(material.name) || !input.array(material.color) ||
            !input.scalar(unlit) || !input.scalar(doubleSided)) {
            setError(error, "Cooked material table is truncated");
            clear();
            return false;
        }
        material.unlit = unlit != 0;
        material.doubleSided = doubleSided != 0;
        if (material.name.empty() || !std::all_of(material.color.begin(), material.color.end(), finite)) {
            setError(error, "Cooked material data is invalid");
            clear();
            return false;
        }
    }

    joints_.resize(jointCount);
    for (std::size_t index = 0; index < joints_.size(); ++index) {
        auto& joint = joints_[index];
        if (!input.string(joint.name) || !input.scalar(joint.parentIndex) ||
            !input.array(joint.localBindMatrix) || !input.array(joint.inverseBindMatrix)) {
            setError(error, "Cooked joint table is truncated");
            clear();
            return false;
        }
        if (joint.name.empty() || joint.parentIndex >= static_cast<std::int32_t>(index) || joint.parentIndex < -1 ||
            !std::all_of(joint.localBindMatrix.begin(), joint.localBindMatrix.end(), finite) ||
            !std::all_of(joint.inverseBindMatrix.begin(), joint.inverseBindMatrix.end(), finite)) {
            setError(error, "Cooked joint hierarchy is invalid");
            clear();
            return false;
        }
    }

    vertices_.resize(vertexCount);
    for (auto& vertex : vertices_) {
        if (!input.array(vertex.position) || !input.array(vertex.normal) ||
            !input.array(vertex.joints) || !input.array(vertex.weights)) {
            setError(error, "Cooked vertex data is truncated");
            clear();
            return false;
        }
        if (!std::all_of(vertex.position.begin(), vertex.position.end(), finite) ||
            !std::all_of(vertex.normal.begin(), vertex.normal.end(), finite) ||
            !std::all_of(vertex.weights.begin(), vertex.weights.end(), finite) ||
            std::any_of(vertex.joints.begin(), vertex.joints.end(),
                        [jointCount](std::uint16_t joint) { return joint >= jointCount; })) {
            setError(error, "Cooked vertex contains invalid skin or geometry values");
            clear();
            return false;
        }
        const float totalWeight = vertex.weights[0] + vertex.weights[1] + vertex.weights[2] + vertex.weights[3];
        if (totalWeight > 0.0f) {
            hasSkinning_ = true;
            if (std::abs(totalWeight - 1.0f) > 0.002f ||
                std::any_of(vertex.weights.begin(), vertex.weights.end(), [](float weight) { return weight < 0.0f; })) {
                setError(error, "Cooked vertex skin weights are not normalized");
                clear();
                return false;
            }
        }
    }

    indices_.resize(indexCount);
    for (auto& index : indices_) {
        if (!input.scalar(index) || index >= vertexCount) {
            setError(error, "Cooked triangle index is invalid");
            clear();
            return false;
        }
    }

    submeshes_.resize(submeshCount);
    std::uint64_t coveredIndices = 0;
    for (auto& submesh : submeshes_) {
        if (!input.scalar(submesh.firstIndex) || !input.scalar(submesh.indexCount) ||
            !input.scalar(submesh.materialIndex)) {
            setError(error, "Cooked submesh table is truncated");
            clear();
            return false;
        }
        const std::uint64_t end = static_cast<std::uint64_t>(submesh.firstIndex) + submesh.indexCount;
        if (submesh.indexCount == 0 || submesh.indexCount % 3 != 0 || end > indexCount ||
            submesh.materialIndex >= materialCount) {
            setError(error, "Cooked submesh range is invalid");
            clear();
            return false;
        }
        coveredIndices += submesh.indexCount;
    }
    if (coveredIndices != indexCount || !hasSkinning_) {
        setError(error, !hasSkinning_ ? "Cooked character does not retain a usable skin" :
                                      "Cooked character contains uncovered geometry");
        clear();
        return false;
    }

    if (version >= 2) {
        std::uint32_t clipCount = 0;
        if (!input.scalar(clipCount) || clipCount > kMaximumClips) {
            setError(error, "Cooked animation table is invalid");
            clear();
            return false;
        }
        animations_.resize(clipCount);
        for (auto& clip : animations_) {
            std::uint8_t looping = 0;
            std::uint32_t trackCount = 0;
            if (!input.string(clip.name) || !input.scalar(clip.duration) || !input.scalar(looping) ||
                !input.scalar(trackCount) || clip.name.empty() || !finite(clip.duration) ||
                clip.duration <= 0.0f || trackCount > kMaximumTracksPerClip) {
                setError(error, "Cooked animation clip header is invalid");
                clear();
                return false;
            }
            clip.looping = looping != 0;
            clip.tracks.resize(trackCount);
            std::vector<bool> trackedJoints(jointCount, false);
            for (auto& track : clip.tracks) {
                std::uint8_t translationInterpolation = 0;
                std::uint8_t rotationInterpolation = 0;
                std::uint8_t scaleInterpolation = 0;
                std::uint32_t translationCount = 0;
                std::uint32_t rotationCount = 0;
                std::uint32_t scaleCount = 0;
                if (!input.scalar(track.jointIndex) || !input.scalar(translationInterpolation) ||
                    !input.scalar(rotationInterpolation) || !input.scalar(scaleInterpolation) ||
                    !input.scalar(translationCount) || !input.scalar(rotationCount) || !input.scalar(scaleCount) ||
                    track.jointIndex >= jointCount || trackedJoints[track.jointIndex] ||
                    translationInterpolation > 1 || rotationInterpolation > 1 || scaleInterpolation > 1 ||
                    translationCount > kMaximumKeysPerChannel || rotationCount > kMaximumKeysPerChannel ||
                    scaleCount > kMaximumKeysPerChannel ||
                    (translationCount == 0 && rotationCount == 0 && scaleCount == 0)) {
                    setError(error, "Cooked animation track header is invalid");
                    clear();
                    return false;
                }
                trackedJoints[track.jointIndex] = true;
                track.translationInterpolation = static_cast<CharacterAnimationInterpolation>(translationInterpolation);
                track.rotationInterpolation = static_cast<CharacterAnimationInterpolation>(rotationInterpolation);
                track.scaleInterpolation = static_cast<CharacterAnimationInterpolation>(scaleInterpolation);

                const auto readVectorKeys = [&](std::vector<CharacterAnimationVectorKey>& keys,
                                                std::uint32_t count, bool requirePositive) {
                    keys.resize(count);
                    float previous = -1.0f;
                    for (auto& key : keys) {
                        if (!input.scalar(key.time) || !input.array(key.value) || !finite(key.time) ||
                            key.time < 0.0f || key.time > clip.duration || key.time <= previous ||
                            !std::all_of(key.value.begin(), key.value.end(), finite) ||
                            (requirePositive && std::any_of(key.value.begin(), key.value.end(),
                                [](float value) { return value <= 0.0f; }))) return false;
                        previous = key.time;
                    }
                    return true;
                };
                track.translations.resize(translationCount);
                float previousTranslation = -1.0f;
                for (auto& key : track.translations) {
                    if (!input.scalar(key.time) || !input.array(key.value) || !finite(key.time) ||
                        key.time < 0.0f || key.time > clip.duration || key.time <= previousTranslation ||
                        !std::all_of(key.value.begin(), key.value.end(), finite)) {
                        setError(error, "Cooked animation translation keys are invalid");
                        clear();
                        return false;
                    }
                    previousTranslation = key.time;
                }
                track.rotations.resize(rotationCount);
                float previousRotation = -1.0f;
                for (auto& key : track.rotations) {
                    if (!input.scalar(key.time) || !input.array(key.value) || !finite(key.time) ||
                        key.time < 0.0f || key.time > clip.duration || key.time <= previousRotation ||
                        !std::all_of(key.value.begin(), key.value.end(), finite)) {
                        setError(error, "Cooked animation rotation keys are invalid");
                        clear();
                        return false;
                    }
                    const float lengthSquared = key.value[0]*key.value[0] + key.value[1]*key.value[1] +
                                                key.value[2]*key.value[2] + key.value[3]*key.value[3];
                    if (std::abs(lengthSquared - 1.0f) > .005f) {
                        setError(error, "Cooked animation quaternion is not normalized");
                        clear();
                        return false;
                    }
                    previousRotation = key.time;
                }
                if (!readVectorKeys(track.scales, scaleCount, true)) {
                    setError(error, "Cooked animation scale keys are invalid");
                    clear();
                    return false;
                }
            }
        }
        std::vector<std::string> names;
        for (const auto& clip : animations_) {
            if (std::find(names.begin(), names.end(), clip.name) != names.end()) {
                setError(error, "Cooked animation clip names are not unique");
                clear();
                return false;
            }
            names.push_back(clip.name);
        }
    }
    if (!input.finished()) {
        setError(error, "Cooked character contains trailing data");
        clear();
        return false;
    }

    valid_ = true;
    return true;
}

const CharacterAnimationClip* CharacterModelAsset::findAnimation(const std::string& name) const {
    const auto found = std::find_if(animations_.begin(), animations_.end(),
                                    [&](const auto& clip) { return clip.name == name; });
    return found == animations_.end() ? nullptr : &*found;
}

bool CharacterModelRepository::load(const std::string& characterId, const std::string& cookedPath,
                                    std::string* error) {
    if (characterId.empty()) {
        setError(error, "Character id cannot be empty");
        return false;
    }
    CharacterModelAsset candidate;
    if (!candidate.loadCooked(cookedPath, error)) return false;
    assets_.insert_or_assign(characterId, std::move(candidate));
    return true;
}

const CharacterModelAsset* CharacterModelRepository::find(const std::string& characterId) const {
    const auto found = assets_.find(characterId);
    return found == assets_.end() ? nullptr : &found->second;
}

} // namespace px
