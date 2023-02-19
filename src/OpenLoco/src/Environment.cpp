#include "Environment.h"
#include "Config.h"
#include "Ui.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <OpenLoco/Platform/Platform.h>
#include <OpenLoco/Utility/Collection.hpp>
#include <OpenLoco/Utility/String.hpp>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace OpenLoco::Interop;

namespace OpenLoco::Environment
{
    loco_global<char[260], 0x009D118E> _pathBuffer;
    loco_global<char[257], 0x0050B0CE> _pathInstall;
    loco_global<char[257], 0x0050B1CF> _pathSavesSinglePlayer;
    loco_global<char[257], 0x0050B2EC> _pathSavesTwoPlayer;
    loco_global<char[257], 0x0050B406> _pathScenarios;
    loco_global<char[257], 0x0050B518> _pathLandscapes;
    loco_global<char[257], 0x0050B635> _pathObjects;

    static fs::path getBasePath(PathId id);
    static fs::path getSubPath(PathId id);
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
            auto g1Path = path / getSubPath(PathId::g1);
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
        static constexpr const char* kSearchPaths[] = {
            "C:/Program Files (x86)/Atari/Locomotion",
            "C:/GOG Games/Chris Sawyer's Locomotion",
            "C:/GOG Games/Locomotion",
        };

        std::cout << "Searching for Locomotion install path..." << std::endl;
        for (auto path : kSearchPaths)
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
        autoCreateDirectory(Platform::getUserDirectory());
        auto& cfg = Config::get();

        // Validate any existing configured install path first
        const auto u8InstallPath = std::u8string(cfg.locoInstallPath.cbegin(), cfg.locoInstallPath.cend());
        auto path = fs::path(u8InstallPath);
        if (!path.empty())
        {
            if (validateLocoInstallPath(path))
            {
                Config::write();
                return path;
            }
            std::cerr << "Configured Locomotion game folder is missing Data/g1.DAT." << std::endl;
        }

        // Try to detect the Locomotion install path
        path = autoDetectLocoInstallPath();
        if (!path.empty())
        {
            const auto installPath = path.make_preferred().u8string();
            cfg.locoInstallPath = std::string(installPath.cbegin(), installPath.cend());
            Config::write();
            return path;
        }

        std::cerr << "Unable to automatically find the Locomotion game folder." << std::endl
                  << "Please provide the location manually." << std::endl;
        Ui::showMessageBox("OpenLoco", "Unable to automatically detect the Locomotion game folder.\n"
                                       "Please locate the Locomotion game folder manually.");

        // Let user manually specify the Locomotion install folder, and verify files are present
        //
        // FIXME: Instead of passing the hwnd we should have a function to bring the window to the front after
        // this call.
        path = Platform::promptDirectory("Locate original Locomotion game files", Ui::hwnd());
        if (validateLocoInstallPath(path))
        {
            const auto installPath = path.make_preferred().u8string();
            cfg.locoInstallPath = std::string(installPath.cbegin(), installPath.cend());
            Config::write();
            return path;
        }

        // Files could not be located -- bail with a helpful error message
        std::cerr << "The selected folder does not contain Data/g1.DAT" << std::endl;
        Ui::showMessageBox("OpenLoco", "The selected folder does not contain Data/g1.DAT, suggesting it is not a Locomotion\n"
                                       "install folder. Please verify the folder contains the original Locomotion game files,\n"
                                       "then restart OpenLoco to try again.");

        std::exit(-1);
    }

    static fs::path getLocoInstallPath()
    {
        const auto len = strnlen(_pathInstall.get(), std::size(_pathInstall));
        const auto u8InstallPath = std::u8string(_pathInstall.get(), _pathInstall.get() + len);
        return fs::path(u8InstallPath);
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
    fs::path getPath(PathId id)
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

    fs::path getPathNoWarning(PathId id)
    {
        auto basePath = getBasePath(id);
        auto subPath = getSubPath(id);
        auto result = (basePath / subPath).lexically_normal();
        return result;
    }

    template<typename T>
    static void setDirectory(T& buffer, fs::path path)
    {
        auto path8 = path.make_preferred().u8string();
        const auto path8s = std::string(path8.cbegin(), path8.cend());
        Utility::strlcpy(buffer, path8s.c_str(), buffer.size());
    }

    void autoCreateDirectory(const fs::path& path)
    {
        try
        {
            if (!fs::is_directory(path))
            {
                auto path8 = path.u8string();
                const auto path8s = std::string(path8.cbegin(), path8.cend());
                std::printf("Creating directory: %s\n", path8s.c_str());
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
            Ui::showMessageBox("Unable to create directory", e.what());
        }
    }

    // 0x004412CE
    void resolvePaths()
    {
        auto basePath = resolveLocoInstallPath();
        setDirectory(_pathInstall, basePath);

        // NB: vanilla routines do not use std::filesystem yet, so the trailing slash is still needed.
        auto saveDirectory = getPathNoWarning(PathId::save) / "";
        auto& configLastSavePath = Config::get().lastSavePath;
        if (!configLastSavePath.empty())
        {
            // Getting the directory can fail if config is bad.
            try
            {
                const auto lastSavePath = std::u8string(configLastSavePath.cbegin(), configLastSavePath.cend());
                auto directory = fs::path(lastSavePath);
                if (fs::is_directory(directory))
                {
                    saveDirectory = directory;
                }
            }
            catch (std::system_error&)
            {
            }
        }
        setDirectory(_pathSavesSinglePlayer, saveDirectory);
        setDirectory(_pathSavesTwoPlayer, saveDirectory);
        autoCreateDirectory(saveDirectory);

        setDirectory(_pathScenarios, basePath / "Scenarios/*.SC5");
        setDirectory(_pathLandscapes, basePath / "Scenarios/Landscapes/*.SC5");
        setDirectory(_pathObjects, basePath / "ObjData/*.DAT");
    }

    static fs::path getBasePath(PathId id)
    {
        switch (id)
        {
            case PathId::plugin1:
            case PathId::plugin2:
            case PathId::gamecfg:
            case PathId::scores:
            case PathId::openlocoYML:
            case PathId::save:
            case PathId::autosave:
                return Platform::getUserDirectory();
            case PathId::languageFiles:
#if defined(__APPLE__) && defined(__MACH__)
                return Platform::GetBundlePath();
#else
                return Platform::getCurrentExecutablePath().parent_path() / "data";
#endif
            default:
                return getLocoInstallPath();
        }
    }

    static fs::path getSubPath(PathId id)
    {
        static constexpr const char* kPaths[] = {
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
            "ObjData",
            "Scenarios",
        };

        size_t index = (size_t)id;
        if (index >= Utility::length(kPaths))
        {
            throw std::runtime_error("Invalid PathId: " + std::to_string((int32_t)id));
        }
        return kPaths[(size_t)id];
    }
}
