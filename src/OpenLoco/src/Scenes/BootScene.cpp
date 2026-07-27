#include "Scenes/BootScene.h"
#include "CommandLine.h"
#include "Graphics/Gfx.h"
#include "Logging.h"
#include "Network/Network.h"
#include "S5/S5.h"
#include "Scenario/Scenario.h"
#include "SceneManager.h"
#include <OpenLoco/Utility/String.hpp>

using namespace OpenLoco::Diagnostics;

namespace OpenLoco::Scenes::BootScene
{
    static bool _introStarted;

    bool loadFile(const fs::path& path)
    {
        auto extension = path.extension().u8string();
        if (Utility::iequals(extension, S5::extensionSC5))
        {
            return Scenario::loadAndStart(path);
        }
        else
        {
            SceneManager::requestSceneLoad(SceneManager::SceneId::gameplay, path, S5::LoadFlags::none);
            return true;
        }
    }

    static bool launchGameFromCmdLineOptions()
    {
        const auto& cmdLineOptions = getCommandLineOptions();
        try
        {
            if (cmdLineOptions.action == CommandLineAction::host)
            {
                Network::openServer();
                return loadFile(fs::u8path(cmdLineOptions.path));
            }
            else if (cmdLineOptions.action == CommandLineAction::join)
            {
                if (cmdLineOptions.port)
                {
                    return Network::joinServer(cmdLineOptions.address, *cmdLineOptions.port);
                }
                else
                {
                    return Network::joinServer(cmdLineOptions.address);
                }
            }
            else if (!cmdLineOptions.path.empty())
            {
                return loadFile(fs::u8path(cmdLineOptions.path));
            }
        }
        catch (const std::exception& e)
        {
            Logging::error("Unable to load park: {}", e.what());
        }
        return false;
    }

    void tick()
    {
        // Gfx::initialise() loads the palette before the window exists, so it has to be
        // applied again here before any scene renders.
        Gfx::loadDefaultPalette();
        Gfx::invalidateScreen();

        SceneManager::addSceneFlags(SceneManager::Flags::initialised);

        if (!_introStarted && getCommandLineOptions().action == CommandLineAction::intro)
        {
            _introStarted = true;
            SceneManager::requestScene(SceneManager::SceneId::intro);
            return;
        }

        if (!launchGameFromCmdLineOptions())
        {
            SceneManager::requestScene(SceneManager::SceneId::title);
        }
    }

    void tickInterface()
    {
    }

    void update()
    {
    }
}
