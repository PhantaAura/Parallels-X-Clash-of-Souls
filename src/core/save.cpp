#include "core/save.hpp"
#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace px {

std::string SaveCodec::serialize(const SaveData& data) {
    std::ostringstream out;
    out << "schema=" << data.schemaVersion << '\n';
    out << "route=" << data.story.routeId << '\n';
    out << "chapter=" << data.story.chapterId << '\n';
    out << "scene=" << data.story.sceneIndex << '\n';
    out << "checkpoint=" << data.story.checkpointId << '\n';
    out << "pendingChapter=" << data.story.pendingChapterId << '\n';
    out << "map=" << data.world.mapId << '\n';
    out << "x=" << data.world.position.x << '\n';
    out << "z=" << data.world.position.z << '\n';
    out << "routeChoice=" << data.world.routeChoice << '\n';
    out << "hp=" << data.world.hp << '\n';
    out << "energy=" << data.world.energy << '\n';
    out << "guard=" << data.world.guard << '\n';
    for (const auto& object : data.world.objects)
        out << "object=" << object.id << '|' << object.position.x << '|' << object.position.z << '|' << (object.active ? 1 : 0) << '\n';
    out << "input=" << data.inputPreset << '\n';
    out << "selectedStoryRoute=" << data.frontend.selectedStoryRoute << '\n';
    out << "recapSection=" << data.frontend.storySoFarSection << '\n';
    for (const auto& route : data.frontend.discoveredStoryRoutes) out << "routeDiscovered=" << route << '\n';
    for (const auto& route : data.frontend.pendingStoryUnlocks) out << "pendingStoryUnlock=" << route << '\n';
    for (const auto& objective : data.frontend.objectiveHistory) out << "objectiveHistory=" << objective << '\n';
    out << "lastMenuMode=" << data.frontend.lastMenuMode << '\n';
    out << "lastBattleSelection=" << data.frontend.lastBattleSelection << '\n';
    out << "lastExtrasSelection=" << data.frontend.lastExtrasSelection << '\n';
    out << "lastOptionsSelection=" << data.frontend.lastOptionsSelection << '\n';
    out << "continueArea=" << data.frontend.currentArea << '\n';
    out << "continueObjective=" << data.frontend.currentObjective << '\n';
    out << "storyProgressPercent=" << data.frontend.storyProgressPercent << '\n';
    out << "playtimeSeconds=" << data.frontend.playtimeSeconds << '\n';
    out << "qolHoldDialogue=" << (data.qol.holdToAdvanceDialogue ? 1 : 0) << '\n';
    out << "qolFirstTimeHints=" << (data.qol.firstTimeHints ? 1 : 0) << '\n';
    out << "qolReducedMotion=" << (data.qol.reducedMotion ? 1 : 0) << '\n';
    out << "qolReducedShake=" << (data.qol.reducedCameraShake ? 1 : 0) << '\n';
    out << "qolReducedFlashes=" << (data.qol.reducedFlashes ? 1 : 0) << '\n';
    out << "qolHighContrast=" << (data.qol.highContrastHud ? 1 : 0) << '\n';
    out << "qolLargerText=" << (data.qol.largerText ? 1 : 0) << '\n';
    out << "qolCombatMessages=" << data.qol.combatMessages << '\n';
    out << "qolObjectiveDisplay=" << data.qol.objectiveDisplay << '\n';
    out << "qolCameraSensitivity=" << data.qol.cameraSensitivity << '\n';
    out << "qolInvertCameraX=" << (data.qol.invertCameraX ? 1 : 0) << '\n';
    out << "qolInvertCameraY=" << (data.qol.invertCameraY ? 1 : 0) << '\n';
    out << "qolGentleRecenter=" << (data.qol.gentleCameraRecenter ? 1 : 0) << '\n';
    out << "qolHudScale=" << data.qol.hudScale << '\n';
    out << "qolDialogueScale=" << data.qol.dialogueScale << '\n';
    out << "qolDialogueSpeed=" << data.qol.dialogueSpeed << '\n';
    out << "qolDialogueAuto=" << (data.qol.dialogueAutoAdvance ? 1 : 0) << '\n';
    out << "cardOwner=" << data.tournamentCard.ownerId << '\n';
    out << "cardLevel=" << data.tournamentCard.level << '\n';
    out << "cardXp=" << data.tournamentCard.xp << '\n';
    out << "cardPending=" << data.tournamentCard.pendingBonusChoices << '\n';
    out << "cardAcquired=" << (data.tournamentCard.acquired ? 1 : 0) << '\n';
    out << "cardBonusHp=" << data.tournamentCard.bonuses.hp << '\n';
    out << "cardBonusPower=" << data.tournamentCard.bonuses.power << '\n';
    out << "cardBonusDefense=" << data.tournamentCard.bonuses.defense << '\n';
    out << "cardBonusSpeed=" << data.tournamentCard.bonuses.speed << '\n';
    out << "cardBonusFocus=" << data.tournamentCard.bonuses.focus << '\n';
    out << "recordWins=" << data.records.wins << '\n';
    out << "recordLosses=" << data.records.losses << '\n';
    out << "recordBestCombo=" << data.records.bestCombo << '\n';
    out << "recordPerfectBlocks=" << data.records.perfectBlocks << '\n';
    out << "recordGuardBreaks=" << data.records.guardBreaks << '\n';
    out << "recordPursuitFinishers=" << data.records.pursuitFinishers << '\n';
    out << "recordEnergyBeams=" << data.records.energyBeamUses << '\n';
    out << "recordObjectSwaps=" << data.records.objectSwaps << '\n';
    out << "recordBestRank=" << (data.records.hasRank ? RpgProgressSystem::rankLabel(data.records.bestRank) : "-") << '\n';
    out << "rpgCoins=" << data.rpg.coins << '\n';
    out << "rpgVendorDiscount=" << data.rpg.vendorDiscountPercent << '\n';
    out << "rpgNextOfficialMeal=" << (data.rpg.nextOfficialMealBoost ? 1 : 0) << '\n';
    out << "rpgAccessory=" << data.rpg.equippedAccessoryId << '\n';
    out << "rpgNecklaceAcquired=" << (data.rpg.weightedNecklaceAcquired ? 1 : 0) << '\n';
    out << "rpgNecklaceMastery=" << data.rpg.weightedNecklaceMastery << '\n';
    out << "rpgNecklaceReward=" << (data.rpg.weightedNecklaceMasteryRewardGranted ? 1 : 0) << '\n';
    for (const auto& title : data.rpg.titles) out << "rpgTitle=" << title << '\n';
    for (const auto& memento : data.rpg.mementos) out << "rpgMemento=" << memento << '\n';
    for (const auto& profile : data.rpg.profileUnlocks) out << "rpgProfile=" << profile << '\n';
    for (const auto& flag : data.story.flags) out << "flag=" << flag << '\n';
    return out.str();
}

SaveData SaveCodec::deserialize(const std::string& text) {
    SaveData data;
    data.frontend.discoveredStoryRoutes.clear();
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        const auto split = line.find('=');
        if (split == std::string::npos) continue;
        const auto key = line.substr(0, split);
        const auto value = line.substr(split + 1);
        if (key == "schema") data.schemaVersion = std::stoi(value);
        else if (key == "route") data.story.routeId = value;
        else if (key == "chapter") data.story.chapterId = value;
        else if (key == "scene") data.story.sceneIndex = static_cast<std::size_t>(std::stoul(value));
        else if (key == "checkpoint") data.story.checkpointId = value;
        else if (key == "pendingChapter") data.story.pendingChapterId = value;
        else if (key == "map") data.world.mapId = value;
        else if (key == "x") data.world.position.x = std::stof(value);
        else if (key == "z") data.world.position.z = std::stof(value);
        else if (key == "routeChoice") data.world.routeChoice = value;
        else if (key == "hp") data.world.hp = std::stof(value);
        else if (key == "energy") data.world.energy = std::stof(value);
        else if (key == "guard") data.world.guard = std::stof(value);
        else if (key == "object") {
            const auto first = value.find('|');
            const auto second = first == std::string::npos ? std::string::npos : value.find('|', first + 1);
            const auto third = second == std::string::npos ? std::string::npos : value.find('|', second + 1);
            if (first != std::string::npos && second != std::string::npos && third != std::string::npos) {
                data.world.objects.push_back({value.substr(0, first),
                    {std::stof(value.substr(first + 1, second - first - 1)),
                     std::stof(value.substr(second + 1, third - second - 1))},
                    value.substr(third + 1) != "0"});
            }
        }
        else if (key == "input") data.inputPreset = value;
        else if (key == "selectedStoryRoute") data.frontend.selectedStoryRoute = value;
        else if (key == "recapSection") data.frontend.storySoFarSection = static_cast<std::size_t>(std::stoul(value));
        else if (key == "routeDiscovered") data.frontend.discoveredStoryRoutes.push_back(value);
        else if (key == "pendingStoryUnlock") data.frontend.pendingStoryUnlocks.push_back(value);
        else if (key == "objectiveHistory") data.frontend.objectiveHistory.push_back(value);
        else if (key == "lastMenuMode") data.frontend.lastMenuMode = value;
        else if (key == "lastBattleSelection") data.frontend.lastBattleSelection = static_cast<std::size_t>(std::stoul(value));
        else if (key == "lastExtrasSelection") data.frontend.lastExtrasSelection = static_cast<std::size_t>(std::stoul(value));
        else if (key == "lastOptionsSelection") data.frontend.lastOptionsSelection = static_cast<std::size_t>(std::stoul(value));
        else if (key == "continueArea") data.frontend.currentArea = value;
        else if (key == "continueObjective") data.frontend.currentObjective = value;
        else if (key == "storyProgressPercent") data.frontend.storyProgressPercent = std::stoi(value);
        else if (key == "playtimeSeconds") data.frontend.playtimeSeconds = std::stof(value);
        else if (key == "qolHoldDialogue") data.qol.holdToAdvanceDialogue = value != "0";
        else if (key == "qolFirstTimeHints") data.qol.firstTimeHints = value != "0";
        else if (key == "qolReducedMotion") data.qol.reducedMotion = value != "0";
        else if (key == "qolReducedShake") data.qol.reducedCameraShake = value != "0";
        else if (key == "qolReducedFlashes") data.qol.reducedFlashes = value != "0";
        else if (key == "qolHighContrast") data.qol.highContrastHud = value != "0";
        else if (key == "qolLargerText") data.qol.largerText = value != "0";
        else if (key == "qolCombatMessages") data.qol.combatMessages = value;
        else if (key == "qolObjectiveDisplay") data.qol.objectiveDisplay = value;
        else if (key == "qolCameraSensitivity") data.qol.cameraSensitivity = std::stof(value);
        else if (key == "qolInvertCameraX") data.qol.invertCameraX = value != "0";
        else if (key == "qolInvertCameraY") data.qol.invertCameraY = value != "0";
        else if (key == "qolGentleRecenter") data.qol.gentleCameraRecenter = value != "0";
        else if (key == "qolHudScale") data.qol.hudScale = std::stof(value);
        else if (key == "qolDialogueScale") data.qol.dialogueScale = std::stof(value);
        else if (key == "qolDialogueSpeed") data.qol.dialogueSpeed = value;
        else if (key == "qolDialogueAuto") data.qol.dialogueAutoAdvance = value != "0";
        else if (key == "cardOwner") data.tournamentCard.ownerId = value;
        else if (key == "cardLevel") data.tournamentCard.level = std::stoi(value);
        else if (key == "cardXp") data.tournamentCard.xp = std::stoi(value);
        else if (key == "cardPending") data.tournamentCard.pendingBonusChoices = std::stoi(value);
        else if (key == "cardAcquired") data.tournamentCard.acquired = value != "0";
        else if (key == "cardBonusHp" || key == "cardBonus0") data.tournamentCard.bonuses.hp = std::stoi(value);
        else if (key == "cardBonusPower" || key == "cardBonus1") data.tournamentCard.bonuses.power = std::stoi(value);
        else if (key == "cardBonusDefense" || key == "cardBonus2") data.tournamentCard.bonuses.defense = std::stoi(value);
        else if (key == "cardBonusSpeed" || key == "cardBonus3") data.tournamentCard.bonuses.speed = std::stoi(value);
        else if (key == "cardBonusFocus" || key == "cardBonus4") data.tournamentCard.bonuses.focus = std::stoi(value);
        else if (key == "recordWins") data.records.wins = std::stoi(value);
        else if (key == "recordLosses") data.records.losses = std::stoi(value);
        else if (key == "recordBestCombo") data.records.bestCombo = std::stoi(value);
        else if (key == "recordPerfectBlocks") data.records.perfectBlocks = std::stoi(value);
        else if (key == "recordGuardBreaks") data.records.guardBreaks = std::stoi(value);
        else if (key == "recordPursuitFinishers") data.records.pursuitFinishers = std::stoi(value);
        else if (key == "recordEnergyBeams") data.records.energyBeamUses = std::stoi(value);
        else if (key == "recordObjectSwaps") data.records.objectSwaps = std::stoi(value);
        else if (key == "recordBestRank") { data.records.hasRank = value != "-"; data.records.bestRank = RpgProgressSystem::rankFromLabel(value); }
        else if (key == "rpgCoins") data.rpg.coins = std::stoi(value);
        else if (key == "rpgVendorDiscount") data.rpg.vendorDiscountPercent = std::stoi(value);
        else if (key == "rpgNextOfficialMeal") data.rpg.nextOfficialMealBoost = value != "0";
        else if (key == "rpgAccessory") data.rpg.equippedAccessoryId = value;
        else if (key == "rpgNecklaceAcquired") data.rpg.weightedNecklaceAcquired = value != "0";
        else if (key == "rpgNecklaceMastery") data.rpg.weightedNecklaceMastery = std::stof(value);
        else if (key == "rpgNecklaceReward") data.rpg.weightedNecklaceMasteryRewardGranted = value != "0";
        else if (key == "rpgTitle") data.rpg.titles.push_back(value);
        else if (key == "rpgMemento") data.rpg.mementos.push_back(value);
        else if (key == "rpgProfile") data.rpg.profileUnlocks.push_back(value);
        else if (key == "flag") data.story.flags.push_back(value);
    }
    if (data.schemaVersion != 2 && data.schemaVersion != 3 && data.schemaVersion != 4 &&
        data.schemaVersion != 5 && data.schemaVersion != 6 && data.schemaVersion != SaveData::kSchemaVersion)
        throw std::runtime_error("Unsupported save schema");

    auto replacePrefix = [](std::string& value, const std::string& oldPrefix, const std::string& newPrefix) {
        if (value.rfind(oldPrefix, 0) == 0) value = newPrefix + value.substr(oldPrefix.size());
    };
    replacePrefix(data.story.routeId, "project_hollow", "organization_red");
    replacePrefix(data.world.mapId, "project_hollow", "organization_red");
    for (auto& flag : data.story.flags) replacePrefix(flag, "project_hollow", "organization_red");

    if (std::find(data.frontend.discoveredStoryRoutes.begin(),
                  data.frontend.discoveredStoryRoutes.end(), "rrvvfo") ==
        data.frontend.discoveredStoryRoutes.end())
        data.frontend.discoveredStoryRoutes.insert(data.frontend.discoveredStoryRoutes.begin(), "rrvvfo");
    if (data.frontend.selectedStoryRoute.empty()) data.frontend.selectedStoryRoute = "rrvvfo";
    if (data.frontend.objectiveHistory.size() > 8)
        data.frontend.objectiveHistory.erase(data.frontend.objectiveHistory.begin(),
                                             data.frontend.objectiveHistory.end() - 8);
    if (data.qol.combatMessages == "important") data.qol.combatMessages = "minimal";
    if (data.qol.combatMessages != "full" && data.qol.combatMessages != "minimal" &&
        data.qol.combatMessages != "off") data.qol.combatMessages = "full";
    if (data.qol.objectiveDisplay != "full" && data.qol.objectiveDisplay != "minimal" &&
        data.qol.objectiveDisplay != "off") data.qol.objectiveDisplay = "full";
    data.qol.cameraSensitivity = std::clamp(data.qol.cameraSensitivity, .75f, 1.25f);
    data.qol.hudScale = std::clamp(data.qol.hudScale, .90f, 1.10f);
    data.qol.dialogueScale = std::clamp(data.qol.dialogueScale, 1.0f, 1.30f);
    if (data.qol.dialogueSpeed != "slow" && data.qol.dialogueSpeed != "normal" && data.qol.dialogueSpeed != "fast")
        data.qol.dialogueSpeed = "normal";
    data.frontend.storyProgressPercent = std::clamp(data.frontend.storyProgressPercent, 0, 100);
    data.frontend.playtimeSeconds = std::max(0.0f, data.frontend.playtimeSeconds);
    data.rpg.weightedNecklaceMastery = std::clamp(data.rpg.weightedNecklaceMastery, 0.0f, 100.0f);
    if (!data.rpg.weightedNecklaceAcquired) data.rpg.equippedAccessoryId.clear();
    data.schemaVersion = SaveData::kSchemaVersion;
    return data;
}

} // namespace px
