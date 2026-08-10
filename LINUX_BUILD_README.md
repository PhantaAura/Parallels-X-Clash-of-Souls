# Parallels X: Clash of Souls 3.0R — Linux

The Linux version uses the same shared gameplay, combat, Chapter 1, save, quest, dialogue, and presentation state as macOS and 3DS. Linux is not a separate remake.

## Build on Linux

Install a C++17 compiler, CMake, Python 3, and SDL2. On Ubuntu/Debian:

```sh
sudo apt-get install build-essential cmake libsdl2-dev python3
```

Then run:

```sh
./scripts/test.sh
./scripts/build-linux.sh
./build-linux/ParallelsX
```

The GitHub Actions workflow in `.github/workflows/linux-build.yml` performs the same tests and produces a native x86_64 Linux package on every push to `main`.

## Current packaging note

The repository handoff includes a Linux source/build package because macOS cannot link or verify a Linux ELF executable. The GitHub Linux runner is the authority for the native binary; after the first successful workflow run, download the artifact named `Parallels-X-Clash-of-Souls-3.0R-Linux-x86_64`.
