#include "content/story_recap_registry.hpp"
#include <stdexcept>

namespace px {

StoryRecapRegistry::StoryRecapRegistry() {
    sections_ = {
        {"the_brothers", "THE BROTHERS", {
            {"brothers_home", "BROTHERS DIVIDED",
             "Rrvvfo is Revvfo's younger brother. Revvfo became the central enemy of Season 1 and destroyed Rrvvfo's home at the very beginning.",
             "recap_brothers_home", 11.0f},
            {"brothers_team", "TEAM NINJA",
             "Rrvvfo later joined Master Tangai, Bark, and Wade. Bark and Wade became the teammates at the center of his fight against Revvfo.",
             "recap_team_ninja", 10.0f},
            {"brothers_public", "ONE FAMILY, TWO SIDES",
             "By the end of the season, the world knew that the two rivals fighting over its future were brothers.",
             "recap_brothers_split", 9.0f},
        }},
        {"the_asrylyte", "THE ASRYLYTE", {
            {"asrylyte_volcano", "THE VOLCANO CRISIS",
             "A volcano crisis forced Rrvvfo and Revvfo to cooperate long enough to seal Avalfi and the Summoner.",
             "recap_volcano_seal", 10.0f},
            {"asrylyte_birth", "POWER GIVEN FORM",
             "Avalfi overextended his power. That power became the Asrylyte, an independent force both brothers could reach.",
             "recap_asrylyte_birth", 10.0f},
            {"asrylyte_refusal", "RYUZAKARO REFUSED",
             "Rrvvfo reached it first, but Ryuzakaro refused it from inside him. Revvfo claimed the Asrylyte instead.",
             "recap_asrylyte_choice", 10.0f},
            {"asrylyte_shards", "THE SHARD CONFLICT",
             "The Asrylyte shattered into shards. The struggle to recover them drove Season 1 until Revvfo finally completed it and reached Perfection.",
             "recap_asrylyte_shards", 11.0f},
        }},
        {"two_sides", "TWO SIDES OF THE CONFLICT", {
            {"sides_ninja", "TEAM NINJA",
             "Rrvvfo, Bark, Wade, and Master Tangai stood against Revvfo's campaign.",
             "recap_team_ninja_lineup", 9.0f},
            {"sides_revvfo", "TEAM REVVFO",
             "Revvfo fought through Alt, Rover, Rev, and Metal as well as his own power.",
             "recap_team_revvfo_lineup", 9.0f},
            {"sides_family", "ALT AND ROVER",
             "Alt is the brothers' cousin and Revvfo's fiercely loyal major fighter. Rover is their younger brother, a mechanical genius whose work is tied to Revvfo's machines.",
             "recap_alt_rover", 12.0f},
        }},
        {"rrvvfos_burden", "RRVVFO'S BURDEN", {
            {"burden_ryuzakaro", "RYUZAKARO",
             "The dangerous Monster Ryuzakaro is sealed inside Rrvvfo. Resisting that presence has been part of Rrvvfo's life for years.",
             "recap_ryuzakaro_seal", 10.0f},
            {"burden_sage", "A SECOND VOICE",
             "Sage also became sealed inside Rrvvfo during the volcano crisis. At first he could not communicate properly; later, the seal loosened enough for him to speak.",
             "recap_sage_within", 11.0f},
            {"burden_abilities", "POWER AT SEASON'S END",
             "Rrvvfo ended the season with Awakened Fire power, developing Energy Power, and the unstable Lens of Truth.",
             "recap_rrvvfo_abilities", 11.0f},
        }},
        {"fake_death", "THE FAKE DEATH", {
            {"fake_return", "FOUND WHERE NO ONE KNEW",
             "Rrvvfo secretly returned to the volcano. Revvfo found him there even though nobody should have known where he went.",
             "recap_fake_death_volcano", 11.0f},
            {"fake_decision", "THE DECISION",
             "Rrvvfo realized that people near him could become targets. He deliberately made everyone believe he had died, then isolated himself to train.",
             "recap_fake_death_decision", 12.0f},
            {"fake_grief", "THE PEOPLE LEFT BEHIND",
             "Bark and Wade genuinely believed he was gone. Tangai understood that the death was staged, while Raggie doubted it because he knew Rrvvfo too well.",
             "recap_fake_death_reactions", 12.0f},
            {"fake_discovery", "RAGGIE'S ANSWER",
             "Months later, Raggie's suspicion became certainty when he personally discovered that Rrvvfo was alive.",
             "recap_raggie_discovers", 9.0f},
        }},
        {"shadow_energy", "SHADOW AND ENERGY POWER", {
            {"shadow_meets", "SHADOW'S LIMIT",
             "Shadow encountered Rrvvfo during his isolation. Asrylyte energy weakened Shadow, so he could not simply solve the Revvfo problem himself.",
             "recap_shadow_meeting", 11.0f},
            {"shadow_trains", "A DIFFERENT ANSWER",
             "Shadow trained Rrvvfo instead and taught him Energy Power.",
             "recap_shadow_training", 9.0f},
            {"shadow_developing", "STILL DEVELOPING",
             "The new power greatly expanded what Rrvvfo could do, but it was not fully mastered.",
             "recap_energy_developing", 9.0f},
        }},
        {"virek", "VIREK", {
            {"virek_rival", "INDEPENDENT RIVAL",
             "Virek belonged to neither main team. He was strong enough to fight Rrvvfo to a draw.",
             "recap_virek_rival", 10.0f},
            {"virek_emerald", "THE EMERALD",
             "Rrvvfo helped protect and return Virek's Emerald. Their rivalry became reluctant, tactical cooperation.",
             "recap_virek_emerald", 10.0f},
            {"virek_finale", "THE FINAL CAMPAIGN",
             "Virek later fought beside the resistance, traveled with Rrvvfo as a frontline scout, and helped gather the Wishing Stars.",
             "recap_virek_wishing_stars", 11.0f},
        }},
        {"oddballs", "THE ODDBALLS", {
            {"oddballs_raggie", "RAGGIE",
             "Raggie knew Rrvvfo before the main series and understands him unusually well. That is why the fake death never fully convinced him.",
             "recap_raggie", 10.0f},
            {"oddballs_three", "JIMMY AND JONATHAN",
             "Jimmy and Jonathan are Raggie's closest friends. Together, the three are the Oddballs, and they joined the resistance against Revvfo.",
             "recap_oddballs_lineup", 10.0f},
            {"oddballs_staff", "THE SEALING STAFF",
             "Rrvvfo entrusted the Sealing Staff to Jimmy. That choice became decisive at the end of the battle.",
             "recap_jimmy_staff", 10.0f},
        }},
        {"fall_of_perfection", "THE FALL OF PERFECTION", {
            {"perfection_rises", "PERFECTION",
             "Revvfo completed the Asrylyte, overwhelmed the heroes, and reshaped the world under his rule.",
             "recap_perfection_world", 11.0f},
            {"perfection_resistance", "THE RESISTANCE",
             "Rrvvfo survived. Virek and the Oddballs helped the resistance continue the campaign against an enemy none of them could simply overpower.",
             "recap_resistance", 10.0f},
            {"perfection_flaw", "THE FLAW",
             "Rrvvfo recognized that Revvfo had focused on conquest instead of stabilizing the enormous power. Perfection was leaking away as Revvfo used it.",
             "recap_perfection_flaw", 12.0f},
            {"perfection_survive", "SURVIVE LONG ENOUGH",
             "Rrvvfo kept fighting until that instability could be exploited. He defeated Revvfo, but he did not simply become stronger than Perfected Revvfo.",
             "recap_perfection_survival", 12.0f},
            {"perfection_sealed", "PETRIFIED",
             "The Completed Asrylyte fused into Revvfo. Jimmy used the Sealing Staff to petrify him, sealing the fused Asrylyte with him before a dark portal carried him away.",
             "recap_revvfo_petrified", 12.0f},
        }},
        {"after_battle", "AFTER THE BATTLE", {
            {"after_recovery", "THE WORLD SURVIVED",
             "The world survived and began recovering. Revvfo remained petrified and gone, with the Completed Asrylyte fused into him.",
             "recap_world_recovery", 10.0f},
            {"after_wish", "SAGE'S OWN BODY",
             "The Wishing Stars mattered in the finale. Rrvvfo used a wish to give Sage his own physical mortal body, which is why Sage stands beside him now.",
             "recap_sage_body", 11.0f},
            {"after_transition", "THE LOST YEAR BEGINS",
             "Shadow stepped away as Rrvvfo's normal trainer. The world survived Revvfo—but the year between Season 1 and Season 2 had only just begun.",
             "recap_clash_of_souls_title", 12.0f},
        }},
    };
}

const StoryRecapSection& StoryRecapRegistry::get(const std::string& id) const {
    for (const auto& section : sections_) if (section.id == id) return section;
    throw std::out_of_range("Unknown Story So Far section: " + id);
}

float StoryRecapRegistry::suggestedRuntimeSeconds() const {
    float seconds = 0.0f;
    for (const auto& section : sections_)
        for (const auto& frame : section.frames) seconds += frame.suggestedSeconds;
    return seconds;
}

std::size_t StoryRecapRegistry::totalFrameCount() const {
    std::size_t count = 0;
    for (const auto& section : sections_) count += section.frames.size();
    return count;
}

} // namespace px
