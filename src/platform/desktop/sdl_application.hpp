#pragma once
#include <string>
#include <vector>

namespace px::desktop {

struct SdlPlatformConfig {
    std::string platformName;
    std::string defaultSaveDirectory;
    std::string saveEnvironmentVariable;
    std::vector<std::string> legacySaveFileNames;
};

int runSdlApplication(int argc, char** argv, const SdlPlatformConfig& platform);

} // namespace px::desktop
