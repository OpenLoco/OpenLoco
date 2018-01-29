
#ifdef _OPENLOCO_USE_BOOST_FS_
#include <boost/filesystem.hpp>
#else
#include <experimental/filesystem>
#endif
#include <fstream>

#ifdef _WIN32
#include <shlobj.h>
#include <windows.h>
#endif

#include "config.h"
#include "interop/interop.hpp"
#include "platform/platform.h"

using namespace openloco::interop;

#ifdef _OPENLOCO_USE_BOOST_FS_
namespace fs = boost::filesystem;
#else
namespace fs = std::experimental::filesystem;
#endif

namespace openloco::config
{
    static loco_global<config_t, 0x0050AEB4> _config;
    static new_config _new_config;

    static fs::path get_new_config_path();
    static fs::path get_user_directory();

    config_t& get()
    {
        return _config;
    }

    new_config& get_new()
    {
        return _new_config;
    }

    // 0x00441A6C
    config_t& read()
    {
        call(0x00441A6C);
        return _config;
    }

    // 0x00441BB8
    void write()
    {
        call(0x00441BB8);
        write_new_config();
    }

    new_config& read_new_config()
    {
        auto configPath = get_new_config_path();
#ifdef _OPENLOCO_USE_BOOST_FS_
        std::ifstream stream(configPath.string());
#else
        std::ifstream stream(configPath);
#endif
        if (stream.is_open())
        {
            std::getline(stream, _new_config.loco_install_path);
            stream >> _new_config.breakdowns_disabled;
        }
        return _new_config;
    }

    void write_new_config()
    {
        auto configPath = get_new_config_path();
        auto dir = configPath.parent_path();
        if (!fs::is_directory(dir))
        {
            fs::create_directories(configPath.parent_path());
            // clang-format off
            fs::permissions(
                dir,
                fs::perms::owner_all |
#ifdef _OPENLOCO_USE_BOOST_FS_
                fs::perms::group_read | fs::perms::group_exe |
                fs::perms::others_read | fs::perms::others_exe
#else
                fs::perms::group_read | fs::perms::group_exec |
                fs::perms::others_read | fs::perms::others_exec
#endif
            );
            // clang-format on
        }

#ifdef _OPENLOCO_USE_BOOST_FS_
        std::ofstream stream(configPath.string());
#else
        std::ofstream stream(configPath);
#endif
        if (stream.is_open())
        {
            stream << _new_config.loco_install_path << std::endl;
            stream << _new_config.breakdowns_disabled << std::endl;
        }
    }

    static fs::path get_new_config_path()
    {
        return platform::get_user_directory() / "openloco.cfg";
    }
}
