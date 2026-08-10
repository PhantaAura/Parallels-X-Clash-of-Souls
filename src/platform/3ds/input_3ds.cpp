#include "platform/3ds/input_3ds.hpp"

namespace px::platform3ds {
void updateInput(px::InputState& input,u32 keysHeld,circlePosition circle){
    constexpr int deadzone=24;const bool abilityLayer=(keysHeld&KEY_L)!=0;const bool launcherChord=!abilityLayer&&(keysHeld&KEY_Y)&&(keysHeld&KEY_X);
    input.beginFrame();
    input.set(Action::MoveLeft,circle.dx<-deadzone);input.set(Action::MoveRight,circle.dx>deadzone);
    input.set(Action::MoveUp,circle.dy>deadzone);input.set(Action::MoveDown,circle.dy<-deadzone);
    input.set(Action::Light,!abilityLayer&&!launcherChord&&(keysHeld&KEY_Y));
    input.set(Action::Heavy,!abilityLayer&&!launcherChord&&(keysHeld&KEY_X));
    input.set(Action::Launcher,launcherChord);
    input.set(Action::Jump,!abilityLayer&&(keysHeld&KEY_B));
    input.set(Action::Cancel,!abilityLayer&&(keysHeld&KEY_B));
    input.set(Action::Grab,!abilityLayer&&(keysHeld&KEY_A));input.set(Action::Interact,!abilityLayer&&(keysHeld&KEY_A));
    input.set(Action::Block,keysHeld&KEY_R);
    input.set(Action::Ability1,abilityLayer&&(keysHeld&KEY_Y));input.set(Action::Ability2,abilityLayer&&(keysHeld&KEY_X));
    input.set(Action::Ability3,abilityLayer&&(keysHeld&KEY_B));input.set(Action::Ability4,abilityLayer&&(keysHeld&KEY_A));
    input.set(Action::Ability5,abilityLayer&&(keysHeld&KEY_DUP));
    input.set(Action::Counter,!abilityLayer&&(keysHeld&KEY_DLEFT));input.set(Action::Breaker,!abilityLayer&&(keysHeld&KEY_DRIGHT));
    input.set(Action::Charge,!abilityLayer&&(keysHeld&KEY_DDOWN));input.set(Action::Dash,!abilityLayer&&(keysHeld&KEY_DUP));
    input.set(Action::Pause,!abilityLayer&&(keysHeld&KEY_START));input.set(Action::Confirm,!abilityLayer&&(keysHeld&KEY_A));
}
} // namespace px::platform3ds
