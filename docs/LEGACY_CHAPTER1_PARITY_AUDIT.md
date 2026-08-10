# Legacy Chapter 1 parity audit

Authoritative Legacy build: `Parallels X Prototype 2.9A.40.7.1.1 — Chapter 2 Optional Fight Hub Return Fix`.

This document freezes the audit performed before the Chapter 1 port. Legacy mission files are evidence for content and behavior only. Their mission architecture is not a 3.0R feature.

## Legacy source inventory

| Legacy source | Chapter 1 evidence examined |
|---|---|
| `manifest.json` | Exact build identity, cache namespace and save-schema provenance. |
| `README.md` | Build scope, Chapter 1/Chapter 2 boundary and declared regression status. |
| `js/story/lost-year-data.js` | Old mission ordering, progression fields, rewards and unlocks. |
| `js/story/lost-year-story.js` | Rrvvfo content order and the old player-facing mission handoff. |
| `js/story/story-polish.js` | Old Chapter 1 grouping and duration. |
| `js/story/story-reliability.js` | Legacy checkpoint/repair expectations; technical architecture only. |
| `js/story/rrvvfo-mission-0.js` | Object Swap anchors, spawns, exact dialogue, trigger rules and result. |
| `js/story/rrvvfo-mission-1.js` | Exact Sage dialogue and all seven guided refresher checks/timings. |
| `js/story/rrvvfo-road-hub.js` | Entire Tournament Road state machine, coordinates, routes, NPCs, QTE, optional fight, checkpoint, Lens roadblock and arrival. |
| `js/story/rrvvfo-mission-2.js` | Chapter 2 boundary only: registration/Card-era content must not move into Chapter 1. |
| `js/story/story-engine.js` | Shared Legacy arena/exploration command semantics and transition behavior. |
| `js/story/story-map.js` | Road map bounds, objective markers and connected-zone presentation. |
| `js/story/hub-camera.js` | Hub follow-camera and fight-framing behavior. |
| `js/story/hub-collision.js` | Shared hub collision boundary behavior. |
| `js/story/hub-landmark-art.js` | Road mountains, fences, shrines, reeds, stones, trail dressing and landmarks. |
| `js/story/combat-manual.js` | Tutorial/manual content cadence; menu architecture is obsolete. |
| `js/story/story-progression.js` | Optional roadside fight level parity and restorative transitions. |
| `js/story/story-rpg-ui.js` | Player-facing combat/action information. |
| `js/story/connected-world.js` | Route visits, reconnects and world-state intent. |
| `js/story/revisit-loop.js` | Legacy fast-travel/revisit behavior; not part of the Chapter 1 critical path. |
| `js/story/quest-variety.js` | Runaway Tournament Cart state and rank rules. |
| `js/story/core-fun.js` | Optional road rewards/build hooks; no chapter-owned combat engine. |
| `js/story/field-skills.js` | Object Swap field mastery and precision-relay bookkeeping. |
| `js/story/rpg-pacing.js` | Arrival/development/crisis/aftermath pacing phases. |
| `js/story/world-delight.js` | Cliff overlook discovery reward. |
| `js/arena/arena-stages.js` | Exact `TRAINING_FIELD` and `TRAINING_ROAD` bounds, spawns, cameras, floors and scenery. |
| `js/arena/arena-stage-renderer.js` | World-space stage primitive interpretation. |
| `js/arena/stage-personality.js` | Stage boundary/geometry interactions used by fights. |
| `js/arena/arena-mode.js` | Active shared combat behavior, timing, AI, abilities, clashes, pursuit and hit resolution. |
| `js/arena/arena-combat-data.js` | Fighter-specific normal definitions and exact Rrvvfo/Sage startup/active/recovery data. |
| `js/arena/arena-math.js` | Hit volumes, facing, projectile overlap and movement math. |
| `js/arena/pursuit-combat.js` | Burst Dash identities, Pursuit windows/follow-ups/tech, wall splat and ground bounce rules. |
| `js/arena/arena-controls.js` | Semantic combat action set and hub-camera inputs. |
| `js/combat-core.js` | Shared KO, Energy, Counter, Breaker, juggle and edge-pressure constants. |
| `js/input-runtime.js` and `js/input.js` | Semantic input intent; browser event plumbing is obsolete. |
| `js/roster.js` | Rrvvfo/Sage identity and presentation references. |

## Scene and state inventory

The actual Legacy player experience is:

1. Sage Object Swap setup.
2. Three-anchor Object Swap field trial.
3. Object Swap result exchange.
4. Sage tournament briefing and combat manual.
5. Seven-step guided refresher.
6. Post-spar pride exchange.
7. Training Road departure exchange.
8. Physical travel east to the river.
9. Far-bank rock Object Swap; the rock trades places and the river remains solid.
10. Legacy `CHOOSE A ROUTE` panel: Main, Forest or Cliff. The selected path is then traversed physically.
11. Main: worker/fallen-log Fire Blast event. Forest: north trail. Cliff: three authored jump markers.
12. Route reconnection and three-position Object Swap relay.
13. Stranded transport wheel recovery.
14. Runaway Tournament Cart four-input sequence with safe retry/fail-forward.
15. Roadside stretch with seven existing NPCs and the Lost Competitor decision.
16. Optional roadside challenger: leave or first-to-one-KO fight, with rematch/leave after defeat.
17. Tournament checkpoint exchange.
18. Mandatory Lens roadblock reveal.
19. Tournament Outskirts arrival exchange and Chapter 1 boundary.

## Exact authored numerical inventory

- Road map: X `-1550..1450`, soft Z `-640..640`; stage bounds X `-1450..1450`, Z `-900..900`.
- `TRAINING_FIELD`: yaw `38°`, FOV `43`, base distance `900`, height `410`, floor `1160x780`, surface `1080x700`.
- `TRAINING_ROAD`: yaw `38°`, FOV `45`, base distance `980`, height `430`, floor `3100x1980`, surface `2980x1860`, hub distance `1010`.
- River collision: X `-25..175`, Z `-700..700`; it never becomes walkable.
- Far-bank rock: `(230, 0)`.
- Fallen tree: X `285..395`; route-dependent center/north/south extents.
- Cliff jumps: `(300,330)`, `(355,400)`, `(412,350)`, radius `72`.
- Relay: `(468,-96)`, `(522,96)`, `(570,0)`; gate release delay `0.52s`.
- Gate collision: X `555..645`, all road widths.
- Lens collision: X `1045..1120`, all road widths.
- Road NPC bases: student `(-900,300)`, traveler `(-520,-300)`, worker `(365,300)`, competitor `(730,-285)`, fan `(940,285)`, vendor `(1130,260)`, painter `(1225,-300)`.
- NPC drift: X amplitude `45` at `0.55`, Z amplitude `24` at `0.48`, with authored phases.
- Runaway cart: Right, Jump, Left, Right; `6.2s`; safe retry, then fail-forward.
- Roadside fight: Rrvvfo `(790,70)`, rival `(940,-70)`, Energy `45`, first to one KO.
- Sage refresher: movement `72`; charge `20->75`; Lens `35->60`; readable telegraph; three unblocked hits.
- Perfect Block window `0.11s`; Flow Cancel window `0.30s`, cost `8`; Counter cost `18`, cooldown `2.4`; Breaker cost `60`, cooldown `6.5`; juggle limit `6`.
- Rrvvfo Dash: speed `720`, duration `0.22s`, cooldown `0.30s`, invulnerability `0.11s`, side feint `22`.

## Legacy-to-0.4D.1 classification

| Feature | Status before port | Audit finding |
|---|---|---|
| Three-anchor field trial | PRESENT AND MATCHING | Behavior/dialogue match after 0.4D.1 coordinate translation. |
| Seven-step Sage refresher | PRESENT AND MATCHING | Required checks and energy starts match; combat underneath remains simplified. |
| Field/Road cameras and stage registry | PRESENT AND MATCHING | Shared data owns yaw/FOV/distance/height and Mac consumes perspective world geometry. |
| Burst Dash and Flow Cancel cost | PRESENT AND MATCHING | Dash is finite, edge-triggered and not held run. |
| Early hotbar | PRESENT AND MATCHING | Fire Blast, Object Swap and Lens only; Shots and unavailable skills are absent. |
| Continuous Story/Chapter architecture | PRESENT AND MATCHING | Internal SceneSteps are not player-facing missions. |
| Sage/opening punctuation | PRESENT BUT DIFFERENT | Several Legacy curly apostrophes were normalized to ASCII. |
| River Object Swap | PRESENT BUT DIFFERENT | 0.4D.1 moves Rrvvfo but does not move the rock to his old position. |
| Route-choice panel | PRESENT BUT DIFFERENT | 0.4E replaced the Legacy popup with a physical fork. 0.4F restores the Legacy panel; the selected routes remain physical geography. |
| Route challenges | PRESENT BUT DIFFERENT | Main is abbreviated; Forest is close; Cliff lacks its three jump checks. |
| Relay | PRESENT BUT DIFFERENT | Coordinates and gate timing differ. |
| Road collision | PRESENT BUT DIFFERENT | Dojo, branch-wide and Lens-wide blockers are incomplete. |
| Road presentation | PRESENT BUT DIFFERENT | Authored base exists, but bridge incorrectly reads intact and dynamic road obstacles/markers are incomplete. |
| Shared combat | PRESENT BUT DIFFERENT | Only immediate Light/Heavy/Launcher/Grab/Block/Flow Cancel subset exists. |
| Road NPC presence | PRESENT BUT DIFFERENT | Five ambient figures exist; exact seven placements, motion, prompts and interactions do not. |
| Transport recovery | MISSING | Exact Object Swap event and dialogue absent. |
| Runaway Tournament Cart | MISSING | Exact four-input authored sequence absent. |
| Lost Competitor decision | MISSING | NPC dialogue, branch flag and repeat states absent. |
| Optional road challenger | MISSING | Choice, dialogue, first-to-one-KO fight, Flow Cancel hint and defeat recovery absent. |
| Tournament checkpoint | MISSING | Exact three-line exchange absent. |
| Mandatory Lens roadblock | PRESENT BUT DIFFERENT | 0.4D.1 invents an optional detour; Legacy requires Lens. |
| Outskirts arrival | MISSING | Scene ID exists but exact five-line scene is absent. |
| Energy-signature post | OBSOLETE TECHNICAL/NEW CONTENT | Invented in 0.4D; not part of Legacy Chapter 1 and must be removed. |
| Bark/Wade reunion | OBSOLETE TECHNICAL/NEW CONTENT | Invented in 0.4D; not part of Legacy Chapter 1 and must be removed. |
| Legacy mission selector/replay/overlays | OBSOLETE TECHNICAL ARCHITECTURE | Content is internalized into one Chapter 1; never port these surfaces. |
| Browser DOM/mobile/global patches | OBSOLETE TECHNICAL ARCHITECTURE | Replaced by shared C++ and platform layers. |
| Chapter 1 Shots unlock | OBSOLETE DUE TO CURRENT CANON | Shots of Agony debuts in Season 2 and stays invisible here. |
| Registration, cracked ring, Tournament Card | OBSOLETE DUE TO CURRENT CANON IN CHAPTER 1 | Protected for Chapter 2; Chapter 1 ends at the outskirts/overlook. |

## Dialogue/canon decision before implementation

All player-facing Legacy Chapter 1 dialogue remains canon-compatible under the supplied Chapter 1 rules and is to be preserved exactly. No semantic dialogue rewrite is authorized. UTF-8 punctuation is preserved. The only chronology edits are removal of the obsolete Chapter 1 Shots unlock and stopping before registration/Card content; neither requires changing a Legacy spoken line.
