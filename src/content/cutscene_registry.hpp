#pragma once
#include "core/math.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace px {

enum class CutsceneTier { FieldDialogue = 1, Directed = 2, Major = 3 };

struct CutsceneBeat {
    std::string id;
    std::string speaker;
    std::string intent;
    std::string cameraCue;
    std::string animationCue;
};

struct ActorStaging {
    std::string actorId;
    Vec2 position{};
    float yawDegrees{0.0f};
    bool visible{true};
};

struct CutsceneDefinition {
    std::string id;
    CutsceneTier tier{CutsceneTier::Directed};
    std::vector<CutsceneBeat> beats;
    std::vector<ActorStaging> staging;
};

class CutsceneRegistry {
public:
    CutsceneRegistry();
    const CutsceneDefinition& get(const std::string& id) const;
    bool has(const std::string& id) const;

private:
    std::unordered_map<std::string, CutsceneDefinition> cutscenes_;
};

} // namespace px
