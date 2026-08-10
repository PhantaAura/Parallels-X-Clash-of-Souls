# 3DS gameplay parity correction — 0.4H.1

> Historical rejected candidate. Emulator inspection of the distributed artifact showed that its UI/model presentation did not meet the claims below. It is superseded by `3DS_LEGACY_UI_MODEL_PARITY_0.4H.2.md` and must not be distributed.

## Problem

The prior `parallels_x_3ds_golden_gate.3dsx` still presented shared Chapter 1 state through a Gate 0 top-down diagnostic. Although it instantiated `RuntimeSession`, it looked and controlled like an alternate game because the shipping artifact rendered map-space dots, bars and a sampled-model inset instead of the authored perspective game world.

That artifact did not satisfy the agreed platform rule: same game, lower presentation tier.

## Correction

- Removed the diagnostic `mapX`/`mapY`, player-dot and `drawModelInset` path.
- Added a native Citro3D depth-buffered perspective renderer.
- Consumed the same `WorldPresentationRegistry` stages and camera definitions as Mac.
- Rendered shared blocker visibility, runtime actors, gameplay markers and effects.
- Rendered the actual cooked Rrvvfo mesh with the shared `SkeletalAnimationPlayer` and cel face expression.
- Consumed shared player/opponent position, height, yaw, hit freeze, camera impulse and dialogue focus.
- Submitted `Essential` geography first, then filled a fixed Old 3DS scenery budget from the shared `Full` detail layer; curved-primitive segments remain reduced without changing geography.
- Replaced model/animation diagnostics on the bottom screen with Chapter 1 dialogue, objectives, choices, QTEs, Combat Manual, hotbar and pause/save information.
- Migrated the platform save path to `save-v4.txt` while retaining one-way loading from the former Gate save.
- Added static parity guards that fail if the dot-map/model-inset renderer returns.

## Acceptance rule

Mac/Linux/3DS must run one `RuntimeSession` and consume the same authored stage/camera definitions. A platform may choose an asset/detail tier; it may not choose a different game camera, map topology, gameplay state machine or encounter flow.

## Verification

- Native devkitARM build: passed.
- PICA shader compile/link: passed.
- Azahar boot: passed.
- Shared perspective training-field rendering: passed.
- Shared Rrvvfo/Sage positioning and Chapter 1 dialogue/exploration transition: passed.
- Azahar reported 100% speed and 59–60 application FPS during the inspected field scene.
- Real Old 3DS XL performance and suspend/resume: still hardware-only.
