# 3.0R Mac 0.4F.1 — Legacy Presentation + Continuity Lock

Authority: exact Legacy `2.9A.40.7.1.1`, the 0.4F Foundation Lock, the supplied Menu and Story So Far plan, and Season 1 continuity. Chapter 2 is not implemented.

## Player-facing presentation

- Boot now enters a Parallels X title screen with `PRESS ANY BUTTON`, then the shared Mode Select state.
- Mode Select is a one-at-a-time horizontal carousel with sharp diagonals, X motifs, charcoal/crimson/ember/gold/off-white colors and responsive non-blocking slide motion.
- Exact order: Story Mode, Arena Battle, Online Play, VS CPU, 2 Player, Training, Extras, Options, Credits, Arcade.
- Story So Far is a contextual Story Mode action, never an eleventh mode.
- Online Play and Arcade are shown honestly as `COMING LATER`; no networking is simulated.
- Story Character Select is another one-character-at-a-time carousel. Fresh saves show Rrvvfo only.

3DS online target: investigate Pretendo compatibility in a later networking milestone.

Mac, Windows and Linux networking are also later milestones. No Pretendo or desktop-networking compatibility is claimed in this build.

## Story routes and persistence

| Route | Discovery trigger | Current build behavior |
|---|---|---|
| Rrvvfo | Available by default | Begin/Continue continuous Story; whole-chapter Replay only |
| Bark | Rrvvfo's Chapter 2 reunion with Bark | Appears after discovery; `STORY COMING LATER` |
| Wade | Rrvvfo's Chapter 2 reunion with Wade | Appears after discovery; `STORY COMING LATER` |
| Virek | Rrvvfo notices Virek's Emerald is no longer on the island | Appears after discovery; `STORY COMING LATER` |

Generic chapter completion never discovers Virek. Semantic flags persist through save schema 3, reconstruct safe unlocks for sufficiently progressed development saves and queue a short `NEW STORY UNLOCKED` notice for the next suitable control transition.

No Bark, Wade or Virek route chapter was fabricated. The route registry supports discovery and honest presentation without fake missions or content.

## Story So Far

The optional recap uses ten replaceable, data-driven sections:

1. The Brothers
2. The Asrylyte
3. Two Sides of the Conflict
4. Rrvvfo's Burden
5. The Fake Death
6. Shadow and Energy Power
7. Virek
8. The Oddballs
9. The Fall of Perfection
10. After the Battle

It provides Previous, Next, section selection, Skip and Exit without changing Story progress. Thirty-four timed frames target approximately 5–7 minutes. Each frame references a replaceable visual ID; final models or illustrations can change without changing recap logic. Forbidden later revelations and Season 2 events are excluded.

## Chapter 1 manual, tutorial and HUD

The combat manual now exposes actual categories, pages and individual entries for movement, basic combat, kinetic combat, resource control, defense, current Rrvvfo techniques, inputs and `HOW THE REFRESHER WORKS`. It includes keyboard/controller prompts and the Guided/Resume/Quick entry hierarchy.

The portable runtime distinguishes the exact Legacy refresher schedules: first Perfect Block attempt at `1.35s`, retries at `1.45s`, final warning window `0.32s`; Lens first attempt `0.90s`, retries `1.10s`, warning `0.32s`, and post-evade confirmation `1.05s`. Existing movement, attack-start/connect, charge, ability, Lens, clean-hit and checkpoint behavior remains covered by end-to-end tests.

HUD/dialogue layout preserves Legacy information priority: contextual objectives, phase-gated health/Energy/Guard/opponent channels, chronology-correct hotbar, interaction prompt, clean speaker hierarchy and no unavailable abilities. Portraits, model renders, final icons, final audio and final device glyph art remain replaceable placeholders.

## Organization of the Red reset

The canonical faction is now `organization_red`. The Lost-Year conspiracy is a regrouped surviving branch of the Season 1 Organization of the Red, preserving tournament sabotage, the hidden facility, cloning/replicated combatants, unstable teleportation, Echo and the ruined post-game teleporter concept. See `ORGANIZATION_OF_THE_RED_LOST_YEAR_CONTINUITY.md`.

The former development ID is recognized only by save migration. Chapters 2–4 and post-game systems remain unimplemented.

## Shared architecture and platform status

Portable `px_core` owns menu state, route discovery, recap state/content, tutorial/manual state, saves and story/runtime logic. Platform layers translate input, render the same shared state, play future audio hooks and select platform save paths.

- macOS: primary visual authority; shared front end integrated into Cocoa/Metal source, but not compiled or visually approved in this Linux workspace.
- Linux: SDL2 software-rendered development/validation shell, real window/input/gameplay flow, isolated saves and deterministic screenshot states.
- Old 3DS: portability gate; shared state maps to top/bottom-screen presentation, but the existing bring-up target still does not run the complete runtime.

Menu events reserve hooks for move, confirm, back, error, title confirm, carousel transition and route unlock. Final audio assets are not included.

## Deterministic review states

`scripts/capture-linux-review.sh` generates the review set under `review/screenshots/`: title, Story/Arena/Online modes, Rrvvfo and discovered routes, Story So Far, combat manual, exploration HUD, combat HUD, dialogue, the Phase 1 model-import proof, and bind/sampled idle deformation proofs.

Linux rendering establishes code-complete layout validation, not Mac visual approval. The user must still review the native Mac build, and any significant renderer mismatch must be investigated.

## Known placeholders and 0.5A boundary

Known placeholders: Rrvvfo has only one foundation-quality idle proof clip, procedural non-Rrvvfo characters, recap stills, final typography/icons/glyphs, final menu and unlock SFX, animation blending/easing, portraits, final materials, audio and Mac-specific polish.

0.5A may port Chapter 2 only after Mac review accepts the 0.4F/0.4F.1 Chapter 1 and presentation foundation. It must follow the updated Legacy audit and Organization continuity; this patch does not add Chapter 2 scenes, fights or reunion events.
