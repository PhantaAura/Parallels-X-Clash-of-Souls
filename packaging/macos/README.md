# macOS packaging

Every update intentionally produces the **same public DMG filename**:

`Parallels-X-Clash-of-Souls.dmg`

The version belongs inside the app/build metadata and changelog, not in the filename. That gives the project one stable thing to replace/share every update.

Run on a Mac:

```bash
./scripts/build-macos.sh
```

Output:

```text
dist/
├── Parallels-X-Clash-of-Souls.dmg
└── Parallels-X-Clash-of-Souls.dmg.sha256
```

0.4A uses ad-hoc signing for local development. Steam/distribution signing + notarization are release engineering tasks, not gameplay architecture.
