# Parallels X 3.0R — U7A–U10 Tournament Golden Candidate

This candidate finishes the cumulative U7A, U7B, U8, U9, and U10 batch on the shared `px_core` runtime.

Included:

- shared cinematic action language and Chapter 1 staging retrofit;
- literal gold Object Swap, purple Lens recovery, and no Chapter 1 Shots of Agony exposure;
- seamless Chapter 1 to Tournament Grounds transition with an internal autosave request;
- Tournament Hub, Lost Bracket, practice content, required tournament order, stock/ring-out rules, and canonical Plouke result;
- Tournament Card progression and schema-6 save round-trip support;
- Mac and Old 3DS Tournament Card/UI presentation plus tournament world-state dressing;
- repaired, whitespace-tolerant, idempotent U7–U10 updater and source verifier.

Validated before packaging:

- `scripts/verify-update7-10.py`: 26/26 source invariants passed;
- `git diff --check`: passed;
- clean native CMake configure/build: passed;
- `scripts/test.sh`: five test targets, headless gameplay smoke, runtime smoke, and content export passed;
- macOS universal app: arm64 and x86_64, ad-hoc signature verified;
- DMG integrity: verified by `hdiutil`;
- Old 3DS: devkitPro build produced a valid 3DSX with matching cooked model data.

Real Old 3DS testing is still required for sustained frame pacing, memory pressure, suspend/resume, SD-card autosave durability, bottom-screen text readability, and long-session model deformation.
