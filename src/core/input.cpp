#include "core/input.hpp"
#include <algorithm>

namespace px {

InputProfile InputProfiles::modern() {
    InputProfile p;
    p.preset = InputPreset::Modern;
    p.bindings = {
        {Action::MoveUp, "W / Left Stick Up"},
        {Action::MoveDown, "S / Left Stick Down"},
        {Action::MoveLeft, "A / Left Stick Left"},
        {Action::MoveRight, "D / Left Stick Right"},
        {Action::Jump, "Space / South Face Button"},
        {Action::Light, "Mouse1 / J / West Face Button"},
        {Action::Heavy, "K / North Face Button"},
        {Action::Launcher, "I / Right Trigger"},
        {Action::Grab, "U / East Face Button"},
        {Action::Block, "Mouse2 / L / Right Shoulder"},
        {Action::Counter, "Q / D-Pad Left"},
        {Action::Breaker, "R / D-Pad Right"},
        {Action::Charge, "C / D-Pad Down"},
        {Action::Dash, "Shift / D-Pad Up / Left Trigger"},
        {Action::Interact, "E / East Face Button"},
        {Action::Ability1, "1 / Left Shoulder + West Face Button"},
        {Action::Ability2, "2 / Left Shoulder + North Face Button"},
        {Action::Ability3, "3 / Left Shoulder + South Face Button"},
        {Action::Ability4, "4 / Left Shoulder + East Face Button"},
        {Action::Ability5, "5 / Left Shoulder + D-Pad Up"},
        {Action::AbilityUse, "O"},
        {Action::Pause, "Escape / Menu Button"}
    };
    return p;
}

InputProfile InputProfiles::legacy() {
    InputProfile p;
    p.preset = InputPreset::Legacy;
    // Gameplay reads Actions, never these literal keys. This profile exists for Legacy muscle memory.
    p.bindings = {
        {Action::MoveUp, "W"},
        {Action::MoveDown, "S"},
        {Action::MoveLeft, "A"},
        {Action::MoveRight, "D"},
        {Action::Jump, "Space"},
        {Action::Light, "Mouse1 / J"},
        {Action::Heavy, "K"},
        {Action::Launcher, "I"},
        {Action::Grab, "U"},
        {Action::Block, "Mouse2 / L"},
        {Action::Counter, "Q"},
        {Action::Breaker, "R"},
        {Action::Charge, "C"},
        {Action::Dash, "Shift"},
        {Action::Interact, "E"},
        {Action::Ability1, "1"},
        {Action::Ability2, "2"},
        {Action::Ability3, "3"},
        {Action::Ability4, "4"},
        {Action::Ability5, "5"},
        {Action::AbilityUse, "O"},
        {Action::Pause, "Escape"}
    };
    return p;
}

void InputState::beginFrame() {
    pressed_.clear();
    released_.clear();
    movementX_ = movementZ_ = 0.0f;
    cameraX_ = cameraY_ = 0.0f;
}

void InputState::set(Action action, bool held) {
    const bool wasHeld = held_.count(action) != 0;
    if (held && !wasHeld) {
        held_.insert(action);
        pressed_.insert(action);
    } else if (!held && wasHeld) {
        held_.erase(action);
        released_.insert(action);
    }
}

bool InputState::down(Action action) const { return held_.count(action) != 0; }
bool InputState::pressed(Action action) const { return pressed_.count(action) != 0; }
bool InputState::released(Action action) const { return released_.count(action) != 0; }

void InputState::setMovementAxes(float x, float z) {
    movementX_ = std::clamp(x, -1.0f, 1.0f);
    movementZ_ = std::clamp(z, -1.0f, 1.0f);
}

void InputState::setCameraAxes(float x, float y) {
    cameraX_ = std::clamp(x, -1.0f, 1.0f);
    cameraY_ = std::clamp(y, -1.0f, 1.0f);
}

} // namespace px
