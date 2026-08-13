# Parallels X: Clash of Souls 3.0R

Multi-platform native content port of the playable browser prototype. The current authority is `2.9A.40.7.2R`, with `2.9A.40.7.1.1` retained as its cumulative reference base.

## Current milestone — 3.0R Chapter 2 restoration • Updates 11–13

For Rrvvfo Chapters 1–4, the latest playable browser build is the primary story and gameplay source. Historical 7.1.1 audits remain the cumulative base; current route documents supply only explicit continuity overrides. See `docs/STORY_SOURCE_RULES.md` and `docs/BROWSER_CONTENT_PORT_EXECUTION_RULES.md`.

The current cumulative branch preserves the Chapter 1 Golden Gate and completes the playable Chapter 2 tournament loop on the same shared runtime for Mac, Linux and Old 3DS. Legacy Chapter 1 story order, map identity, combat values, ability chronology and route structure remain locked; Chapter 2 continues directly from Tournament Road without returning to a menu.

The game boots through `PRESS ANY BUTTON` into the shared menu. `FIGHT` immediately starts Rrvvfo versus the shared CPU combatant, `TRAINING` starts the Sage's playable lessons, and Story Mode enters the Chapter 1 content port. Rrvvfo is the only fresh-save story route; Bark, Wade and Virek appear only after their semantic story discovery events and honestly report `STORY COMING LATER` until implemented.

Chapter 1 includes:

- exact Legacy Sage/Object Swap opening and represented dialogue;
- optional Sage training with Full Challenge, Quick Ability Refresher and Skip routes; the full path preserves the seven-step order, requirements and checkpoints;
- phase-correct HP/Energy/Guard/opponent/hotbar information;
- Tournament Road, river/Object Swap, Legacy `CHOOSE A ROUTE` panel, physical Main/Forest/Cliff content and reconnection;
- relay, transport, runaway cart, seven NPC interactions, the complete lost-competitor/pass story, the optional Lens/Object Swap sign story, checkpoint and mandatory Lens roadblock;
- Chapter 1 completion at Tournament Outskirts, pointing continuously to Chapter 2;
- no Bark/Wade reunion, registration, cracked ring, Tournament Card or Shots of Agony in Chapter 1.

Chapter 2 includes:

- a recomposed, continuous Tournament Grounds hub with Entrance, Registration, Market, Practice, Spectator, Contestant and Main Arena districts;
- Lost Bracket, Wade's timed shortcut with separate win/loss reactions, Bark's unresolved Cracked Ring investigation and changing seven-phase tournament life;
- practice, Hamual, Daniel, Bark/Pouki, Wade and Plouke tournament beats, with named characters remaining in the grounds after their matches;
- Wade's Lost Fan, Fake Champion/Lens fight, Runaway Training Dummy, Bark practice, food, photo and live bracket-board interactions;
- Rrvvfo's pure Energy Beam, Energy-charge power state, exhausted failed Fire Awakening and a playable final beam clash that still ends in Plouke's canonical victory;
- post-Chapter-2 Fire Awakening progression and awakened Solar Weave, with schema-6 migration for completed U10 saves and no Shots of Agony exposure.

The shared engine includes timed Legacy-derived combat, burst Dash, save/reload, content-driven world/camera data, a protected Tournament Card progression foundation, generic quest/party state and one runtime for all routes/platforms.

The Mac renderer consumes the shared world through perspective Metal rendering. The repaired Rrvvfo 15 asset keeps its silhouette and 39-joint rig across Mac, Linux and the 3DS Gate. The current cooked asset carries 36 shared clips: the exact 0.75-second Legacy hub idle plus Chapter-1 locomotion, combat, defense, ability and presentation clips. A lightweight head-attached cel layer supplies eyes, brows and mouth expressions without altering the source head, hair or outfit. Final art/effects/audio polish, native Legacy side-by-side approval and Old 3DS hardware performance remain incomplete. `Jimmy.glb` is unrelated and is not Rrvvfo's binding.

Golden Gate QoL adds an interactive pause menu, safe manual saves with backup recovery, checkpoint restart, objective history, controls reference, accessibility toggles, dialogue hold-to-advance, mild dash steering, dedicated combat arenas and controller-disconnect pausing on desktop. See `docs/GOLDEN_GATE_QOL_0.4H.md`.

U11–U13 add live round/next-match information, Recent Dialogue, persisted optional results and Wade best time, held skipping for already-seen scenes, shared cinematic-camera parity and lightweight recognizable Legacy-style tournament silhouettes. See `docs/U11_U13_CH2_RESTORATION.md`.

Updates 4–6 focus on performance headroom, Rrvvfo animation/combat personality, and the final Chapter-1 Golden Gate pass. Optimization is behavior-preserving: no story, route, animation, graphics or mechanics are removed to gain performance. See `UPDATE_6_GOLDEN_GATE.md`.

## Story and Replay

Normal Story Mode is continuous: Chapter 1 → Chapter 2 → Chapter 3 → Chapter 4. Internal scenes are not missions. There is no mission select, mission replay, chapter menu between normal chapters or return to character select.

Replay is whole completed chapters only.

## Audits and report

- `docs/LEGACY_CHAPTER1_PARITY_AUDIT.md`
- `docs/LEGACY_CHAPTER2_PARITY_AUDIT.md`
- `docs/LEGACY_CHAPTER3_PARITY_AUDIT.md`
- `docs/LEGACY_CHAPTER4_PARITY_AUDIT.md`
- `docs/UI_PARITY_0.4F.md`
- `docs/LEGACY_FOUNDATION_LOCK_0.4F_REPORT.md`
- `docs/MAC_0.4F.1.md`
- `docs/ORGANIZATION_OF_THE_RED_LOST_YEAR_CONTINUITY.md`
- `docs/RRVVFO_3D_MODEL_INTEGRATION_PHASE1.md`
- `docs/RRVVFO_3D_PHASE1_VISUAL_ACCEPTANCE_REPAIR.md`
- `docs/RRVVFO_SHARED_SKELETAL_ANIMATION_FOUNDATION.md`
- `docs/GOLDEN_GATE_QOL_0.4H.md`
- `docs/3DS_LEGACY_UI_MODEL_PARITY_0.4H.2.md`
- `docs/U11_U13_CH2_RESTORATION.md`

## Build shared core

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/px_runtime_smoke
```

## Linux graphical validation

Linux is the Codex QA/development target, not the lead visual platform:

```bash
./scripts/build-linux.sh
./scripts/run-linux.sh
./scripts/test-linux.sh
./scripts/capture-linux-review.sh
```

The graphical `build-linux/ParallelsX` shell consumes the same shared menu, recap, route, Chapter 1, manual, HUD and dialogue state as Mac. Review and automated runs use isolated/disposable saves. Screenshots are in `review/screenshots/`.

For the model-import proof specifically:

```bash
./scripts/run-linux.sh --review rrvvfo-model
./scripts/run-linux.sh --review rrvvfo-legacy-comparison
./scripts/run-linux.sh --review rrvvfo-idle-bind
./scripts/run-linux.sh --review rrvvfo-idle-sampled
```

The comparison and idle-proof states use the same Legacy-derived training-field content and camera as Mac, with Linux providing a reduced perspective renderer. The bind and 0.75-second sampled frames prove deformation in the actual gameplay renderer.

## Build Mac app and DMG

On macOS:

```bash
./scripts/build-macos.sh
```

The stable output is `dist/Parallels-X-Clash-of-Souls.dmg`.

## Old 3DS

With devkitPro installed:

```bash
./scripts/build-3ds.sh
```

The U11–U13 candidate runs the same `RuntimeSession`, Chapters 1–2 content, fight rules, optional activities, save progression and cinematic shot values as desktop. Its Citro3D renderer keeps the authored perspective camera and byte-identical cooked Rrvvfo asset while culling distant nonimportant actors and bounding decorative geometry. Legacy's blue, white, black and yellow interface language covers title, mode/route selection, dialogue, objectives, Tournament Card, action sequences, hotbar, pause, Recent Dialogue and saves. A successful cross-build is still not real-hardware acceptance; Azahar review plus Old 3DS XL frame-pacing, memory, suspend/resume and long-session testing remain required.

3DS online target: investigate Pretendo compatibility in a later networking milestone. No networking or Pretendo compatibility is claimed in 0.4H.

## Non-negotiable engine rule

A chapter provides data/content. It does not own a movement, combat, input, camera, UI, dialogue, save, RPG, Replay, character-model or animation engine.
