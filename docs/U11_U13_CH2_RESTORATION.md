# U11–U13 — Chapter 2 Restoration Batch

## What changed

U11 restores Chapter 2's Legacy tournament density without reintroducing a mission engine. Tournament Grounds are one continuous, recomposed hub: Entrance, Registration, Market/Festival, Practice, Spectator, Contestant and Main Arena. Bent routes, reconnecting side paths, district-specific silhouettes and multiple views of the same arena create scale without empty open-world space.

The playable loop now covers Lost Bracket, practice, Wade's shortcut, Cracked Ring, registration, opening ceremony, Hailey/Plouke, Hamual, Daniel, Bark/Pouki, Wade, four Plouke observations, final preparation, Plouke and aftermath. Seven authored phase states relocate recurring people and props between fight-to-hub returns. Named defeated contestants remain present instead of disappearing.

The first restored optionals are Wade's Lost Fan, Fake Champion and Runaway Training Dummy. Bark remains available for clean shared practice. Food, photo and live bracket-board interactions provide small festival residue. Results, Bark practice count and Wade's best time persist as backward-compatible schema-6 flags.

## Plouke finale

Rrvvfo enters the final with aggressively depleted Energy. Fire Awakening begins, flickers and collapses because exhaustion prevents it stabilizing. The player then performs a short final Energy rhythm and beam clash. Strong execution wins the power exchange but carries Rrvvfo outside the ring; timeout loses the clash from exhaustion. Plouke is the canonical tournament winner in both cases, followed by the existing performance-reactive result and Sage reveal.

## Rrvvfo Energy Power rules

- The existing Energy bar is the only meter.
- Charge uses blue Energy Power pulses, growing stronger near full Energy.
- The upper half of the Energy bar grants Rrvvfo a linear, capped 0–8% damage/knockback benefit and modest guard expression. Spending Energy naturally removes it.
- Chapter 2 adds a pure blue Energy Beam. It has no fire component before Awakening.
- Rrvvfo already knows Fire Awakening in lore, but it is not player-usable before the Plouke story attempt because he considers earlier rounds beneath using his full power.
- Completing Chapter 2 grants the saved `rrvvfo_fire_awakening_unlocked` progression flag. U10 saves already pending Chapter 3 receive it during load/save migration.
- Fire Awakening costs 35 Energy up front, drains Energy while active and ends after a short six-second maximum.
- While Fire Awakening is active, Energy Beam becomes blue-white Solar Weave with fire woven through the Energy stream. Outside Awakening it remains the pure beam.
- Shots of Agony is absent from Chapters 1–2 hotbars, tutorials, UI and Story use.

## Cinematic parity

Cutscene actions, moving actors and camera direction are evaluated by the shared runtime. `camera_policy` resolves the final shot for every renderer. The Tournament entrance uses a front-facing moving shot, Sage side entry and small impulse, walking side-eye/shrug, brief arena reveal and smooth gameplay return. Practice, reunion, card handoff, ceremony, spectator matches, final preparation, failed ignition, Sage reveal and aftermath receive restrained intentional framing.

Reduced Motion preserves focus/composition while removing the Sage-entry bump. Previously seen scenes require a deliberate Cancel hold. Dense hub geometry can invoke a shared close-in occlusion rescue rather than hiding Rrvvfo.

## NPC presentation tiers

- Tier A: Rrvvfo's existing full cooked model and face layer.
- Tier B: authored lightweight Legacy silhouettes for Sage, Wade, Bark, Hamual, Daniel, Hailey, Pouki, Plouke, Practice Fighter and recurring tournament workers. Proportions, hair/hat/hood shapes and major colors carry identity without requiring full rigs.
- Tier C: cheap procedural background crowd variation.

Old 3DS keeps required/named characters at conversational distance, culls distant nonimportant actors, submits Essential geography first, caps Full decoration and reuses the existing static world cache/vertex budget. Routes, story, fights, required props and shot framing are never cut as a performance strategy.

## QoL and save safety

Pause and Objective History show the current round, next match and live bracket summary. The bracket board stays useful, match calls are nonpunitive, official losses immediately retry without replaying intros, optional fights can Run and official fights cannot. Hub entry, post-match and pre-final transitions request autosaves. Recent Dialogue retains the newest twelve session lines.

Save schema remains version 6. Existing Chapter 1 flags, current Chapter 2 progress, Tournament Card data, physical Object Swap props and completed optionals are preserved. Chapter Replay is not unlocked by Chapter 2; the complete-character-Story rule remains unchanged.

## Intentionally not restored or added

- no saboteur reveal, suspect board, clue notebook or Chapter 2 detective boss;
- no quest currency, reputation, inventory economy, daily missions or giant quest log;
- no Chapter 3 content or future-faction expansion;
- no Shots of Agony chronology change;
- no separate Chapter 2/tournament/cinematic engine;
- no full high-poly rigs for every NPC;
- no district audio rewrite. The current project has no stable district-mixer/voice-budget foundation, so world-audio scale is deferred instead of introducing an unstable platform divergence.

## Human Old 3DS checklist

The automated pipeline can prove cross-compilation, asset inclusion, shared camera-field use and bounded renderer submission. It cannot certify real hardware. Verify on an Old 3DS XL:

1. sustained frame pacing while crossing all seven districts in every phase;
2. memory/load behavior through repeated hub → match → hub returns;
3. entrance, spectator, failed Awakening and reveal shot readability;
4. named silhouette recognition at conversation distance;
5. bottom-screen text, final-clash prompts and Recent Dialogue legibility;
6. suspend/resume, save/load, autosave and long-session stability;
7. effects/flash comfort with default and Reduced Motion/Reduced Flashes.
