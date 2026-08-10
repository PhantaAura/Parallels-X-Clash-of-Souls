# Rrvvfo DEV model and lightweight face layer

`rrvvfo-dev.glb` is the temporary, intentionally faceless Rrvvfo source model.
The current DEV source is the assisted garment-weight repair derived from the
artist-supplied `rrvvfo15.glb` (source SHA-256
`12956d3512493aaf89a10024417502d114658cf397ac2bc285bdf2f08b0fadf4`).
The untouched artist file remains outside the project. The reproducible repaired
copy is `rrvvfo15-weight-repaired.glb`, and the identical active
`rrvvfo-dev.glb` has SHA-256
`f43363e91b596c88ccc63a6b45ed8d50ec3c10243d3a313878915a79088cf887`.
It must not be remodeled or given facial geometry as part of engine integration.

The approved runtime face is intentionally separate from this source GLB. The
shared `character_face` presentation attaches small cel-shaded eyes, brows and
mouth shapes to the existing `spine.006` head joint. It supplies neutral,
half-blink, blink, confident, focused, grit, hurt and shout states without
adding bones, textures, morph data or cooked-model vertices. Mac and Linux draw
the selected face mesh; the Old 3DS inset uses a fixed maximum of eight marks.
No head, hair, body, garment or weight data is changed by the face layer.

`scripts/repair-rrvvfo-garment-weights.py` repairs only 47 underlying torso
vertices, all 56 shirt vertices, and all 214 jacket vertices. It uses the shared
spine chain for the three torso layers, isolates both short sleeves, rebuilds
each jacket sleeve from shoulder through hand, cleans sub-0.01 influences, and
limits export to four normalized weights. Geometry, materials, skeleton,
inverse-bind matrices, and all unrelated vertices remain byte-identical.

The GLB contains the `metarig` character root, a 39-joint skin, body/clothing
meshes, and unrelated Blender reference objects. The deterministic cooker only
exports descendants of `metarig`; this keeps reference planes and the separate
training-suit reference mesh out of gameplay without changing the GLB.

The first five GLB material slots retain semantic names but contain no exported
`baseColorFactor`. `rrvvfo-dev.materials.json` supplies replaceable DEV colors
derived from the established red/maroon/brown Legacy presentation. It does not
change model geometry or rigging.

Regenerate the shared desktop asset with:

```sh
python3 scripts/repair-rrvvfo-garment-weights.py \
  path/to/rrvvfo15.glb \
  assets/characters/rrvvfo/rrvvfo15-weight-repaired.glb
python3 scripts/author-rrvvfo-idle.py \
  assets/characters/rrvvfo/rrvvfo-dev.glb \
  assets/characters/rrvvfo/rrvvfo-idle.animation.json
python3 scripts/author-rrvvfo-core-animations.py \
  assets/characters/rrvvfo/rrvvfo-dev.glb \
  assets/characters/rrvvfo/rrvvfo-idle.animation.json \
  assets/characters/rrvvfo/rrvvfo-core.animation.json
python3 scripts/cook-character-glb.py \
  assets/characters/rrvvfo/rrvvfo-dev.glb \
  assets/characters/rrvvfo/rrvvfo-dev.pxskel \
  --settings assets/characters/rrvvfo/rrvvfo-dev.materials.json \
  --animations assets/characters/rrvvfo/rrvvfo-core.animation.json
```

`rrvvfo-idle.animation.json` remains the deterministic six-frame Legacy idle
source. `rrvvfo-core.animation.json` preserves that exact 0.75-second clip and
adds Chapter-1 locomotion, combat, defense and ability clips. Their timing and
key-pose vocabulary are derived from Legacy 2.9A.40.7.1.1; smooth 3D motion is
created by skeletal interpolation between those authored key silhouettes.
The GLB itself remains unchanged and contains no embedded clips.
