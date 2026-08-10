#pragma once
#include "core/input.hpp"
#include <3ds.h>

namespace px::platform3ds {

// 3DS hardware is translated into the exact same semantic Actions used by
// desktop. Nothing in combat or chapter content checks KEY_A/KEY_B/etc.
void updateInput(px::InputState& input, u32 keysHeld, circlePosition circle);

} // namespace px::platform3ds
