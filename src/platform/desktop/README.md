# Desktop platform layer

Target: Windows `.exe` and macOS app/`.dmg` using the same core.

Likely renderer/input shell: SDL2/SDL3 or another portable native library after evaluation.
Platform code owns windows, audio devices, filesystem paths, controllers, and rendering — not gameplay rules.
