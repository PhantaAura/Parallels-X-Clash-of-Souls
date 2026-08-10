# Rrvvfo Chapters 1–4 Source Rules — 0.4H Golden Gate Lock

## Source priority

1. Latest playable browser content: `2.9A.40.7.2R`, Complete Chapter 1 Adventure Rebuild + Mobile Playtest Lab.
2. Explicit approved 3.0R canon locks, including the scoped Rrvvfo/Bark/Wade Tournament Card documents, only where they intentionally resolve a continuity conflict.
3. Exact cumulative Legacy build `2.9A.40.7.1.1`, cache tag `29a40711-chapter2-hub-return-20260802`, for content not replaced by 7.2R.
4. Current route documents only when they identify a genuine continuity conflict.
5. Season 1, Season 2 and OVA bibles for surrounding continuity.

The latest playable browser version is the primary story and gameplay source for Rrvvfo Chapters 1–4. A change already present in 3.0R has no authority merely because it exists. Unless it is covered by an approved category below, port the browser behavior.

## Approved global differences

- Clean portable shared C++ instead of browser, DOM, global patch or duplicated chapter engines.
- Real 3D presentation and platform-specific presentation tiers.
- macOS, Old 3DS, Windows and Linux portability.
- Verified bug and softlock fixes.
- Current ability chronology.
- One continuous Story Mode and whole-chapter Replay only.
- The explicit chapter boundaries and Tournament Card rules below.

“Modernization” is not a gameplay-change category.

## Player-facing structure

Story Mode is one continuous adventure: Chapter 1 leads directly to Chapter 2, then Chapter 3, Chapter 4 and later chapters. Internal `SceneStep` and state objects are implementation details. There is no mission select, mission replay, chapter menu between normal chapters or return to character select.

Replay lists completed whole chapters only. Legacy files named `mission-*.js` are behavior and content sources, never a player-facing architecture source.

## Chapter boundaries and chronology

- Chapter 1 begins with Sage, preserves the Legacy Object Swap lesson, seven-step refresher and Tournament Road adventure, and ends at Tournament Outskirts.
- Cracked practice ring, registration and Tournament Card acquisition are Chapter 2.
- Bark and Wade reunite with Rrvvfo in Chapter 2, not Chapter 1.
- Shots of Agony debuts in Season 2 and is absent from Chapter 1. No hidden, locked or placeholder slot may disclose it.
- The Lens already exists in Chapter 1 and remains unstable; use its exact compatible Legacy teaching and road behavior.
- Sage does not secretly know the Organization of the Red's Lost-Year operation before the tournament. Suspicion develops from witnessed events.
- Chapter 3 preserves the Strange Man uncertainty. No later explanatory memory answer overrides the final Legacy mystery.
- Chapter 4 preserves the full Legacy chapter. Bark and Wade may already have searched or fought while Rrvvfo was missing; their serious defenses are established after Rrvvfo later leaves for the mountain. No other Chapter 4 rewrite follows from that clarification.

## Tournament Card

At Chapter 2 registration, Rrvvfo, Bark and Wade receive Tournament Cards. This is the first player-facing RPG-stat presentation, although Chapter 1 may track XP internally. The card contains Level, XP, HP, Power, Defense, Speed and Focus. All stats rise automatically on level-up, followed by a compact `+1/+2/+3` wheel and one player-selected bonus stat. It must not reveal future abilities.

The authoritative character-scoped update documents are:

- `docs/route-sources/RRVVFO_TOURNAMENT_CARD_UPDATE.md`
- `docs/route-sources/BARK_TOURNAMENT_CARD_UPDATE.md`
- `docs/route-sources/WADE_TOURNAMENT_CARD_UPDATE.md`

Their authority is limited to Tournament Card timing, progression, XP philosophy and presentation. They do not authorize replacing compatible Legacy scenes, personalities or dialogue.

## Dialogue rule

Use exact Legacy dialogue wherever canon-compatible. Change the minimum possible wording only for a documented conflict, and record every changed line and reason.

Legacy's dialogue style is also the voice authority for newly added optional content. Do not flatten Rrvvfo, Bark or Wade into clean generic RPG exposition. New lines must be checked against each speaker's actual Legacy rhythm, bluntness, humor, vocabulary, interruption habits and relationships. Character identity comes before tutorial clarity; mechanical instructions should be carried by staging, objectives or UI when forcing them into spoken dialogue would make the character sound unnatural.

For any new side quest, maintain a dialogue ledger containing:

- every newly written line;
- the Legacy scene(s) used as the voice reference;
- the gameplay reason the new line is necessary;
- confirmation that it does not disclose future abilities or Chapter 2 information early.

## Lost-Year faction continuity

The active faction is the Organization of the Red, stable ID `organization_red`. Surviving cells from the damaged Season 1 Clone incident regroup during the Lost Year and own the audited sabotage, hidden facility, cloning/replication, unstable teleportation and Echo-linked events. Rrvvfo breaks this branch badly enough to assume it disbanded, preserving the later Season 2/Clones OVA return.

The former development name is not a second faction and is not valid for new content. It may appear only when documenting exact Legacy identifiers or migration compatibility. Do not use this reset to over-explain the Strange Man or expose later Organization secrets.
