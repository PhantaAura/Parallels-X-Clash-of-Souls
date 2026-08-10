#pragma once
#include "core/input.hpp"
#include "core/world.hpp"

namespace px {

struct FieldMovementConfig {
    float walkSpeed{260.0f};
    float dashSpeed{720.0f};
    float dashSeconds{0.22f};
    float dashCooldownSeconds{0.30f};
    // A small amount of steering removes accidental wrong-way dashes while
    // keeping the committed Legacy burst instead of turning it into a sprint.
    float dashSteeringPerSecond{1.6f};
    float jumpVelocity{430.0f};
    float gravity{1150.0f};
    // Small grace windows make the Legacy-style burst movement feel responsive
    // without changing jump height or route geometry.
    float jumpBufferSeconds{0.10f};
    float coyoteSeconds{0.09f};
};

struct FieldMovementState {
    Vec2 facing{1.0f, 0.0f};
    Vec2 dashDirection{1.0f, 0.0f};
    // False until the player gives a direction. This lets authored cutscene
    // staging remain intact while exploration remembers the last real heading.
    bool hasFacing{false};
    float dashTime{0.0f};
    float dashCooldown{0.0f};
    float height{0.0f};
    float verticalVelocity{0.0f};
    float jumpBufferTime{0.0f};
    float coyoteTime{0.0f};
    bool dashStartedThisFrame{false};
    bool jumpStartedThisFrame{false};
    bool landedThisFrame{false};
    bool dashing{false};
};

class FieldMovementSystem {
public:
    static Vec2 tick(
        const MapDefinition& map,
        Vec2 position,
        FieldMovementState& state,
        const InputState& input,
        float dt,
        const FieldMovementConfig& config = {},
        const std::vector<std::string>& disabledBlockers = {}
    );

    // Compatibility wrapper for low-level callers that do not need animation state.
    static Vec2 tick(
        const MapDefinition& map,
        Vec2 position,
        const InputState& input,
        float dt,
        const FieldMovementConfig& config = {},
        const std::vector<std::string>& disabledBlockers = {}
    );
};

} // namespace px
