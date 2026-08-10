# 3.0R 0.4H — Golden Gate QoL

## Scope lock

Legacy `2.9A.40.7.1.1` remains the primary Chapter 1 source. This pass does not change story order, dialogue canon, Main/Forest/Cliff route order, combat balance, ability chronology or the Chapter 2 boundary. The approved follow-up face pass adds only eyes, brows and mouth; the source head, hair, body and clothing remain unchanged.

## Shared QoL

- Interactive pause menu with safe Save Game, checkpoint restart, recent objective history, controls, accessibility and Return to Title.
- Schema-4 save data with objective history and QoL settings.
- Atomic platform writes, backup recovery and migration from the previous Mac/Linux development save names.
- Optional hold-to-advance dialogue with a deliberate delay; choices still require a fresh press.
- First-time hints plus reduced motion, camera shake and flashes; high-contrast HUD, larger text and combat-message detail.
- Mild in-dash correction that preserves the short committed Legacy burst.
- Shared lightweight facial states for blinking, dialogue and combat, attached to the existing head joint without changing the model skeleton.
- Separate collision bounds for Sage's tutorial spar and the optional roadside challenge.
- Exact road-position restoration after the roadside fight is protected by the end-to-end test.

## Platform status

### Mac

- Native Cocoa/Metal app compiles on macOS.
- Shared pause/QoL state is rendered by the native HUD.
- Mouse and controller sources are composed once per frame.
- Controller disconnect pauses gameplay.
- Save schema 4 uses `save-v4.txt` and can import `save-v3.txt` or either backup.

### Linux

- Linux gameplay and renderer sources compile cleanly as C++17 against the local SDL ABI contract.
- Shared pause/QoL state has an SDL presentation.
- Hot-plug reconnect is supported; disconnect pauses gameplay.
- Linux development saves remain isolated and import the 0.4G filename when no 0.4H save exists.
- A native Linux executable is not produced on macOS; it remains covered by the Linux build scripts and shared automated tests.

### Old 3DS XL

- Gate 0 uses the full shared Chapter 1 `RuntimeSession`.
- Bottom-screen pause pages and save status consume the same shared QoL state.
- SELECT and the touch utility strip request safe saves; START and the touch utility strip pause.
- Save writes use a temporary file and backup recovery.
- The Makefile now passes the shared source include path and the Gate uses the current libctru `TickCounter` API.
- A genuine Nintendo 3DS Homebrew Application (`.3dsx`) cross-build succeeds with devkitARM/libctru/Citro2D/Citro3D.
- The face adds no bones, textures or cooked-model data; the shared 3DS cel layer remains capped at 48 tiny triangles.
- Azahar execution and the inspected opening's 59–60 application FPS are verified. Physical-hardware FPS, memory headroom and suspend/resume remain unverified.

## Validation gate before Chapter 2

1. Play Chapter 1 on Mac next to exact Legacy and approve geography, pacing, combat feel and presentation.
2. Run the `.3dsx` on the user's Old 3DS XL and record boot, controls, save/load, model stability, frame rate and crashes.
3. Fix measured Chapter 1 problems on all affected platforms.
4. Only after Chapter 1 passes both reviews, begin the Legacy-led Chapter 2 remake.
