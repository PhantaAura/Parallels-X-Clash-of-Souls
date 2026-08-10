#include "content/story_route_registry.hpp"
#include <stdexcept>

namespace px {

StoryRouteRegistry::StoryRouteRegistry() {
    routes_ = {
        {"rrvvfo", "RRVVFO", "THE LOST YEAR",
         "A continuous story beginning with Sage's training and Tournament Road.",
         "route_rrvvfo_pose", "crimson", true},
        {"bark", "BARK", "BARK STORY",
         "This discovered character story is not playable in the current build.",
         "route_bark_pose", "earth", false},
        {"wade", "WADE", "WADE STORY",
         "This discovered character story is not playable in the current build.",
         "route_wade_pose", "lightning", false},
        {"virek", "VIREK", "VIREK STORY",
         "This discovered character story is not playable in the current build.",
         "route_virek_pose", "emerald", false},
    };
}

const StoryRouteDefinition& StoryRouteRegistry::get(const std::string& id) const {
    for (const auto& route : routes_) if (route.id == id) return route;
    throw std::out_of_range("Unknown story route: " + id);
}

} // namespace px
