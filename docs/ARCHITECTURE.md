# 3.0R Architecture Rules

## One engine, all content

There is one `px_core`.

A chapter may provide map IDs, scene IDs, quests, encounters, playable-character IDs, story flags, allowed techniques, arena rules and audio IDs.

A chapter may **not** replace input, movement, combat, save, dialogue, camera framework, UI framework, loading or progression.

If Perfect Block timing is fixed once, every chapter and route inherits the fix. If the HUD is upgraded once, every route gets it.

## Story and replay boundary

`SceneStep` is an internal continuous-flow state, not a player-facing mission. Normal Story Mode starts a character story once and advances Chapter 1 → Chapter 2 → Chapter 3 without returning to chapter select, character select or a mission menu.

Chapters remain valid internal boundaries for content registration, chapter titles, loading, saves/checkpoints and replay. Replay exposes only complete chapters the player has completed. Individual fights, roads, tutorials, quests, scenes and checkpoints are never replay entries. Legacy `rrvvfo-mission-*.js` classes are content references only and must not be reconstructed as progression architecture.

`ChapterDefinition::nextChapterId` is the direct Story Mode transition hook. If
the audited next chapter has not yet been registered, shared `Game` records it
as `pendingChapterId` and holds the current completion state. It does not expose
a menu or fabricate placeholder gameplay. 0.4F/0.4F.1 therefore points Chapter 1 at
`rrvvfo_ch2` without implementing Chapter 2.

`story_progression.*` owns portable Level/XP/stat math and Tournament Card state.
`quest.*` owns generic quest and party state. Chapters provide definitions and
flags only; these are not chapter-specific RPG or quest engines.

## Platform split

`core/` owns game rules and runtime state.

`content/` owns authored story/map/dialogue definitions.

`core/menu_state.*` owns title, mode, route, recap and unlock-notification flow. `core/story_unlocks.*` owns semantic route discovery. `content/menu_registry.*`, `story_route_registry.*`, `story_recap_registry.*`, `combat_manual_registry.*` and `ui_presentation_registry.*` own portable presentation data. No platform shell decides unlocks, story availability or menu order.

`content/world_presentation_registry.*` owns portable stage camera/environment/landmark transforms. `content/character_presentation_registry.*` owns replaceable model bindings and fallback identity. Platform renderers choose fidelity but do not author the world.

`core/camera_policy.*` resolves gameplay and cinematic camera state into one platform-neutral focus/yaw/distance/height/FOV result. Cutscene actions update `RuntimeView`; Mac, Linux and 3DS consume the same resolver. Platform tiers may reduce easing, shake and decoration, but cannot reinterpret shot direction or actor focus.

Tournament progression remains authored data and state inside the same `RuntimeSession`. The Chapter 2 hub, optionals, official fights, Energy Power and Fire Awakening do not own separate movement, combat, camera, UI or save implementations.

`platform/` translates device input, rendering, audio and filesystem services.

Current targets:

- macOS: lead graphical development build.
- Linux: graphical Codex development/validation shell using SDL2 software rendering and isolated development saves.
- Old 3DS XL: compatibility gate and official portable target.
- Windows: later desktop target; the Linux shell is intentionally structured to help future desktop reuse.

Web/mobile gameplay targets are intentionally removed.

## Mac + 3DS development rule

Mac leads feature implementation, but 3DS is tested continuously. A system is not considered architecture-safe if its assumptions make an Old 3DS implementation unreasonable.

The two versions may use different renderers, UI composition and asset fidelity. They do not get different gameplay engines.

## Front-end platform mapping

Mac and Linux render the same `MenuSnapshot`, `RuntimeView`, recap definitions, manual pages and UI theme/layout contracts. The Mac renderer remains the visual authority; Linux provides launchable, screenshot-capable validation without a gameplay fork.

The later Old 3DS renderer should map selected title/mode/character art to the top screen and supplemental actions/status/Story So Far to the bottom screen. Physical input must reach every action; touch can only be an optional shortcut.

Menu events expose replacement-friendly audio/motion hooks for move, confirm, back, error, title confirm, carousel transition and route unlock. Navigation changes state immediately; the short slide is decorative and never blocks input. Reduced motion can disable the transient offset without changing state.

## Canonical faction data

Lost-Year Organization content uses stable ID `organization_red`. The deprecated development alias exists only in the faction compatibility table, schema migration and migration tests. New chapters, maps, flags and UI labels must never depend on it. See `ORGANIZATION_OF_THE_RED_LOST_YEAR_CONTINUITY.md`.
