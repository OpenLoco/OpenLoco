#include "Environment.h"
#include "Config.h"
#include "Interop/Interop.hpp"
#include "Platform/Platform.h"
#include "Ui.h"
#include "Utility/Collection.hpp"
#include "Utility/String.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include "SystemPopups/system_message_popup.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Environment
{
    loco_global<char[260], 0x009D118E> _path_buffer;
    loco_global<char[257], 0x0050B0CE> _path_install;
    loco_global<char[257], 0x0050B1CF> _path_saves_single_player;
    loco_global<char[257], 0x0050B2EC> _path_saves_two_player;
    loco_global<char[257], 0x0050B406> _path_scenarios;
    loco_global<char[257], 0x0050B518> _path_landscapes;
    loco_global<char[257], 0x0050B635> _path_objects;

    static fs::path getBasePath(path_id id);
    static fs::path getSubPath(path_id id);
#ifndef _WIN32
    static fs::path findSimilarFile(const fs::path& path);
#endif

    static bool validateLocoInstallPath(const fs::path& path)
    {
        if (path.empty())
        {
            return false;
        }
        else
        {
            auto g1Path = path / getSubPath(path_id::g1);
            bool g1Exists = fs::exists(g1Path);
#ifndef _WIN32
            if (!g1Exists)
            {
                g1Path = findSimilarFile(g1Path);
                g1Exists = !g1Path.empty();
            }
#endif
            return g1Exists;
        }
    }

    static fs::path autoDetectLocoInstallPath()
    {
        static constexpr const char* searchPaths[] = {
            "C:/Program Files (x86)/Atari/Locomotion",
            "C:/GOG Games/Chris Sawyer's Locomotion",
            "C:/GOG Games/Locomotion",
        };

        std::cout << "Searching for Locomotion install path..." << std::endl;
        for (auto path : searchPaths)
        {
            if (validateLocoInstallPath(path))
            {
                std::cout << "  found: " << path << std::endl;
                return path;
            }
        }
        return fs::path();
    }

    static fs::path resolveLocoInstallPath()
    {
        auto& cfg = Config::getNew();
        auto path = fs::path(cfg.loco_install_path);
        if (!path.empty())
        {
            if (validateLocoInstallPath(path))
            {
                Config::writeNewConfig();
                return path;
            }
            std::cerr << "Configured install path for Locomotion is missing Data/g1.DAT." << std::endl;
        }

        path = autoDetectLocoInstallPath();
        if (!path.empty())
        {
            cfg.loco_install_path = path.make_preferred().u8string();
            Config::writeNewConfig();
            return path;
        }
        else
        {
            std::cerr << "Unable to find install path for Locomotion." << std::endl
                      << "You will need to manually provide it." << std::endl;
            Ui::showMessageBox("OpenLoco", "Select your Locomotion install path.");
            path = platform::promptDirectory("Select your Locomotion install path.");
            if (validateLocoInstallPath(path))
            {
                cfg.loco_install_path = path.make_preferred().u8string();
                Config::writeNewConfig();
                return path;
            }

            std::cerr << "Path is missing g1.dat." << std::endl;
            Ui::showMessageBox("OpenLoco", "Path is missing Data/g1.DAT.");
            std::exit(-1);
        }
    }

    static fs::path getLocoInstallPath()
    {
        return _path_install.get();
    }

#ifndef _WIN32
    /**
     * Performs a case-insensitive search on the containing directory of
     * the given file and returns the first match.
     */
    static fs::path findSimilarFile(const fs::path& path)
    {
        try
        {
            auto expectedFilename = path.filename().generic_string();
            auto directory = path.parent_path();
            for (auto& item : fs::directory_iterator(directory))
            {
                auto& p = item.path();
                if (Utility::iequals(p.filename().generic_string(), expectedFilename))
                {
                    return p;
                }
            }
        }
        catch (const std::exception&)
        {
            // Ignore errors when searching, most common will be that the
            // parent directory does not exist
        }
        return fs::path();
    }
#endif // _WIN32

    // 0x004416B5
    fs::path getPath(path_id id)
    {
        auto basePath = getBasePath(id);
        auto subPath = getSubPath(id);
        auto result = (basePath / subPath).lexically_normal();
        if (!fs::exists(result))
        {
#ifndef _WIN32
            auto similarResult = findSimilarFile(result);
            if (similarResult.empty())
            {
                std::cerr << "Warning: file " << result << " could not be not found" << std::endl;
            }
            else
            {
                result = similarResult;
            }
#else
            std::cerr << "Warning: file " << result << " could not be not found" << std::endl;
#endif
        }
        return result;
    }

    fs::path getPathNoWarning(path_id id)
    {
        auto basePath = getBasePath(id);
        auto subPath = getSubPath(id);
        auto result = (basePath / subPath).lexically_normal();
        return result;
    }

    template<typename T>
    static void setDirectory(T& buffer, fs::path path)
    {
        Utility::strcpy_safe(buffer, path.make_preferred().u8string().c_str());
    }

    void autoCreateDirectory(const fs::path& path)
    {
        try
        {
            if (!fs::is_directory(path))
            {
                auto path8 = path.u8string();
                std::printf("Creating directory: %s\n", path8.c_str());
                fs::create_directories(path);
                // clang-format off
                fs::permissions(
                    path,
                    fs::perms::owner_all |
                    fs::perms::group_read | fs::perms::group_exec |
                    fs::perms::others_read | fs::perms::others_exec);
                // clang-format on
            }
        }
        catch (const std::exception& e)
        {
            std::fprintf(stderr, "Unable to create directory: %s\n", e.what());
            platform::system_message_popup("Unable to create directory:", e.what()); 
        }
    }

    // 0x004412CE
    void resolvePaths()
    {
        auto basePath = resolveLocoInstallPath();
        setDirectory(_path_install, basePath);

        // NB: vanilla routines do not use std::filesystem yet, so the trailing slash is still needed.
        auto saveDirectory = getPathNoWarning(path_id::save) / "";
        auto& configLastSavePath = Config::getNew().last_save_path;
        if (!configLastSavePath.empty())
        {
            auto directory = fs::u8path(configLastSavePath);
            if (fs::is_directory(directory))
            {
                saveDirectory = directory;
            }
        }
        setDirectory(_path_saves_single_player, saveDirectory);
        setDirectory(_path_saves_two_player, saveDirectory);
        autoCreateDirectory(saveDirectory);

        setDirectory(_path_scenarios, basePath / "Scenarios/*.SC5");
        setDirectory(_path_landscapes, basePath / "Scenarios/Landscapes/*.SC5");
        setDirectory(_path_objects, basePath / "ObjData/*.DAT");
    }

    static fs::path getBasePath(path_id id)
    {
        switch (id)
        {
            case path_id::plugin1:
            case path_id::plugin2:
            case path_id::gamecfg:
            case path_id::scores:
            case path_id::openloco_yml:
            case path_id::save:
            case path_id::autosave:
                return platform::getUserDirectory();
            case path_id::language_files:
#if defined(__APPLE__) && defined(__MACH__)
                return platform::GetBundlePath();
#else
                return platform::GetCurrentExecutablePath().parent_path() / "data";
#endif
            default:
                return getLocoInstallPath();
        }
    }

    static fs::path getSubPath(path_id id)
    {
        static constexpr const char* paths[] = {
            "Data/g1.DAT",
            "plugin.dat",
            "plugin2.dat",
            "Data/CSS1.DAT",
            "Data/CSS2.DAT",
            "Data/CSS3.DAT",
            "Data/CSS4.DAT",
            "Data/CSS5.DAT",
            "game.cfg",
            "Data/KANJI.DAT",
            "Data/20s1.DAT",
            "Data/20s2.DAT",
            "Data/20s4.DAT",
            "Data/50s1.DAT",
            "Data/50s2.DAT",
            "Data/70s1.DAT",
            "Data/70s2.DAT",
            "Data/70s3.DAT",
            "Data/80s1.DAT",
            "Data/90s1.DAT",
            "Data/90s2.DAT",
            "Data/rag3.DAT",
            "Data/Chrysanthemum.DAT",
            "Data/Eugenia.DAT",
            "Data/Rag2.DAT",
            "Data/Rag1.DAT",
            "Data/20s3.DAT",
            "Data/40s1.DAT",
            "Data/40s2.DAT",
            "Data/50s3.DAT",
            "Data/40s3.DAT",
            "Data/80s2.DAT",
            "Data/60s1.DAT",
            "Data/80s3.DAT",
            "Data/60s2.DAT",
            "Data/60s3.DAT",
            "Data/80s4.DAT",
            "Data/20s5.DAT",
            "Data/20s6.DAT",
            "Data/title.dat",
            "scores.dat",
            "Scenarios/Boulder Breakers.SC5",
            "Data/TUT1024_1.DAT",
            "Data/TUT1024_2.DAT",
            "Data/TUT1024_3.DAT",
            "Data/TUT800_1.DAT",
            "Data/TUT800_2.DAT",
            "Data/TUT800_3.DAT",
            "openloco.yml",
            "language",
            "save",
            "save/autosave",
            "1.TMP",
        };

        size_t index = (size_t)id;
        if (index >= Utility::length(paths))
        {
            throw std::runtime_error("Invalid path_id: " + std::to_string((int32_t)id));
        }
        return paths[(size_t)id];
    }
}
