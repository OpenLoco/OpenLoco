#pragma once

#include <cstdint>

#ifdef OPENLOCO_USE_BOOST_FS
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#else
#include <filesystem>
namespace fs = std::experimental::filesystem;
#endif

namespace openloco::platform
{
    uint32_t get_time();
    fs::path get_user_directory();
}
