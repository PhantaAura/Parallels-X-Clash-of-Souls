# 3.0R Mac 0.4F — Legacy Foundation Lock Report

Authority: exact Legacy `2.9A.40.7.1.1`, Chapter 2 Optional Fight Hub Return Fix. This report distinguishes implemented portable behavior from native visual validation.

## 1. Every 0.4E → Legacy Chapter 1 mismatch fixed

- Reversed the incorrect documentation priority. Legacy is now primary for Rrvvfo Chapters 1–4; current route documents are continuity overrides only.
- Restored Legacy's `CHOOSE A ROUTE` panel. 0.4E's in-world route fork was not an approved difference. Main, Forest and Cliff remain physical routes after selection.
- Preserved the exact 22-state Chapter 1 story order and exact represented dialogue sets; no Bark/Wade reunion, Virek setup, energy-signature lesson, emotional-thesis insert or unnecessary conspiracy foreshadowing is present.
- Restored the tutorial manual's sectioned information and Guided/Resume/Quick entry modes instead of the 0.4E all-steps summary.
- Added real tutorial checkpoints at Movement, Perfect Block, Core Abilities, Lens preparation and Final Spar. Restart and save/reload resume the latest checkpoint.
- Corrected the Step 3 telegraph cycle to `1.35s` with the last `0.32s` as the readable block window.
- Corrected Step 6 evasion so a Dash begins an attempted read; success is awarded only when Sage's predicted attack resolves without damage. Perfect Block remains a valid alternate success.
- Restored Legacy-phase HUD disclosure. Energy, Guard, opponent health and hotbar are not permanently exposed.
- Added the missing Guard information channel, contextual interaction prompt and shared pause/story information surface.
- Removed the Mac's 0.4E/debug build kicker from the objective panel.
- Extended saves to restore meaningful checkpoint position, route, HP, Energy, Guard, chapter flags and tutorial checkpoint.
- Set the Chapter 1 boundary target to `rrvvfo_ch2`. Because Chapter 2 is intentionally not implemented, the runtime holds a pending continuous-story transition rather than showing a menu or fabricating content.

The substantial post-training road content restored in 0.4E remains: river Object Swap, all routes, relay, transport, cart, seven NPCs, optional fight, checkpoint, Lens roadblock and Tournament Outskirts.

## 2. Exact tutorial comparison

| Legacy behavior | 0.4F behavior | Result |
|---|---|---|
| Exact eight-line Sage/Rrvvfo tournament/manual briefing | Same registry lines and order | MATCHING |
| Sectioned manual; Guided, Resume, Quick choices | Sectioned portable model; same three training choices | MATCHING; obsolete mission-exit choice omitted |
| Step 1: accumulate `72` movement units, Jump and completed burst Dash | Same three independent checks | MATCHING |
| Step 2: Light/Heavy/Launcher count on action start; Grab only on close connected hit | Same action-start/connection split | MATCHING |
| Step 3: Sage cycle `1.35s`; readable final `0.32s`; timed Guard | `1.03s` telegraph start, `1.35s` resolve; connected Perfect Block required | MATCHING |
| Step 4: start `20`, stationary Charge to `75` | Same values and movement interruption | MATCHING |
| Step 5: Fire Blast then Object Swap | Same actions with current visible slots `[1]` and `[2]` | APPROVED CHRONOLOGY/UI OVERRIDE |
| Step 6: start `35`, Charge to `60`, pay Lens costs, read, evade or Perfect Block | Same preparation/cost/read sequence; result waits for attack resolution | MATCHING behavior; current visible Lens slot `[3]` |
| Step 7: three connected, unblocked clean hits | Same; blocked/Perfect Block/counter outcomes do not count | MATCHING |
| Checkpoints: movement, parry, abilities, Lens charge, final | Same five checkpoints represented by step indices `0/2/4/5/6` | MATCHING |
| Failure/retry keeps a safe tutorial checkpoint | Reset and serialized reload return to manual with Resume preselected | MATCHING player experience |
| Tutorial ends in six-line post-spar exchange, then Road departure | Same internal continuous flow | MATCHING |

The Legacy browser mission wrapper, mission-complete surface and per-mission exit are not ported.

## 3. Exact UI comparison

The screen-by-screen Legacy / 0.4E / 0.4F table is in `docs/UI_PARITY_0.4F.md`. Source-level structural fixes cover HP, Energy, Guard, tutorial visibility, current abilities, objective/detail, dialogue, interaction prompt, route choice, QTE, pause and chapter handoff. Final visual parity is not claimed.

## 4. Intentional Chapter 1 differences that remain

- Shared portable C++ and data registries replace the browser/DOM/global patch architecture.
- Internal `SceneStep`s replace player-facing missions; Story is continuous and Replay is whole chapters only.
- Real perspective 3D presentation replaces the Legacy renderer.
- Current chronology hides Shots of Agony and Solar Weave completely. Current abilities compress to Fire Blast `[1]`, Object Swap `[2]`, Lens `[3]`.
- Chapter 1 stops at Tournament Outskirts. Cracked ring, registration and Tournament Card are Chapter 2.
- Verified softlock/state fixes remain: safe cart fail-forward, optional-fight rematch/leave, stable final completion, physical river rock trade, persistent HP and checkpoint restoration.
- The old tutorial's mission-exit option is omitted; leaving normal Story to a mission shell is obsolete.

No Legacy Chapter 1 spoken line was changed. All 67 represented lines remain exact.

## 5. Chapter 2 full audit

See `docs/LEGACY_CHAPTER2_PARITY_AUDIT.md`. It traces 42 transitive source modules, exact hub/tournament order, dialogue-set anchors, maps/camera, 15 named hub actors plus activities, six optional quest lines, all fights, RPG state, UI, save behavior and hub returns. Chapter 2 is not implemented in 0.4F.

## 6. Chapter 3 full audit

See `docs/LEGACY_CHAPTER3_PARITY_AUDIT.md`. It traces 40 transitive modules and the exact 33-state sabotage → witnesses → Strange Man/hat → maintenance → Organization of the Red facility → Sage/clone → unstable teleporter → collapse sequence, including evidence, optional quests, fights, three stages, saves and the protected uncertainty. Chapter 3 is not implemented in 0.4F/0.4F.1.

## 7. Chapter 4 full audit

See `docs/LEGACY_CHAPTER4_PARITY_AUDIT.md`. It traces 42 transitive modules and the full 14-state spine plus village/party/cavern detail, defense, both potion routes, Ryuzankaro, Vibration Sense, Lens/Object Swap rewards, solo mountain, Hollow Watcher, summit pebble, playable Lookout, Shadow approach and collapse. Chapter 4 is not implemented in 0.4F.

## 8. Current `.md` material rejected in favor of Legacy

- Chapter 1 rewrite as the primary source.
- Chapter 1 Bark/Wade reunion.
- Added emotional-thesis scenes, Virek setup, energy-signature lesson, extra Shadow/Sage exposition and unnecessary conspiracy foreshadowing.
- 0.4E's claim that the physical route fork was an approved replacement for Legacy's choice panel.
- Any Chapter 2 reduction that removes the Legacy hub, festival, activities, fights, state changes, Plouke content or hub returns.
- A Chapter 3 memory-manipulation solution to the Strange Man contradiction.
- A shortened Chapter 4 summary that cuts village/party/cavern/revisit content, either potion route, Ryuzankaro, Watcher, solo ascent or playable Lookout.
- Any Chapter 4 version that moves the following Shadow discussion out of Chapter 5.

## 9. Current `.md` material retained as true continuity overrides

- Season 2 establishes Shots of Agony's debut; it is absent from Chapter 1 and not unlocked by the Chapter 3 blue clone.
- Chapter 1 ends at Tournament Outskirts. Cracked ring, registration and Tournament Card belong to Chapter 2.
- At Chapter 2 registration, Rrvvfo, Bark and Wade receive Tournament Cards; this is the first player-facing Level/XP/HP/Power/Defense/Speed/Focus display.
- Level-up automatic growth plus compact `+1/+2/+3` wheel and one selected bonus stat.
- Continuous Story Mode and whole-chapter Replay only.
- Chapter 4's minimal Bark/Wade clarification: they may have searched/fought during Rrvvfo's absence; serious defenses follow his later mountain departure.

Season 1, Season 2 and the working OVA bible were checked only for surrounding continuity after the higher-priority sources.

## 10. Shared-engine capabilities added

- Pending continuous chapter boundary (`pendingChapterId`) without a menu or fake content.
- Save/load of scene checkpoint, position, route, HP, Energy, Guard, tutorial checkpoint and chapter outcomes.
- Shared tutorial entry/checkpoint/retry state and phase visibility contract.
- Shared UI state for HP, opponent HP, Energy, Guard, hotbar, objective, dialogue, interaction, choice, QTE and pause.
- Shared `StoryProgressionSystem` using Legacy thresholds/stat growth, hidden-card state and deterministic `+1/+2/+3` bonus application.
- Shared data-only `QuestSystem`, `QuestState` and `PartyState` interfaces needed by Chapters 2–4.

No chapter-specific movement, combat, input, camera, UI, dialogue, save, RPG or Replay engine was added.

## 11. Tests run and results

Portable strict C++17 compile and expanded core regression: PASS.

Source architecture/UI/audit guard: PASS.

Coverage includes exact opening order/dialogue presence, seven tutorial steps and success semantics, checkpoint reset/reload, phase hotbar/HUD visibility, Sage flow, road/river/routes/relay/cart/NPC/optional fight/Lens/outskirts, persistent saves, pending Chapter 2 target, pause UI, progression/bonus math, quest foundation and platform-boundary rules.

CMake/CTest wrapper is unavailable in this workspace; equivalent targets are compiled directly with `-Wall -Wextra -Werror -pedantic`.

## 12. Mac build result

Not validated in this Linux workspace. Cocoa, Metal and Apple's SDK are unavailable. The Objective-C++ source was updated to 0.4F and the shared C++ compiles strictly, but no `.app`, DMG, screenshot or native input/visual result is claimed.

## 13. Remaining visual/playtest items

- Side-by-side Legacy/native Mac tutorial and full Chapter 1 playthrough.
- Final Rrvvfo/Sage/NPC models, cooked mesh path, animation, portraits, expressions, materials, cel shading, effects and audio.
- Native validation of UI size, text wrapping, objective/HUD overlap and controller glyphs.
- Exact camera scale/fog/clipping tuning and route landmark readability.
- Combat hitstop, motion, knockback, wall reactions, clashes and result presentation feel.
- Final pause/map/settings and animated results treatment.

`Jimmy.glb` remains registered and retained but is not rendered; the procedural character fallback remains.

## 14. Old 3DS portability impact

All new gameplay, save, tutorial, progression, quest and UI-state logic is standard portable C++17 with no DOM, browser, Cocoa or Metal dependency. The 3DS renderer may reduce models, textures, particles, transparency, draw calls and ambient density while consuming the same content/state.

The current 3DS executable remains a bring-up proof and does not run `RuntimeSession`. No claim to the contrary is made. Before Chapter 2 implementation, the shared runtime and representative Chapter 1 field/combat/UI flow still need a real Old 3DS integration and memory/performance profile.

## Acceptance status

Source and portable-behavior checks now align Chapter 1 much more closely with Legacy than 0.4E, and the Chapter 2–4 source foundation is locked. Final acceptance still requires the requested Legacy-versus-Mac playtest. Chapter 2 implementation must not begin if that playtest finds Chapter 1 feels worse.

## Appendix: files changed from 0.4E

Added:

- `docs/LEGACY_CHAPTER2_PARITY_AUDIT.md`
- `docs/LEGACY_CHAPTER3_PARITY_AUDIT.md`
- `docs/LEGACY_CHAPTER4_PARITY_AUDIT.md`
- `docs/LEGACY_FOUNDATION_LOCK_0.4F_REPORT.md`
- `docs/UI_PARITY_0.4F.md`
- `src/core/quest.cpp`, `src/core/quest.hpp`
- `src/core/story_progression.cpp`, `src/core/story_progression.hpp`

Changed:

- `CHANGELOG.md`, `CMakeLists.txt`, `README.md`
- `docs/ARCHITECTURE.md`, `docs/CHAPTER1_LEGACY_PARITY_REPORT.md`, `docs/LEGACY_CHAPTER1_PARITY_AUDIT.md`, `docs/NEXT.md`, `docs/STORY_SOURCE_RULES.md`
- `review/README_FOR_REVIEWERS.md`, `review/content.json`
- `src/content/chapter_registry.cpp`, `chapter_registry.hpp`, `exploration_registry.cpp`, `exploration_registry.hpp`, `training_registry.hpp`
- `src/core/game.cpp`, `game.hpp`, `runtime.cpp`, `runtime.hpp`, `save.cpp`, `types.hpp`
- `src/platform/3ds/README.md`, `src/platform/desktop/headless_main.cpp`, `src/platform/macos/main.mm`
- `src/tools/content_export.cpp`
- `tests/core_tests.cpp`, `tests/source_architecture_tests.py`
