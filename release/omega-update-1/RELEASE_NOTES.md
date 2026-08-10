# Parallels X 3.0R — OMEGA Update 1 candidate

This prerelease contains the completed OMEGA builds currently requested:

- Nintendo 3DS homebrew (`.3dsx`)
- Universal macOS application disk image (`.dmg`, Intel and Apple Silicon)
- Original OMEGA source overlay ZIP

Linux packaging is intentionally deferred for a later pass.

Local verification:

- Shared C++ build succeeded.
- All five local test suites passed after synchronizing the freshly cooked Rrvvfo model into 3DS ROMFS.
- Native 3DS compilation succeeded and produced a valid 3DSX.
- The universal Mac application is signed and the DMG verified successfully.
- Desktop and 3DS ROMFS use the same Rrvvfo PXSKEL hash.

This remains a candidate. Real Old 3DS hardware remains the authority for performance, memory, deformation, text readability, and suspend/resume testing.
