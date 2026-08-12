# U7A — Cinematic Foundation

Locked scope:
- shared MoveTo / FaceActor / PlayAnimation / LookAt / CameraTrack / CameraFocus / TriggerWorldEvent / Wait / Dialogue / Expression actions;
- Chapter 1 retrofit only; no story rewrite;
- cutscenes hand control back to the same RuntimeSession;
- Reduced Motion falls back to the authored stage camera;
- existing 0.75 second Legacy idle and dedicated combat_retreat assets are untouched;
- Tournament arrival is staged as movement toward the threshold, not a player-facing chapter-complete menu.

Milestone gate: BUG CHECK -> exact-behavior simplification when safe -> Chapter 1 regression -> continue.
