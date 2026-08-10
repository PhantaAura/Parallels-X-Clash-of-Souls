# Chapter 1 UI Information-Structure Comparison — 0.4F

This is a structural/source audit, not a visual-parity claim. The exact Legacy build is `2.9A.40.7.1.1`.

| Screen / state | Legacy | Current 0.4E | 0.4F Foundation Lock | Intentional difference / remaining work |
|---|---|---|---|---|
| Field HUD | Compact objective/detail, interaction prompt, current field abilities; contextual information appears when useful. | Objective and controls/hotbar were always broad bottom panels; nearby interactions existed in state but Mac did not render them. | Compact top objective, separate contextual `E • actor/activity` prompt, current-technique hotbar. | Art, icons and exact sizing need native comparison. |
| Combat HUD | Player HP, opponent HP when relevant, Energy when introduced, Guard when introduced, readable combat state. | Player/opponent HP and Energy only; no Guard. Whole combat HUD appeared as one fixed group. | Player HP, opponent HP, Energy and Guard are separate phase-gated channels. | Final numbers/icons, effects and opponent identity art remain placeholder. |
| Tutorial manual | Sections for movement, basic/kinetic combat, resources, hotbar and Sage note; Guided, Resume and Quick entry modes. | One giant two-line summary of all seven exercises; only “begin.” | Restores sectioned information and Guided/Resume/Quick choices, with saved checkpoint label. | Legacy's “return to mission/story” exit is omitted because the mission shell is obsolete; normal Story remains continuous. |
| Guided tutorial card | One current step, controls, live instruction/checklist, progress and urgent telegraph. | Objective text represented the step, but HUD disclosure/checkpoint choice was incomplete. | One current objective/detail, exact count/check, step progress and `BLOCK NOW`/Lens read prompt. | Final coach-card layout and animation need Mac playtest. |
| Tutorial resource disclosure | Energy from resource lesson onward; Guard for parry/Lens/final; hotbar only for ability/Lens/final; opponent HUD only when needed. | The Mac combat HUD exposed HP/Energy as a fixed bundle and the full current hotbar throughout the arena. | Shared booleans gate player HP, opponent HP, Energy, Guard and hotbar by exact lesson phase. | This is source-verified structure, not visual placement proof. |
| Ability hotbar | Legacy slot layout included later chronology placeholders. | Three current abilities were correctly hidden from future-technique disclosure, but always visible. | Only existing abilities appear. Tutorial hides the bar until Step 5, shows Fire Blast/Object Swap there, then adds Lens for Step 6 onward. | Current chronology compresses to visible slots 1/2/3; no `???`, locked Shots or Solar slot. |
| Objective presentation | Separate title/detail that changes with state and does not erase critical combat data. | Hidden during all arena states; fixed debug-style build kicker. | Remains visible during training below the combat channels; build/debug label removed. Hidden only for dialogue/manual/choice/QTE/pause. | Native layout must confirm no overlap at supported window sizes. |
| Dialogue | Speaker, styled panel, text, advance state; portraits/focus where available. | Game-owned panel existed with placeholder silhouette, speaker/expression/count and advance prompt. | Preserved; remains game-owned and content-driven. | Final portraits, expressions, animation, voice/audio and exact Legacy timing remain. |
| Interaction prompt | Small local prompt naming the nearby NPC/activity. | Runtime exposed the name, Mac ignored it. | Dedicated compact prompt renders `E` plus the shared interaction label. | Controller glyph switching remains presentation work. |
| Route choice | `CHOOSE A ROUTE` with Main Road, Forest Shortcut and Cliff Route. | Replaced by walking into a physical fork. | Exact three-choice information structure is restored; selected route then plays physically. | Panel art needs native comparison. |
| Runaway Cart | Dedicated sequence, timer, progress and retry feedback. | Dedicated QTE panel existed and was structurally close. | Retained with authored sequence, timer, attempt and completion dialogue. | Timing is automated-tested; visual urgency/effects need playtest. |
| Optional fight result | KO/result, reward/leave/rematch feedback, then hub/road continuation. | Portable state handled rematch/leave but presentation remained mostly dialogue/objective based. | State/result branches remain safe and explicit. | A final animated result treatment is still missing; do not call visual parity. |
| Pause / story menu | Current objective, controls/manual, stats/progression when available, field skills and return. | Escape was mapped but Runtime did not open a pause/menu information surface. | Shared pause state now exposes current objective, controls, current techniques, checkpoint and return. Chapter 1 does not expose Tournament Card stats. | Map/art/settings pages remain future shared UI work. |
| Chapter completion | Clear completion and next-story handoff. | Stable outskirts completion, but no registered next target. | Completion records `rrvvfo_ch2` as the pending continuous Story target; no chapter/mission menu appears. | Actual Chapter 2 content is intentionally absent in 0.4F. |

## Structural invariants tested

- Guard exists as a distinct player information channel.
- Energy, Guard, opponent health and hotbar use tutorial-phase visibility rules.
- Future abilities never produce placeholder slots.
- Objective, dialogue, choice, QTE, pause and interaction surfaces are mutually prioritized rather than stacked redundantly.
- All displayed content comes from shared `RuntimeView`; the Mac view does not own tutorial/story rules.
- Native screenshots are still required before any claim of visual parity.

## 0.4F.1 presentation lock

0.4F.1 keeps the accepted 0.4F information structure and replaces the remaining six-summary-line runtime fallback with the actual shared `CombatManualPage` model. Mac and Linux now render category, title, summary, individual entries, keyboard/controller prompts, page position and Guided/Resume/Quick choices from the same state.

The Linux graphical review state in `review/screenshots/combat-manual.png` is an actually rendered validation image. It confirms hierarchy and safe layout in the Linux shell; the corresponding Cocoa/Metal layout still requires native Mac inspection.
