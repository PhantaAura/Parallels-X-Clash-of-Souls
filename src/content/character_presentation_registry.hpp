#pragma once
#include "content/world_presentation_registry.hpp"
#include <string>
#include <unordered_map>

namespace px {

enum class CharacterFallbackKind {
    ProceduralHumanoid,
    ProceduralMentor,
    LegacySwift,
    LegacySturdy,
    LegacyHeavy,
    LegacyCasual,
    LegacyFighter,
    LegacyDisguise,
    LegacyTrainingDummy
};

struct CharacterPresentationDefinition {
    std::string characterId;
    std::string sourceModelPath;
    std::string desktopCookedAsset;
    std::string reducedCookedAsset;
    bool cookedAssetReady{false};
    CharacterFallbackKind fallback{CharacterFallbackKind::ProceduralHumanoid};
    float worldHeight{150.0f};
    PresentationColor primaryColor{};
    PresentationColor secondaryColor{};
    float modelYawOffsetDegrees{0.0f};
};

// Gameplay knows character ids only. A DEV/final desktop model or reduced 3DS
// model can replace this binding without changing RuntimeSession.
class CharacterPresentationRegistry {
public:
    CharacterPresentationRegistry();
    const CharacterPresentationDefinition& get(const std::string& id) const;
    bool has(const std::string& id) const;

private:
    std::unordered_map<std::string, CharacterPresentationDefinition> characters_;
};

} // namespace px
