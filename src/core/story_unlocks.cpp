#include "core/story_unlocks.hpp"
#include <algorithm>

namespace px {
namespace {

bool contains(const std::vector<std::string>& values, const std::string& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

bool hasStoryFlag(const SaveData& save, const std::string& value) {
    return contains(save.story.flags, value);
}

std::string routeForEvent(StoryDiscoveryEvent event) {
    switch (event) {
        case StoryDiscoveryEvent::BarkReunion: return "bark";
        case StoryDiscoveryEvent::WadeReunion: return "wade";
        case StoryDiscoveryEvent::VirekEmeraldMissingNoticed: return "virek";
    }
    return {};
}

void ensureDefaultRoute(SaveData& save) {
    if (!contains(save.frontend.discoveredStoryRoutes, "rrvvfo"))
        save.frontend.discoveredStoryRoutes.insert(save.frontend.discoveredStoryRoutes.begin(), "rrvvfo");
    if (save.frontend.selectedStoryRoute.empty()) save.frontend.selectedStoryRoute = "rrvvfo";
}

} // namespace

bool StoryUnlockSystem::discover(SaveData& save, StoryDiscoveryEvent event) {
    ensureDefaultRoute(save);
    const auto route = routeForEvent(event);
    if (route.empty() || contains(save.frontend.discoveredStoryRoutes, route)) return false;
    save.frontend.discoveredStoryRoutes.push_back(route);
    if (!contains(save.frontend.pendingStoryUnlocks, route)) save.frontend.pendingStoryUnlocks.push_back(route);
    return true;
}

void StoryUnlockSystem::reconstructFromProgress(SaveData& save) {
    ensureDefaultRoute(save);

    // Reconstruction is driven by semantic story moments. Chapter completion by
    // itself is deliberately insufficient, especially for Virek.
    if (hasStoryFlag(save, "bark_route_discovered") ||
        hasStoryFlag(save, "ch2_rrvvfo_bark_reunion"))
        discover(save, StoryDiscoveryEvent::BarkReunion);
    if (hasStoryFlag(save, "wade_route_discovered") ||
        hasStoryFlag(save, "ch2_rrvvfo_wade_reunion"))
        discover(save, StoryDiscoveryEvent::WadeReunion);
    if (hasStoryFlag(save, "virek_route_discovered") ||
        hasStoryFlag(save, "rrvvfo_noticed_virek_emerald_missing"))
        discover(save, StoryDiscoveryEvent::VirekEmeraldMissingNoticed);
}

bool StoryUnlockSystem::isDiscovered(const SaveData& save, const std::string& routeId) {
    if (routeId == "rrvvfo") return true;
    return contains(save.frontend.discoveredStoryRoutes, routeId);
}

std::vector<std::string> StoryUnlockSystem::visibleRoutes(const SaveData& save) {
    std::vector<std::string> visible{"rrvvfo"};
    for (const auto* route : {"bark", "wade", "virek"})
        if (isDiscovered(save, route)) visible.emplace_back(route);
    return visible;
}

std::string StoryUnlockSystem::nextQueuedUnlock(const SaveData& save) {
    return save.frontend.pendingStoryUnlocks.empty() ? std::string{} : save.frontend.pendingStoryUnlocks.front();
}

void StoryUnlockSystem::acknowledgeQueuedUnlock(SaveData& save, const std::string& routeId) {
    const auto it = std::find(save.frontend.pendingStoryUnlocks.begin(),
                              save.frontend.pendingStoryUnlocks.end(), routeId);
    if (it != save.frontend.pendingStoryUnlocks.end()) save.frontend.pendingStoryUnlocks.erase(it);
}

} // namespace px
