#pragma once
#include "core/math.hpp"
#include <string>
#include <cstddef>
#include <unordered_map>
#include <vector>

namespace px {

enum class CutsceneTier { FieldDialogue = 1, Directed = 2, Major = 3 };

// Shared story-staging language. These actions are content data consumed by the
// same RuntimeSession on Mac, Linux and 3DS; they are not a second cutscene engine.
enum class CutsceneActionKind {
    MoveTo,
    FaceActor,
    PlayAnimation,
    LookAt,
    CameraTrack,
    CameraFocus,
    TriggerWorldEvent,
    Wait,
    Dialogue,
    Expression
};

struct CutsceneAction {
    std::size_t dialogueIndex{0};
    CutsceneActionKind kind{CutsceneActionKind::Wait};
    std::string actorId;
    std::string targetActorId;
    Vec2 position{};
    float speed{150.0f};
    float durationSeconds{0.0f};
    float cameraYawDegrees{38.0f};
    float cameraDistance{900.0f};
    float cameraHeight{410.0f};
    float cameraFovDegrees{43.0f};
    std::string cue;
};

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
    std::vector<CutsceneAction> actions;
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
