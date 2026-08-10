# Rrvvfo 3D Phase 1 — Visual Acceptance Repair

Scope: repair the Phase 1 visual proof around the already imported, intentionally faceless DEV Rrvvfo. This change does not add animation, facial work, abilities, Chapter 2 content, or a replacement character model.

## Result

The deterministic review state is:

```bash
./scripts/run-linux.sh --review rrvvfo-legacy-comparison
```

Its captured frame is `review/screenshots/rrvvfo-legacy-comparison.png`.

The state uses the shared Chapter 1 training-field presentation consumed by Mac and Linux. Linux applies a reduced software renderer to that content; it does not substitute a separate review-only map.

## Legacy source inspected

Exact source: `Parallels X Prototype 2.9A.40.7.1.1 — Chapter 2 Optional Fight Hub Return Fix`.

- `js/arena/arena-stages.js`: `TRAINING_FIELD` bounds, spawn points, 38-degree camera, 43-degree field of view, clear/fog values, grass surface, tile lines, central ring/cross, rails, banks, trees, and stage draw order.
- `js/arena/arena-stage-renderer.js`: stage geometry construction, material/color treatment, and authored layer order.
- `js/arena/arena-mode.js`: initial camera state, target/follow behavior, fighter placement, and stage/fighter ordering.
- `js/arena/webgl-renderer.js`: perspective and look-at projection behavior.
- `js/story/rrvvfo-mission-0.js` and `js/story/rrvvfo-mission-1.js`: Chapter 1 training-field use and Rrvvfo/Sage staging context.
- `assets/fighters/rrvvfo/rrvvfo-atlas.webp` and `assets/fighters/sage/sage-atlas.webp`: established silhouettes, scale, color balance, and screen importance.
- The supplied Legacy gameplay screenshot: final composition reference for the elevated field, character diagonal, center circle, rails, trees, and bottom dialogue layer.

## Preserved model-import architecture

The repair does not change or recook the supplied GLB or cooked character asset. It preserves:

- `CharacterModelAsset`;
- `CharacterModelRepository`;
- the deterministic GLB cooker;
- the original Phase 1 `rrvvfo-dev.glb` identity is preserved in history; the active DEV source has since advanced to the assisted garment-weight repair derived from `rrvvfo15.glb`, SHA-256 `f43363e91b596c88ccc63a6b45ed8d50ec3c10243d3a313878915a79088cf887`;
- current cooked model: 2,524 vertices, 3,206 triangles, 7 submeshes, and 6 materials;
- the retained 39-joint hierarchy, inverse bind matrices, joint indices, and normalized skin weights;
- the shared Mac/Linux character-model binding and repository;
- the existing importer, deterministic-recook, rig, material, and skin-weight tests.

Rrvvfo remains in the source bind/T-pose because animation belongs to a later phase.

## Presentation repair

- Reauthored the shared training-field presentation from the Legacy stage definition: elevated rectangular turf, lower base, subtle surface tiles, segmented center ring/cross, perimeter rails/posts, block banks, and block-canopy trees.
- Preserved the Legacy stage origin, route width, camera yaw, field of view, clear/fog language, and upper-left/lower-right Rrvvfo/Sage composition.
- Projected the imported GLB in world space at its shared authored height. The Linux review no longer places it as a tiny fixed-size screen marker.
- Added a perspective software world canvas that consumes the same shared camera and scene primitives as the Mac renderer.
- Removed debug perspective guides, generic diagonal road bands, and the debug model-review banner from player-facing Linux world reviews.
- Replaced Sage's stick-figure fallback with an intentional chunky mentor blockout that preserves the Legacy silhouette, scale, and location without claiming to be final character art.
- Corrected stage/actor painter layering, back-face handling, and per-vertex fog so the field and imported model remain readable in the deterministic Linux frame.

## Acceptance comparison

| Question | Result | Evidence |
|---|---|---|
| Would a Legacy player recognize the location without being told? | Yes | The elevated green training field, central ring/cross, low rails, block trees/banks, sky balance, and diagonal fighter layout reproduce the supplied frame's dominant read. |
| Is Rrvvfo approximately the correct visual size relative to the world? | Yes | The GLB is projected at the shared 154-unit world height and occupies Legacy-like screen importance instead of appearing as a tiny marker. |
| Does the camera communicate the same gameplay space? | Yes | The state uses the shared Legacy-derived 38-degree yaw, 43-degree field of view, height, distance, target, and perspective projection. |
| Are the major landmarks recognizably equivalent? | Yes | Surface boundary, center circle/cross, perimeter rail, corner/edge trees and banks, and the Rrvvfo/Sage diagonal occupy equivalent locations. |
| Is anything player-facing still obviously debug geometry? | No for the approval state | Guide lines, debug road bands, stick figures, and the debug banner are absent. Sage is visibly an intentional clean blockout, not a final NPC model. |

## Validation

```text
./scripts/test-linux.sh
PASS: Chapter 1 core regression
PASS: menu, Story So Far, routes, manual, and continuity regression
PASS: faceless DEV Rrvvfo bind pose, materials, 39-joint hierarchy, and skin weights
PASS: deterministic GLB recook
PASS: portable/shared-content architecture boundary
PASS: every required SDL dummy/software review state

./scripts/capture-linux-review.sh
PASS: all deterministic Linux review frames recaptured at 1280x720
```

Native Cocoa/Metal compilation remains unavailable in this Linux environment. The Old 3DS shell remains outside this visual-acceptance repair.

## Known remaining visual differences

- Rrvvfo remains in T-pose; no clip player or authored animation has started.
- Sage is an intentional mentor blockout rather than a final model.
- Linux cel lighting, fog, occlusion, and anti-aliasing are reduced approximations of the Mac Metal path.
- The remake retains the current 0.4F.1 dark dialogue presentation rather than recreating the older white Legacy dialogue panel; UI redesign was not part of this repair.
- Fine foliage, textures, character portraits, face work, effects, and final lighting remain later art work. The simple block-tree language is retained deliberately because it is part of this Legacy location's recognizable composition.

Phase 1 stops here pending visual approval. Do not begin animation work from this repair.
