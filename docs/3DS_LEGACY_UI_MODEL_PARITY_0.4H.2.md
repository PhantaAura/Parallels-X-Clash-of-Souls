# 3DS Legacy UI and Rrvvfo model correction — 0.4H.2

## Why this correction exists

The distributed 0.4H.1 candidate did not meet the agreed boundary of “the same game with a lower visual tier.” Direct Azahar inspection found unreliable asset packaging and presentation that could expose a debug/fallback-looking character and interface. That candidate is rejected and must not be distributed.

## Corrected release boundary

- The `.3dsx` embeds SMDH metadata and its ROMFS instead of assuming an external asset folder.
- The 3DS loads the same cooked `rrvvfo-dev.pxskel` bytes used by desktop: 2,524 vertices, 3,206 triangles, 7 submeshes, 6 materials, 39 joints and 26 Chapter 1 clips.
- A failed ROMFS/model load blocks the build with an explicit error. There is no playable substitute Rrvvfo.
- The repaired shirt/jacket weights, silhouette, proportions, hair, outfit and animation personality are preserved.
- The lightweight eyes, brows and mouth remain attached to the gameplay head joint and stay within the 48-triangle face budget.
- The Legacy-style route screen presents the live cooked/skinned model, not unrelated character art.
- Title, mode/route selection, dialogue, objectives, Combat Manual, hotbar, pause and save screens use a native two-screen adaptation of Legacy's blue field, hard white/black cards, yellow selection strips, speaker/emotion tabs and compact RPG utility layout.
- Gameplay still comes from the shared `RuntimeSession`, registries, authored perspective camera, map/blocker state, dialogue, movement, combat and save codec.

## Performance-safe model policy

3DS optimization may reduce hidden geometry, material/draw-call count, distant expression detail, particles, transparency, environment density and curve tessellation. It may not change Rrvvfo's recognizable hair/jacket/proportions, red-black silhouette, gameplay poses, eyes/mouth readability, mechanics or personality. The current Gate keeps the complete cooked model because Azahar remains at 59–60 application FPS in the inspected opening; actual Old 3DS measurements decide whether a silhouette-safe lower LOD is necessary.

## Renderer correction

Citro3D owns the perspective world/model pass. Citro2D is explicitly re-prepared and flushed afterward before it draws either UI target. This fixes the screen-filling triangles caused by carrying raw PICA state into the two-dimensional overlay pass.

## Verified in Azahar

- embedded `.3dsx` boot and Homebrew metadata;
- Legacy-style title and front end;
- real-model Rrvvfo route screen;
- shared Chapter 1 Sage Training Field;
- stable Rrvvfo/Sage staging and authored depth camera;
- Legacy dialogue layout;
- objective/hotbar/save/pause utility presentation;
- 59–60 application FPS at 100% emulation speed in inspected opening scenes.

## Still required on physical hardware

- complete Chapter 1 playthrough;
- sustained field/tutorial/combat frame pacing;
- memory stability and load times;
- every physical control and touch shortcut;
- save/load/backup migration;
- sleep/wake and Home Menu return;
- visual artifacts across every map/camera;
- final decision on silhouette-safe LODs, texture atlases, particles and environment budgets.

Chapter 2 remains blocked until Chapter 1 passes the Mac/Legacy recognition gate and this real Old 3DS XL gate.
