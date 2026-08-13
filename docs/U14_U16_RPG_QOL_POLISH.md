# U14-U16 Front-End, RPG Rewards, and QoL Polish

This batch continues the shared `px_core` game from U11-U13. It does not add
Chapter 3, replace Chapter 1 or Chapter 2, or create platform-specific gameplay.
Mac, Linux, and Old 3DS use the same menu state, saves, movement, combat,
progression, quests, and rank calculations; only their presentation differs.

## U14: finished-game front end

- The top level is now `Continue`, `Story`, `Battle`, `Training`, `Extras`, and
  `Options`. Future battle types live inside Battle instead of occupying dead
  top-level slots.
- Continue is the fast path when a valid Story save exists and previews the
  character, current area, objective, Story progress, and playtime.
- Replay is locked per character until that character's complete Story flag is
  present. Starting Chapter 1 or reaching Chapter 2 cannot unlock Replay.
- Replay starts a separate whole-Story run and cannot overwrite the main Story
  save.
- Extras includes Adventure Records, the Combat Manual, Story So Far,
  Character Profiles/Gallery, and Credits.
- Submenu and option selections persist when the player backs out.
- Substantial Story and arena fights receive a quick S-E rank. The event-driven
  calculation considers result, damage, combo quality, Perfect Blocks, Guard
  Breaks, Pursuit finishers, action variety, and stocks lost.
- Adventure Records persist wins, losses, best combo, Perfect Blocks, Guard
  Breaks, Pursuit finishers, best rank, Energy Beam uses, and Object Swaps.

## U15: optional stories with restrained rewards

- **Missing Prize Envelope:** a movement/festival search rewarding 60 coins, a
  10% tournament-vendor discount, and the Recovered Prize Seal memento.
- **One Match Anyway:** an eliminated fighter offers an optional final match.
  The player may run, and a win grants a choice of Power +1 or Speed +1.
- **Controlled Flame:** using Fire Blast correctly prepares a meal for the next
  official fight. That fight gets +10 maximum HP and +5% defense. Retrying that
  official fight keeps the already-consumed boost; it cannot be farmed.
- **Alt and Rover:** Rrvvfo follows moved carts, misleading signs, maintenance
  paths, and service-area clues after they take a tournament maintenance worker
  and use him to disrupt the grounds. Alt wins the physical confrontation and
  escapes with Rover, then drops Alt's Weighted Necklace.
- Existing optionals now have modest authored rewards: Wade's lost fan grants
  Focus +1 and Wade's profile; the fake champion grants Defense +1 and the
  `Truth Breaker` title; the runaway dummy grants Speed +1; the festival photo
  grants a memento.
- There is one accessory/training-item slot. No rarity, gear score, random drop,
  or farming system was added.

### Alt's Weighted Necklace: exact values

While equipped, Power is multiplied by `1.04`. Movement, dash, and Energy
charge use the same mastery curve:

```text
multiplier = 0.95 + (0.05 * mastery / 100)
```

That is a 5% penalty at 0 mastery, about 3.35% at 33, about 1.7% at 66, and no
penalty at 100. It never changes input response, jump buffering, coyote time,
combo timing, Object Swap, or camera response. Meaningful equipped traversal,
fights, Perfect Blocks, Pursuits, and objectives add mastery; distance is
accumulated in spaced thresholds so standing still or pushing into a wall does
not farm it. Reaching 100 once grants Tournament Card Power +1 and Speed +1.

### Baseline Rrvvfo control changes

- run-start blend: 0.165s to 0.105s
- run-stop blend: 0.180s to 0.120s
- jump buffer: 0.10s to 0.14s
- coyote time: 0.09s to 0.12s
- hard landing recovery: 0.22s to 0.16s
- normal landing recovery: 0.13s to 0.08s
- dash steering: 1.6 to 2.4
- air control: 0.88, with analog magnitude and immediate redirect support

The dedicated combat retreat animation remains separate. Raw speed was not
inflated to hide response problems.

## U16: QoL, accessibility, and save safety

- Saved controls include camera sensitivity, X/Y inversion, and gentle recenter.
- Saved readability options include HUD scale, dialogue scale/speed,
  auto-advance, high contrast, reduced motion/shake/flashes, combat messages
  (`Full`, `Minimal`, `Off`), and objectives (`Full`, `Minimal`, `Off`).
- Interaction gets priority when a nearby world interaction and an ability could
  compete. Analog movement and camera input share one platform-neutral path.
- Desktop controller disconnect pauses safely. Optional fights return Rrvvfo to
  the exact stored hub position.
- Optional losses offer `Retry` and `Leave`. Official tournament losses offer
  only `Retry`; Story cannot be forfeited accidentally.
- Save schema 7 migrates schema 2-6 saves, including U11-U13 schema 6. It keeps
  existing Story flags and checkpoints, adds the Continue preview, selections,
  settings, records, rewards, and necklace progress, and maps the former
  `important` combat-message value to `minimal`.
- Existing backup, corruption fallback, checkpoint restore, and isolated
  Replay/test save behavior remain active.

## Performance and acceptance notes

Ranks and records update on combat events, the necklace has no equipment
simulation, and UI/save data are small fixed state. Old 3DS uses the same logic
with its existing low-cost native presentation. Automated coverage preserves
Chapter 1, Chapter 2, Plouke's finale, early-Replay locking, save migration, and
the rule that Shots of Agony remains unavailable.

Successful compilation and emulation are not real Old 3DS hardware
certification. Physical-device checks still required are sustained tournament
hub/final performance, memory over the Chapter 1-to-2 transition, text scale on
both panels, Circle Pad/dead-zone feel, controller/sleep recovery, and SD-card
save backup/fallback behavior.
