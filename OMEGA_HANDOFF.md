# OMEGA Update 1 handoff

OMEGA is applied on the `agent/omega-builds` branch from exact baseline commit `851b8613fac6451f62ef492ad0ee454622096f63`.

## Implemented scope

- Rrvvfo now has 31 authored Chapter 1 clips, including dedicated combat-ready, combat-relax, combat-advance, combat-retreat, and hard-landing motion.
- Hub idle remains separate from active fighting stance.
- Fire Blast uses the representable browser values: 22 Energy, 1.05-second cooldown, 15 damage, and 9 Guard damage.
- Main Road has four work beats, Forest four bells, Cliff five ledges, escalating guidance, two-sided transport recovery, and a persistent Lens-versus-southern-detour finish.
- Replay is isolated from the real Story save and imports QoL/seen-cutscene history without overwriting Continue.
- Save schema 5 uses stable checkpoint IDs, validated backup recovery, and captured checkpoint snapshots.
- Linux adds XDG/user-data save paths, Shift Dash, mouse Light/Block, and packages the required Rrvvfo PXSKEL.
- The 3DS build synchronizes the same cooked PXSKEL used by desktop before compilation.

## Verification completed locally

- OMEGA strict check: passed after the three documented patcher corrections.
- Shared automated suites: 5/5 passed.
- macOS universal build: x86_64 + arm64, signed, DMG verified.
- Nintendo 3DS build: valid native 3DSX.
- Desktop and 3DS ROMFS Rrvvfo PXSKEL hashes match: `6fab4bb0e4b5b879f02e82e1b190f1dd75901ab74888e9898d3ca88a21e77279`.

## Intentional limitations

- Browser Fire Blast projectile speed 500, radius 25, and clash power 1.3 still require a real projectile-object simulation rather than fake melee geometry.
- Graphics Update 2 and Optimize Update 3 are separate future passes.
- Real Old 3DS hardware remains the authority for performance, memory, text readability, model deformation, suspend/resume, and control feel.
- Chapter 2 has not started.
