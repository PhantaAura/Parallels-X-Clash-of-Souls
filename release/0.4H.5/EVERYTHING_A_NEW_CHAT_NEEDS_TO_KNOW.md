# Parallels X: Clash of Souls 3.0R — Everything a New Chat Needs to Know

Updated: August 9, 2026

Handoff package: 0.4H.5 candidate

Project owner: PhantaAura / emeraldhunter

This is the single starting document for continuing the project. Read it before changing gameplay, story, UI, models, maps, or platform code. More detailed audits remain under `docs/`.

## 1. The non-negotiable product contract

Parallels X 3.0R is a **content port/remake of the existing playable browser game**, not a tech demo or engine showcase.

- The latest playable browser prototype in the 2.9A.40.x line is the gameplay/content authority.
- Legacy supplies the personality, dialogue style, geography, pacing, character identity, and unchanged story material.
- macOS, Linux, and Nintendo 3DS must be the same game: same chapter, maps, encounters, controls, progression, save state, and combat rules.
- The 3DS may use fewer polygons, smaller textures, cheaper effects, simpler lighting, and less background detail. It may not become a different top-down game or lose menu/story features.
- Playable loops and content come before architecture, advanced lighting, tooling, or animation-graph work.
- Do not add a new QoL idea without explaining it to the owner and receiving approval first. Fixing a confirmed bug does not require pretending it is a new feature.
- Chapter 2's remake does not begin until Chapter 1 passes its final acceptance gate.

## 2. Source hierarchy

When sources disagree, use this order:

1. Browser 2.9A.40.7.2R content/mechanics where it explicitly changes the cumulative game.
2. Legacy 2.9A.40.7.1.1 for unchanged story, dialogue, world structure, mechanics, and character personality.
3. Explicit current owner corrections and approvals.
4. The character tournament-card updates in `docs/route-sources/` for Rrvvfo, Wade, and Bark.
5. New remake material only when it strengthens an existing beat without replacing its purpose.

Legacy dialogue has more personality and should be retained wherever current canon allows it. New dialogue must sound specific, proud, impatient, funny, and occasionally strange—not like generic tutorial or quest text.

Important reference files:

- `docs/legacy-reference/LEGACY.md`
- `docs/BROWSER_CONTENT_PORT_EXECUTION_RULES.md`
- `docs/CHAPTER1_FINAL_CHANCE_CHAT_AND_BRANCH_HANDOFF.md`
- `docs/LEGACY_CHAPTER1_PARITY_AUDIT.md`
- `docs/CHAPTER1_GOLDEN_GATE_CHECKLIST.md`
- `docs/route-sources/RRVVFO_TOURNAMENT_CARD_UPDATE.md`
- `docs/route-sources/WADE_TOURNAMENT_CARD_UPDATE.md`
- `docs/route-sources/BARK_TOURNAMENT_CARD_UPDATE.md`

## 3. Final-chance goal for Chapter 1

The owner is giving the remake one final chance to prove it can deliver a fun, polished, fast-to-iterate Chapter 1 with minimal bugs and honest 3DS parity.

- Target length is roughly **45–60 minutes** for an exploratory run with conversations and side stories.
- A direct run or tutorial-skipping run may be shorter.
- Length must come from varied play, traversal, character, and optional authored stories—not slow walking, repeated fights, padding, or “collect five fruit” errands.
- Large events should not happen every few seconds. Quiet movement, ambience, NPCs, and seeing the tournament in the distance give the obstacles and fights contrast.

Preferred pacing:

> travel → small interaction → travel → meaningful obstacle → quiet space → optional character story → larger traversal sequence → fight or story beat

## 4. World and traversal lock

The world target is **Legacy authored exploration with the spatial readability of the Mario & Luigi 3DS remakes**. It is neither an open world nor a flat horizontal strip.

Every Chapter 1 area should have real depth, curved/offset paths, foreground/background layers, useful elevation, compact landmarks, side pockets, and readable authored cameras. Main, Forest, and Cliff must be physical routes that branch and reconnect rather than labels on one corridor.

Golden rule:

> A traversal obstacle needs a world reason and a gameplay purpose.

The accepted Tournament Road direction is a major crash or terrain collapse blocking the direct route:

> blocked road → climb side path → jump broken ground → dash a wider gap → Object Swap through an obstruction → cross an elevated/unstable section → reconnect with the road

The player should later recognize the original blockage from the far side. Related approved ideas include bridge supports used as platforms, a rockslide with one Fire Blast-cleared obstruction, a washed-out riverbank climb, real Cliff elevation/recovery ledges, and optional scenic ledges with a small discovery or reward.

Acceptance question: does Chapter 1 feel like a real 3D remake of Legacy, or Legacy projected onto a straight line? If it is the second, it is not ready.

## 5. Current playable content

Implemented in the shared runtime:

- Controllable Rrvvfo movement, jumping, dashing, combat-facing state, and shared animation state.
- HP, Energy, Guard, hit reactions, knockdown, simple opponents, and combat HUD state.
- Pursuit, Flow Cancel, the Chapter 1 ability set, dedicated arena transitions, and restoration of exploration state.
- Chapter 1 training/road progression, maps, objectives, interactions, choices, QTE/training states, save/checkpoint state, and the Chapter 2 boundary.
- Direct **Fight** and **Training** menu entries that do not damage Story Mode saves.
- Story So Far, safe save, checkpoint restart, objective history, controls, and accessibility/QoL settings.
- Shared player/opponent position, facing, world, combat, story, and save logic consumed by all three platform layers.

Do not replace these with platform-specific gameplay.

## 6. Sage tutorial

The tutorial must be fun, useful, and skippable. It currently offers:

1. **Sage's Full Challenge** — all seven lessons.
2. **Quick Ability Refresher** — starts at the ability-dependent road material.
3. **Skip • Start the Road** — immediately continues the Legacy story safely.
4. **Resume** — appears when a tutorial checkpoint exists.

The full path teaches movement, jump, directional dash, light/heavy/launcher/grab, perfect block, energy charge, Fire Blast, Object Swap, Lens prediction, and a final three-hit spar. Skipping persists as `ch1_tutorial_skipped` and must reach the same post-training story beat without softlocking.

## 7. Implemented side stories

Optional stories should feel like short anime OVA episodes: premise, escalation, character response, payoff. They should not be task lists.

**The Loudest Seat**: a competitor forgot to register but wants to spectate; a roadside fighter challenges Rrvvfo for walking as if he has already won; winning earns a spectator pass; returning it produces a character payoff. Delivery persists as `ch1_spectator_pass_delivered`.

**The Sign That Points Back**: a sign keeps pointing at Rrvvfo. Lens reveals it is lying and calling him “CHAMPION”; Object Swap moves it to the correct post; the changed sign persists with `ch1_sign_that_points_back_complete` and awards `ch1_wayfinder_badge`.

Keep future side content local, mechanically distinct, memorable, and optional. Do not use arbitrary resource counts or long backtracking.

## 8. Rrvvfo model, face, animation, and facing

Current shared gameplay assets:

- `assets/characters/rrvvfo/rrvvfo-dev.glb`
- `assets/characters/rrvvfo/rrvvfo-dev.pxskel`
- repaired source: `assets/characters/rrvvfo/rrvvfo15-weight-repaired.glb`

Current hashes:

- GLB: `8a9dd514a23835a7842b68e560c887b4088a25585f474b5438eaf01225e1d0d5`
- PXSKEL: `fbd0fe714020b10c802e5cff0c8c0769a604d528ec8d08ffd76f1eb42eb1a2a3`

The 3DS ROMFS copy of the cooked model is byte-identical to desktop. Performance reductions may remove hidden geometry or reduce material/texture/background cost only when the silhouette, hair, jacket/shirt identity, face placement, animation readability, and personality survive.

Weight-paint concerns were shirt/torso deformation, jacket clipping, and arms. Keep deliberate Spine/Spine1/Spine2 contribution, remove unrelated limb influence from garments, and normalize after assigning intended bones. Do not disable Auto Normalize as a blanket fix. Test actual movement and combat clips, not only the bind pose.

The face direction is inspired by DBZ: Kakarot only for expressive eyes, brows, and mouth motion. Do not change the rest of Rrvvfo. Facial elements must remain attached to the head; the 3DS path uses lightweight near-surface cel layers/expression state. Character portraits come after the owner visually approves the in-world face.

Facing rules:

- Exploration follows travel direction with a responsive turnaround.
- Combat faces the actual opponent, never the arena center.
- Moving away uses a readable backpedal/reversed run while watching the opponent, in the spirit of Sonic Battle and Superstar Saga character readability.
- Every renderer consumes the same shared facing/animation state.

## 9. UI lock, especially Nintendo 3DS

Legacy provides the information structure and personality. The visual influence for the 3DS bottom screen is specifically **Mario & Luigi: Superstar Saga + Bowser's Minions on Nintendo 3DS—not the GBA original**. Do not copy Nintendo assets; use its large-button hierarchy and touch/physical-control clarity with Parallels X's own blue, white, black, yellow, and X-motif identity.

Required on every platform:

- Press Start/title flow and Story Mode prominence.
- One-character-at-a-time route presentation.
- Visible Story So Far access.
- Story, route, objective, dialogue, choice, QTE, training, combat, pause, save, settings, and results states.
- Readable text designed for each screen rather than desktop copy squeezed onto 3DS.
- Current-canon abilities only.
- Predictable back behavior and complete physical controls.

The 3DS may reorganize the same utilities across the bottom screen but may not omit features or invent a different chapter. Current Azahar review image: `artifacts/screenshots/3ds-remake-bottom-ui-gameplay.png`.

## 10. Controls

3DS controls currently map into the same semantic actions as desktop:

- Circle Pad: move
- Y: light attack
- X: heavy attack
- Y+X: launcher
- B: jump/context cancel
- A: grab/interact/confirm
- R: block
- D-Pad Up/Left/Right/Down: dash/counter/breaker/charge
- L + face buttons: ability slots
- START: pause
- SELECT: safe manual save
- L+START: save and exit

Desktop bindings and live prompts are defined by the shared input/menu state. Do not create divergent timing or combat rules for 3DS.

## 11. Build and verification state

As of this handoff:

- Shared automated tests: **5/5 passing**.
- Native 3DS cross-build: successful; output is a valid 3DSX.
- macOS universal Intel/Apple Silicon build: successful, signed, packaged, and DMG checksum verified.
- Linux shared sources/tests: passing. A native Linux executable must be produced and verified on Linux; GitHub Actions is included for this.

Release artifacts are under `release/0.4H.5/`:

- `Parallels-X-Clash-of-Souls-3.0R-Mac-0.4H.5-CANDIDATE.dmg`
  - SHA-256: `40617733e6ec7184ef5bcf7f2cb3846ff715ba11c44e580dbb739a70fcf5a37c`
- `Parallels-X-Clash-of-Souls-3.0R-3DS-0.4H.5-CANDIDATE.3dsx`
  - SHA-256: `74a5ca8a0e546433d7e8710e9bae57a848bcaf36e8b735e1239835dafd96300c`
- `Parallels-X-Clash-of-Souls-3.0R-Linux-Build-Source-0.4H.5.zip`
  - See `release/0.4H.5/SHA256SUMS.txt` for the generated hash.

Build/test commands:

```sh
./scripts/test.sh
./scripts/build-macos.sh
./scripts/build-linux.sh
make -C src/platform/3ds
```

The Linux GitHub workflow is `.github/workflows/linux-build.yml` and uploads a native x86_64 build artifact after a successful run.

## 12. Honest limitations

- The 3DS build has booted in Azahar and a screenshot exists, but real Old 3DS hardware remains the authority for frame pacing, memory, suspend/resume, battery behavior, text readability, arm/garment deformation, and face attachment.
- The Linux package initially committed from this Mac is source/build-ready, not a fake macOS-linked “Linux binary.” Use the included GitHub Linux workflow or a real Linux machine for the native executable.
- The build is a **candidate**, not release-final. Do not claim Chapter 1 is perfect solely because it compiles or tests pass.
- No Chapter 2 remake work should begin yet.

## 13. Known remaining browser/Legacy parity work

Before Chapter 1 can be called complete, explicitly audit the remaining high-value 7.2R/Legacy details, including:

- exact Main route four-beat structure;
- Forest route four-bell structure;
- Cliff route five-ledge structure and recovery behavior;
- exact 18/36 guidance and related dialogue/state;
- two-sided transport behavior;
- Lens-versus-southern-detour outcome;
- persistence through save/reload and arena return;
- exact Fire Blast costs/timing/values against the browser authority.

Do not silently reinterpret these. Compare the browser build and Legacy audit, then implement only the missing parity.

## 14. QoL proposals that still require approval

Discuss before adding: extra interaction magnetism, overlap priority between Interact/Object Swap, animated cooldown icons, new low-energy warnings, surface-specific footsteps, extra dynamic ambience, new scenic rewards, arena fly-ins, added wall/landing reactions, settings previews, expanded objective history, extra fast-forward/scene-skip behavior, or new checkpoint messaging.

For any proposal, show the owner where it appears, how it changes play on all three platforms, and its performance/save risk.

## 15. Chapter 1 release gate

Do not move to Chapter 2 until:

- The owner approves pacing, personality, model presentation, UI, and gameplay feel.
- Direct and exploratory runs both pace well.
- Full, quick, skip, resume, save, and reload tutorial paths never break progression.
- Side stories feel authored rather than like errands.
- Mac and Linux complete the same route with matching gameplay state.
- Azahar shows readable text, the correct Rrvvfo, an attached face, good arm/garment deformation, and the complete menu feature set.
- Real Old 3DS hardware passes performance, memory, save/load, suspend/resume, and text tests.
- No high-severity progression, save, arena-return, tutorial, or parity bugs remain.
- Chapter 1 ends cleanly at Tournament Outskirts and leads continuously into Rrvvfo Chapter 2.

## 16. First actions for the next chat

1. Read this file and the browser-port rules.
2. Launch the current Mac and 3DS candidates; do not judge from old screenshots.
3. Run the complete Chapter 1 regression route on Mac/Linux and review Azahar/real hardware separately.
4. Log bugs by severity and parity impact; fix progression, save, readable UI, face attachment, and deformation before adding polish.
5. Present any new QoL idea for approval before implementing it.
6. Finish the remaining browser/Legacy parity audit.
7. Ask for explicit owner approval before starting Chapter 2.

The central rule is simple: **ship the same fun Chapter 1 everywhere, using Legacy's personality and the browser game's content—not three loosely related demos.**
