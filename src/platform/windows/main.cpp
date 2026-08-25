#include <SDL.h>
#include "platform/desktop/sdl_application.hpp"
#include <cstdlib>
#include <filesystem>

namespace {

std::string defaultSaveDirectory() {
    if (const char* localAppData = std::getenv("LOCALAPPDATA"))
        return (std::filesystem::path(localAppData) / "ParallelsX/ClashOfSouls").string();
    if (const char* userProfile = std::getenv("USERPROFILE"))
        return (std::filesystem::path(userProfile) / "AppData/Local/ParallelsX/ClashOfSouls").string();
    return "ParallelsX/ClashOfSouls";
}

} // namespace

int main(int argc, char** argv) {
    return px::desktop::runSdlApplication(argc, argv, {
        "Windows",
        defaultSaveDirectory(),
        "PX_WINDOWS_SAVE_DIR",
        {},
    });
}
