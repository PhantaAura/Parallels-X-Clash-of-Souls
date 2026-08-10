# Rrvvfo Assisted Garment-Weight Repair

## Source safety

- Untouched artist file: `rrvvfo15.glb`
- Artist-file SHA-256: `12956d3512493aaf89a10024417502d114658cf397ac2bc285bdf2f08b0fadf4`
- Repaired project source: `assets/characters/rrvvfo/rrvvfo15-weight-repaired.glb`
- Active identical source: `assets/characters/rrvvfo/rrvvfo-dev.glb`
- Active SHA-256: `f43363e91b596c88ccc63a6b45ed8d50ec3c10243d3a313878915a79088cf887`

The repair changes only JOINTS_0 and WEIGHTS_0 for the selected body/garment
regions. It does not change geometry, vertex positions, topology, materials,
skeleton nodes, bind transforms, or inverse-bind matrices.

## Repaired regions

- Underlying body torso: 47 vertices
- Shirt torso: 28 vertices
- Shirt sleeves: 28 vertices
- Jacket torso panels: 102 vertices
- Jacket shoulder/collar caps: 16 vertices
- Jacket sleeves/cuffs: 96 vertices

Body, shirt, and jacket torso regions now use a consistent `spine` through
`spine.004` gradient. Only the lowest body waist retains controlled pelvis
influence. Shirt sleeves use the corresponding shoulder and upper arm. Jacket
sleeves use the corresponding shoulder, upper arm, forearm, and hand; torso
panels contain no forearm, hand, or pelvis contamination.

Every repaired vertex is normalized, cleaned below 0.01, and limited to four
influences for the shared cooker/runtime.

## Acceptance evidence

- Deterministic cook: 2,524 vertices, 3,206 triangles, 39 joints, 6 materials,
  7 submeshes, and 26 Chapter-1 clips.
- Active cooked asset SHA-256:
  `daacfa1f6537565719a5c54735c7cde73b3a1b40430a16557a2e18e5c3349e23`.
- Desktop and 3DS ROMFS cooked assets compare byte-for-byte.
- Five CMake test suites pass.
- Garment edge coherence across all clips at 20%, 50%, and 80% phase:
  repaired `0.589602x..1.25783x`; both pre-repair sources
  `0.155324x..6.51652x` and fail the regression gate.
- Stress-pose renders are under `review/screenshots/weight-repair/` for dash,
  heavy, air-heavy, Object Swap, Lens, and hurt, plus retained old comparisons.

The native Mac app's accessibility review hook did not respond during automated
QA, so the stress captures use the same cooked asset and the engine's exact
`SkeletalAnimationPlayer` matrices through the cross-platform offline review
renderer. This validates deformation without claiming final in-game lighting.
