# PARALLELS X: CLASH OF SOULS
## RRVVFO ROUTE — TOURNAMENT CARD INTEGRATION

**Applies to:** Rrvvfo's Lost Year character-route bible.

**Placement:** Chapter 2 — the tournament, during the shared registration sequence with Rrvvfo, Bark, and Wade.

**Architecture rule:** Story Mode remains one continuous adventure. This card system does not create a mission menu, chapter break, or separate progression mode. Chapters remain internal save/replay boundaries only. Replay remains chapter-based only.

---

# TOURNAMENT CARD — RRVVFO

At tournament registration, the announcer/registration staff asks Rrvvfo, Bark, and Wade for their Tournament Cards. None of the three has one. Registration provides spare cards to all three.

This is the first point where the **Tournament Card becomes player-facing**. Chapter 1 may already have accumulated hidden Story XP, but the player does not see the card before registration.

Rrvvfo's card becomes his persistent RPG/progression screen for the rest of his route.

## Card contents

The card shows:

- Rrvvfo's name;
- a small portrait on the side of the card;
- current Level;
- current Story XP and progress toward the next level;
- HP;
- Power;
- Defense;
- Speed;
- Focus;
- later, any route-specific mastery/progression information that genuinely belongs on the card.

The portrait should eventually come from Rrvvfo's actual 3D model: pose/expression -> cel-shaded render -> cropped portrait. Do not require hand-drawn portrait art.

## Level-up behavior

Fights, helping people, and other approved story/side activities award Story XP.

When Rrvvfo levels up:

1. **All core stats rise automatically.**
2. A short level-up presentation appears.
3. A bonus wheel rolls **+1, +2, or +3**.
4. The number landed on becomes bonus stat points.
5. The player chooses **one** stat to receive that bonus.
6. The updated Tournament Card is shown briefly, then normal play resumes.

The bonus can be assigned to one of:

- HP
- Power
- Defense
- Speed
- Focus

The level-up sequence should become fast once familiar — roughly 4–6 seconds unless the player deliberately spends longer choosing the bonus stat.

## Presentation target

Desktop/Mac:
- stylized physical-card presentation;
- 3D Rrvvfo pose or portrait during level-up;
- stats tick upward clearly;
- bonus wheel is readable and quick;
- card should feel like an in-world tournament object, not a generic RPG spreadsheet.

Old 3DS:
- Tournament Card primarily lives on the bottom screen;
- portrait sits on the side of the card;
- Level/XP/stats remain readable at 320x240-class UI density;
- every touch action must also have a physical-button route.

## Rrvvfo-specific rule

The Tournament Card is progression UI, **not an ability-unlock spoiler**.

Do not list future techniques before Rrvvfo invents/learns them. In particular, Shots of Agony must not appear early as `???`, `LOCKED`, or `NOT INVENTED`.

Unavailable techniques simply do not appear.

## Legacy preservation rule

Use the Legacy game's Story progression values/behavior as the baseline where they are still compatible with the current design. The Legacy progression system already uses Level, Story XP, and the core stats HP / Power / Defense / Speed / Focus. Port the behavior into shared portable C++ rather than copying the old JavaScript architecture.

The Tournament Card itself is a protected feature and must not be removed during later UI cleanup.
