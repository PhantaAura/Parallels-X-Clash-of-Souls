# Legacy Chapter 2 Parity Audit

Status: audit-only for 0.4F. Chapter 2 gameplay is deliberately not implemented in this patch.

Authority: exact Legacy `2.9A.40.7.1.1`, build/cache `29a40711-chapter2-hub-return-20260802`. The build manifest records 483/483 smoke checks and identifies this revision's sole patch as returning the Rejected Challenger, Fake Champion and Runaway Dummy optional fights to the hub.

## Exact Legacy source inventory

Primary content:

- `js/story/rrvvfo-mission-2.js` — chapter controller, inline dialogue, NPCs, hub state, tournament sequence, fights, UI and persistence.
- `js/story/chapter2-hub-quests.js` — districts, mandatory tasks, optional quests and readiness rules.
- `js/arena/arena-stages.js` — `tournament-hub` and tournament arena stage/camera definitions.

All recursively imported player-experience dependencies:

- `js/arena/arena-combat-data.js`, `arena-controls.js`, `arena-math.js`, `arena-mode.js`, `arena-stage-renderer.js`, `arena-stages.js`, `pursuit-combat.js`, `signature-combat.js`, `stage-personality.js`, `webgl-renderer.js`
- `js/build-info.js`, `js/combat-core.js`, `js/focus-recovery.js`, `js/input-runtime.js`, `js/input.js`, `js/mastery-records.js`, `js/qol-settings.js`, `js/roster.js`, `js/sonic-battle-dialogue.js`, `js/training-trials.js`
- `js/story/chapter2-hub-quests.js`, `combat-manual.js`, `connected-world.js`, `core-fun.js`, `field-skills.js`, `hub-camera.js`, `hub-collision.js`, `hub-landmark-art.js`, `lost-year-data.js`, `quest-overhaul.js`, `quest-variety.js`, `revisit-loop.js`, `rpg-pacing.js`, `rrvvfo-mission-2.js`, `story-engine.js`, `story-interiors.js`, `story-map.js`, `story-progression.js`, `story-reliability.js`, `story-rpg-ui.js`, `story-ux.js`, `world-delight.js`

## Exact scene and state order

1. Continue from Tournament Outskirts into the west-gate arrival and registration delay.
2. Announcer/Sage orientation and free tournament-hub exploration.
3. Reunite Rrvvfo with Bark and Wade at their Legacy hub placement.
4. Complete the four required pre-tournament activities: recover/reconstruct the Lost Bracket, Wade's Shortcut race, Bark's cracked-ring repair, and the festival exhibition.
5. Preserve the festival hub, changing NPC state, optional quests, activities, interiors, repeat visits and optional fight returns.
6. Inspect the bracket and complete registration.
7. Opening ceremony.
8. Spectate Hailey versus Plouke.
9. Rrvvfo round one versus Hamual; return to the hub.
10. Rrvvfo quarterfinal versus Daniel; return to the hub.
11. Spectate Bark versus Pouki; return to the hub.
12. Rrvvfo semifinal versus Wade; return to the hub.
13. Final preparation and Rrvvfo versus Plouke.
14. Fatigue sequence, attempted Fire Awakening, beam clash, ring-out resolution and Plouke/Sage reveal.
15. Save the Chapter 2 ending and continue directly to Chapter 3 in normal Story Mode.

## Dialogue sets

The exact sets are inline in `rrvvfo-mission-2.js`, anchored by `start`, `talkToAnnouncer`, `meetBarkAndWade`, `talkToWade`, `talkToBark`, `useRegistration`, `startTournamentStep`, `startHaileyPrelim`, `startPoukiExhibition`, `startFinalPreparation`, `startFinal`, `beginFinalFatigue`, `offerAwakening`, `triggerAwakeningAttempt`, `finishBeamClash` and `afterTournamentFight`. Optional dialogue sets are anchored by `talkToLocal`, `talkToVendor`, `talkToVeteran`, `beginPracticeBrawl`, `beginFoodQuest`, `beginFakeChampionQuest`, `beginLostFanQuest`, `beginDummyQuest`, `beginPrizeCartQuest`, `beginRejectedChallengerQuest`, `offerCurrentPloukeClue` and each corresponding completion function.

Port these arrays exactly unless the locked overrides below require the smallest possible edit. Sage's knowledge of the Organization of the Red's Lost-Year activity must grow from witnessed events; do not add pre-tournament knowledge.

## Map, NPC and activity data

`tournament-hub` bounds are `x -1800..1800`, `z -1120..1120`; west-gate spawn is approximately `(-1510,80)`. Camera: yaw `34°`, FOV `45`, base distance `1040`, height `455`, focus clamps `1710/1030`.

Tournament arena stage `tournament`: bounds `x -750..750`, `z -450..450`; spawns `(-370,78)` and `(370,-78)`; camera yaw `35°`, FOV `44`, base distance `1100`, height `500`, distance range `1120..1760`, focus clamps `290/185`. Preserve ring-out boundary, ropes, crowd, stage personality and fight-specific framing.

Districts from `chapter2-hub-quests.js`: West Gate `(-1510,80,r300)`, practice `(-1120,560,r380)`, market `(-500,620,r360)`, registration `(-120,-560,r360)`, central `(-280,40,r420)`, spectator `(640,-520,r390)`, stadium `(1280,40,r420)`.

Named hub actors from `createNpcs`: Sage `(-1340,-20)`, Announcer `(-250,-500)`, Fan `(-720,250)`, Vendor `(-520,570)`, Registration Worker `(-80,-390)`, Old Competitor `(-860,-280)`, Practice Fighter `(-1120,560)`, Bark `(120,130)`, Wade `(250,20)`, Loud Champion `(420,600)`, Lost Wade Fan `(520,-690)`, Family `(-360,650)`, Mechanic `(-980,760)`, Cashier `(600,-380)`, Rejected Challenger `(-1450,520)`, two roaming grunts, bracket kiosk, photo stand and exhibition marker.

The cracked-ring supports are west/south/east at about `(-1370,760)`, `(-1120,825)`, `(-870,760)`. Wade's race uses five physical checkpoints across market, practice, registration roof, spectator and stadium districts. Lost Bracket uses the Wade fan card, Bark vendor-roof card and veteran maintenance-cart card, with a two-card plus administration reconstruction route. Plouke investigation clues are stillness, positioning, timing and edge.

Interiors: Tournament Administration, medical area and backstage. Preserve map markers, hub camera follow/snap, authored collisions, world shortcuts, ambient dialogue, festival photo stand, food activity, bracket display and return spawns.

## Encounters and optional fights

- Bark optional spar: guard archetype, `78` HP, `85` XP.
- Fake Champion: `100` HP, `55` XP.
- Runaway Dummy: `100` HP, `65` XP; tutorial emphasis on parry, pursuit and grab.
- Rejected Challenger: `100` HP, `70` XP; player chooses Power or Speed bonus.
- Optional hub grunts: `44` HP, `40` XP; failed escape may enter the same fight.
- Tournament: Hamual (`110` XP, heavy, `1.18` HP scale), Daniel (`125` XP, trickster, `1.04`), Wade (`90` HP, `140` XP, rushdown), Plouke final (`100` HP, ranged).
- Spectator prelims/exhibitions and the final beam-clash sequence are gameplay, not menu summaries.

Optional quests: Controlled Flame, Fake Champion, Wade's Biggest Fan, Dummy on the Loose, Missing Prize Envelope and One Match Anyway. Preserve rewards, quest journal state, hub return and changing dialogue. The exact 40.7.1.1 return fix is required behavior, not an obsolete patch artifact.

## Abilities, RPG, UI and save behavior

Combat uses the complete shared Legacy combat vocabulary, hotbar rules, AI archetypes, pursuit/signature systems and stage personality. Abilities remain chronology-gated.

Legacy story progression uses XP thresholds `[0,100,250,450,700,1000,1360,1780,2260,2810]`. Base derived stats are HP `100 + 4*(level-1)`, Power `10 + round(1.25*growth)`, Defense/Speed `10 + round(.75*growth)`, Focus `10 + growth`, plus saved bonuses. Preserve compatible progression behavior in shared C++.

Legacy information surfaces include objective/detail, attack strip, interaction prompt, dialogue, hub map, story menu, stats/XP, field-skill journal, quest journal/tracker, area title, activity HUD, fight card, level-up, beam clash and results. Their information structure is authoritative; browser markup is not.

Save state includes mandatory task completion, optional quest phases, Plouke clues, race/ring/bracket/exhibition state, hub/revisit/interior state, progression, tournament checkpoint and return spawn. Replay restores underlying save after a whole-chapter replay. Do not recreate mission completion menus or individual replay entries.

## 0.4F classification ledger

| Feature | Classification | Evidence / action |
|---|---|---|
| Tournament arrival continuation, hub, festival, reunion, required activities | MISSING | No Chapter 2 content is registered in 3.0R; port later from the sources above. |
| Optional quests/fights and fixed hub returns | MISSING | Preserve all, including the exact 40.7.1.1 return behavior. |
| Tournament bracket, spectator matches and Rrvvfo fights | MISSING | Port as continuous Chapter 2 internal states. |
| Shared combat vocabulary | DIFFERENT | 0.4E has a portable subset; verify every fighter/encounter constant during Chapter 2 port. |
| Shared camera/world data model | MATCHING | Registry-owned, platform-neutral presentation model can carry the authored hub/arena data. |
| Tournament Card timing/content | APPROVED CURRENT-CANON OVERRIDE | At registration all three receive cards; first player-facing RPG display; automatic growth plus compact `+1/+2/+3` selected bonus. |
| Sage secretly knowing the Organization's Lost-Year operation | APPROVED CURRENT-CANON OVERRIDE | Prohibited. Preserve Legacy observation-driven suspicion and investigation. |
| Legacy mission ID, exit screen, mission replay and browser UI | OBSOLETE TECHNICAL ARCHITECTURE | Translate content into shared C++ and whole-chapter Story/Replay. |
| Chapter ending | MISSING | Must transition directly toward Chapter 3, not a mission/chapter menu. |

## Current-document disposition

Retained: Chapter 1/2 boundary, current ability chronology, Tournament Card addition and continuous Story/whole-chapter Replay architecture. Rejected: any Chapter 2 rewrite that removes the Legacy tournament hub, festival, reunion, activities, Plouke content, optional fights, hub returns or changing hub state. No other supplied current route document establishes a conflict.
