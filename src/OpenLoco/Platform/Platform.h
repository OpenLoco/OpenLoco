#pragma once

#include <cstdint>
#include <vector>

#include "../Core/FileSystem.hpp"

namespace OpenLoco::platform
{
    uint32_t getTime();
    fs::path getUserDirectory();
    fs::path promptDirectory(const std::string& title);
    fs::path GetCurrentExecutablePath();
    std::vector<fs::path> getDrives();
    bool isRunningInWine();
#if defined(__APPLE__) && defined(__MACH__)
    fs::path GetBundlePath();
#endif
}
