# One Engine Rule

A chapter can configure content. It cannot own an engine.

## Shared across every chapter and route
- input
- movement
- collision
- camera framework
- combat
- arena rules framework
- UI/HUD framework
- dialogue
- cutscenes
- quests/objectives
- RPG/Tournament Card
- save/checkpoint
- audio
- controller support
- platform UI input (including 3DS touch where useful)
- loading/transitions

## Chapter data may choose
- map
- playable character
- NPCs
- dialogue/cutscene IDs
- encounters
- allowed techniques
- route-specific objectives
- arena stage/rules
- music/ambience IDs
- story flags

Fix Perfect Block once → all routes inherit it.
Improve HUD once → all chapters inherit it.
Fix controller reconnect once → whole game inherits it.

No chapter-level monkey patching.

## Platform presentation rule

A platform renderer may select lower-detail meshes, textures, effects, lighting,
audio and decorative scenery. It must still render the same authored camera,
map topology, actor positions, blocker state, encounters and runtime feedback.
A compatibility diagnostic is never a gameplay-parity release artifact.
