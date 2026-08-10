# Historical 0.4E Chapter 1 parity implementation report

> Superseded by `docs/LEGACY_FOUNDATION_LOCK_0.4F_REPORT.md`. In particular, 0.4E incorrectly treated physical in-world route selection as an approved difference. 0.4F restores Legacy's `CHOOSE A ROUTE` panel and retains physical gameplay within the selected route.

Build: **Parallels X: Clash of Souls 3.0R Mac 0.4E — Chapter 1 Legacy Parity Port**

Legacy source of truth: **Parallels X Prototype 2.9A.40.7.1.1 — Chapter 2 Optional Fight Hub Return Fix**.

This is a source and portable-runtime parity candidate. It is not declared final visual parity because the native Mac/Metal app could not be built or compared in this Linux workspace.

## 1. Every Legacy Chapter 1 source examined

Build/story shell: `manifest.json`, `README.md`, `js/story/lost-year-data.js`, `lost-year-story.js`, `story-polish.js`, `story-reliability.js`, `story-engine.js`, `story-map.js`, `story-progression.js`, `story-rpg-ui.js`, `connected-world.js`, `revisit-loop.js`, `rpg-pacing.js`.

Rrvvfo Chapter 1 content: `js/story/rrvvfo-mission-0.js`, `rrvvfo-mission-1.js`, `rrvvfo-road-hub.js`. `rrvvfo-mission-2.js` was examined only to establish the Chapter 2 boundary.

Field/side systems: `js/story/hub-camera.js`, `hub-collision.js`, `hub-landmark-art.js`, `combat-manual.js`, `quest-variety.js`, `core-fun.js`, `field-skills.js`, `world-delight.js`.

Combat/stages/input: `js/arena/arena-stages.js`, `arena-stage-renderer.js`, `stage-personality.js`, `arena-mode.js`, `arena-combat-data.js`, `arena-math.js`, `pursuit-combat.js`, `arena-controls.js`, `js/combat-core.js`, `js/input-runtime.js`, `js/input.js`, `js/roster.js`.

The pre-implementation feature matrix and numerical ledger are in `docs/LEGACY_CHAPTER1_PARITY_AUDIT.md`.

## 2. Legacy gameplay reconstructed

- Sage Object Swap setup, three physical anchor swaps and result exchange.
- Sage opening, complete seven-step refresher and post-spar exchange.
- Training Field and Training Road stage/camera data.
- Physical departure, solid river and two-way far-bank rock trade.
- Main, Forest and Cliff geography. 0.4F restores the Legacy route-selection panel before physical traversal.
- Main worker/log Fire Blast event, Forest route and all three Cliff jump markers.
- Shared reconnection, exact three-position relay and authored gate delay.
- Transport wheel Object Swap rescue.
- Runaway Tournament Cart input sequence, time limit, safe retry and fail-forward.
- Seven roadside NPCs with authored positions, drift, prompts, repeat states and dialogue.
- Five birds, moving delivery cart and saved-cart world state.
- Lost Competitor help/decline branch.
- Optional challenger choice, first-to-one-KO fight, safe defeat rematch/leave and spectator-pass result.
- Tournament checkpoint, mandatory Lens roadblock and exact HP behavior.
- Tournament Outskirts arrival and canonical Chapter 1 completion.
- Persistent route, transport, cart rank, spectator pass, precision relay and Cliff rewards.
- Timed shared combat actions and Legacy-derived constants for ground/air normals, pursuit, defense, Energy, guard, combo scaling, clashes, wall/ground reactions, burst Dash, AI and Flow Cancel.

## 3. Legacy dialogue preserved exactly

All **67 spoken Chapter 1 lines** represented by these dialogue sets are preserved without semantic changes:

| Dialogue set | Lines |
|---|---:|
| Object Swap setup/result | 7 |
| Sage opening/post-spar | 14 |
| Tournament Road departure | 4 |
| Main Road worker event | 4 |
| Transport and runaway cart | 7 |
| Six non-branching roadside NPC conversations | 13 |
| Lost Competitor branch/repeat responses | 6 |
| Optional challenger/leave | 3 |
| Tournament checkpoint | 3 |
| Lens reaction | 1 |
| Tournament Outskirts | 5 |

UTF-8 curly apostrophes and emphatic capitalization are preserved. Automated tests pin the Object Swap exchange and the dialogue-set sizes; the registry is the platform-independent source consumed by the runtime.

## 4. Legacy dialogue changed

**None.** No spoken Chapter 1 line required a current-canon rewrite. Earlier 0.4D ASCII apostrophe normalization and shortened Object Swap wording were reverted to the exact Legacy content.

## 5. Legacy mechanics intentionally not ported

| Legacy item | Exact reason |
|---|---|
| Mission classes, mission IDs, completion panels, mission selector and mission replay | Obsolete player-facing architecture. 3.0R uses one continuous Story and whole-chapter Replay only. |
| Route-selection popup | Restored in 0.4F. Its removal was not authorized by the Foundation Lock. |
| Browser DOM, CSS overlays, JavaScript globals, event patch stack and `setTimeout` state flow | Obsolete technical architecture; behavior is shared portable C++. |
| Mobile/touch controls | Mobile gameplay is not a target. Semantic actions remain platform-neutral. |
| Legacy save-repair chains and browser storage | Replaced by the versioned shared save codec and explicit chapter flags. |
| Build Lab/adventure-mission menus and reward popups | Menu/mission architecture is obsolete. Canon-compatible gameplay results are stored as chapter flags instead. |
| Unreachable roadside escape-QTE function | No player-facing control in the exact build invokes it; the actual Leave choice is preserved. |
| Chapter 1 `fieldShotsOfAgony` unlock | Contradicts current chronology; Shots of Agony is invisible and introduced later. |
| Registration, cracked practice ring and Tournament Card acquisition | Explicit current-canon Chapter 2 boundary. |

## 6. Current-canon changes

- Shots of Agony and every unavailable ability are completely absent from the Chapter 1 hotbar.
- Chapter 1 stops at the Tournament Outskirts/arrival exchange.
- Registration, cracked ring and Tournament Card acquisition are not present.
- Hidden progression is stored as save flags; no Tournament Card UI appears.
- Legacy's `CHOOSE A ROUTE` panel is restored in 0.4F; route gameplay remains physical.

No Legacy spoken line was altered for canon.

## 7. Actual bug/jank fixes

- Removed scene-local HP/Energy resets that broke continuous progression.
- Field anchor completion no longer inherits the road gate's `0.52s` delay.
- Lost Competitor interaction can no longer be pre-empted by the optional fighter trigger.
- River remains collidable after Object Swap.
- The swapped rock trades to the player's old position instead of becoming a fake teleport target.
- QTE failure has a safe retry and then fail-forward; no softlock.
- Optional defeat always offers rematch or safe continuation.
- Final Chapter 1 dialogue closes into a stable chapter-complete state rather than looping its last line.
- Dynamic blockers and their presentation primitives disable from the same shared state.
- Sage/cutscene actors use authored transforms instead of default world origin.

## 8. Remaining map/layout differences

- Legacy coordinates are preserved in shared X/Z world units, but final terrain meshes and exact art silhouettes are not available.
- The three routes are physically selected rather than chosen from Legacy's popup.
- Environment geometry uses portable primitive definitions; final textures, cel-shaded materials, foliage, water current effects and prop meshes remain placeholders.
- Legacy yaw/FOV/distance/height values are present, but native Mac framing has not been visually compared side-by-side.
- No final free-camera/right-stick tuning has been validated.

## 9. Remaining combat differences

- Startup/active/recovery, normals, defense, Energy, Pursuit, clashes, scaling, guard and AI are in the shared core and exercised by tests.
- Renderer-side hitstop, final hit reactions, launch arcs, impact effects, animation cancels and wall/ground reaction animation remain placeholder or incomplete.
- Fire Blast currently resolves through the shared projectile attack state without a final traveling projectile mesh/effect.
- Melee/projectile clash resolution exists, but final clash presentation and all fighter-specific animation bindings are not complete.
- The optional road opponent uses the shared Balanced archetype; native feel tuning against Legacy remains unverified.

## 10. Remaining NPC/quest differences

- All seven NPCs, their interactions, motion and branch state are implemented.
- Character models, facial animation, portraits, voices and bespoke idle animations are placeholders.
- Birds, moving cart and saved supply cart use low-cost primitive presentation.
- Legacy adventure-mission/result overlays are intentionally absent; outcomes persist as save flags.

## 11. Files changed in 3.0R

Root/docs/review: `CMakeLists.txt`, `README.md`, `CHANGELOG.md`, `docs/LEGACY_CHAPTER1_PARITY_AUDIT.md`, `docs/CHAPTER1_LEGACY_PARITY_REPORT.md`, `docs/STORY_SOURCE_RULES.md`, `docs/ARCHITECTURE.md`, `docs/NEXT.md`, `review/README_FOR_REVIEWERS.md`, `review/content.json`.

Shared content: `src/content/adventure_registry.hpp`, `adventure_registry.cpp`, `chapter_registry.cpp`, `character_presentation_registry.cpp`, `cutscene_registry.cpp`, `dialogue_registry.cpp`, `exploration_registry.hpp`, `exploration_registry.cpp`, `map_registry.cpp`, `world_presentation_registry.hpp`, `world_presentation_registry.cpp`.

Shared core: `src/core/ability_hotbar.cpp`, `combat.hpp`, `combat.cpp`, `field_movement.cpp`, `runtime.hpp`, `runtime.cpp`.

Platforms/tools/tests: `src/platform/macos/main.mm`, `src/platform/desktop/headless_main.cpp`, `src/platform/desktop/runtime_smoke.cpp`, `src/platform/3ds/README.md`, `src/tools/content_export.cpp`, `tests/core_tests.cpp`, `tests/source_architecture_tests.py`.

## 12. Automated tests added/run

Passed with C++17 `-Wall -Wextra -Werror -pedantic`:

- exact opening staging/dialogue and field-trial reset;
- all seven Sage refresher steps;
- meaningful movement, action-start versus connected Grab, telegraph and Lens preparation/read;
- timed attack startup/active/recovery and representative combat mechanics;
- Main route complete path through cart, NPC decision, optional encounter, checkpoint, Lens and outskirts;
- Forest and Cliff physical routes and reconnection;
- river collision and physical rock trade;
- relay coordinates/gate delay, QTE success and persistent outcomes;
- save serialization and canonical completion flag;
- shared world/adventure ownership;
- no browser/mobile dependency, obsolete renderer path or Legacy mission filename;
- truthful 3DS boundary.

Also passed: desktop runtime smoke, headless chapter/content listing, content exporter and JSON validation.

## 13. Mac build result

**Not built.** The source uses Cocoa/Metal and this workspace is Linux without Apple's SDK. Shared C++ and source-boundary tests pass, but no claim is made that the native `.app` or DMG compiled, launched or matched Legacy visually.

## 14. Old 3DS portability concerns

- New gameplay/content/state code is portable C++17 and does not depend on Metal, Cocoa, DOM or mobile APIs.
- World primitives and character bindings are shared definitions suitable for a reduced Citro3D/Citro2D consumer.
- Main risks remain model/animation memory, texture budgets, particles/transparency, draw calls, crowds and world-marker density.
- Birds/carts and full-detail scenery need a reduced presentation tier, not different gameplay.
- Corrected by 0.4H.2 after rejecting the 0.4H.1 package: the 3DS program now embeds its ROMFS, blocks on a missing model, instantiates `RuntimeSession`, renders the shared Chapter 1 stage definitions through native perspective Citro3D and presents the live cooked Rrvvfo model through a native Legacy-style front end. Azahar boot, route model, dialogue, exploration and early training-field UI have been verified. Full Chapter 1 playthrough, memory, suspend/resume and sustained performance still require actual Old 3DS XL hardware.

## 15. Parity checklist

| Area | Implemented | Verified here | Notes |
|---|---:|---:|---|
| Scene order and continuous Chapter flow | 100% | 100% | Ends at Outskirts; no mission surfaces. |
| Spoken dialogue | 100% | 100% source-level | 67 lines; no canon edits. |
| Traversal/routes/triggers | 98% | 98% | All three routes complete; native feel comparison pending. |
| NPCs/side encounter/world life | 97% | 95% | Logic complete; presentation placeholder. |
| Rewards/save progression | 94% | 94% | Outcomes saved; old mission overlays excluded. |
| Shared combat rules | 92% | 90% | Core mechanics present; final motion/feedback bindings incomplete. |
| Camera/map presentation data | 90% | 78% | Legacy values and geometry present; Mac visual comparison unavailable. |
| Final 3D visual presentation | 68% | 35% | Primitive world/actors; final models, materials, animation and effects pending. |

**Portable gameplay/content reconstruction: 95%.**

**Overall Chapter 1 remake parity estimate, including unfinished presentation: 89%.**

The acceptance statement “Yeah. That's Chapter 1.” still requires a native Mac playthrough against the exact Legacy build. This report does not claim final Legacy parity from architecture or placeholders alone.
