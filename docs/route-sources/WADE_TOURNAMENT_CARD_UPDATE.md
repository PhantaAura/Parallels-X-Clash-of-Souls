# PARALLELS X: CLASH OF SOULS
## WADE ROUTE — TOURNAMENT CARD INTEGRATION

**Applies to:** Wade's Lost Year parallel-route bible.

**Placement:** Chapter 2 — Wade's tournament, during the shared registration sequence with Rrvvfo and Bark.

**Architecture rule:** Wade's Story Mode remains one continuous adventure. The Tournament Card does not create missions, a chapter-return menu, or a separate progression mode. Replay remains chapter-based only.

---

# TOURNAMENT CARD — WADE

At tournament registration, the announcer/registration staff asks Rrvvfo, Bark, and Wade for their Tournament Cards. None of the three has one. Registration provides spare cards to all three.

This is the first point where Wade's **Tournament Card becomes player-facing**. Chapter 1 may accumulate hidden Story XP from Wade's traversal, relays, fights, and approved activities, but the player does not see the card before registration.

Wade's card becomes his persistent RPG/progression screen for the rest of his route.

## Card contents

The card shows:

- Wade's name;
- a small Wade portrait on the side;
- current Level;
- current Story XP and progress toward the next level;
- HP;
- Power;
- Defense;
- Speed;
- Focus.

The portrait should eventually be rendered from Wade's own 3D model through the same cel-shaded portrait pipeline used by the other playable characters.

## Level-up behavior

Fights, traversal challenges, relay restoration, helping people, and other approved activities award Story XP.

When Wade levels up:

1. **All core stats rise automatically.**
2. A short level-up presentation appears.
3. A bonus wheel rolls **+1, +2, or +3**.
4. The landed number becomes bonus stat points.
5. The player chooses **one** stat to receive that bonus.
6. The updated Tournament Card appears briefly before play resumes.

Bonus choices:

- HP
- Power
- Defense
- Speed
- Focus

Do not force the bonus into Speed. Wade's natural fighter profile already makes speed part of his identity; the bonus remains a real player choice.

## Wade-route XP philosophy

Wade's route must not reward only enemy defeats.

Meaningful XP sources may include:

- tournament fights;
- traversal chains completed correctly;
- relay restoration;
- rescuing/reaching objectives under time pressure;
- approved optional fights and side activities;
- story-important observation/traversal objectives where appropriate.

Avoid rewarding empty running in circles or trivial repeatable speed farming.

## Presentation target

Desktop/Mac:
- same shared Tournament Card implementation used by Rrvvfo and Bark;
- Wade-specific portrait/pose/data only.

Old 3DS:
- bottom-screen Tournament Card;
- portrait on the side;
- readable Level/XP/stats;
- physical-button alternatives for touch interactions.

## Shared-engine rule

Wade must NOT receive a separate card/progression engine.

Rrvvfo, Bark, and Wade all use the same shared RPG/Tournament Card system. Their routes only provide character data and XP sources.

The Tournament Card is a protected feature and must not be removed during later UI cleanup.
