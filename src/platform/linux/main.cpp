#include "platform/desktop/sdl_application.hpp"
#include <cstdlib>
#include <filesystem>

namespace {

std::string defaultSaveDirectory() {
    if (const char* xdg = std::getenv("XDG_DATA_HOME"))
        return (std::filesystem::path(xdg) / "ParallelsX/ClashOfSouls").string();
    if (const char* home = std::getenv("HOME"))
        return (std::filesystem::path(home) / ".local/share/ParallelsX/ClashOfSouls").string();
    return "dev-saves/linux";
}

} // namespace

int main(int argc, char** argv) {
    return px::desktop::runSdlApplication(argc, argv, {
        "Linux",
        defaultSaveDirectory(),
        "PX_LINUX_SAVE_DIR",
        {"ParallelsX-0.4H-GOLDEN-GATE-QOL-linux-dev.save",
         "ParallelsX-0.4G-GOLD-linux-dev.save"},
        {},
        true,
    });
}
