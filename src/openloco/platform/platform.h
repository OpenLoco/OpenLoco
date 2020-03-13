#pragma once

#include <cstdint>
#ifdef _OPENLOCO_USE_BOOST_FS_
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

namespace openloco::platform
{
    uint32_t get_time();
    fs::path get_user_directory();
    std::string prompt_directory(const std::string& title);
    fs::path GetCurrentExecutablePath();
    std::vector<fs::path> getDrives();
#if defined(__APPLE__) && defined(__MACH__)
    fs::path GetBundlePath();
#endif
}
