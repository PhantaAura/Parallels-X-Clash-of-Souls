# PARALLELS X: CLASH OF SOULS
## BARK ROUTE — TOURNAMENT CARD INTEGRATION

**Applies to:** Bark's Lost Year parallel-route bible.

**Placement:** Chapter 2 — Bark's tournament, during the shared registration sequence with Rrvvfo and Wade.

**Architecture rule:** Bark's Story Mode remains one continuous adventure. The Tournament Card does not create a mission menu or interrupt the route with chapter selection. Replay remains chapter-based only.

---

# TOURNAMENT CARD — BARK

At tournament registration, the announcer/registration staff asks Rrvvfo, Bark, and Wade for their Tournament Cards. None of the three has one. Registration provides spare cards to all three.

This is the first point where Bark's **Tournament Card becomes player-facing**. Chapter 1 may accumulate hidden Story XP from Bark's road assistance, traversal, fights, and other approved activities, but the card is not shown before registration.

Bark's card becomes his persistent RPG/progression screen for the rest of his route.

## Card contents

The card shows:

- Bark's name;
- a small Bark portrait on the side;
- current Level;
- current Story XP and progress toward the next level;
- HP;
- Power;
- Defense;
- Speed;
- Focus.

The portrait should eventually be rendered from Bark's own 3D model using the same cel-shaded portrait pipeline as the other playable characters.

## Level-up behavior

Fights, helping people, defensive objectives, traversal assistance, and other approved activities award Story XP.

When Bark levels up:

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

Do not force Bark players to dump bonuses into Defense just because Bark is defense-oriented. His natural stat growth/fighter profile should establish his identity; the bonus choice belongs to the player.

## Bark-route XP philosophy

Bark's route is about stabilizing and protecting, so Story XP must not come only from defeating enemies.

Meaningful XP sources may include:

- tournament fights;
- defensive stands;
- protecting travelers/civilians;
- stabilizing required structures;
- completing important Earth-field interactions;
- approved optional fights and side activities.

Do not award XP for repeatedly farming trivial interactions.

## Presentation target

Desktop/Mac:
- same shared Tournament Card system as Rrvvfo;
- Bark's portrait/pose and character data are supplied as content, not a Bark-only UI engine.

Old 3DS:
- bottom-screen card layout;
- portrait on the side;
- readable Level/XP/stats;
- physical-button alternative for all touch interactions.

## Shared-engine rule

Bark must NOT receive his own Tournament Card implementation.

Use one shared Tournament Card/RPG progression system for Rrvvfo, Bark, Wade, and later playable characters. Character routes provide only data such as portrait, current stats, XP, and character-specific presentation bindings.

The Tournament Card is a protected feature and must not be removed during later UI cleanup.
