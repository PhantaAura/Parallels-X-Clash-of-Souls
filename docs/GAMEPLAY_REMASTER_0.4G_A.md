# 3.0R 0.4G-A — Chapter 1 Gameplay Remaster Foundation

This is the historical first slice of the Chapter 1 gameplay-remaster milestone. It is superseded by `GAMEPLAY_REMASTER_0.4G_GOLD.md`. It deliberately changes feel/QoL without changing Chapter 1 story order, map identity, combat balance values, or ability chronology.

## Legacy references carried forward

- Legacy pursuit-combat buffer grace: 0.08 seconds.
- Flow Cancel identity: hit -> Dash -> keep moving.
- Chapter 1 cliff World Delight: `CLIFFSIDE VIEW`, discovered only after clearing the risky route.
- No mission/checklist shell was restored. The discovery remains part of continuous Story.

## Implemented

- 100 ms jump input buffer and 90 ms ground/coyote grace in the shared field-movement system. Jump height, gravity, route geometry and dash values are unchanged.
- This slice initially used an 80 ms follow-up buffer; 0.4G GOLD replaces it with Rrvvfo's exact Legacy 0.135-second fighter-feel buffer.
- Portable gameplay notice state used for combat timing and world moments.
- Flow Cancel readiness/success feedback. A successful Flow Cancel is persisted as `ch1_flow_cancel_learned`.
- Cliff-route adaptive guidance: first block is subtle, repeated misses reveal the remaining jump count, and only later does the prompt become explicit.
- Cliff reward/discovery now triggers after actually clearing the route instead of immediately upon selecting it.
- Linux and Mac HUD paths consume the same shared gameplay-notice state.

## Explicit non-goals for this slice

- No Chapter 2 work.
- No combat damage/cooldown rebalance.
- No new abilities.
- No Shots of Agony.
- No facial system.
- No environment redesign.
- No mission UI.
- No final hitstop/camera/effect pass yet.

## Validation

The Linux CMake build succeeds and all five current automated tests pass, including source-architecture, model, skeletal-animation and end-to-end Chapter 1 runtime coverage. Core tests now explicitly cover buffered landing jumps and the corrected cliff-discovery timing.
