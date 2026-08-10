# 0.4G GOLD validation record

Validated in the Linux development environment on the 0.4G GOLD source tree.

## Completed successfully

- `cmake --build build -j2`
- `ctest --test-dir build --output-on-failure`: 5/5 suites passed (`px_core_tests`, `px_menu_tests`, `px_model_asset_tests`, `px_skeletal_animation_tests`, `px_source_architecture_tests`).
- Deterministic Rrvvfo idle sidecar regeneration matched the committed file byte-for-byte.
- Deterministic 26-clip Rrvvfo core-animation sidecar regeneration matched byte-for-byte.
- Deterministic cooked `rrvvfo-dev.pxskel` regeneration matched byte-for-byte.
- The 3DS ROMFS Rrvvfo cooked asset matches the desktop cooked asset byte-for-byte.
- 25 deterministic Linux graphical review states rendered successfully through SDL dummy/software mode.
- Persistent 0.4G review screenshots were captured for Chapter 1 exploration/combat and selected Rrvvfo locomotion/combat/ability poses.
- `scripts/build-3ds.sh` fails truthfully with exit code 2 when `DEVKITPRO` is unset instead of pretending to produce a 3DS executable.

## Environment boundary

The Linux environment does not contain devkitPro/devkitARM/libctru/Citro2D/Citro3D, so the 3DS target could not be compiled here and no `.3dsx` is included. Gate 0 source and ROMFS packaging are prepared for that compile. The next validation event is a real devkitPro build followed by an Old 3DS XL hardware run.

The convenience `scripts/test-linux.sh` performs the strict Linux build, fresh CMake test build, deterministic asset checks and all graphical review states in one command. In this constrained sandbox that combined command exceeded the per-command execution window, so the constituent validations above were executed separately and completed successfully.
