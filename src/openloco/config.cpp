
#ifdef _OPENLOCO_USE_BOOST_FS_
#include <boost/filesystem.hpp>
#else
#include <experimental/filesystem>
#endif
#include <fstream>

#ifdef _WIN32
// Ignore warnings generated from yaml-cpp
#pragma warning(push)
#pragma warning(disable : 4127)
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
#pragma warning(disable : 4996)
#include <yaml-cpp/yaml.h>
#pragma warning(pop)
#else
#include <yaml-cpp/yaml.h>
#endif

#ifdef _WIN32
#include <shlobj.h>
#include <windows.h>
#endif

#include "config.h"
#include "environment.h"
#include "interop/interop.hpp"

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
        auto configPath = environment::get_path(environment::path_id::openloco_yml);

        if (!fs::exists(configPath))
            return _new_config;

#ifdef _OPENLOCO_USE_BOOST_FS_
        YAML::Node config = YAML::LoadFile(configPath.string());
#else
        // WARNING: YAML::LoadFile only supports ANSI 260 paths
        YAML::Node config = YAML::LoadFile(configPath.string());
#endif

        if (config["loco_install_path"])
            _new_config.loco_install_path = config["loco_install_path"].as<std::string>();
        if (config["breakdowns_disabled"])
            _new_config.breakdowns_disabled = config["breakdowns_disabled"].as<bool>();

        return _new_config;
    }

    void write_new_config()
    {
        auto configPath = environment::get_path(environment::path_id::openloco_yml);
        auto dir = configPath.parent_path();
        if (!fs::is_directory(dir))
        {
            fs::create_directories(dir);
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

        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "loco_install_path" << YAML::Value << _new_config.loco_install_path;
        out << YAML::Key << "breakdowns_disabled" << YAML::Value << _new_config.breakdowns_disabled;
        out << YAML::EndMap;

#ifdef _OPENLOCO_USE_BOOST_FS_
        std::ofstream stream(configPath.string());
#else
        std::ofstream stream(configPath);
#endif
        if (stream.is_open())
        {
            stream << out.c_str();
        }
    }
}
