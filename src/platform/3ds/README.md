# Old 3DS XL Chapter 1 gameplay-parity build — 0.4H.1

This target is the native Old 3DS presentation of the same Chapter 1 game used by Mac/Linux. It is not an alternate top-down game and no platform-specific movement, combat, story, quest or encounter code is allowed.

## Shared without reduction

- `RuntimeSession` Chapter 1 flow and checkpoints
- map bounds, collision, blockers and route state
- exploration, platforming and interactions
- combat, arena rules, timing and abilities
- dialogue, choices, QTEs, objectives and progression
- player/opponent world position, facing and animation state
- authored perspective camera definitions and gameplay feedback
- save format

## Old 3DS presentation tier

The native `WorldRenderer3ds` consumes the shared `WorldPresentationRegistry`, `CharacterPresentationRegistry` and `RuntimeView`. It uses Citro3D perspective/depth rendering, the same authored camera transforms as desktop, the actual cooked/skinned Rrvvfo asset, the lightweight cel face, runtime actors and world markers.

The 3DS submits all `Essential` geography first, then fills a fixed scenery budget from the shared `Full` place-making layer. It also reduces cylinder/cone segments, uses cheaper CPU cel bands and simplifies HUD presentation. These changes do not alter map layout or play.

The old Gate 0 dot-map/model-inset renderer has been removed. Static tests reject its diagnostic identifiers and require the perspective camera, depth target, shared stage lookup and skinned-model contracts.

## Controls

- Circle Pad: move
- Y: Light
- X: Heavy
- Y+X: Launcher chord
- B: Jump / cancel where contextual
- A: Grab / interact / confirm where contextual
- R: Block
- D-Pad Up: Dash
- D-Pad Left: Counter
- D-Pad Right: Breaker
- D-Pad Down: Charge
- L+Y/X/B/A: visible ability slots 1–4
- L+D-Pad Up: visible ability slot 5
- START: Pause
- SELECT: manual save when safe
- Touch bottom-left utility strip: save when safe
- Touch bottom-right utility strip: pause
- L+START: save and exit

All hardware buttons translate to the same semantic `px::Action` values used by desktop.

## Build and emulator verification

From the project root:

```sh
./scripts/build-3ds.sh
```

Output:

`src/platform/3ds/parallels_x_3ds_golden_gate.3dsx`

The August 9, 2026 parity build was cross-compiled with devkitARM/libctru/Citro2D/Citro3D and booted in Azahar. The training field rendered in perspective with the shared field camera, Rrvvfo/Sage world actors, Chapter 1 dialogue and exploration state at full emulated speed. Real Old 3DS XL profiling is still required for physical-hardware frame pacing, memory, suspend/resume and battery behavior.
