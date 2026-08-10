# Rrvvfo 3D Model Integration — Phase 1

Scope: model import and proof only. No animation player, authored clip, ability presentation, face, gameplay redesign or Chapter 2 content is included.

Historical checkpoint note: this Phase 1 scope remains unchanged. The later shared animation foundation and its single idle proof are documented separately in `RRVVFO_SHARED_SKELETAL_ANIMATION_FOUNDATION.md`.

Visual acceptance repair: the original engineering-view screenshot was rejected. The shared Legacy training-field scene, world-space model scale, camera projection, and deterministic Linux comparison were repaired in `docs/RRVVFO_3D_PHASE1_VISUAL_ACCEPTANCE_REPAIR.md`. Review `review/screenshots/rrvvfo-legacy-comparison.png`; the importer described below remains intact.

## Source and integrity

- Source: `assets/characters/rrvvfo/rrvvfo-dev.glb`
- Original Phase 1 supplied filename: `rrvvfo(5).glb`
- Original Phase 1 SHA-256: `780e9894887fa7b1b95473eda57a66a6a3b1bd07f5eefa2da1e1c8e64f134725`
- Current artist source filename: `rrvvfo15.glb`
- Untouched artist-source SHA-256: `12956d3512493aaf89a10024417502d114658cf397ac2bc285bdf2f08b0fadf4`
- Current assisted-repair DEV SHA-256: `f43363e91b596c88ccc63a6b45ed8d50ec3c10243d3a313878915a79088cf887`
- Role: temporary, intentionally faceless DEV Rrvvfo
- Character root: `metarig`
- Skin: one skin, 39 joints
- Authored clips in source: none

The original Phase 1 source GLB was retained unchanged during importer bring-up.
The current DEV source retains the artist's shoe/cuff cleanup and adds a
deterministic torso/garment repair. Body, shirt, and jacket torso layers now
share the spine chain; shirt sleeves use their corresponding shoulder/upper-arm
bones; jacket panels no longer inherit arm/pelvis motion; and jacket sleeves use
the corresponding shoulder, upper arm, forearm, and hand. The 39-joint skeleton,
geometry, materials, and character design remain unchanged.

The Blender scene also exports three empty reference-plane nodes and a separate unskinned `Training Suit` reference mesh outside `metarig`. The cooker traverses only the character root, so those reference objects never enter gameplay. This is selection during import, not a source-model edit.

## Shared architecture

`scripts/cook-character-glb.py` deterministically converts the supported GLB subset to `rrvvfo-dev.pxskel`. The cooked file retains:

- bind-pose vertex positions and normals;
- material names/colors and submesh ranges;
- joint names and parent hierarchy;
- local bind transforms;
- inverse bind matrices;
- four joint indices and normalized weights per skinned vertex.

`CharacterModelAsset` validates and owns this portable data. `CharacterModelRepository` loads it once under the stable character id `rrvvfo`. Both the Mac Metal shell and Linux validation renderer consume the same repository. Runtime gameplay continues to supply Rrvvfo's existing player position, height and yaw, so replacing the visual does not alter collision/controller logic.

The system is not Rrvvfo-specific: later characters can register and load their own cooked assets through the same repository. Phase 2 may consume the retained skeleton data to add a shared animation player.

## Materials

The GLB preserves six named material slots. `Material.001` contains an exported white base-color factor, but `Red`, `Maroon`, `Skin`, `Hair` and `Head` contain no exported `baseColorFactor`; glTF would otherwise render all five white.

`rrvvfo-dev.materials.json` supplies replaceable DEV colors matching the established Legacy red/maroon/brown presentation without modifying the GLB. Mac quantizes lighting into cel-style bands. Linux uses the same colors with a software-rendered toon band/outline approximation for deterministic review.

## Legacy references inspected

- `Parallels-X-Fighting/assets/fighters/rrvvfo/rrvvfo-atlas.webp`
- `Parallels-X-Fighting/assets/fighters/rrvvfo/rrvvfo-animations.json`
- `Parallels-X-Fighting/docs/RRVVFO-SPRITE-PIPELINE.md`
- `Parallels-X-Fighting/js/fighter-visuals.js` model-independent visual-state/effect mapping boundary

Phase 1 used these only to preserve Rrvvfo's established presentation palette and to confirm that animation/gameplay mapping remains separate. No Legacy pose was authored into the bind pose. Frame-by-frame pose study remains mandatory before Phase 3 animation authoring.

## Validation

The model validation test proves the committed asset loads with:

- 2,524 vertices;
- 3,206 triangles;
- 7 submeshes;
- 6 named materials;
- 39 joints;
- all 2,524 cooked gameplay vertices weighted with normalized weights.
- all Chapter-1 clips keep shirt/jacket triangle edges within 0.590x..1.258x
  their bind lengths; both pre-repair sources failed at 0.155x..6.517x.

The Linux development shell provides `--review rrvvfo-model` for a clean model proof and `--review rrvvfo-legacy-comparison` for the deterministic player-facing comparison. Both render the faceless model's source bind pose inside the shared Legacy-derived Chapter 1 training field. The comparison state retains the runtime dialogue layer and Legacy-equivalent Rrvvfo/Sage staging. The normal exploration/combat/dialogue review states continue to exercise the usual runtime UI.

## Known issues and explicit Phase 1 boundaries

- The GLB has no authored clips. Rrvvfo therefore appears in its existing T-shaped bind pose; this is proof of mesh/rig/material import, not an idle animation.
- Linux uses an approximate software 3D projection for deterministic review, but now consumes the shared perspective camera and authored scene rather than a separate debug reconstruction. Mac Metal remains final visual authority.
- Native Cocoa/Metal compilation and visual approval cannot be performed in the Linux Codex environment.
- No reduced Old 3DS character asset has been cooked yet. The shared loader is portable, but Phase 1 does not claim the full 3DS runtime renders this desktop mesh.
- The source's missing five base-color factors require the replaceable DEV material sidecar until a corrected source export is supplied.
- Facial geometry, decals, eyes, expressions and facial rigging are deliberately absent.
- Phase 2, Phase 3 and Phase 4 have not started.

Do not begin animation work until the user approves the source model's scale, orientation, material interpretation and faceless presentation from the Mac build/review.
