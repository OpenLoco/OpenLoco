
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

#include "config.convert.hpp"
#include "config.h"
#include "environment.h"
#include "interop/interop.hpp"
#include "utility/yaml.hpp"

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
    static YAML::Node _config_yaml;

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

        // WARNING: on Windows, YAML::LoadFile only supports ANSI paths
        _config_yaml = YAML::LoadFile(configPath.string());

        const auto& config = _config_yaml;
        auto& displayNode = config["display"];
        if (displayNode && displayNode.IsMap())
        {
            auto& displayConfig = _new_config.display;
            displayConfig.mode = displayNode["mode"].as<screen_mode>(screen_mode::window);
            displayConfig.index = displayNode["index"].as<int32_t>(0);
            displayConfig.window_resolution = displayNode["window_resolution"].as<resolution_t>();
            displayConfig.fullscreen_resolution = displayNode["fullscreen_resolution"].as<resolution_t>();
        }

        if (config["loco_install_path"])
            _new_config.loco_install_path = config["loco_install_path"].as<std::string>();
        if (config["language"])
            _new_config.language = config["language"].as<std::string>();
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

        auto& node = _config_yaml;

        const auto& displayConfig = _new_config.display;
        auto displayNode = node["display"];
        displayNode["mode"] = displayConfig.mode;
        if (displayConfig.index != 0)
        {
            displayNode["index"] = displayConfig.index;
        }
        else
        {
            displayNode.remove("index");
        }
        displayNode["window_resolution"] = displayConfig.window_resolution;
        displayNode["fullscreen_resolution"] = displayConfig.fullscreen_resolution;
        node["display"] = displayNode;

        node["loco_install_path"] = _new_config.loco_install_path;
        node["language"] = _new_config.language;
        node["breakdowns_disabled"] = _new_config.breakdowns_disabled;

#ifdef _OPENLOCO_USE_BOOST_FS_
        std::ofstream stream(configPath.string());
#else
        std::ofstream stream(configPath);
#endif
        if (stream.is_open())
        {
            stream << node << std::endl;
        }
    }
}
