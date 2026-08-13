# Parallels X 3.0R — U11–U13 Chapter 2 Restoration Candidate

This cumulative candidate keeps the Chapter 1 Golden Gate intact and restores the Chapter 2 Tournament Grounds on the same shared Mac/Linux/Old 3DS runtime.

## Player-facing changes

- Rebuilt Tournament Grounds as a compact seven-district, seven-phase hub anchored by one visible Main Arena.
- Restored Lost Bracket, Wade's timed shortcut, Bark's unresolved Cracked Ring material, registration, ceremony, official fights, spectator scenes and tournament aftermath.
- Added Wade's Lost Fan, Fake Champion, Runaway Training Dummy and repeatable Bark practice, with persistent results.
- Added Rrvvfo's pure Energy Beam and modest high-Energy strength state.
- Added the exhausted failed Fire Awakening and playable Plouke finish while preserving Plouke's canonical victory.
- Fire Awakening becomes player-usable after Chapter 2; Energy Beam becomes Solar Weave only while Awakening is active. Shots of Agony remains unavailable.
- Added shared directed cameras, recognizable lightweight Legacy-style tournament silhouettes, live bracket/next-match information, Recent Dialogue and safe held skip for previously seen scenes.

## Validation performed

- Full project script: 5/5 automated test groups passed; headless gameplay, runtime and content-export smokes passed.
- Separate clean CMake configure/build: passed; 5/5 tests and all three smokes passed again.
- macOS universal app: compiled, ad-hoc signed and packaged successfully as the included DMG.
- Old 3DS devkitPro build: compiled cleanly as the included 3DSX; static ELF size is approximately 1.52 MB plus a 273 KB cooked Rrvvfo ROMFS asset.
- Linux renderer sources: strict warning-as-error syntax check passed. A native Linux graphical link/run was not performed on this Mac host.

## Human verification still required

This is a candidate, not hardware certification. Test the full Chapters 1–2 route on Mac, Azahar and a real Old 3DS XL. On Old 3DS, specifically verify sustained frame pacing, repeated hub/fight returns, long-session memory/save behavior, suspend/resume, text and final-clash prompts, directed camera readability, named silhouettes, Rrvvfo's face/garment deformation and Reduced Motion/Reduced Flashes.

Chapter 3 and early character Replay are not included.
