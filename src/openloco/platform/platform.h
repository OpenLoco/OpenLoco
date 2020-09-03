#pragma once

#include <cstdint>
#include <vector>

#include "../core/FileSystem.hpp"

namespace openloco::platform
{
    uint32_t getTime();
    fs::path getUserDirectory();
    std::string promptDirectory(const std::string& title);
    fs::path GetCurrentExecutablePath();
    std::vector<fs::path> getDrives();
#if defined(__APPLE__) && defined(__MACH__)
    fs::path GetBundlePath();
#endif
}
