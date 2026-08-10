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
    out << "qolHoldDialogue=" << (data.qol.holdToAdvanceDialogue ? 1 : 0) << '\n';
    out << "qolFirstTimeHints=" << (data.qol.firstTimeHints ? 1 : 0) << '\n';
    out << "qolReducedMotion=" << (data.qol.reducedMotion ? 1 : 0) << '\n';
    out << "qolReducedShake=" << (data.qol.reducedCameraShake ? 1 : 0) << '\n';
    out << "qolReducedFlashes=" << (data.qol.reducedFlashes ? 1 : 0) << '\n';
    out << "qolHighContrast=" << (data.qol.highContrastHud ? 1 : 0) << '\n';
    out << "qolLargerText=" << (data.qol.largerText ? 1 : 0) << '\n';
    out << "qolCombatMessages=" << data.qol.combatMessages << '\n';
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
        else if (key == "qolHoldDialogue") data.qol.holdToAdvanceDialogue = value != "0";
        else if (key == "qolFirstTimeHints") data.qol.firstTimeHints = value != "0";
        else if (key == "qolReducedMotion") data.qol.reducedMotion = value != "0";
        else if (key == "qolReducedShake") data.qol.reducedCameraShake = value != "0";
        else if (key == "qolReducedFlashes") data.qol.reducedFlashes = value != "0";
        else if (key == "qolHighContrast") data.qol.highContrastHud = value != "0";
        else if (key == "qolLargerText") data.qol.largerText = value != "0";
        else if (key == "qolCombatMessages") data.qol.combatMessages = value;
        else if (key == "flag") data.story.flags.push_back(value);
    }
    if (data.schemaVersion != 2 && data.schemaVersion != 3 && data.schemaVersion != SaveData::kSchemaVersion)
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
    if (data.qol.combatMessages != "full" && data.qol.combatMessages != "important" &&
        data.qol.combatMessages != "off") data.qol.combatMessages = "full";
    data.schemaVersion = SaveData::kSchemaVersion;
    return data;
}

} // namespace px
