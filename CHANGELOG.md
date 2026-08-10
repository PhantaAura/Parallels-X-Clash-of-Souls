# 3.0R 0.4H — Golden Gate QoL

## 0.4H.5 candidate — playable Fight/Training content entry

- Promoted `FIGHT` and `TRAINING` from nonfunctional menu shells to playable shared-runtime destinations on Mac, Linux and 3DS.
- `FIGHT` launches Rrvvfo directly against the existing balanced CPU in the roadside arena, with Story movement, HUD, HP/Energy/Guard, attacks, Pursuit, Flow Cancel and Chapter 1 techniques intact.
- `TRAINING` launches the existing Sage challenge and returns to the menu without continuing or overwriting Story.
- Prevented standalone Fight/Training sessions from being merged into the Story save on every platform shell.
- Added runtime, menu and platform-boundary regression coverage for both direct modes.
- Promoted the latest available browser build, `2.9A.40.7.2R`, to content authority and recorded the concrete remaining Chapter 1 route parity gaps.

## 0.4H.4 candidate — Chapter 1 pacing and optional-story pass

- Made the Sage's tutorial explicitly optional with Full Challenge, Quick Ability Refresher and Skip routes; checkpoint saves add a fourth Resume choice.
- Reframed the seven full lessons with Legacy-style Sage challenge headings and character barks without changing their instructional requirements.
- Completed the lost-competitor story: winning the optional road fight now returns Rrvvfo to the road with the pass, and progression waits for the nearby promised delivery and dialogue payoff.
- Added `The Sign That Points Back`, a short optional outskirts story using Lens and Object Swap with a persistent corrected sign and Wayfinder Badge reward.
- Added shared save flags and regression coverage for tutorial skip, pass delivery and sign-puzzle completion.
- Expanded the 3DS training menu to display all four checkpoint choices without changing gameplay or content by platform.
- Made the strict Linux build script compatible with the Bash version bundled by macOS and silenced two legitimate cross-compiler unused-field diagnostics.
- Passed all five shared suites, rebuilt the universal Mac app/DMG and cross-compiled the native 3DS candidate. Azahar/Old 3DS visual and hardware approval remain required.

## 0.4H.2 — Legacy UI and real-model 3DS correction

- Rejected and superseded the 0.4H.1 package after emulator review showed that its distributable did not reliably embed/present the promised final UI and model path.
- Embedded both SMDH metadata and ROMFS in the `.3dsx`; a missing Rrvvfo asset now blocks startup instead of launching a fake playable fallback.
- Replaced 3DS debug/console presentation with a native Citro2D adaptation of Legacy's title, mode/route, dialogue, objective, Combat Manual, hotbar, pause and save language.
- Added the real cooked/skinned Rrvvfo 15 model to the Legacy route screen and retained the exact same byte-identical model, 39-joint rig, 26 clips and lightweight expression geometry in gameplay.
- Corrected the raw Citro3D-to-Citro2D GPU handoff, removing screen-filling triangles and restoring stable two-screen overlays.
- Kept 3DS optimization presentation-only: bounded scenery/tessellation and restrained face/effect work; no silhouette, outfit, gameplay, personality, map, story or combat substitution.
- Cross-compiled the embedded `.3dsx` and verified title, front end, real-model route screen, Chapter 1 field, Legacy dialogue and utility UI in Azahar at 59–60 application FPS. Real Old 3DS XL profiling remains required.

- Replaced Rrvvfo's frozen ring-facing yaw with character-aware facing: exploration follows real movement and dash direction, combatants face one another, retreating preserves opponent focus with reversed run playback, and Object Swap immediately restores the correct orientation on Mac, Linux and 3DS.
- Added Rrvvfo's approved eyes/brows/mouth as a lightweight head-attached cel layer with neutral, blink, confident, focused, grit, hurt and shout states. The source GLB, head shape, hair, outfit, 39-joint rig and cooked 3DS model remain unchanged; the layer stays within a 48-triangle portable budget.
- Repaired the `rrvvfo15.glb` body, shirt and jacket weights without changing geometry or the 39-joint rig; torso layers now share the spine chain, garment panels no longer inherit forearm/pelvis motion, and all 26 clips pass the new garment-coherence regression gate.
- Added a shared interactive pause menu with Resume, safe Save Game, Restart from Checkpoint, Objective History, Controls, Accessibility & QoL and Return to Title.
- Added schema-4 save migration, atomic writes, `.bak` recovery and platform-isolated save locations. Existing Mac schema-3 and Linux 0.4G development saves remain importable.
- Added first-time hints, hold-to-advance dialogue, reduced motion/shake/flashes, high-contrast HUD, larger text and combat-message detail settings.
- Added dedicated collision maps for the Legacy Sage tutorial arena and optional roadside challenge ring while preserving the exact road return position.
- Added restrained dash steering without changing Legacy dash speed/duration, jump physics, combat values or Chapter 1 route geometry.
- Added controller-disconnect pausing on Mac/Linux and corrected Mac mouse input so held clicks no longer retrigger as new presses every frame.
- Added 3DS title/pause/save utility presentation and touch shortcuts while retaining complete physical-button access.
- Repaired the 3DS Makefile include configuration and current libctru timer type; a real `parallels_x_3ds_golden_gate.3dsx` now cross-compiles successfully.
- Kept every non-face part of Rrvvfo and all Chapter 2 content out of scope. Chapter 2 remains blocked until Chapter 1 receives final Mac/Legacy and Old 3DS hardware approval.

# 3.0R 0.4G GOLD — Chapter 1 Gameplay Remaster / Old 3DS Gate

- Promoted Chapter 1 into the first gameplay-remaster golden slice while preserving Legacy story, map, combat identity and ability chronology.
- Replaced the provisional combat buffer with Rrvvfo's exact Legacy 0.135-second input-buffer value and retained buffered commands through actual shared hit freeze.
- Added shared combat feel feedback for normal hits, Perfect Block, Guard Break, Pursuit finishers, wall/ground reactions, final hits, clashes and Flow Cancel.
- Added road-combat knockback/launch/landing presentation, exact pre-fight return position, compact checkpoints, adaptive Cliff guidance, World Delight, Object Swap mastery and small capped field-target forgiveness.
- Added Replay-only skipping for previously seen Chapter 1 directed cutscenes with persisted seen-scene state.
- Expanded Rrvvfo to 26 deterministic Legacy-timed shared clips, including locomotion, combat, defense, Charge, Counter, Breaker, Fire Blast, Object Swap and Lens.
- Added shared three-band cel-lighting direction and runtime ability-effect markers consumed by Mac/Linux renderers.
- Brought Mac/Linux controller layouts into the same modern semantic profile, including the LB ability layer and alternate LT Dash; Linux remaps held controls safely when the layer changes.
- Replaced the old isolated 3DS proof with Gate 0: the same Chapter 1 `RuntimeSession`, ROMFS copy of the exact cooked Rrvvfo model, shared skeletal animation sampling, live CPU-skinned model inset, shared save/load and full Old 3DS control mapping.
- Gate 0 source is ready for a real devkitPro build/hardware test; no `.3dsx` is claimed in the Linux environment where the 3DS toolchain is unavailable.

# 3.0R 0.4G-A — Chapter 1 Gameplay Remaster Foundation

- Added shared jump buffering/ground grace without changing Legacy jump physics.
- Added a provisional combat follow-up buffer, later replaced by the exact 0.135-second Legacy Rrvvfo value in 0.4G GOLD.
- Added shared gameplay notices for Flow Cancel timing and road discoveries.
- Moved the Chapter 1 cliff reward/discovery from route selection to actual route completion.
- Added escalating cliff-route guidance that stays quiet until the player repeatedly needs help.
- Preserved the current Rrvvfo model, skeletal animation foundation, Chapter 1 story flow and ability chronology.

# Changelog

## 0.4H.1 — Old 3DS gameplay-parity correction

> Historical rejected artifact. Superseded by 0.4H.2 after direct Azahar review found that the packaged UI/model presentation did not meet the claimed parity boundary.

- Replaced the Gate 0 top-down dot-map/model-inset artifact with a native Citro3D perspective renderer.
- Reused the same authored world stages, cameras, blockers, actor transforms and runtime state as Mac/Linux.
- Added full cooked/skinned Rrvvfo rendering, lightweight face expressions, Sage/NPC presentation, world markers and combat feedback on 3DS.
- Kept Old 3DS reductions presentation-only by prioritizing `Essential` geography, bounding the shared `Full` scenery layer and lowering curved-primitive tessellation.
- Reworked the bottom screen around actual Chapter 1 dialogue, objectives, choices, QTEs, Combat Manual, hotbar and pause/save information instead of model diagnostics.
- Added parity guards that reject the former alternate-looking diagnostic path.
- Cross-compiled successfully and verified the perspective field build in Azahar at full emulated speed.

## Rrvvfo clean-weight DEV model integration

- Replaced the active faceless DEV source with the artist-cleaned `rrvvfo.3.glb` export while preserving the 39-joint skeleton and shared character pipeline.
- Recooked the desktop character asset: 2,524 cooked vertices, 3,206 triangles, 7 submeshes, 6 materials, 39 joints and the existing 0.75-second Legacy idle clip.
- Preserved rigid shoe weighting to `foot.L`/`foot.R` and removed the jacket cuff finger/thumb deformation carried by the older auto-weighted export.
- Hardened model validation so every cooked gameplay vertex must be normalized and skinned; the unskinned Training Suit reference remains excluded from gameplay.
- Re-rendered deterministic idle review poses with the cleaned weights. No new animation, gameplay, environment, face or Chapter 2 work is included.

## Shared skeletal animation foundation — idle proof

- Added one reusable shared skeletal player with named clips, loop/non-loop timing, playback speed, state switching, bind fallback, LINEAR/STEP vector sampling, quaternion slerp, hierarchy evaluation and CPU skin matrices.
- Extended the deterministic cooked character format to retain clip/track/key data without changing Rrvvfo's source GLB, skeleton, geometry or weights.
- Authored one `idle` proof clip from Legacy's six-frame idle through a deterministic asset-generation script and source sidecar; no choreography was hardcoded into gameplay or either renderer.
- Wired the same shared animation player and cooked data into the Linux software and macOS Metal character paths.
- Added bind/sampled Linux proof frames plus focused loader, sampling, loop, speed, fallback and stable-deformation tests.
- Did not add run, dash, jump, attacks, abilities, facial work, environment polish or Chapter 2.

## Rrvvfo 3D integration — Phase 1 visual acceptance repair

- Preserved the supplied faceless GLB, `CharacterModelAsset`, `CharacterModelRepository`, deterministic cooker, 39-joint rig/weights, shared Mac/Linux character pipeline, and importer validation tests unchanged.
- Rebuilt the shared Chapter 1 training-field presentation from the exact Legacy stage and supplied screenshot: elevated turf/base, tile treatment, central ring/cross, low rails, block banks, trees, and fighter staging.
- Added world-space Linux model projection driven by the shared Legacy-derived perspective camera so Rrvvfo has the correct screen importance and feet placement.
- Replaced Sage's stick figure with an intentional mentor blockout and removed debug guides, generic diagonal bands, and the debug review banner from the player-facing comparison.
- Added deterministic `rrvvfo-legacy-comparison` capture and regression coverage.
- Kept Rrvvfo in bind/T-pose and did not start animation, facial work, abilities, Chapter 2, or gameplay changes.

## Rrvvfo 3D integration — Phase 1

- Replaced only Rrvvfo's procedural gameplay stand-in with the supplied, intentionally faceless `rrvvfo(5).glb` DEV model.
- Added a deterministic GLB cook that includes only the `metarig` character tree and preserves 39 joints, hierarchy, inverse bind matrices, joint indices and normalized skin weights.
- Added an engine-owned `CharacterModelAsset`/`CharacterModelRepository` shared by Mac and Linux; model data loads once rather than once per frame.
- Added shared character binding for source/cooked assets while leaving collision, movement, combat, camera, progression and Chapter 1 logic unchanged.
- Added flat/cel-compatible material rendering and a replaceable material fallback sidecar because the GLB's named Red/Maroon/Skin/Hair/Head slots do not export base-color factors.
- Added the deterministic Linux `rrvvfo-model` review state and screenshot.
- Added cooked-asset, rig, material, skin-weight, deterministic-recook and platform-boundary tests.
- Did not add a face, animations, abilities, Chapter 2 content or Rrvvfo-specific gameplay logic.

## 3.0R Mac 0.4F.1 — Legacy Presentation + Continuity Lock

- Added the proper Parallels X title/start screen and shared one-mode-at-a-time horizontal carousel in the exact ten-mode order.
- Added contextual Story So Far and a ten-section, 5–7 minute Season 1 recap with replaceable visual IDs and keyboard/controller navigation.
- Added a shared Story Character carousel with fresh-save Rrvvfo-only visibility, semantic Bark/Wade/Virek discovery, save migration and queued unlock notices.
- Removed fabricated unfinished-route chapter placeholders; discovered unfinished routes now show `STORY COMING LATER` without launching fake gameplay.
- Added the real categorized combat manual hierarchy and exact distinct first/retry Perfect Block and Lens timing values.
- Added shared Parallels X UI composition data consumed by Mac and Linux; the Mac shell no longer boots directly into Chapter 1.
- Added a graphical SDL2 Linux validation executable, isolated saves, headless software rendering, deterministic review states and screenshot capture scripts.
- Reframed the Lost-Year conspiracy as the Organization of the Red using stable ID `organization_red`, with compatibility migration for the deprecated development alias.
- Reserved Online Play structurally and labeled it `COMING LATER`; no networking or Pretendo compatibility is simulated or claimed.
- Preserved Chapter 1 and the continuous Story boundary; Chapter 2 remains unimplemented.

## 3.0R Mac 0.4F — Legacy Foundation Lock

- Made exact Legacy `2.9A.40.7.1.1` the primary Rrvvfo Chapters 1–4 story/gameplay source; current route documents are continuity overrides only.
- Added complete dependency-traced Chapter 2, 3 and 4 parity audits without implementing those chapters.
- Restored Legacy's `CHOOSE A ROUTE` panel before physical Main/Forest/Cliff traversal.
- Restored tutorial manual sections, Guided/Resume/Quick modes, exact checkpoint bands and serialized retry/reload behavior.
- Matched the `1.35s` Perfect Block cycle and deferred Lens-evade success until the predicted attack safely resolves.
- Added phase-gated player/opponent HP, Energy, Guard and hotbar UI state, plus interaction and pause information surfaces.
- Extended save/reload with HP, Energy, Guard and pending continuous Chapter 2 target.
- Added shared Legacy-derived Story progression/Tournament Card math and generic quest/party state interfaces for later chapters.
- Preserved the full 0.4E road content and exact Chapter 1 dialogue while removing 0.4E documentation claims that the physical fork was approved parity.
- Kept Chapter 2–4 gameplay unimplemented as required.

## 3.0R Mac 0.4E — Chapter 1 Legacy Parity Port

- Completed a source-level audit of the exact Legacy `2.9A.40.7.1.1` Chapter 1 path before implementation.
- Replaced invented 0.4D road scenes with the Legacy river, three routes, relay, transport, cart, optional encounter, checkpoint, Lens roadblock and outskirts progression.
- Restored the seven Legacy road NPCs, exact dialogue, authored motion, birds, moving delivery cart and saved-cart world state.
- Added a shared `AdventureRegistry` for route, NPC, QTE, encounter and ambient-life content.
- Restored the physical far-bank rock trade, exact relay positions and `0.52s` gate release.
- Added the Main Fire Blast event, Forest traversal and all three Cliff jump checks.
- Restored the four-input Runaway Tournament Cart sequence with safe retry and fail-forward.
- Restored the Lost Competitor choice and optional first-to-one-KO roadside fight, including safe defeat recovery.
- Expanded the shared combat core with Legacy normal timing, aerial/pursuit actions, Block/Perfect Block, Counter, Breaker, Charge, burst/Air Dash, Flow Cancel, guard pressure, combo scaling, clashes, wall/ground reactions and AI archetypes.
- Preserved exact Legacy Chapter 1 dialogue and UTF-8 punctuation; no spoken line required a canon rewrite.
- Kept Shots of Agony and all unavailable abilities invisible.
- Saved transport/cart/pass/route/relay/Cliff outcomes without exposing mission architecture or the Tournament Card.
- Removed the invented energy-signature post and Bark/Wade reunion from Chapter 1.
- Kept Chapter 1 completion at the Tournament Outskirts; registration and Card acquisition remain Chapter 2.
- Added strict end-to-end tests for all three routes, training, NPC choice, QTE, optional encounter, Lens gate, outskirts completion, saves, combat behavior and architecture boundaries.
- Native Mac/Metal and Old 3DS builds remain unverified in this Linux environment; the 3DS shell is still a bring-up proof, not the full chapter runtime.

## 3.0R Mac 0.4D.1 — Legacy Opening Parity Correction

- Re-opened exact Legacy `2.9A.40.7.1.1` stage and refresher code before correcting 0.4D.
- Replaced the Mac screen-space `project()` renderer with perspective view/projection matrices, world-space triangle meshes and depth testing.
- Added shared `training-field` and `training-road` presentation registries with Legacy camera, floor, sanctuary, focus pillars, bell, road, river, bridge, gate, prop, tree and ambient-actor transforms.
- Added a replaceable character presentation/model-binding boundary. `Jimmy.glb` remains retained but is not parsed or rendered yet.
- Authored opening Rrvvfo/Sage staging inside cutscene content, preventing Sage from falling back to world origin.
- Restored the two shortened Object Swap lines and exact Legacy result dialogue.
- Restored the accumulated 72-unit movement check; tiny movement no longer completes Step 1.
- Changed Step 2 so Light, Heavy and Launcher count when their actions start while Grab still must connect at close range.
- Expanded the defense telegraph to a readable authored window.
- Restored Step 6 preparation: 35 Energy → charge to 60 → activate Lens → evade or Perfect Block the predicted attack.
- Kept Step 7 at exactly three connected, unblocked hits.
- Reduced the early chronology hotbar to Fire Blast / Object Swap / Lens; Shots of Agony and Solar Weave are not displayed.
- Replaced utility-style text rectangles with game-owned objective, dialogue, hotbar and combat HUD panels prepared for portraits, expressions, focus and advance state.
- Documented that Old 3DS remains a low-level bring-up proof and does not yet run `RuntimeSession` chapter flow.
- Locked continuous Story Mode and whole-chapter-only replay; Legacy mission classes and mission replay were not ported.

## 3.0R Mac 0.4D — Legacy Visual Parity Reset

- Inspected the exact Legacy 2.9A.40.7.1.1 opening implementation before changing the remake.
- Restored the separate three-anchor Sage Field Object Swap trial and its surrounding dialogue.
- Restored the complete seven-step Sage refresher instead of the former three-hit debug spar.
- Added data-driven training definitions consumed by the shared runtime.
- Replaced held-Shift run speed with a finite, edge-triggered burst dash shared by field and arena movement.
- Preserved Flow Cancel as melee recovery → energy spend → burst dash.
- Added shared Grab behavior and training coverage.
- Restored the six-line post-spar exchange and moved the four-line road-release exchange back to the road handoff.
- Added reset-safe training/relay state and data-driven blocker release.
- Reframed the Mac renderer around the Legacy 38° camera language and closer character/world scale.
- Added Sage sanctuary, training field, focus posts, bell, torii-style road threshold, narrow trail, physical river/broken bridge, trees, mountains, anchors and Legacy-position roadside NPC stand-ins.
- Replaced diamond player/debug markers with replaceable humanoid presentation stand-ins; no sprite/atlas pipeline was ported.
- Kept the early four-ability chronology hotbar and did not expose Shots of Agony.
- Added end-to-end regression coverage for field trial reset, all seven training steps, Perfect Block timing, arena return, road handoff, river crossing, route choice and relay continuation.
- Kept save schema v2; no platform or browser dependency was added to gameplay code.

## 3.0R Mac 0.4C — Grand Adventure I

- Replaced the early fake/locked Shots slot with a true four-slot Rrvvfo Story hotbar.
- Early display order is Fire Blast / Object Swap / Lens of Truth / Solar Weave.
- Added the later five-slot layout contract: Shots of Agony inserts at display slot 2 when it is actually invented; Object Swap/Lens/Solar shift to 3/4/5.
- Added display-slot → ability-id resolution so Mac and 3DS controls can follow the visible bar without changing gameplay ability identities.
- Added a shared `ExplorationRegistry`; Chapter 1 traversal rules are data consumed by the same runtime rather than Mac-only logic.
- Wired Sage marker interaction → Tournament Road departure → broken river → Object Swap crossing.
- River remains solid after crossing, matching Legacy's final behavior instead of turning water into floor.
- Replaced Legacy's route-choice popup with a physical Main / Forest / Cliff fork.
- Added route-specific traversal state. Main Road currently requires Fire Blast at the fallen log; Forest and Cliff use their physical branches.
- Added the three-position precision Object Swap relay and full-width relay gate blockers.
- Added the first Lens shortcut-or-detour runtime rule.
- Added current-area/objective detail to the Mac developer HUD.
- Improved the developer map renderer so the river and three route bands are spatially readable before final environment meshes exist.
- Expanded the AI-review content export with hotbar and Grand Adventure exploration data.
- Added regression coverage for chronology, river collision, physical route choice, Main Road field ability and swap relay.
- Selected X-emblem app icon remains intentionally **unimplemented**.

## 3.0R Mac 0.4B — Shared Hotbar Contract

- Added a platform-independent five-slot ability hotbar catalog to `px_core`.
- Established the canonical ability identities used for later progression.
- Added Mac HUD consumption of the shared hotbar data.
- Added hotbar regression tests.
- Kept stable package filename `Parallels-X-Clash-of-Souls.dmg`.

## 3.0R Mac 0.4A

- Promoted macOS to the lead graphical development target.
- Removed Web/mobile gameplay platform folders from the active project.
- Added `RuntimeSession`, a platform-independent playable scene runtime.
- Added Legacy Chapter 1 dialogue registry.
- Added opening → Sage spar → post-spar → exploration runtime flow.
- Added a Cocoa + Metal macOS app shell driven entirely by the shared runtime.
- Added stable DMG packaging script.
- Added macOS GitHub Actions build workflow.
- Kept Old 3DS XL as the compatibility gate developed alongside Mac.
