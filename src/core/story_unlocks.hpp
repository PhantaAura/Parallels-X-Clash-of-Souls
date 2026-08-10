#pragma once
#include "core/save.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace px {

enum class StoryDiscoveryEvent : std::uint8_t {
    BarkReunion,
    WadeReunion,
    VirekEmeraldMissingNoticed
};

class StoryUnlockSystem {
public:
    static bool discover(SaveData& save, StoryDiscoveryEvent event);
    static void reconstructFromProgress(SaveData& save);
    static bool isDiscovered(const SaveData& save, const std::string& routeId);
    static std::vector<std::string> visibleRoutes(const SaveData& save);
    static std::string nextQueuedUnlock(const SaveData& save);
    static void acknowledgeQueuedUnlock(SaveData& save, const std::string& routeId);
};

} // namespace px
