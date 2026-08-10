#pragma once
#include <string>
#include <vector>

namespace px {

struct StoryRouteDefinition {
    std::string id;
    std::string characterName;
    std::string title;
    std::string description;
    std::string artId;
    std::string accentId;
    bool implemented{false};
};

class StoryRouteRegistry {
public:
    StoryRouteRegistry();
    const std::vector<StoryRouteDefinition>& routes() const { return routes_; }
    const StoryRouteDefinition& get(const std::string& id) const;

private:
    std::vector<StoryRouteDefinition> routes_;
};

} // namespace px
