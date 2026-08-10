# Desktop Legacy UI + Rrvvfo Parity — 0.4H.2

This pass carries the approved 3DS presentation corrections into the Mac and Linux versions without creating a gameplay fork.

## Locked parity

- Mac, Linux and 3DS continue to run the shared `RuntimeSession`, Chapter 1 scene flow, combat, movement, route geometry, saves and facing state.
- The shared UI palette now uses the Legacy navy/blue field, white text and borders, warm-gold hierarchy, and red/orange route identity accents.
- Rrvvfo's repaired cooked skeletal model, animation player and lightweight expressive face layer are used by desktop gameplay and the Rrvvfo menu/route presentation.
- The playable Rrvvfo path no longer substitutes the old procedural humanoid when the required model is unavailable. Linux fails startup with a clear error; Mac blocks story launch and labels the missing-model bay.
- Opponent-aware facing and reverse run playback remain owned by the shared runtime, so every version keeps the same Sonic Battle / Superstar Saga-style character readability.
- Performance reductions remain presentation-only and may not alter Rrvvfo's silhouette, gameplay collision, ability timing, personality or chapter content.

## Mac presentation

- Native Cocoa front-end moved to the Legacy-blue card language.
- Story Mode and Rrvvfo Route Select expose a transparent character bay rendered by Metal with the exact live cooked model and shared confident face.
- Gameplay panels use hard-edged paper/black/gold Legacy hierarchy rather than the previous rounded charcoal cards.
- Rrvvfo dialogue no longer draws a fake head/body portrait; his expressive 3D actor remains the character authority.
- The objective panel was enlarged so tutorial instructions and progress are not clipped.

## Linux presentation

- SDL front-end moved to the same Legacy navy/blue/gold hierarchy.
- Story Mode, Rrvvfo Route Select and Rrvvfo dialogue render the actual cooked model with shared animation and face expressions.
- Menu animation advances the shared idle clip.
- Gameplay and route presentation cannot silently replace playable Rrvvfo with the procedural placeholder.

## Tutorial repair

- A dash with no movement direction no longer creates a zero-distance burst or satisfies the tutorial.
- Movement progress rejects impossible teleport-sized frame deltas and displays a capped `MOVE n / 72` value.
- Step 1 now explicitly asks the player to dash while moving.
- The end-to-end Chapter 1 test still completes all seven training exercises, checkpoint resume, Lens reaction, final spar and post-training transition.

## Validation

- Shared C++/Mac build: passed.
- Five automated test suites: passed.
- Linux platform source strict C++17 syntax check: passed on macOS.
- Mac universal app build and ad-hoc signature verification: passed.
- Mac DMG created with SHA-256 sidecar.
- Native Linux executable packaging remains a Linux-host task because macOS cannot produce the SDL/Linux binary.
- The rebuilt native Mac screenshot pass is pending only because the Mac locked before Computer Use could reopen it.
