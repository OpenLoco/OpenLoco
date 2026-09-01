#pragma once

#include <OpenLoco/Core/FileSystem.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace OpenLoco::Platform
{
    void initialise();
    uint32_t getTime();
    fs::path getUserDirectory();
    fs::path showFolderPicker(const std::string& title);
    fs::path getCurrentExecutablePath();
    std::vector<fs::path> getDrives();
    std::string getEnvironmentVariable(const std::string& name);

    // Well-known locations the original Locomotion game files may be installed to,
    // in order of preference. The returned paths are candidates only; the caller is
    // responsible for checking whether they actually contain the game files.
    std::vector<fs::path> getLocoInstallSearchPaths();

    bool isRunningInWine();
#if defined(__APPLE__) && defined(__MACH__)
    fs::path GetBundlePath();
#endif
    bool isStdOutRedirected();
    bool hasTerminalVT100Support();
    bool enableVT100TerminalMode();
    std::vector<std::string> getCmdLineVector(int argc, const char** argv);

    // Prevents multiple instances of OpenLoco running at the same time
    // returns true if this is the only instance
    bool lockSingleInstance();
}
