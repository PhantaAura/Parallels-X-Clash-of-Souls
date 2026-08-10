# Parallels X 3.0R — Mac 0.4D.1: Legacy Opening Parity Correction

## Scope and claim boundary

This correction ends at the opening Tournament Road presentation. It does not
expand later Chapter 1 routes, quests, bosses, or chapters. It establishes a
genuine perspective 3D presentation foundation, but it does **not** claim final
visual parity: scenery and actors still use authored primitive/fallback geometry,
and final models, animation, effects, audio, and portrait art remain outstanding.

## Legacy experience reconstructed

- Training Field and Training Road camera language: 38-degree yaw, 43/45-degree
  FOV, and each stage's authored distance, height, and focus limits.
- Field sanctuary, focus pillars, Sage bell, road, river, bridge, gates, terrain,
  props, trees, and ambient actor placement as reusable world presentation data.
- Exact four-line Object Swap setup and exact three-line result exchange.
- Authored Rrvvfo/Sage cutscene transforms, including the opening scene, so Sage
  never falls back to world origin.
- Seven-step refresher behavior: 72-unit movement plus Jump/Dash; action-start
  detection for Light/Heavy/Launcher and connection for Grab; readable Perfect
  Block telegraph; 20-to-75 stationary charge; Fire Blast plus Object Swap; the
  complete 35-to-60 Lens charge/activate/read/evade-or-block sequence; and three
  clean unblocked hits.
- Chronology-safe early hotbar: Fire Blast, Object Swap, and Lens only. Shots of
  Agony and unavailable abilities are absent rather than shown as fake slots.

## Clean architecture boundary

- `WorldPresentationRegistry` owns renderer-independent camera, environment,
  landmark, prop, and ambient-actor definitions. Mac consumes the full tier;
  a future Old 3DS renderer can consume essential/reduced tiers.
- `CutsceneRegistry` owns actor staging; the renderer contains no scene-specific
  Sage position override.
- `CharacterPresentationRegistry` separates gameplay character IDs from model
  bindings and fallback rendering. `Jimmy.glb` is retained as the temporary
  Rrvvfo source model but is not rendered because no cooked model path exists yet.
- `TrainingRegistry` and `RuntimeSession` remain shared portable C++ gameplay.
- The Mac layer owns Metal rendering, Cocoa input translation, and game UI only.

## Story and replay structure

Legacy `rrvvfo-mission-*.js` files were mined only for content and player-facing
behavior. Their mission architecture was not restored. Story Mode is a continuous
chapter-to-chapter adventure. `SceneStep` objects are private implementation
details. Replay is designed to list completed whole chapters only; there is no
mission-select or per-scene/fight/tutorial replay surface.

## Renderer and UI status

The Mac renderer now submits world-space triangles through perspective view and
projection matrices with depth testing, lighting, fog, and authored world-space
character/scenery transforms. Dialogue, objective, hotbar, and combat information
use game-owned drawn panels with speaker/focus/expression metadata and an advance
indicator instead of plain `NSTextField` debug rectangles.

This remains a foundation rather than final art. Procedural humanoids, primitive
terrain/scenery, simple lighting, placeholder portrait silhouettes, and absent
animation/audio are the main visible differences from the Legacy presentation.

## Platform truth

The Old 3DS `main.cpp` remains a bring-up proof and does not run the complete
`RuntimeSession` chapter flow. This patch does not claim otherwise and does not
attempt a full 3DS port. Shared world, staging, training, collision, and chapter
data are portable; a reduced Citro3D/Citro2D consumer still needs implementation.

## Verification record

Verified in the available Linux environment:

- all shared `core/*.cpp` and `content/*.cpp` sources compiled as C++17 with
  `-Wall -Wextra -Werror -pedantic` for the unit-test, runtime-smoke, headless,
  and content-exporter targets;
- the core regression executable passed exact dialogue, opening Sage staging,
  field reset, the full seven-step refresher, field/arena/field return, road and
  river continuation, route state, and save round-trip checks;
- the static architecture test passed shared world ownership, genuine 3D Mac
  renderer markers, absence of the obsolete screen-space renderer functions,
  absence of web/mobile dependencies in shared C++, and absence of Legacy
  mission architecture filenames;
- runtime smoke and headless targets ran successfully;
- `review/content.json` exported and parsed as valid JSON;
- `Jimmy.glb` remained present and unchanged.

Not validated here: the CMake/CTest wrapper because CMake is unavailable; a
native Mac compile/run/DMG or visual capture because Apple SDKs and macOS are
unavailable; a 3DS binary because `DEVKITPRO` is unavailable. Consequently this
report does not claim native Mac visual parity, native Metal execution, or a
working full-flow 3DS build.
