# Old 3DS XL Target

The full game is the goal. Graphical parity with Mac is not; gameplay/story parity is.

## Preserve on Old 3DS

- same continuous Story/chapter route structure,
- same dialogue/cutscene content,
- same quests and progression,
- same combat and arena rules,
- same map logic and major geography,
- compatible save progression,
- same character mechanics and ability chronology.

## Scale down as needed

Mesh/texture tiers, particles, lighting/shadows, crowds, post effects, audio bitrate and environment draw-call density may be reduced. Those are platform presentation tiers, not different gameplay.

## Performance direction

Initial target is 30 FPS gameplay on Old 3DS XL with low-poly runtime models, atlases, cheap/baked lighting, controlled draw calls and bottom-screen UI where useful. Hardware measurement decides exact budgets; do not invent a fixed reduced skeleton count until the real 39-joint Rrvvfo gate is profiled.

## Compatibility gates

**Gate 0 — completed historical bring-up:** the shared Chapter 1 `RuntimeSession`, cooked Rrvvfo asset, 39-joint/26-clip skeletal player, controls and save codec were brought up through a temporary diagnostic view. That view is not a distributable game version and has been removed from the current target.

**Gate 1 — corrected in 0.4H.2:** the current build embeds its ROMFS, uses the real Citro3D perspective character/world renderer, shared authored camera, full skinned Rrvvfo mesh/face, native Legacy-style interface, runtime actors/markers, mandatory Essential geography and a bounded share of Full scenery. A missing model blocks launch instead of selecting a substitute. Azahar validates boot, title/front end, real-model route presentation, visual orientation, depth rendering and shared Chapter 1 flow; real Old 3DS XL profiling still decides final draw/particle budgets.

**Gate 2:** exercise the full Chapter 1 golden slice end to end on real hardware and set the asset-tier budgets that Chapter 2+ must obey.

The compatibility gate happens before Chapter 2 so renderer/skeleton/memory assumptions are corrected once instead of retrofitted across several completed chapters.
