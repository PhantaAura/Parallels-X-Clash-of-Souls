# Parallels X 3.0R — Mac 0.4B

## Patch focus

This patch locks the shared five-slot ability hotbar contract before visual icon work.
The app icon selected during planning is intentionally **not implemented in this patch**.

### Shared hotbar contract

Rrvvfo Chapter 1 uses the Legacy slot identity:

1. Fire Blast — ready
2. ??? — technique not invented
3. Object Swap — ready
4. Lens of Truth — early / unstable
5. Solar Weave — visible for continuity, story-restricted here

The hotbar is now data in `px_core`, not Mac UI code. The Mac renderer and the future
3DS bottom screen consume the same five definitions. This prevents platform drift.

### Canon guard

Slot 2 deliberately does not reveal or enable Shots of Agony in Chapter 1. The remake
keeps the current chronology where the technique is invented later.

### Packaging

The macOS packaging output remains exactly:

`Parallels-X-Clash-of-Souls.dmg`

Internal build metadata advances to 0.4B; the DMG filename does not.
