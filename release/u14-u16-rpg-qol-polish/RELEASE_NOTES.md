# Parallels X 3.0R U16 - RPG and QoL Polish

This release applies cumulative U14, U15, and U16 to the U11-U13 Chapter 2
restoration branch.

## Player-facing changes

- Clean six-item main menu with a useful Continue card, functional Options,
  Extras, and whole-Story-only Replay unlocking.
- Fast S-E ranks and persistent Adventure Records for substantial fights.
- Missing Prize Envelope, One Match Anyway, Controlled Flame, and the Alt/Rover
  maintenance-worker side story, all with modest persistent rewards.
- Alt's Weighted Necklace as a single-slot training item with a mild 5%-to-0%
  mastery curve, +4% Power while equipped, and Power +1/Speed +1 at mastery.
- Faster, more predictable baseline Rrvvfo control: better redirect, jump
  buffer, coyote time, dash steering, and landing/run transitions.
- Saved camera, readability, combat-message, objective, and dialogue options.
- Optional-fight Retry/Leave, official-match Retry-only, safe save migration,
  and protected Story/Replay state.

## Compatibility

- Save schema 7 migrates U11-U13 schema 6 and earlier supported saves.
- Gameplay and content live in the shared engine for Mac, Linux, and Old 3DS.
- No Chapter 3 content is introduced.
- Shots of Agony remains unavailable.

See `docs/U14_U16_RPG_QOL_POLISH.md` in the source tree for exact control,
necklace, reward, persistence, and hardware-check details.

The packaged 3DSX is a devkitPro build. Real Old 3DS hardware certification is
not claimed by this release.
