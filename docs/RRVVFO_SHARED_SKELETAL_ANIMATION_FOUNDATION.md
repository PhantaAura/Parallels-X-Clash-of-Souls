# Rrvvfo Shared Skeletal Animation Foundation — Idle Proof

Scope: the minimum shared skeletal-animation foundation and one Rrvvfo `idle` proof clip. No run, dash, jump, attack, Object Swap, Lens of Truth, ability, facial, Chapter 2 or environment work is included.

## Shared animation architecture added

- `CharacterModelAsset` cooked format version 2 retains named clips, duration, loop policy, joint tracks, key times, LINEAR/STEP channel interpolation and translation/rotation/scale values. Version 1 bind-only assets remain loadable and safely produce no clips.
- `SkeletalAnimationPlayer` binds any `CharacterModelAsset`, switches named states, tracks clip time, supports playback speed, wraps looping clips, clamps non-looping clips and falls back to the bind pose for missing clips.
- Sampling uses linear interpolation for translation/scale and shortest-path quaternion slerp for rotation, then evaluates the existing parent hierarchy and multiplies each world joint by its retained inverse-bind matrix.
- CPU-skinned vertex positions feed both the macOS Metal geometry path and Linux software geometry path. Gameplay collision, movement and character world placement are unchanged.
- The player, clip format and sampler contain no Rrvvfo-specific pose logic and can be reused for Bark, Wade, Sage and other imported characters.

## Legacy idle reference inspected

- `work/legacy/Parallels-X-Fighting/assets/fighters/rrvvfo/rrvvfo-atlas.webp`, specifically `idle_01` through `idle_06`.
- `work/legacy/Parallels-X-Fighting/assets/fighters/rrvvfo/rrvvfo-animations.json`: six frames, 125 ms per frame, looping, constant ground pivot `[96, 178]`.
- `work/legacy/Parallels-X-Fighting/js/fighter-visuals.js`: gameplay visual state resolves to the named `idle` clip rather than embedding poses in the renderer.

The idle authoring was subsequently repaired against the actual six Legacy silhouettes rather than a generic breathing interpretation. Legacy still supplies the six key poses; the 3D sampler supplies the in-betweens.

## Idle asset authoring and storage

The source GLB contains no authored clips. The active DEV source is the
deterministically repaired `rrvvfo15.glb` export, copied into `rrvvfo-dev.glb`;
animation remains a deterministic sidecar. `scripts/author-rrvvfo-idle.py`
reads its existing joint names and bind rotations and deterministically generates
`assets/characters/rrvvfo/rrvvfo-idle.animation.json`. The character cooker
validates that sidecar against the existing skeleton and embeds the authored
clips into `rrvvfo-dev.pxskel`.

The repaired authored clip is 0.75 seconds, looping, with six Legacy key poses at 125 ms spacing plus a closure key at 0.750 s. It contains 35 joint tracks spanning torso, shoulders, arms, hands/fingers, head chain, pelvis, thighs, shins and feet. First and last samples match for a clean loop. No skeleton replacement, facial data or gameplay-code choreography is involved.

## Tests and builds

- `./scripts/build-linux.sh` — passed.
- `./scripts/test-linux.sh` — passed completely.
- Existing Chapter 1, menu, save/migration, model/material/39-joint/weight, content-export and architecture tests — passed.
- Focused animation tests — named lookup, missing-clip bind fallback, 39 skin matrices, sampled deformation, finite/bounded vertices, loop wrap and playback speed passed.
- Deterministic idle authoring and cooked asset regeneration compare byte-for-byte with committed files.
- Every required Linux SDL dummy/software review state, including the two new idle proofs, rendered successfully.
- Native Cocoa/Metal compilation remains unavailable in the Linux validation environment; the Mac source uses the same player, asset and sampled skin data.

## Review captures

- Bind pose: `review/screenshots/rrvvfo-idle-bind.png`
- Sampled idle: `review/screenshots/rrvvfo-idle-sampled.png`
- Legacy pose review: `review/screenshots/rrvvfo-idle-pose-1.png`, `rrvvfo-idle-pose-3.png`, `rrvvfo-idle-pose-5.png`, `rrvvfo-idle-pose-6.png`

The sampled capture lowers Rrvvfo's arms into the Legacy-derived relaxed posture and applies the small torso weight shift. Feet and world placement remain aligned with the bind capture, and no vertex explosion or mesh detachment is visible.

## Known issues and stop boundary

- The idle is a proof clip, not final animation polish; there is no blend/crossfade layer in this minimum patch.
- The repaired idle keys 35 joints. Cloth/hair secondary simulation and facial animation remain untouched.
- The Linux screenshot proves two sampled poses, not the full temporal smoothness of the loop. Mac remains final visual authority.
- The current CPU skin path is intentionally simple and correct for this DEV mesh; later performance work may move skinning to the GPU without changing shared clip/player state.
- No reduced Old 3DS animation/model asset is included.
- Stop after `idle`. No other clip or ability has started.
