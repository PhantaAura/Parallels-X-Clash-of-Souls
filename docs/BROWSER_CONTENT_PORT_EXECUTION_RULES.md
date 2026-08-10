# Browser Content Port Execution Rules

## Product rule

Parallels X 3.0R is a content port of the playable browser game. It is not a native-engine showcase.

Before starting any task, ask:

> Does this let the player fight, move through the story, or use existing browser content on macOS, Linux, or 3DS now?

If the answer is no, do not prioritize it over missing playable content.

## Current browser authority

The latest browser package available for the port audit is:

`/Users/emeraldhunter/Downloads/Parallels-X-2.9A.40.7.2R-FULL-UPDATE.zip`

Build identity:

`Prototype 2.9A.40.7.2R — Complete Chapter 1 Adventure Rebuild + Mobile Playtest Lab`

It is a full-update deployer built from the verified cumulative `2.9A.40.7.1` base. The deployer itself must not be run against the user's browser repository during native-port work. Read or extract its embedded source in a temporary location when exact behavior or values are needed.

`2.9A.40.7.1.1` remains a useful cumulative reference for content that `2.9A.40.7.2R` did not replace, but it is no longer the last word where 7.2R contains an explicit change.

## Required shared ownership

The shared C++ runtime owns:

- movement, jump and dash;
- hit detection, HP, Energy, Guard, knockdown and CPU decisions;
- Pursuit and buffered follow-ups;
- Flow Cancel, including its 8-Energy cost;
- techniques and Field Skills;
- Chapter and Adventure Mission state;
- save/progression state;
- ranks, builds, party/squad state and later chapter content.

The macOS, Linux and 3DS shells translate input, render shared state, play platform audio and read/write saves. They do not implement different combat or story rules.

3DS reductions are presentation-only: fewer triangles, smaller textures, simpler materials/effects and a dual-screen UI. The player still gets the same fight, route, objectives, outcomes and story.

## Immediate playable phases

### Phase 1 — playable loop

Current state: **playable in shared runtime**.

- Rrvvfo walks, jumps and dashes.
- Light, heavy, launcher, grab, guard, Counter and Breaker are interactive.
- HP, Energy, Guard, hit reactions, launches, knockdown and a balanced CPU work.
- The Combat HUD consumes the same runtime state on all three platforms.
- `FIGHT` now launches Rrvvfo versus the real shared CPU arena directly from the menu.
- `TRAINING` now launches the real Sage training directly from the menu.
- Standalone Fight/Training sessions do not overwrite Story progress.
- Pause → Return to Title exits a standalone fight; Training's final choice is `EXIT • RETURN TO MENU`.

### Phase 2 — core combat identity

Current state: **playable, pending exact 7.2R value audit**.

- Launcher opens Pursuit.
- Dash starts the chase.
- Light/Heavy can buffer into Pursuit follow-ups and finishers.
- Connected melee opens Flow Cancel; Dash spends exactly 8 Energy.
- Chapter 1 Fire Blast, Object Swap and Lens are playable.

Next content work must compare every player-facing timing/value against the 7.2R browser source before adding more combat presentation. One known audit item is 7.2R's Chapter 1 story-only Fire Blast focus: 22 Energy, 1.05-second cooldown, 15 damage, 500 speed, 25 radius, 9 Guard damage and 1.3 clash power while Shots of Agony remains unavailable.

### Phase 3 — Chapter 1

Current state: **playable native route, not yet complete 7.2R parity**.

The native version already includes the Sage/Object Swap opening, tutorial, Tournament Road, river swap, three-way route choice, Main/Forest/Cliff paths, route reconnection, transport, optional road fight, Lens roadblock and Tournament Outskirts ending.

The next content-port pass should implement the remaining 7.2R Chapter 1 adventure details in this order:

1. Main Road's four work-lane navigation beats after controlled Fire Blast.
2. Forest Trail's four sequential blue-bell navigation beats.
3. Cliff Pass's five airborne ledges; the native route currently has only three.
4. Route guidance escalation at approximately 18 and 36 seconds.
5. The two-sided transport rescue: wheel swap strands Rrvvfo, then a second return-anchor swap gets him back.
6. The final choice between direct Lens of Truth and the longer southern detour.
7. Persistence for the southern detour as the browser World Delight/story state.

These are content/mechanic ports, not reasons to create a new route framework or renderer.

## Later browser content that must survive

Do not discard or replace these while expanding the port:

- Chapter 1–4 continuous story structure;
- builds, combat ranks and character identities;
- Field Skills and Adventure Missions;
- Chapter 3's sabotage-first investigation, three evidence points, worker/security/medical witness chain, medical contradiction, Strange Man uncertainty, real-Sage encounter and clone/teleporter separation;
- Chapter 4 squad combat and ending state;
- Pursuit and Flow Cancel as core combat identity.

## Anti-demo delivery rule

Every milestone report must begin with:

1. what the player can now do;
2. which platforms use it;
3. which browser behavior supplied the rule;
4. what remains visibly unported.

Architecture, rendering or tooling work is secondary and should be mentioned only when it directly unblocked playable content.
