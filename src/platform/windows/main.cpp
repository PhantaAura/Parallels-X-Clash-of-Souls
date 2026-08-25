#include <SDL.h>
#include <windows.h>
#include "platform/desktop/sdl_application.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {

std::string defaultSaveDirectory() {
    if (const char* localAppData = std::getenv("LOCALAPPDATA"))
        return (std::filesystem::path(localAppData) / "ParallelsX/ClashOfSouls").string();
    if (const char* userProfile = std::getenv("USERPROFILE"))
        return (std::filesystem::path(userProfile) / "AppData/Local/ParallelsX/ClashOfSouls").string();
    return "ParallelsX/ClashOfSouls";
}

std::filesystem::path executableDirectory() {
    std::wstring buffer(32768, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size())
        throw std::runtime_error("Windows could not resolve the executable location");
    buffer.resize(length);
    return std::filesystem::path(buffer).parent_path();
}

void showStartupError(const std::filesystem::path& logPath, const std::wstring& detail) {
    const std::wstring message =
        L"Parallels X could not start.\n\n" + detail +
        L"\n\nExtract the entire ZIP before launching the EXE.\nStartup log:\n" + logPath.wstring();
    MessageBoxW(nullptr, message.c_str(), L"Parallels X - Startup Error", MB_OK | MB_ICONERROR);
}

} // namespace

int main(int argc, char** argv) {
    const std::filesystem::path saveDirectory = defaultSaveDirectory();
    const std::filesystem::path logPath = saveDirectory / "ParallelsX-startup.log";
    try {
        std::filesystem::create_directories(saveDirectory);
        std::ofstream log(logPath, std::ios::trunc);
        auto* previousErrorBuffer = std::cerr.rdbuf(log.rdbuf());
        auto* previousLogBuffer = std::clog.rdbuf(log.rdbuf());

        const auto exeDirectory = executableDirectory();
        SetCurrentDirectoryW(exeDirectory.c_str());
        std::clog << "Parallels X U16 Windows startup\n"
                  << "Executable directory: " << exeDirectory.string() << '\n';
        const int result = px::desktop::runSdlApplication(argc, argv, {
            "Windows",
            saveDirectory.string(),
            "PX_WINDOWS_SAVE_DIR",
            {},
            exeDirectory,
            false,
        });
        std::clog << "Exit code: " << result << '\n';
        log.flush();
        std::cerr.rdbuf(previousErrorBuffer);
        std::clog.rdbuf(previousLogBuffer);
        if (result != 0)
            showStartupError(logPath, L"Startup stopped with error code " + std::to_wstring(result) + L".");
        return result;
    } catch (const std::exception& error) {
        std::ofstream log(logPath, std::ios::app);
        log << "Fatal startup error: " << error.what() << '\n';
        log.flush();
        showStartupError(logPath, L"A required game file or Windows service was unavailable.");
        return 10;
    }
}
