#include <boost/filesystem.hpp>
#include <fstream>
//#include <shlobj.h>
//#include <windows.h>
#include "interop/interop.hpp"
#include "config.h"

using namespace openloco::interop;
namespace fs = boost::filesystem;

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
//        std::ifstream stream(configPath);
//        if (stream.is_open())
//        {
//            std::getline(stream, _new_config.loco_install_path);
//            stream >> _new_config.breakdowns_disabled;
//        }
        return _new_config;
    }

    void write_new_config()
    {
        auto configPath = get_new_config_path();
        auto dir = configPath.parent_path();
        if (!fs::is_directory(dir))
        {
            fs::create_directories(configPath.parent_path());
            fs::permissions(dir, fs::perms::owner_read | fs::perms::owner_write |
                                 fs::perms::group_read | 
                                 fs::perms::others_read);
        }

//        std::ofstream stream(configPath);
//        if (stream.is_open())
//        {
//            stream << _new_config.loco_install_path << std::endl;
//            stream << _new_config.breakdowns_disabled << std::endl;
//        }
    }

    static fs::path get_new_config_path()
    {
        return get_user_directory() / "openloco.cfg";
    }

    static fs::path get_user_directory()
    {
        auto result = fs::path();
//        PWSTR path = nullptr;
//        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, nullptr, &path)))
//        {
//            result = fs::path(path) / "OpenLoco";
//        }
//        CoTaskMemFree(path);
        return "~";
    }
}
