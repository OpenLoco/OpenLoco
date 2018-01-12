#include <filesystem>
#include <fstream>
#include <shlobj.h>
#include <windows.h>
#include "interop/interop.hpp"
#include "config.h"

namespace fs = std::experimental::filesystem;

namespace openloco::config
{
    static loco_global<config_t, 0x0050AEB4> _config;
    static new_config _new_config;

    static void read_new_config();
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
    void read()
    {
        LOCO_CALLPROC_X(0x00441A6C);
        read_new_config();
    }

    // 0x00441BB8
    void write()
    {
        LOCO_CALLPROC_X(0x00441BB8);
    }

    static void read_new_config()
    {
        auto configPath = get_new_config_path();
        std::ifstream stream(configPath, std::ios::binary);
        if (stream.is_open())
        {
            stream.read((char *)&_new_config, sizeof(_new_config));
        }
    }

    static fs::path get_new_config_path()
    {
        return get_user_directory() / "openloco.cfg";
    }

    static fs::path get_user_directory()
    {
        auto result = fs::path();
        PWSTR path = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, nullptr, &path)))
        {
            result = fs::path(path) / "OpenLoco";
        }
        CoTaskMemFree(path);
        return result;
    }
}
