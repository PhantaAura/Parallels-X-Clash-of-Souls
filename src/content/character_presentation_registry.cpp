#include "content/character_presentation_registry.hpp"
#include <stdexcept>

namespace px {

CharacterPresentationRegistry::CharacterPresentationRegistry() {
    characters_.emplace("rrvvfo", CharacterPresentationDefinition{
        "rrvvfo",
        "assets/characters/rrvvfo/rrvvfo-dev.glb",
        "assets/characters/rrvvfo/rrvvfo-dev.pxskel",
        "",
        true,
        CharacterFallbackKind::ProceduralHumanoid,
        154.0f,
        {0.78f, 0.08f, 0.07f, 1.0f},
        {0.16f, 0.035f, 0.025f, 1.0f},
        0.0f
    });
    characters_.emplace("sage", CharacterPresentationDefinition{
        "sage", "", "", "", false, CharacterFallbackKind::ProceduralMentor, 160.0f,
        {0.72f, 0.80f, 0.84f, 1.0f}, {0.20f, 0.27f, 0.32f, 1.0f}
    });
    characters_.emplace("ambient", CharacterPresentationDefinition{
        "ambient", "", "", "", false, CharacterFallbackKind::ProceduralHumanoid, 142.0f,
        {0.48f, 0.51f, 0.52f, 1.0f}, {0.22f, 0.25f, 0.27f, 1.0f}
    });
    const auto ambient = [&](const std::string& id, PresentationColor primary, PresentationColor secondary) {
        characters_.emplace(id, CharacterPresentationDefinition{
            id, "", "", "", false, CharacterFallbackKind::ProceduralHumanoid, 142.0f, primary, secondary
        });
    };
    ambient("ambient_student", {0.29f,0.56f,0.91f,1}, {0.13f,0.25f,0.43f,1});
    ambient("ambient_traveler", {0.89f,0.42f,0.28f,1}, {0.38f,0.20f,0.15f,1});
    ambient("ambient_worker", {0.43f,0.67f,0.35f,1}, {0.84f,0.69f,0.30f,1});
    ambient("ambient_competitor", {0.83f,0.53f,0.27f,1}, {0.35f,0.22f,0.16f,1});
    ambient("ambient_fan", {0.31f,0.64f,0.66f,1}, {0.15f,0.31f,0.34f,1});
    ambient("ambient_vendor", {0.54f,0.39f,0.81f,1}, {0.27f,0.18f,0.42f,1});
    ambient("ambient_painter", {0.78f,0.37f,0.47f,1}, {0.37f,0.16f,0.21f,1});
    ambient("checkpoint_worker", {0.62f,0.68f,0.74f,1}, {0.23f,0.28f,0.33f,1});
    ambient("tournament_fan", {0.31f,0.64f,0.66f,1}, {0.15f,0.31f,0.34f,1});
    ambient("sign_painter", {0.78f,0.37f,0.47f,1}, {0.37f,0.16f,0.21f,1});
    ambient("road_fighter", {0.50f,0.42f,0.93f,1}, {0.20f,0.16f,0.42f,1});
}

const CharacterPresentationDefinition& CharacterPresentationRegistry::get(const std::string& id) const {
    const auto it = characters_.find(id);
    if (it != characters_.end()) return it->second;
    const auto ambient = characters_.find("ambient");
    if (ambient == characters_.end()) throw std::out_of_range("No character presentation: " + id);
    return ambient->second;
}

bool CharacterPresentationRegistry::has(const std::string& id) const {
    return characters_.find(id) != characters_.end();
}

} // namespace px
