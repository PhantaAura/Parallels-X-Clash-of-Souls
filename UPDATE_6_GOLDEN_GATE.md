# Parallels X 3.0R — Updates 4–6

This source is the complete Chapter 1 Golden Gate line after OMEGA + Update 2 + Update 3.

## Update 4 — Pure Optimize + Model Stability

- Reuses skeletal animation local/world/skin buffers instead of allocating every sample.
- Caches bind-pose decomposition once when a model is bound.
- Reuses static Chapter-1 hotbar catalog data.
- Old 3DS static world geometry stays resident in the shared linear GPU buffer until stage/blocker state changes; only dynamic actors/markers/skinned geometry copy each frame.
- Character preview invalidates the resident static upload so gameplay always restores it.
- No gameplay values, story, route counts, animation durations, or visual content were removed.

## Update 5 — Combat Feel + Rrvvfo Personality

- Legacy sprite key poses remain authoritative.
- Light/Heavy/Launcher key silhouettes are aligned to the real combat active windows.
- Heavy/Launcher gain deliberate fast-striker recoveries without changing gameplay durations.
- Pursuit gains a committed chase silhouette and readable lock marker.
- Exploration turning is snappier while combat facing remains unchanged; Dash and hard landings gain low-cost directional/ground dust cues.
- Successful Flow Cancel gains a presentation-only 0.18 s snap clip and VFX.
- First-time Flow Cancel hint retires after successful use.
- Update 2 ability-failure explanations remain intact; Flow Cancel teaching now retires after successful use.
- The exact 0.75 s Legacy hub idle is untouched.
- `combat_retreat` remains a dedicated positive-time backpedal.

## Update 6 — Chapter 1 Golden Gate Final

- Main / Forest / Cliff gain stronger authored visual language without changing 4 / 4 / 5 route requirements.
- Cliff completion gets a short quiet-overlook reaction instead of another training fight.
- Existing Lost Competitor/cart successes leave visible outskirts aftermath.
- Pause Objective History distinguishes NOW vs DONE and includes a playtest build label.
- Existing competitor/cart success states become visible at the outskirts without duplicate actors or extra UI popups.

## Locked boundaries

- Chapter 1 only.
- Shots of Agony remains unavailable in Chapter 1.
- No Chapter 2 registration, Tournament Card, Bark/Wade reunion, or later-story material.
- Optimization may reduce redundant work/code but must not cut gameplay, story, graphics, animations, routes, Object Swap targets, Lens paths, or platform parity.

## Golden Gate validation

Before declaring this source hardware-approved:
1. run shared CMake/CTest suite;
2. run source architecture tests;
3. regenerate/cook Rrvvfo PXSKEL;
4. build macOS DMG, Linux shell, and 3DSX;
5. play all three routes;
6. save/reload/restart across the collapse detour, roadside optional fight, Lens/direct and southern detour;
7. test real Old 3DS suspend/resume, memory, text readability, frame pacing, and model deformation.

Real Old 3DS hardware remains the final authority for the portable target.
