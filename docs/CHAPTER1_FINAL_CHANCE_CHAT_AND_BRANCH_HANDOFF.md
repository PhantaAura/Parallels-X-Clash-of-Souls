# Parallels X 3.0R — Chapter 1 Final-Chance Handoff

This document consolidates the decisions, concerns, accepted direction, implementation state, and strongest ideas remembered from the current project chat and the referenced ChatGPT conversation **“Branch · Parallels X Patch Summary”** (`6a7244e0-cf34-83ea-bf80-9d10f3439f9b`). It exists so Chapter 1 does not lose its direction between updates.

## The final-chance agreement

Chapter 1 gets one focused chance to prove that the remake can be fun, fast to iterate on, visually polished, and minimally buggy. If it cannot meet that standard, returning to a Legacy-based definitive edition is a valid outcome.

The proof gate is:

- The latest playable browser build is the source of truth for story, character personality, dialogue style, route identity, gameplay rhythm, and UI information structure.
- Mac, Linux, and 3DS run the same game. The 3DS may reduce polygon count, texture size, effects, shadows, and background density, but may not replace the gameplay, map, story flow, menus, character, or controls with a different game.
- Chapter 1 should be a strong **45–60 minute** first chapter when the player explores, talks to people, and tries optional content. A direct or tutorial-skipping playthrough can be shorter.
- Chapter 2's remake does not begin until Chapter 1 passes its acceptance gate.
- New QoL ideas must be shown to the project owner before implementation. Bug fixes and work explicitly approved in the current request are not hidden QoL additions.
- A build is not called perfect merely because it compiles. The 3DS version needs readable text, attached facial features, sound deformation, complete Legacy UI options, and acceptable performance on target hardware.

## Source hierarchy

Use these sources in this order:

1. Browser `2.9A.40.7.2R` content and mechanics where it explicitly changes the cumulative game.
2. Legacy `2.9A.40.7.1.1` story, dialogue, world structure, mechanics, and personality for content unchanged by 7.2R.
3. Explicit current-project corrections and approvals only where they intentionally resolve a conflict.
4. Character route documents for the details they intentionally replace:
   - `docs/route-sources/RRVVFO_TOURNAMENT_CARD_UPDATE.md`
   - `docs/route-sources/BARK_TOURNAMENT_CARD_UPDATE.md`
   - `docs/route-sources/WADE_TOURNAMENT_CARD_UPDATE.md`
5. New remake material only where it strengthens an existing beat without rewriting its purpose.

Legacy dialogue should be retained when it remains compatible with current canon. New lines should sound like the same cast: specific, impatient, funny, proud, occasionally strange, and never like generic tutorial or quest text.

## Chapter 1 pacing target

The chapter should not behave like a constant trailer. A large event does not need to happen every few seconds. Quiet running, ambient conversation, a view of the tournament, and simple readable traversal give the stronger moments contrast.

Preferred rhythm:

> travel → small interaction → travel → meaningful obstacle → quiet space → optional character story → larger traversal sequence → fight or story beat

Avoid:

> fight → puzzle → explosion → cutscene → fight → boss with no breathing room

The chapter earns its 45–60 minute length through varied play and character, not slow walking, repeated fights, dialogue padding, collectibles, or “bring me five fruit” errands.

## Meaningful traversal rule

> **A traversal obstacle needs a world reason and a gameplay purpose.**

Platforming should teach or reinforce an ability, make the geography believable, reveal character or story, create a satisfying route choice, or reward observation. Platforms should not exist only because the game “needs platforming.”

The strongest Tournament Road proposal from the Branch conversation is a major crash or terrain collapse blocking the direct road. Rrvvfo sees why the road is unusable, takes a longer old route, and later reconnects where the starting obstruction is visually recognizable.

Suggested sequence:

> blocked road → climb the side path → jump broken ground → dash a wider gap → Object Swap through an obstruction → cross an elevated or unstable section → drop back onto the original road

Other accepted traversal ideas:

- A broken bridge uses remaining supports as a short platforming route instead of becoming an Interact prompt.
- A rockslide requires Fire Blast on one key obstruction while the player moves around the rest; the whole landslide should not disappear magically.
- A washed-out riverbank creates a brief vertical climb before the established far-bank Object Swap.
- The Cliff route uses actual height changes, readable recovery ledges, and scenic payoff.
- The Main route periodically exposes the blocked original road so the detour remains geographically connected.
- A successful swap may place Rrvvfo on a briefly unstable surface that asks for immediate movement.
- Optional ledges can contain a small scenic discovery, character moment, or useful reward while the main route stays readable.

## World presentation lock

The target is **Legacy authored exploration with Mario & Luigi 3DS-style spatial presentation**, not a giant open world and not Legacy projected onto a flat strip.

Every Chapter 1 area should provide:

- true movement depth rather than only left/right travel;
- an authored, readable camera;
- foreground and background layers;
- curved or offset paths;
- meaningful elevation, slopes, ledges, banks, bridges, and ramps;
- physical Main, Forest, and Cliff routes that branch and reconnect;
- landmarks and buildings with real volume that shape movement;
- compact, dense, learnable spaces instead of empty acreage;
- small side pockets for NPCs, scenery, discoveries, and optional interactions;
- collision that is simpler than the art but agrees with what the player sees;
- obstruction handling that keeps Rrvvfo readable behind scenery;
- one underlying map layout across Mac, Linux, and 3DS, with presentation tiers only.

Acceptance question:

> **Does this look and move like a real 3D remake of Legacy, or like Legacy arranged on a straight line?**

If the answer is the second one, Chapter 1 is not ready.

## Side-story rule

Optional content should feel like a small anime OVA episode that happened naturally along the road. It needs a premise, escalation, character response, and payoff. It should not be a task list or a hub full of markers.

Useful tone references recovered from the Branch conversation:

- An ordinary problem can become strangely personal or supernatural.
- A tournament contestant may challenge Rrvvfo for an unbelievably stupid reason.
- A tiny optional cave or overlook may contain someone who knows an impossible fact.
- A Raggie appearance can increase the weirdness.
- An apparently ridiculous optional story can occasionally expose something important.
- Not every optional scene should be absurd; weirdness needs ordinary scenes around it to stay funny.

Each optional story should ideally include at least two of the following:

- character conflict or comedy;
- a distinct mechanic or ability application;
- a change to the local world;
- a memorable reward or callback;
- information that changes how the player reads a later moment.

### Implemented side story: The Loudest Seat

The existing lost-competitor interaction is now a complete character story rather than a detached optional fight:

1. The competitor admits that he forgot to register but still wants to spectate.
2. Rrvvfo may agree or refuse without blocking the chapter.
3. The roadside fighter challenges Rrvvfo because he hates the way Rrvvfo walks as if he has already won.
4. Winning awards the spectator pass.
5. The game returns to the exact road state and asks the player to take the pass back to the competitor.
6. The competitor promises to cheer; Rrvvfo tells him to make it loud because he does not do favors quietly.
7. Delivery persists in the save as `ch1_spectator_pass_delivered`.

The fight is the task, and the return is a nearby character payoff. There is no arbitrary collection count or cross-map backtracking.

### Implemented side story: The Sign That Points Back

The sign painter near the tournament has a sign that keeps turning to point at Rrvvfo. The optional story uses two abilities and changes the world:

1. Talk to the painter near the outskirts.
2. Use Lens of Truth on the suspicious sign.
3. Discover that it is not painted backward; it is lying and calling Rrvvfo “CHAMPION.”
4. Use Object Swap to move it onto the correct empty post.
5. The corrected sign points both toward the tournament and toward Rrvvfo, which he considers “right twice.”
6. The corrected sign remains visible, and the save receives `ch1_sign_that_points_back_complete` and `ch1_wayfinder_badge`.

This story is optional, short, local, strange, mechanically distinct, and does not delay the entrance if the player ignores it.

## The Sage tutorial

Forcing a bad tutorial damages pacing and teaches the player to resent the game. The Sage tutorial now provides three initial paths:

1. **Sage's Full Challenge** — all seven lessons.
2. **Quick Ability Refresher** — starts at Fire Blast/Object Swap and continues through Lens/final spar.
3. **Skip • Start the Road** — immediately continues the Legacy story without a softlock.

When a tutorial checkpoint exists, **Resume** is added as a fourth choice.

The full route still teaches movement, jump, directional dash, light/heavy/launcher/grab, perfect block timing, energy charge, Fire Blast, Object Swap, Lens prediction, and a final three-hit spar. The mechanical requirements remain readable, but the step names and Sage barks frame them as a challenge between characters instead of seven sterile checklist pages.

Skip state persists as `ch1_tutorial_skipped`. Skipping does not remove later access to gameplay or alter the Chapter 1 story order.

Tutorial acceptance tests:

- A new player understands the input and why it matters.
- An experienced player can skip in seconds.
- The quick path teaches the chapter's ability-dependent road content.
- Failure never destroys progress; resume checkpoints work.
- Training UI fits and remains readable on the 3DS bottom screen.
- Skipping, resuming, restarting, and completing all reach the same post-training story beat.

## Rrvvfo presentation and controls

### Model and deformation

The approved model source is the repaired `rrvvfo15.glb` line, cooked as the shared Rrvvfo gameplay asset. Mac and 3DS currently embed byte-identical `rrvvfo-dev.pxskel` data.

Model-performance changes may reduce hidden geometry, material complexity, texture resolution, or distant detail only if they preserve:

- the silhouette;
- jacket and shirt identity;
- hair shape;
- facial placement;
- animation readability;
- gameplay personality.

The shirt/torso, jacket clipping, and arm weighting were identified as major visible problems. Weight repair must keep sensible Spine/Spine1/Spine2 contribution, avoid painting garments to unrelated limbs, prevent shoulder collapse, and test the actual gameplay clips rather than only the bind pose. Auto Normalize should not be disabled as a blanket workaround; weights should be deliberately assigned and normalized after the intended influences are present.

### Face

The face direction is inspired by Dragon Ball Z: Kakarot only for expressive eyes, brows, and mouth animation. It must retain Rrvvfo's model, head, hair, clothes, silhouette, and personality. Facial layers must sit on and follow the head rather than floating in front of it.

The 3DS face path must remain lightweight. Expression state is shared, while lower tiers may use simpler near-surface cel planes/material changes rather than expensive deformation. Character portraits should be revisited only after the in-world face receives the owner's visual green light.

### Animation and facing

The supplied sprite sheet remains the motion-reference sheet for idle, run, combat stance, jump/fall, dash, light/heavy/launcher/air attacks, block/perfect block, hurt/knockdown, Fire Blast, Shots of Agony, Lens, Object Swap, ultimate, victory, and turnaround poses.

Control target:

- Exploration facing follows travel direction with a responsive turn.
- Combat facing locks to the actual opponent, not the ring center.
- Moving away keeps Rrvvfo watching the opponent and uses a readable backpedal/reversed run, similar in intent to Sonic Battle and Mario & Luigi/Superstar Saga character readability.
- Every renderer consumes the same shared facing and animation state.

## UI and platform parity

Legacy's information structure and personality are the UI base. Remake-era Mario & Luigi influences may improve button size, screen use, hierarchy, animation, and readability without erasing Parallels X's blue/white/black/yellow identity.

Required across all versions:

- Press Start/title flow;
- Story Mode prominence;
- one-character-at-a-time route presentation;
- visible Story So Far access;
- shared story, route, objective, dialogue, choice, QTE, training, combat, pause, save, and results states;
- live Rrvvfo model and expressions where the model is expected;
- readable text with no desktop-sized copy squeezed onto 3DS;
- predictable back navigation and physical controls;
- current-canon abilities only, with no hidden future move clutter.

The 3DS bottom screen may reorganize utilities for touch and physical buttons, but it may not invent a different chapter or omit the desktop feature set. A lower renderer tier is acceptable; different gameplay is not.

## QoL proposal backlog — approval required before adding

The following ideas were discussed in the Branch conversation. They are useful candidates, but this document does **not** grant blanket approval to implement them if they are not already present:

- small interaction magnetism/cone assistance for NPCs and objects;
- explicit priority when Interact and Object Swap targets overlap;
- cooldown icon motion beyond numeric timers;
- low-energy warning before repeated failed ability input;
- surface-specific footsteps;
- more dynamic hazard/fight ambience;
- additional optional scenic discoveries and rewards;
- short arena intro fly-ins;
- additional context-sensitive landing and wall reactions;
- settings preview behavior;
- live controller-prompt swapping refinements;
- deeper objective-history presentation;
- expanded dialogue fast-forward settings;
- additional replay-scene skip presentation;
- extra checkpoint feedback and save-state messaging.

Before implementing any of these, show the exact idea, where it appears, what it changes on all three platforms, and its performance/save-risk cost.

## Implemented supporting QoL already in the project

These systems predate or are part of the approved Golden Gate work and should not be removed accidentally:

- safe manual save during valid exploration states;
- backup save recovery;
- restart from checkpoint;
- objective history;
- controls reference;
- accessibility/QoL settings;
- deliberate hold-to-advance dialogue;
- tutorial resume checkpoints;
- route and cutscene state persistence;
- shared combat input buffering and readable combat feedback;
- dedicated combat arena with exact exploration return;
- Story So Far access;
- Legacy-styled 3DS bottom-screen utility UI.

## Menu and world polish checklist

Keep for the final visual gate:

- Story Mode is visually dominant.
- Focus animation and carousel easing are quick and clear.
- There is no unnecessary dead time from title to menu.
- Parallax and animated X motifs are restrained.
- Rrvvfo's pose and lighting match gameplay rather than a debug viewer.
- Disabled modes are honest and legible.
- Save, Options, Replay, Story So Far, and pause screens share one visual language.
- Debug information requires a deliberate developer action and is absent from release builds.
- Every world area contains at least one spatial feature that makes it a place rather than a corridor.
- Arenas are authored spaces, not empty rectangles.
- Returning from an arena never duplicates NPCs, snaps the camera, carries victory input into exploration, or loses world state.

## Current implementation and verification — 2026-08-09

Implemented in the shared runtime:

- optional/full/quick Sage training selection;
- personality-driven tutorial headings and Sage barks;
- complete lost-competitor/pass side story;
- optional Lens + Object Swap sign story;
- persistent tutorial and side-story flags/rewards;
- 3DS four-choice tutorial layout;
- platform-independent test coverage for the new branches.

Verification completed:

- 5/5 shared automated suites pass.
- The complete Chapter 1 regression route passes, including all three routes, tutorial completion, tutorial skip, optional fight, pass return, sign puzzle, save/reload, and the Chapter 2 boundary.
- Native 3DS cross-build succeeds.
- Mac universal Intel/Apple Silicon build succeeds and is packaged as a DMG.
- Mac and 3DS Rrvvfo cooked assets are byte-identical: `fbd0fe714020b10c802e5cff0c8c0769a604d528ec8d08ffd76f1eb42eb1a2a3`.

Artifacts:

- Mac: `dist/Parallels-X-Clash-of-Souls.dmg`
  - SHA-256: `0458170b853fb8528dbb03cdd4b9e1c6108d24cbc9a71c3a19fbea4cf27c1723`
- 3DS candidate: `dist/Parallels-X-Clash-of-Souls-3.0R-3DS-0.4H.4-CANDIDATE.3dsx`
  - SHA-256: `9d0de2fb79c21652f0864adf04c73e4fcfd0ce023a4c26c5ed5adfe7a9ee31b9`

Honest limits:

- The Linux source passed the strict warning compile stage, but a Linux binary cannot be linked on this Mac because Linux's `libSDL2-2.0.so.0` is not installed or usable on macOS. The gameplay/content changes are in the shared source used by Linux; a native Linux build still needs to be run on Linux.
- The 3DS candidate compiles, but current screen composition and face placement have not been visually re-approved in Azahar or on hardware during this pass because the local graphical session was unavailable. Do not label it release-final until that review happens.
- Old 3DS hardware remains the authority for performance, memory, suspend/resume, text readability, and facial attachment.
- No Chapter 2 remake content has been started.

## Chapter 1 release gate

Do not move to Chapter 2 until all of the following are true:

- The owner approves Chapter 1's feel, pacing, and personality.
- A direct run and an exploratory run both have satisfying pacing.
- The full tutorial is useful and fun; quick and skip paths never break progression.
- Side stories feel authored and memorable, not like resource collection.
- Mac and Linux pass the complete route with matching gameplay state.
- Azahar shows readable UI, correct model, attached face, sound arms/garments, and the complete menu feature set.
- Real Old 3DS hardware passes performance, memory, save/load, suspend/resume, and text tests.
- No high-severity progression, save, combat-return, tutorial, or platform-parity bugs remain.
- Chapter 1 still ends at the Tournament Outskirts and points continuously to Rrvvfo Chapter 2.

Only after this gate should Chapter 2's remake planning begin.
