# Parallels X 3.0R — Mac 0.4D: Legacy Visual Parity Reset

> Historical 0.4D milestone notes, superseded by `MAC_0.4D.1.md`. The Old 3DS
> shell described here was and remains a bring-up proof; it did not run the full
> `RuntimeSession` chapter flow.

## Scope

This milestone deliberately stops at the opening foundation: Sage Field, Sage's training/spar, and the opening Tournament Road. River and later 0.4C content remain connected for regression coverage, but this pass does not claim to finish their visual/content remakes.

## Direct Legacy comparison

| Player experience | Exact Legacy behavior | 0.4C loss | 0.4D reconstruction |
|---|---|---|---|
| First active lesson | Separate three-anchor Object Swap field trial | Missing | Restored as shared exploration data and reset-safe relay state |
| Sage refresher | Seven guided steps ending in three clean hits | Reduced to an immediate three-hit spar | Restored as a data-driven shared training state machine |
| Dash | Committed burst movement | Held movement-speed modifier | Finite edge-triggered burst in shared field movement |
| Flow Cancel | Melee hit recovery cancelled into Dash for Energy | Dash request did not have Legacy burst behavior | Hit window spends Energy and opens the shared burst |
| Dialogue cadence | Field lesson → tournament/manual → refresher → pride exchange → road argument | Pride exchange missing; road argument placed immediately after spar | Original placements restored |
| Field identity | Angled camera, broad training ground, sanctuary/focus props | Flat debug-map read | 38° composition, sanctuary, field ring, focus posts and bell |
| Road identity | Narrow authored trail, gate, river/bridge, trees, people and distant silhouettes | Broad abstract stripe with markers | Recognizable 2.5D composition and Legacy-position NPC stand-ins |
| Characters | Readable Sage/Rrvvfo silhouettes | Diamonds/debug markers | Replaceable humanoid stand-ins pending final 3D models |

## Clean implementation boundary

- `TrainingRegistry` describes the seven steps; it does not replace the engine.
- `RuntimeSession` executes training, exploration and arena transitions in the
  shared core; the Mac and desktop test shells consume it. The Old 3DS bring-up
  proof does not yet run that full flow.
- `FieldMovementSystem` owns burst dash and jump state without platform APIs.
- `ExplorationDefinition::blockersToDisable` removes hard-coded road gate IDs from runtime logic.
- Stable ability IDs remain separate from visible chronology order.
- The Mac shell owns only rendering, Cocoa input translation and UI layout.

## Intentionally not ported

- browser DOM/canvas state;
- mobile controls;
- global patch hooks and chapter replacement engines;
- sprite atlases that are being replaced by 3D models;
- the fake early `???` / Shots of Agony slot;
- route-selection popup;
- timeout-driven cutscene progression;
- Legacy save-patch chains;
- hard-coded relay gate names in shared runtime logic.

## Known remaining parity gaps

- Final character/environment 3D meshes, animation, facial acting, hit effects and audio are not available.
- The current Mac scene is a deliberately lightweight 2.5D Metal presentation, not the final 3D renderer.
- Roadside NPCs restore population and placement but do not yet have interaction/dialogue behavior.
- The opening spar restores the refresher structure, but full pursuit, aerial combat, clashes, Counter and Breaker remain later shared-combat work.
- A native Mac app/DMG and Old 3DS binary require their respective toolchains and were not produced in the Linux verification environment.

## Old 3DS judgment

The Legacy browser build itself is not a reasonable Old 3DS target. The rebuilt experience is: compact maps, small actor counts and arena combat suit the hardware. The compatibility risks are renderer/asset budgets—character animation memory, texture sizes, transparency, particles and draw calls—not the shared quest/training/combat state machines added here.
