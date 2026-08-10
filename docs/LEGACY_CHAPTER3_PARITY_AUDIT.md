# Legacy Chapter 3 Parity Audit

Status: audit-only for 0.4F. Chapter 3 gameplay is deliberately not implemented in this patch.

Authority: final Chapter 3 in exact Legacy `2.9A.40.7.1.1`.

Continuity lock: the active faction is the Organization of the Red (`organization_red`). Exact camel-case state/function names below are quoted Legacy source identifiers, not valid new faction IDs.

## Exact Legacy source inventory

Primary content:

- `js/story/rrvvfo-chapter-3.js` — controller, coordinates, inline dialogue, investigation, encounters, UI and transitions.
- `js/story/chapter3-content.js` — required sequence, evidence, mandatory/optional story definitions, state normalization and completion.
- `js/arena/arena-stages.js` — `after-hours-tournament`, `resonance-facility` and `remote-highlands` stage/camera data.

All recursively imported player-experience dependencies:

- `js/arena/arena-combat-data.js`, `arena-controls.js`, `arena-math.js`, `arena-mode.js`, `arena-stage-renderer.js`, `arena-stages.js`, `pursuit-combat.js`, `signature-combat.js`, `stage-personality.js`, `webgl-renderer.js`
- `js/build-info.js`, `js/combat-core.js`, `js/focus-recovery.js`, `js/input-runtime.js`, `js/input.js`, `js/mastery-records.js`, `js/qol-settings.js`, `js/roster.js`, `js/sonic-battle-dialogue.js`, `js/training-trials.js`
- `js/story/chapter3-content.js`, `combat-manual.js`, `connected-world.js`, `core-fun.js`, `field-skills.js`, `hub-camera.js`, `hub-collision.js`, `lost-year-data.js`, `quest-overhaul.js`, `quest-variety.js`, `rpg-pacing.js`, `rrvvfo-chapter-3.js`, `story-engine.js`, `story-interiors.js`, `story-map.js`, `story-progression.js`, `story-reliability.js`, `story-rpg-ui.js`, `story-ux.js`, `world-delight.js`

## Exact required scene order

`chapter3-content.js` defines this ordered 33-state spine:

1. `opening`
2. `sabotageInvestigationStarted`
3. `ringEvidence1Found`
4. `ringEvidence2Found`
5. `ringEvidence3Found`
6. `sabotageConfirmed`
7. `workerQuestioned`
8. `securityQuestioned`
9. `medicalWorkerFirstConversationComplete`
10. `strangeManWarningSeen`
11. `medicalWorkerRevisited`
12. `strangeManHatCollected`
13. `maintenanceInvestigationStarted`
14. `hiddenInfrastructureFound`
15. `sageTrailFound`
16. `findSageObjectiveStarted`
17. `projectHollowFacilityEntered`
18. `tournamentDataDiscovered`
19. `projectHollowNameRevealed`
20. `realSageFound`
21. `facilityLockdownStarted`
22. `teleporterEscapeStarted`
23. `sageBlueCloneCreated`
24. `rrvvfoEnteredTeleporterRoom`
25. `blueCloneIdentityRevealed`
26. `blueCloneLessonSeen`
27. `blueCloneTechniqueFoundationLearned`
28. `teleporterActivated`
29. `blueCloneDisappeared`
30. `rrvvfoTeleportedToEchoRegion`
31. `rrvvfoUnconscious`
32. `echoOperationTimeSkipStarted`
33. `chapterSaved`

Player flow: enter the damaged after-hours tournament; inspect three ring clues; establish sabotage; question worker, security and medical witnesses; receive the Strange Man warning; revisit the contradictory medical account; collect the abandoned hat; follow maintenance evidence below the ring; confirm Sage independently followed the trail; enter the Organization of the Red's Lost-Year facility; inspect tournament data; find Sage; survive facility lockdown; escape toward the unstable teleporter; Sage creates the blue clone; Rrvvfo and the clone pass the closing door while Sage remains outside; learn the clone's principle without unlocking Shots of Agony; activate the teleporter; the clone disappears; Rrvvfo reaches the Echo Region and collapses; time advances into Chapter 4.

## Dialogue sets and protected uncertainty

Exact inline sets are anchored by `showOpeningScene`, `inspectSabotageEvidence`, `questionSabotageWitness`, `beginMedicalWitness`, `beginStrangeManWarning`, `revisitMedicalWorker`, `collectStrangeManHat`, `inspectStrangeManHatWithLens`, `investigateMaintenanceEntry`, `inspectHiddenInfrastructure`, `inspectSageTrail`, `revealProjectHollow`, `findRealSage`, `beginLockdownFight`, `beginTeleporterEscape`, `startDoorSequence`, `finishBlueCloneDoorSequence`, `showBlueCloneLesson`, `activateTeleporter`, `enterRemoteRegion` and `commitCompletion`. Optional dialogue sets are anchored by `startOptionalQuest`, `runReplacementQuest` and their activity functions.

The case board deliberately records the medical contradiction without resolving it. The source names multiple possibilities but confirms none. Preserve the warning, disappearance and hat; do not replace uncertainty with a memory-manipulation explanation. Sage reaches the Organization facility independently based on evidence he witnesses. Do not add pre-tournament knowledge or setup exposition.

## Maps, camera and state data

After-hours tournament reuses the Chapter 2 hub geometry with night presentation. Spawn `(1040,110)`; Strange Man `(-1510,-760)`; east support `(1260,330)`; maintenance entry `(1190,40)`. Ring evidence: damaged support `(1280,120)`, unregistered component `(1450,280)`, open maintenance panel `(1160,-140)`. Worker `(760,560)` and security `(960,-240)` are mandatory witnesses.

Facility spawn `(-900,0)`. `resonance-facility` bounds `x -1180..1180`, `z -720..720`; camera yaw `34°`, FOV `46`, base distance `940`, height `390`. Preserve hidden-infrastructure, Sage-trail, tournament-record, scanner, containment, door, clone and teleporter placements from the controller.

Remote spawn `(-820,130)`. `remote-highlands` bounds `x -1250..1250`, `z -820..820`; camera yaw `39°`, FOV `44`, base distance `1040`, height `475`. It is the transition destination, not a shortened explanatory epilogue.

The controller also defines after-hours districts, a six-point night route, three ring collectors, five bag-search locations, four Lens-trail points and exact optional activity locations. Preserve collision, interior, hub-camera, map and revisit data through shared systems.

## Evidence, NPCs, quests and encounters

Evidence: weakened support, unregistered component, maintenance access, worker testimony, security testimony, medical testimony/contradiction, Sage trail and tournament data. Mandatory story groups are sabotage, Strange Man/witnesses and the Organization facility.

Optional quests: Unpaid Snacks, One Last Match, Pouki Equipment, Fake Ploukes, Prize Envelope, Announcer's Final Announcement, Late Fan, Cleanup Echoes, Medical Follow-up and Controlled Flame Follow-up. Preserve the public/private booth recordings, bracket puzzle, night traversal, ring sweep, bag evidence board, Lens use and all reward/state effects.

Encounters: optional Early Contestant (`0.88` HP scale, `85` XP), Organization Scanner (`1.08`, `150` XP), Organization Containment Unit (`1.22`, `190` XP), plus recorded-attack and door escape gameplay. Preserve fight restart/assist behavior through the shared combat and checkpoint systems.

## UI, save and transition behavior

Legacy information surfaces: objective/detail, attack strip, prompts, dialogue, area title, map, case board/evidence tracker, optional quest journal, story menu/stats/field skills, Lens contradiction presentation, task/choice panels, fight status and completion. Preserve information hierarchy without browser markup.

State persists the 33 required flags, evidence set, three mandatory stories, ten optional quest states, investigation activities, location/interior, world/revisit data and checkpoints. Whole-chapter Replay must restore the player's underlying save after replay. Chapter 3 ends by flowing directly to Chapter 4.

## 0.4F classification ledger

| Feature | Classification | Evidence / action |
|---|---|---|
| One shared engine and internal continuous scene states | MATCHING | 3.0R already supplies the correct non-mission architecture; Chapter 3 remains content/data. |
| Full sabotage/evidence/witness sequence | MISSING | No Chapter 3 content exists in 3.0R. Port later in exact order. |
| Strange Man warning, contradiction, disappearance and hat | MISSING | Preserve ambiguity exactly. |
| Maintenance route, Organization facility, Sage investigation, lockdown and escape | MISSING | Port from controller/content/stage sources; use stable ID `organization_red`. |
| Blue Sage clone and principle lesson | MISSING | Preserve without unlocking Shots of Agony. |
| Optional quests and investigation activities | MISSING | Do not reduce to the large mandatory scenes. |
| Shared combat/camera/content interfaces | DIFFERENT | Portable foundations exist but do not yet cover the full investigation, evidence and hub-state model. |
| Memory-manipulation explanation | APPROVED CURRENT-CANON OVERRIDE | Rejected by 0.4F; final Legacy uncertainty wins. |
| Mission IDs, browser UI, exit screen and per-mission Replay | OBSOLETE TECHNICAL ARCHITECTURE | Translate content into continuous Story Mode and whole-chapter Replay. |
| Echo Region transition and collapse | MISSING | Required Chapter 3 ending; Chapter 4 owns the wake-up. |

## Current-document disposition

Retained: current ability chronology, continuous Story/whole-chapter Replay, the rule that Shots of Agony remains locked until its Season 2 debut, and the Organization of the Red continuity reset. Rejected: explanatory memory manipulation, extra faction foreshadowing and exposition not present in final Legacy. No other supplied current route material establishes a conflict.
