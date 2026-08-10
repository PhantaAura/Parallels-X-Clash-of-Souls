# 3DS Bring-Up 0.3A

This milestone starts the real native Old 3DS XL path without forking gameplay.

## What is implemented in source

- Native `libctru` entry point.
- Citro2D/Citro3D initialization.
- Top-screen visualization of the same `training_region` map data used by desktop/tests.
- Circle Pad movement plus Old-3DS-compatible face/shoulder/D-Pad buttons translated into shared semantic `px::Action` inputs (no ZL/ZR dependency).
- Movement through shared `FieldMovementSystem` and `WorldCollision`.
- Y button calls the shared `CombatSystem::light()` against a temporary Sage proof target.
- Save proof writes shared SaveCodec data to `sdmc:/3ds/ParallelsX/proof_save.txt` on exit.
- Bottom-screen debug/status console.

## What this is NOT yet

- Not the final renderer.
- Not a finished Chapter 1 port.
- Not a Jimmy/Rrvvfo GLB loader on 3DS.
- Not a promise that the final game will run at full desktop visual quality.

The top screen deliberately uses cheap shapes. This proves native 3DS initialization plus reuse of shared low-level map, collision, movement, combat, input and save components. It does **not** yet prove the full `RuntimeSession` chapter flow, shared dialogue/training state machine, or new world-presentation rendering on 3DS.

## Why this comes before the model

The final 3DS build should not parse full GLB files at runtime. Blender remains the source of truth, but 3DS assets should be cooked into a compact format with:

- low-poly mesh data,
- quantized/compact vertices where useful,
- texture atlases,
- small animation clips,
- simplified materials,
- minimal draw calls.

Once Rrvvfo's model is finished, the next hardware milestone is to cook a reduced copy of that model and replace the red proof circle with the actual character.

## Local build

The source target needs devkitPro/devkitARM + libctru/Citro3D/Citro2D.

From the project root:

```bash
./scripts/build-3ds.sh
```

If the toolchain is missing, the script exits with a clear message rather than pretending it built a `.3dsx`.

## Old 3DS acceptance test

On real Old 3DS XL hardware, record:

1. boots to top/bottom screens,
2. stable movement through the training-region map,
3. river collision works,
4. Light combo reduces the proof opponent HP,
5. save file is created on START,
6. no crash after five minutes,
7. approximate FPS / visible stalls,
8. memory or asset-load issues.

Only after this passes do we move the proof from shapes to a cooked 3D character/environment.
