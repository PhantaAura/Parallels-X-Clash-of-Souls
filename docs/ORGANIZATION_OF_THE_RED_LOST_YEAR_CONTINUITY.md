# Organization of the Red — Lost Year Continuity Lock

Status: planning/data lock for Mac 0.4F.1. It does not implement Chapters 2–4 or post-game content.

## Canonical identity

- Stable shared ID: `organization_red`
- Player-facing name: `ORGANIZATION OF THE RED`
- Deprecated development alias: `project_hollow`, accepted only while migrating old development saves

New story, UI, map, flag and faction data must use the canonical ID. No player-facing system may depend on the deprecated alias.

## Continuity sequence

1. During the Season 1 Clone incident, an Organization of the Red operation is damaged and its members are scattered.
2. During the Clash of Souls Lost Year, surviving cells quietly regroup. They continue cloning, artificial/replicated-combatant, Echo Region and unstable-teleportation experiments.
3. The tournament sabotage, Strange Man investigation and hidden facility lead Rrvvfo into conflict with this Lost-Year branch.
4. By the end of Clash of Souls, that branch is broken and scattered badly enough that Rrvvfo reasonably assumes the Organization has disbanded.
5. During Season 2, Gregradro's return is eventually noticed and surviving members prepare again.
6. In the Clones OVA period, the Organization of the Red returns openly.

This preserves Rrvvfo's later reaction that the Organization's return seems impossible because its members separated and he assumed they disbanded.

## Lost-Year story role

Preserve the audited game events:

- tournament sabotage mystery;
- Strange Man investigation and deliberate uncertainty;
- hidden facility;
- cloning experiments and replicated combatants;
- unstable teleportation;
- Echo Region connection;
- later ruined facility;
- a surviving/restorable post-game teleporter.

Do not make the Strange Man an exposition device and do not reveal the Organization's full structure, leadership or future plans. The branch is a mystery Rrvvfo uncovers from evidence, not information Sage secretly knew before the tournament.

Exact Legacy dialogue remains the default. During the later Chapter 3 port, references that directly conflict with this continuity use the smallest wording change needed, with every changed line documented.

## Echo Free Roam preparation

Echo Region remains disconnected from the normal world. Post-game access must not add a normal road or bridge.

The planned route is:

1. Echo initially appears inaccessible.
2. The player finds the destroyed Lost-Year Organization facility.
3. Most of the base remains ruined.
4. One teleporter still works or can be restored.
5. Activating it establishes the permanent post-game route to Echo.

0.4F.1 documents this ownership and reserves stable data identities only. It does not implement the facility, teleporter restoration or Echo free roam.

## Save compatibility

Save schema 3 canonicalizes old development-only faction prefixes in route IDs, map IDs and story flags. Migration never erases other progress. The deprecated alias remains in the compatibility table and migration test only; it is not valid for new content.
