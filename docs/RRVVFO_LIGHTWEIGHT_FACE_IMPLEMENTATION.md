# Rrvvfo lightweight face implementation

## Scope

The approved face pass adds only Rrvvfo's eyes, brows and mouth. The source and
cooked model remain byte-identical: 2,524 vertices, 3,206 triangles, 7
submeshes, 6 materials, 39 joints and 26 body clips. Head shape, hair, skin,
clothing, weights, movement, combat and Chapter 1 content are unchanged.

## Animation treatment

The shared face layer provides neutral, half-blink, blink, confident, focused,
grit, hurt and shout states. A slow clock creates a natural three-stage blink.
Dialogue maps Rrvvfo's authored `annoyed` and `realizing` tags into focused and
confident faces. Locomotion, defense, attacks, abilities and hit reactions choose
readable startup/active/recovery expressions from the same shared table.

All face points are weighted 100% to the existing `spine.006` head joint, so
they follow every current animation without an additional bone or skin matrix.
The desktop expression meshes are immutable and prebuilt once, with a hard
budget of 48 tiny triangles for the most detailed expression.

## Old 3DS budget

The 3DS ROMFS model is unchanged. The native character renderer reuses the same
head-attached face triangles as desktop, capped at 48 tiny triangles for the
most detailed expression. There are no facial textures, texture uploads, new
bones or morph buffers, and the renderer reuses its reserved frame storage.
Native hardware FPS and memory headroom still need the existing Old 3DS
approval run.

## Actual-model review

The deterministic review sheet renders the real cooked Rrvvfo model in one
unchanged idle body pose so every difference on the sheet comes from the face:

`review/screenshots/face-implementation/rrvvfo-face-model-sheet.png`
