# 3.0R Technical Stack Lock

## Shared core

C++17 + CMake. No chapter-specific engines. No browser runtime dependency.

## macOS — lead build

- Cocoa window/input shell
- Metal/MetalKit renderer
- shared `px_core` runtime
- full desktop asset tier
- stable DMG packaging name: `Parallels-X-Clash-of-Souls.dmg`

The Mac shell now renders shared world-presentation definitions through Metal using a perspective camera, world-space meshes, lighting/fog inputs and a depth buffer. Its current scenery is primitive-based and its actors are procedural world-space stand-ins, so this is a genuine 3D foundation but not final visual parity or final art.

## Old 3DS XL — native parity target

- libctru
- Citro3D for 3D
- Citro2D for UI and lightweight 2D work
- cooked low-memory model/texture format
- bottom screen for map, Tournament Card, quests, menus and hotbar information
- all combat actions remain usable with physical controls
- L acts as the ability/hotbar modifier on Old 3DS

The checked-in 0.4H.2 Old 3DS program instantiates the full shared `RuntimeSession` and consumes the same stage, camera, blocker, dialogue, combat, character-presentation and save contracts as desktop. Citro3D renders the authored perspective world and byte-identical cooked Rrvvfo mesh with a depth buffer. Native Citro2D presentation adapts Legacy's title, menus, dialogue, objective, manual, hotbar, pause and save language; no debug console is part of the release path. The 3DS tier may reduce model detail, scenery density, particles and effects, but it must not change the game design, map flow or combat system.

The native `.3dsx` has passed an Azahar smoke test. A full playthrough and profiling pass on an actual Old 3DS XL remains required before hardware compatibility is claimed.

## Windows + Linux

Later desktop shells use the same `px_core` and desktop asset tier. Their renderer/window backends remain platform code only.

## Asset rule

Blender is the master source. Desktop can use a high-quality cooked export derived from GLB. 3DS receives a lighter cooked export generated from the same model, rig and animation sources.

## Why native platform shells

The Legacy browser game proved the design, but browser-specific patches, touch controls and chapter-local behavior made consistency harder. 3.0R keeps OS/device APIs at the edge so combat/story/map fixes are shared everywhere.
