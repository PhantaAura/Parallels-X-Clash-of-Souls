#include "content/arena_encounter_registry.hpp"
#include <stdexcept>

namespace px {

ArenaEncounterRegistry::ArenaEncounterRegistry() {
    const Rect ring{-450.0f,450.0f,-350.0f,350.0f};
    encounters_.emplace("ch2_practice_brawl", ArenaEncounterDefinition{
        "ch2_practice_brawl","tournament_arena","practice_fighter","Practice Ring Fighter",AiArchetype::Balanced,
        {-280.0f,0.0f},{280.0f,0.0f},ring,1,45.0f,154.0f,1,45,false,StoryArenaResolution::PlayerVictory,"ch2_practice_brawl_result"});
    encounters_.emplace("ch2_vs_hamual", ArenaEncounterDefinition{
        "ch2_vs_hamual","tournament_arena","hamual","Hamual",AiArchetype::Aggressive,
        {-300.0f,0.0f},{300.0f,0.0f},ring,3,55.0f,158.0f,1,120,true,StoryArenaResolution::PlayerVictory,"ch2_hamual_result"});
    encounters_.emplace("ch2_vs_daniel", ArenaEncounterDefinition{
        "ch2_vs_daniel","tournament_arena","daniel","Daniel",AiArchetype::Defensive,
        {-300.0f,0.0f},{300.0f,0.0f},ring,3,58.0f,148.0f,2,150,true,StoryArenaResolution::PlayerVictory,"ch2_daniel_result"});
    encounters_.emplace("ch2_vs_wade", ArenaEncounterDefinition{
        "ch2_vs_wade","tournament_arena","wade","Wade",AiArchetype::Aggressive,
        {-300.0f,0.0f},{300.0f,0.0f},ring,3,62.0f,212.0f,3,190,true,StoryArenaResolution::PlayerVictory,"ch2_wade_result"});
    encounters_.emplace("ch2_vs_plouke", ArenaEncounterDefinition{
        "ch2_vs_plouke","tournament_arena","plouke","Plouke",AiArchetype::Adaptive,
        {-300.0f,0.0f},{300.0f,0.0f},ring,3,38.0f,138.0f,4,240,true,StoryArenaResolution::PloukeStoryFinal,""});
    // Optional tournament activities still run through the one shared combat
    // simulation. They differ only in content data and may be left with Run.
    encounters_.emplace("ch2_fake_champion_optional", ArenaEncounterDefinition{
        "ch2_fake_champion_optional","tournament_arena","fake_champion","Fake Champion",AiArchetype::Defensive,
        {-280.0f,0.0f},{280.0f,0.0f},ring,1,55.0f,142.0f,2,0,false,StoryArenaResolution::PlayerVictory,"ch2_fake_champion_done"});
    encounters_.emplace("ch2_runaway_dummy_optional", ArenaEncounterDefinition{
        "ch2_runaway_dummy_optional","tournament_arena","runaway_dummy","Runaway Training Dummy",AiArchetype::Aggressive,
        {-320.0f,0.0f},{180.0f,0.0f},ring,1,60.0f,185.0f,2,0,false,StoryArenaResolution::PlayerVictory,"ch2_runaway_dummy_done"});
    encounters_.emplace("ch2_bark_practice_optional", ArenaEncounterDefinition{
        "ch2_bark_practice_optional","tournament_arena","bark","Bark",AiArchetype::Balanced,
        {-280.0f,0.0f},{280.0f,0.0f},ring,1,65.0f,162.0f,3,0,false,StoryArenaResolution::PlayerVictory,"ch2_bark_practice_done"});
    encounters_.emplace("ch2_one_match_anyway_optional", ArenaEncounterDefinition{
        "ch2_one_match_anyway_optional","tournament_arena","eliminated_fighter","Eliminated Fighter",AiArchetype::Adaptive,
        {-280.0f,0.0f},{280.0f,0.0f},ring,1,58.0f,154.0f,2,0,false,StoryArenaResolution::PlayerVictory,"ch2_one_match_anyway_done"});
}

const ArenaEncounterDefinition& ArenaEncounterRegistry::get(const std::string& id) const {
    const auto it=encounters_.find(id); if(it==encounters_.end()) throw std::out_of_range("Unknown arena encounter: "+id); return it->second;
}
bool ArenaEncounterRegistry::has(const std::string& id) const { return encounters_.find(id)!=encounters_.end(); }

} // namespace px
