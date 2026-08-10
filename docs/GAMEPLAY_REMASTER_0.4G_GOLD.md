# 3.0R 0.4G GOLD — Chapter 1 Gameplay Remaster / 3DS Gate

0.4G GOLD turns the Chapter 1 Legacy port into the first gameplay-remaster vertical slice. The story order, authored road, route structure, combat balance identity and ability chronology remain Legacy-led; the update concentrates on responsiveness, combat readability, animation coverage, failure recovery and platform portability.

## Legacy identity preserved

- Chapter 1 remains Sage setup → Object Swap field trial → full combat refresher → Tournament Road → river → Main/Forest/Cliff → relay → transport/cart → roadside encounter → Lens roadblock → Tournament Outskirts.
- No Bark/Wade reunion, Virek setup, Tournament Card or registration was moved into Chapter 1.
- Shots of Agony and other not-yet-invented abilities remain hidden.
- No player-facing mission architecture was restored.
- Rrvvfo's gameplay buffer uses the exact Legacy 2.9A.40.7.1.1 feel value: 0.135 seconds.
- Rrvvfo animation timing/key-pose vocabulary comes from the Legacy sprite metadata; 3D interpolation supplies the in-betweens.

## Movement and exploration QoL

- 100 ms jump buffering and 90 ms coyote/ground grace improve 3D traversal without changing jump height, gravity, route geometry or burst-Dash identity.
- Object Swap interaction checks get a small capped target-forgiveness margin without selecting a different target or solving the puzzle.
- Cliff guidance escalates only after repeated trouble.
- The Cliff World Delight reward is granted on actual completion rather than route selection.
- Precision relay completion surfaces a short Object Swap mastery reaction.
- Cart, roadside and Lens progress points surface compact checkpoint notices.
- Roadside combat returns Rrvvfo to the exact pre-fight road position instead of a generic fight-return point.
- Already-seen directed Chapter 1 cutscenes can be skipped in Replay with Cancel; first Story viewing remains paced normally. Seen state persists in saves.

## Combat feel remaster

- Combat input is retained through shared hit freeze instead of being eaten by presentation freeze.
- Hit freeze now actually pauses combat/world simulation during relevant fights.
- Normal hits, Perfect Block, Guard Break, Pursuit finisher, wall/ground reactions, final hits, clashes and Flow Cancel publish shared feedback intensity for renderers.
- Flow Cancel retains its Legacy identity and receives a short readiness/success cue instead of a new subsystem.
- Roadside knockback, launch height, stun/knockdown movement lock and landing presentation are reflected in shared runtime state.
- Player combat state maps to shared animation names rather than platform-specific pose logic.

## Rrvvfo animation coverage

The cooked faceless DEV Rrvvfo keeps the cleaned 39-joint skeleton and currently ships 26 shared clips:

`idle`, `fighting_stance`, `run`, `dash`, `jump_start`, `fall`, `land`, `light_1`, `light_2`, `light_3`, `heavy`, `launcher`, `air_light`, `air_heavy`, `pursuit_light`, `pursuit_heavy`, `grab`, `block`, `perfect_block`, `hurt`, `charge`, `counter`, `breaker`, `fire_blast`, `object_swap`, `lens_activate`.

The authoring sidecars remain deterministic and cooker-owned. Gameplay selects named clips; renderers do not hardcode Rrvvfo choreography.

## Presentation

- Mac and Linux consume the same shared animation and combat-feedback state.
- Character lighting uses a three-band toon/cel direction rather than ordinary smooth realistic lighting.
- Shared Fire Blast, Object Swap and Lens presentation markers are emitted from runtime state for platform renderers.
- Linux deterministic review states now cover locomotion, charge, heavy, Fire Blast, Object Swap and Lens alongside the earlier idle/model/Chapter 1 states.
- Final Mac outline/facial presentation remains a later visual polish task and is not required for the first Old 3DS compatibility gate.

## Controller parity

Desktop controller mappings follow the shared modern profile. Left stick moves; West/North/South/East map to Light/Heavy/Jump/Grab; RB blocks; RT launches; D-Pad Up/Down/Left/Right maps to Dash/Charge/Counter/Breaker; LB layers visible ability slots over the face buttons and D-Pad Up; LT is an alternate Dash on desktop. Linux remaps held buttons immediately when the LB layer changes so actions do not remain stuck on the old layer.

## Old 3DS Gate 0

Gate 0 is source-ready and intentionally diagnostic rather than final visual parity. It boots the same `RuntimeSession`, Chapter 1 registries, combat, exploration, dialogue, training, adventure and save code as desktop.

The ROMFS now packages the exact shared `rrvvfo-dev.pxskel`. On hardware the gate loads the actual 2,524-vertex / 39-joint / 26-clip cooked model, runs the shared `SkeletalAnimationPlayer`, follows the runtime's current animation state, freezes animation during shared hit freeze, and CPU-skins a representative vertex sample every frame. A top-screen model inset plots those skinned vertices so exploding deformation or clip problems can be seen while the full shared Chapter 1 runtime is being exercised. The bottom screen reports model/joint/clip/skin status alongside objective/combat diagnostics.

This is specifically meant to answer the first hardware questions: can the Old 3DS load the real cooked asset, evaluate the 39-joint hierarchy, sustain representative CPU skinning while the real Chapter 1 runtime runs, save/load correctly, and accept the full semantic control set?

Gate 1 comes after the first real hardware result and replaces the diagnostic inset/world view with the native Citro3D shaded 3D character/world renderer. Gate 0 does not pretend Citro2D points are the shipping renderer.

## Validation boundary

Linux builds/tests can validate shared gameplay and portable source boundaries. macOS remains the final high-tier visual authority. The current Linux environment does not include devkitPro/devkitARM/libctru/Citro3D, so no `.3dsx` is claimed until the 3DS toolchain is actually present and the target is compiled.

Chapter 2 remains blocked until the Chapter 1 desktop golden slice and the first Old 3DS compatibility gate are exercised.
