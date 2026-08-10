# 3.0R Mac 0.4F.1 Implementation Report

Milestone: Legacy Presentation + Continuity Lock  
Base: Mac 0.4F Legacy Foundation Lock  
Legacy authority: `2.9A.40.7.1.1` Chapter 2 Optional Fight Hub Return Fix

## Outcome

0.4F.1 replaces the development front end with a shared Parallels X title/carousel flow, adds an optional Season 1 Story So Far, adds persistent semantic Story route discovery, restores the real combat-manual hierarchy and exact refresher timing distinctions, prepares Organization of the Red continuity, and adds a real Linux graphical validation shell. Chapter 2 is not implemented.

## Implementation phases and commits

| Phase | Commit | Result |
|---|---|---|
| Imported base | `c500117` | Clean 0.4F baseline checkpoint |
| Shared front-end/content foundation | `5dbe04f` | Menu, routes, recap, unlocks, save schema and faction data |
| Linux graphical validation | `1ac6bd4` | SDL2 shell, review states, isolated saves and screenshots |
| Manual/tutorial parity | `22237d6` | Real pages/entries and distinct exact Legacy schedules |
| macOS shared front end | `9e1f0c2` | Cocoa/Metal shell consumes shared menu/recap/route/manual state |
| Continuity/docs/final verification | Recorded by the final phase commit | Organization reset, no fake routes, motion, reports and final tests |

## Definition-of-done matrix

| Requirement | Result | Evidence |
|---|---|---|
| Press Start boot | Implemented | Shared `MenuState` begins at `Title`; Mac/Linux render it |
| Horizontal ten-mode carousel | Implemented | Exact registry order and wrap tests |
| Parallels X identity | Implemented with replaceable art | Shared charcoal/crimson/ember/gold/off-white theme, diagonals and X motifs |
| Story So Far beside Story | Implemented | Context action in Story Mode; absent from main mode registry |
| Story route carousel | Implemented | One visible selected route at a time |
| Fresh save Rrvvfo only | Implemented/tested | Save default and visibility tests |
| Bark/Wade semantic discoveries | Implemented/tested | Reunion events/flags; no Chapter 1 triggers |
| Virek Emerald trigger | Implemented/tested | Generic chapter completion explicitly fails to unlock him |
| Persistent unlocks/migration | Implemented/tested | Save schema 3 and semantic reconstruction |
| Unfinished route honesty | Implemented/tested | Discovered route → `STORY COMING LATER`; no route chapter stubs |
| Continuous Rrvvfo Story | Preserved/tested | Begin/Continue and pending Chapter 2 boundary; no mission menu |
| Whole-chapter Replay | Preserved/tested | Route action uses chapter replay only |
| Ten-section recap | Implemented/tested | 34 timed, replaceable-visual frames; no Story mutation |
| Legacy manual hierarchy | Implemented/tested/rendered | Categories, entries, prompts, navigation and refresher choices |
| Legacy tutorial timing distinctions | Implemented/tested | First/retry/warning values for Perfect Block and Lens |
| HUD/dialogue hierarchy | Implemented/rendered | Shared contextual presentation and Linux screenshots |
| Organization of the Red | Implemented as planning/data lock | Stable `organization_red`; deprecated alias migration only |
| Online structural entry | Implemented honestly | `COMING LATER`; no networking launch path |
| Shared architecture | Implemented/tested | No platform headers in core/content; no Linux gameplay fork |
| Linux graphical build | Implemented/tested | `build-linux/ParallelsX`, SDL dummy/software validation |
| Old 3DS gate | Architecture-safe; runtime integration pending | Static boundary passes; native toolchain may be unavailable |

## Tutorial timing comparison

| Trial | First schedule | Retry schedule | Final warning / confirmation |
|---|---:|---:|---:|
| Perfect Block | 1.35 s | 1.45 s | `BLOCK NOW` during final 0.32 s |
| Lens predicted attack | 0.90 s | 1.10 s | warning final 0.32 s; safe-evade confirmation 1.05 s |

Movement still requires meaningful accumulated distance plus Jump and completed burst Dash. Light, Heavy and Launcher follow Legacy action-start rules; Grab must connect. Charge thresholds, Fire Blast, Object Swap, Lens preparation and three clean connected hits remain in the tested continuous tutorial.

## Screenshot review set

The Linux build rendered and captured:

- `press-start.png`
- `mode-story.png`
- `mode-arena.png`
- `mode-online.png`
- `story-so-far.png`
- `story-character-rrvvfo.png`
- `story-character-bark.png`
- `story-character-wade.png`
- `story-character-virek.png`
- `combat-manual.png`
- `chapter1-exploration.png`
- `chapter1-combat.png`
- `chapter1-dialogue.png`

These are implementation-validation images, not a claim of native Mac visual approval.

## Continuity review

The Organization is not a text substitution. The new continuity explicitly connects the damaged/scattered Season 1 Clone operation to regrouping Lost-Year cells, Rrvvfo's destruction of that branch and reasonable disbandment assumption, later Season 2 preparation and the Clones OVA return. It preserves the tournament mystery, Strange Man uncertainty, hidden facility, experiments, replicated combatants, unstable teleportation, Echo connection and destroyed-facility post-game teleporter plan.

Exact Legacy code-state names remain quoted in parity audits where necessary to trace the source. New data uses `organization_red`. The deprecated development alias is accepted only by the faction compatibility table and schema migration.

## Placeholders and boundaries

- Character/menu/recap art uses replaceable procedural presentation placeholders.
- Final menu, confirm, back, error, title, carousel and unlock audio assets are not included; shared event hooks exist.
- Mac source integration is complete, but Cocoa/Metal compilation and native visual review cannot be performed in Linux.
- Linux is representative QA/development presentation, not the shipping visual authority.
- The current 3DS executable remains a bring-up proof and does not yet run the complete shared runtime/menu.
- No DMG is generated outside macOS.
- No Chapter 2 scenes, fights, registration, reunion or route content were added.

## Validation results

- Strict direct C++17 core and menu suites with `-Wall -Wextra -Wpedantic -Werror`: PASS.
- Source architecture, platform dependency, no-mission, no-fake-route and deprecated-faction guards: PASS.
- Content exporter and JSON parse: PASS.
- Linux SDL2 graphical build: PASS.
- All ten required headless review launches and non-empty software-rendered frames: PASS.
- Full thirteen-image capture set and human montage inspection: PASS; no overlapping critical panels were found in the reviewed Linux frames.
- CMake/CTest wrapper: not run because CMake is unavailable; `scripts/test.sh` falls back to the equivalent strict Linux direct-build suite.
- Old 3DS native build: not run because `DEVKITPRO` is unavailable; the source boundary guard passed.
- Mac Cocoa/Metal build and visual approval: not available from Linux; still required on the user's Mac.

## Next milestone boundary

0.5A remains the Chapter 2 Legacy port only after native Mac review accepts the title/menu/manual/HUD work and confirms Chapter 1 has not regressed. Chapter 2 must use the updated parity audit and Organization continuity document.
