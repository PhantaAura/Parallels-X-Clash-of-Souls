# Legacy Chapter 4 Parity Audit

Status: audit-only for 0.4F. Chapter 4 gameplay is deliberately not implemented in this patch.

Authority: full final Chapter 4 in exact Legacy `2.9A.40.7.1.1`.

Continuity lock: the Lost-Year facility and combatants belong to the Organization of the Red (`organization_red`). `Hollow Watcher` remains the protected Legacy enemy name; camel-case names below may quote exact Legacy state identifiers.

## Exact Legacy source inventory

Primary content:

- `js/story/rrvvfo-chapter-4.js` — controller, coordinates, dialogue, party flow, fights, optional quest, UI and ending.
- `js/story/chapter4-content.js` — ordered chapter state, beacon/cavern/lift/potion/signal content and completion rules.
- `js/story/chapter4-enemy-roles.js` — enemy role behavior and readable combat identity.
- `js/arena/arena-stages.js` — `echo-village`, `echo-caverns`, `echo-sky` and `echo-mountain` stage/camera data.

All recursively imported player-experience dependencies:

- `js/arena/arena-combat-data.js`, `arena-controls.js`, `arena-math.js`, `arena-mode.js`, `arena-stage-renderer.js`, `arena-stages.js`, `pursuit-combat.js`, `signature-combat.js`, `stage-personality.js`, `webgl-renderer.js`
- `js/build-info.js`, `js/combat-core.js`, `js/focus-recovery.js`, `js/input-runtime.js`, `js/input.js`, `js/mastery-records.js`, `js/qol-settings.js`, `js/roster.js`, `js/sonic-battle-dialogue.js`, `js/training-trials.js`
- `js/story/chapter4-content.js`, `chapter4-enemy-roles.js`, `combat-manual.js`, `connected-world.js`, `core-fun.js`, `field-skills.js`, `hub-camera.js`, `hub-collision.js`, `lost-year-data.js`, `quest-overhaul.js`, `quest-variety.js`, `revisit-loop.js`, `rpg-pacing.js`, `rrvvfo-chapter-4.js`, `story-engine.js`, `story-interiors.js`, `story-map.js`, `story-progression.js`, `story-reliability.js`, `story-rpg-ui.js`, `story-ux.js`, `world-delight.js`

## Exact required scene order

`chapter4-content.js` defines this ordered spine:

1. `opening`
2. `villageReached`
3. `barkWadeArrive`
4. `beaconRestored`
5. `cavernsEntered`
6. `liftPartsRecovered`
7. `villageDefended`
8. `mountainDecision`
9. `mountainEntered`
10. `mountainSignals`
11. `hollowWatcherDefeated`
12. `lookoutReached`
13. `shadowArrival`
14. `chapterSaved`

Expanded player flow: Rrvvfo wakes after being out for days near the damaged teleporter; reaches Echo Village; reunites with Bark and Wade; explores village life; restores the beacon with party field roles; repairs the cavern approach; enters Echo Caverns; opens the fire/earth/lightning routes and recovers lift parts; returns to defend the village; receives the mountain decision; may complete the protected Old Man Potion Quest; leaves Bark and Wade behind; climbs the mountain solo; triangulates any two Organization signals; defeats the Hollow Watcher; reaches the summit; uses the pebble and Object Swap to reach the floating Lookout; retains player control on the Lookout; approaches Shadow; says `Shadow... it's been a while.`; collapses. Chapter 5 owns the discussion that follows.

## Dialogue sets

Exact inline sets are anchored by `showOpening`, `reachVillage`, `barkWadeArrival`, village landmark/party functions, `finishBeaconRepair`, `enterCaverns`, `collectLiftPart`, `returnToVillageAfterParts`, village-defense functions, `startOldManQuest`, `revealRyuzankaro`, Ryuzankaro phase/return/finale functions, `enterMountain`, signal functions, Hollow Watcher fight functions, Lookout landing/approach functions and `commitCompletion`. Port the complete arrays, including optional branches, not a later summary.

Protected exact ending line: `Shadow... it's been a while.` Do not append the Chapter 5 discussion to Chapter 4.

## Maps, positions and camera data

Spawns: region `(-1363,100)`, village `(-820,60)`, beacon `(-680,300)`, cavern `(-980,0)`, village return `(777,450)`, mountain `(-1240,0)`, Lookout `(1120,-330)`.

`echo-village`: bounds `x -1600..1600`, `z -920..920`; camera yaw `38°`, FOV `44`, base distance `1120`, height `490`. Village points include gate, damaged teleporter, beacon, cavern, old man, recovery area, party route, mountain gate, water lift, apothecary passage and revisit shrine. Beacon nodes are Rrvvfo signal blocker `(-520,520)`, Bark stone support `(-350,610)` and Wade energy feed `(-180,520)`.

`echo-caverns`: bounds `x -1200..1200`, `z -700..700`; camera yaw `42°`, FOV `43`, base distance `980`, height `430`. Element doors: fire `(-720,-260)`, earth `(-120,280)`, lightning `(470,-250)`. Lift parts: drive gear `(720,280)`, resonance coil `(890,-40)`, brake assembly `(720,-330)`.

`echo-sky`: bounds `x -720..720`, `z -480..480`; camera yaw `35°`, FOV `45`, base distance `890`, height `420`. It owns the Ryuzankaro aerial phase.

`echo-mountain`: bounds `x -1450..1450`, `z -760..760`; camera yaw `40°`, FOV `44`, base distance `1060`, height `500`. Signal routes: bridge `(-720,240)`, Organization relay `(-180,-280)`, Lookout signal `(430,250)`; any two required. Preserve the summit pebble near `(1010,-250)` and floating Lookout at `(1120,-330)`.

Lookout constants: height `500`, landing `2.4s`, fade delay `520ms`, completion delay `1850ms`, bounds `x 920..1320`, `z -505..-155`, entrance `(1120,-205, y500)`. The Lookout is a playable space, not a cutscene-only teleport.

## Party, quests, encounters and rewards

Party field gameplay assigns meaningful fire/earth/lightning roles to Rrvvfo, Bark and Wade. Preserve beacon repair, cavern approach, elemental doors, lift recovery, village defense, team commands/status, changing village state, interior/revisit content and Echo chimes.

Protected Old Man Potion Quest, both routes:

1. Gather Ember Bloom `(460,600)`, Rootstone `(180,520)` in the cavern, Thunder Dew `(-980,-520)` and Triad Seed `(760,-620)`.
2. Discover the Old Apothecary Formula and use any two valid catalysts.

Both routes lead to Ryuzankaro and must remain playable. Preserve impact control, aerial combat, Vibration Sense/Lens relation, Object Swap return, village finale and sealing gameplay. Rewards include Vibration Sense, Lens Mastery 1, improved Object Swap range, Ryuzankaro codex and Echo team badge where Legacy awards them.

Encounters:

- Village defense: three-role team battle/waves culminating in the Organization Commander (`1.25` HP scale, `260` XP).
- Ingredient swarm where invoked: two-enemy team composition, `0.72` lead scale, `120` XP.
- Ryuzankaro aerial: `2.05` HP scale, `520` XP; later return/finale phases are distinct gameplay states.
- Hollow Watcher: `1.55` HP scale, `330` XP. Three scan phases learn repeated action, then range, then route/approach. The player breaks prediction by varying behavior.

## Mechanics, UI, save and ending

Mechanic introductions/rewards include party field roles, elemental route repair, team commands, Vibration Sense, Lens mastery, Object Swap progression, adaptive-pattern combat and the summit Object Swap traversal. Implement all through shared engines.

Legacy information surfaces include objective/detail, attack strip, interaction prompt, dialogue, team status/waves, enemy-role indicator, Watcher scan state, Vibration overlay, map, story menu/stats/field skills, quest journal, area title, choice/QTE, rewards and completion. Preserve their information structure without browser/mobile implementation.

State persists the 14 ordered steps, village/cavern/mountain location, party field actions, beacon nodes, cavern doors, lift parts, defense, both potion-route states, ingredients/formula/catalysts, Ryuzankaro checkpoints, rewards, mountain signals, Watcher memory/results, Lookout state, interiors/revisits and whole-chapter checkpoints. Replay must preserve the underlying normal-story save.

## 0.4F classification ledger

| Feature | Classification | Evidence / action |
|---|---|---|
| One shared engine and internal continuous scene states | MATCHING | 3.0R already supplies the correct non-mission architecture; Chapter 4 remains content/data. |
| Wake-up, Echo Village, Bark/Wade, party route, beacon/lift/caverns | MISSING | No Chapter 4 content exists in 3.0R. Port full Legacy later. |
| Village defense and changed/revisited world state | MISSING | Preserve sequence and state effects. |
| Old Man Potion Quest, both routes and Ryuzankaro | MISSING | Protected optional content; never cut. |
| Vibration Sense, Lens/Object Swap mastery and rewards | MISSING | Preserve at Legacy placement, subject only to ability chronology. |
| Solo mountain, Hollow Watcher and Organization signal routes | MISSING | Port exact route/AI behavior through shared systems. |
| Summit pebble, Object Swap, playable Lookout and Shadow approach | MISSING | Required full ending, including exact line and collapse. |
| Bark/Wade search and defense timing | APPROVED CURRENT-CANON OVERRIDE | They may have searched/fought while Rrvvfo was missing; serious defenses occur after his mountain departure. Change only the minimum lines/state timing. |
| Shared combat/camera/content interfaces | DIFFERENT | Portable foundations exist; party, quest-route, adaptive-AI and multi-region persistence interfaces still need later content integration. |
| Browser UI, DOM/mobile controls, mission ID/completion and mission replay | OBSOLETE TECHNICAL ARCHITECTURE | Translate behavior into shared C++ and whole-chapter Story/Replay. |
| Chapter 5 discussion inside Chapter 4 | APPROVED CURRENT-CANON OVERRIDE | Prohibited. Chapter 4 ends with approach line and collapse. |

## Current-document disposition

Retained: the minimal Bark/Wade search-versus-defense timing clarification, current ability chronology, continuous Story Mode and whole-chapter Replay. Rejected: any shortened Chapter 4 summary that removes village/party/cavern/revisit content, either potion route, Ryuzankaro, Watcher, solo mountain traversal, playable Lookout or the exact ending. No other supplied current route material establishes a conflict.
