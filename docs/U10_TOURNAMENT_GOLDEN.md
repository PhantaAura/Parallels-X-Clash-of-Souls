# U10 — Tournament Golden Pass

## Presentation
- Tournament hub now reads as entrance / registration / market / practice / spectator / contestant / arena-gate districts.
- Waiting tent, medical tent, photo stand, bracket update board, race best-time board, extra crowd silhouettes and cleanup/repair states add life without a second hub engine.
- Intermissions visibly change between festival, active tournament, final preparation and cleanup.
- macOS and Old 3DS receive a Tournament Card reveal using the same saved card data. The card never spoils future abilities.
- Normal Story objective headers favor the current area rather than internal chapter numbers.

## Gameplay locks
- Official opponent identities remain Hamual -> Daniel -> Wade -> Plouke, with Bark vs Pouki as the spectator set piece.
- Fixed recommended levels remain authoritative. If future balancing adds soft scaling, it may only add +1 opponent level when the player is severely overleveled; U10 does not silently scale today.
- No Run/forfeit command is introduced to official matches.
- Plouke remains the story winner.
- Shots of Agony remains unavailable until Rrvvfo actually learns it later in Story.

## Save safety
- Chapter-1 transient progression is updated only while Chapter 1 is active.
- Crossing into Chapter 2 preserves completed Chapter-1 flags instead of re-authoring them from reset runtime fields.
- Tournament Card and physical free-swap props remain portable SaveData state.
