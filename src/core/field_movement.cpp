#include "core/field_movement.hpp"
#include <algorithm>
#include <cmath>

namespace px {

Vec2 FieldMovementSystem::tick(
    const MapDefinition& map,
    Vec2 position,
    FieldMovementState& state,
    const InputState& input,
    float dt,
    const FieldMovementConfig& config,
    const std::vector<std::string>& disabledBlockers
) {
    dt = std::max(0.0f, dt);
    state.dashStartedThisFrame = false;
    state.jumpStartedThisFrame = false;
    state.landedThisFrame = false;
    state.dashCooldown = std::max(0.0f, state.dashCooldown - dt);
    state.jumpBufferTime = std::max(0.0f, state.jumpBufferTime - dt);
    state.coyoteTime = std::max(0.0f, state.coyoteTime - dt);

    float x = input.movementX();
    float z = input.movementZ();
    if (std::abs(x) < .001f && std::abs(z) < .001f) {
        if (input.down(Action::MoveLeft))  x -= 1.0f;
        if (input.down(Action::MoveRight)) x += 1.0f;
        if (input.down(Action::MoveUp))    z -= 1.0f;
        if (input.down(Action::MoveDown))  z += 1.0f;
    }

    const float rawLength = std::sqrt(x * x + z * z);
    const float magnitude = std::clamp(rawLength, 0.0f, 1.0f);
    if (rawLength > 0.0001f) {
        x = x / rawLength * magnitude;
        z = z / rawLength * magnitude;
        state.facing = {x / magnitude, z / magnitude};
        state.hasFacing = true;
    }

    const bool hasDashDirection = rawLength > 0.0001f || state.hasFacing;
    if (input.pressed(Action::Dash) && hasDashDirection && state.dashCooldown <= 0.0f && state.dashTime <= 0.0f) {
        state.dashDirection = state.facing;
        state.hasFacing = true;
        state.dashTime = config.dashSeconds;
        state.dashCooldown = config.dashCooldownSeconds;
        state.dashStartedThisFrame = true;
    }
    if (state.dashTime > 0.0f && rawLength > 0.0001f) {
        const float steering = std::clamp(config.dashSteeringPerSecond * dt, 0.0f, 1.0f);
        const float steeredX = state.dashDirection.x * (1.0f - steering) + state.facing.x * steering;
        const float steeredZ = state.dashDirection.z * (1.0f - steering) + state.facing.z * steering;
        const float steeredLength = std::sqrt(steeredX * steeredX + steeredZ * steeredZ);
        if (steeredLength > 0.0001f)
            state.dashDirection = {steeredX / steeredLength, steeredZ / steeredLength};
    }
    state.dashing = state.dashTime > 0.0f;
    const float dashDt = std::min(dt, state.dashTime);
    const float walkDt = dt - dashDt;
    state.dashTime = std::max(0.0f, state.dashTime - dashDt);

    const bool groundedAtFrameStart = state.height <= 0.0f && state.verticalVelocity <= 0.0f;
    if (groundedAtFrameStart) state.coyoteTime = config.coyoteSeconds;
    if (input.pressed(Action::Jump)) state.jumpBufferTime = config.jumpBufferSeconds;

    const auto startJump = [&]() {
        state.verticalVelocity = config.jumpVelocity;
        state.jumpBufferTime = 0.0f;
        state.coyoteTime = 0.0f;
        state.jumpStartedThisFrame = true;
    };
    if (state.jumpBufferTime > 0.0f && (groundedAtFrameStart || state.coyoteTime > 0.0f)) startJump();

    if (state.height > 0.0f || state.verticalVelocity > 0.0f) {
        state.height += state.verticalVelocity * dt;
        state.verticalVelocity -= config.gravity * dt;
        if (state.height <= 0.0f) {
            state.height = 0.0f;
            state.verticalVelocity = 0.0f;
            state.landedThisFrame = true;
            state.coyoteTime = config.coyoteSeconds;
            // A jump pressed just before landing should fire immediately instead of
            // disappearing between frames. This is QoL only; the jump itself is unchanged.
            if (state.jumpBufferTime > 0.0f) startJump();
        }
    }

    const float airControl = state.height > 0.0f ? config.airControlMultiplier : 1.0f;
    const Vec2 requested{
        position.x + state.dashDirection.x * config.dashSpeed * dashDt + x * config.walkSpeed * walkDt * airControl,
        position.z + state.dashDirection.z * config.dashSpeed * dashDt + z * config.walkSpeed * walkDt * airControl
    };
    return WorldCollision::move(map, position, requested, disabledBlockers);
}

Vec2 FieldMovementSystem::tick(
    const MapDefinition& map,
    Vec2 position,
    const InputState& input,
    float dt,
    const FieldMovementConfig& config,
    const std::vector<std::string>& disabledBlockers
) {
    FieldMovementState state;
    return tick(map, position, state, input, dt, config, disabledBlockers);
}

} // namespace px
