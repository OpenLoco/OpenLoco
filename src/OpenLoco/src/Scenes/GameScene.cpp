#include "Scenes/GameScene.h"
#include "Audio/Audio.h"
#include "Config.h"
#include "Date.h"
#include "Economy/Economy.h"
#include "Effects/EffectsManager.h"
#include "Environment.h"
#include "Game.h"
#include "GameState.h"
#include "GameStateFlags.h"
#include "Localisation/StringIds.h"
#include "Logging.h"
#include "Map/AnimationManager.h"
#include "Map/TileManager.h"
#include "Map/WaveManager.h"
#include "MessageManager.h"
#include "Network/Network.h"
#include "Objects/ObjectManager.h"
#include "OpenLoco.h"
#include "Random.h"
#include "S5/S5.h"
#include "Scenario/Scenario.h"
#include "Scenario/ScenarioManager.h"
#include "Scenario/ScenarioOptions.h"
#include "SceneManager.h"
#include "Ui/WindowManager.h"
#include "Vehicles/VehicleManager.h"
#include "World/CompanyManager.h"
#include "World/IndustryManager.h"
#include "World/StationManager.h"
#include "World/TownManager.h"
#include <OpenLoco/Utility/String.hpp>
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <vector>

using namespace OpenLoco::Diagnostics;
using namespace OpenLoco::Input;
using namespace OpenLoco::Ui;

namespace OpenLoco::Scenes::GameScene
{
    static int32_t _monthsSinceLastAutosave;

    static void tickDate();

    void autosaveReset()
    {
        _monthsSinceLastAutosave = 0;
    }

    static void autosaveClean()
    {
        try
        {
            auto autosaveDirectory = Environment::getPath(Environment::PathId::autosave);
            if (fs::is_directory(autosaveDirectory))
            {
                std::vector<fs::path> autosaveFiles;

                // Collect all the autosave files
                for (auto& f : fs::directory_iterator(autosaveDirectory))
                {
                    if (f.is_regular_file())
                    {
                        auto& path = f.path();
                        auto filename = path.filename().u8string();
                        if (Utility::startsWith(filename, "autosave_") && Utility::endsWith(filename, S5::extensionSV5, true))
                        {
                            autosaveFiles.push_back(path);
                        }
                    }
                }

                auto amountToKeep = static_cast<size_t>(std::max(1, Config::get().autosaveAmount));
                if (autosaveFiles.size() > amountToKeep)
                {
                    // Sort them by name (which should correspond to date order)
                    std::sort(autosaveFiles.begin(), autosaveFiles.end());

                    // Delete excess files
                    auto numToDelete = autosaveFiles.size() - amountToKeep;
                    for (size_t i = 0; i < numToDelete; i++)
                    {
                        auto path8 = autosaveFiles[i].u8string();
                        Logging::info("Deleting old autosave: {}", path8.c_str());
                        fs::remove(autosaveFiles[i]);
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            Logging::error("Unable to clean autosaves: {}", e.what());
        }
    }

    static void autosave()
    {
        // Format filename
        auto time = std::time(nullptr);
        auto localTime = std::localtime(&time);
        char filename[64];
        snprintf(
            filename,
            sizeof(filename),
            "autosave_%04u-%02u-%02u_%02u-%02u-%02u%s",
            localTime->tm_year + 1900,
            localTime->tm_mon + 1,
            localTime->tm_mday,
            localTime->tm_hour,
            localTime->tm_min,
            localTime->tm_sec,
            S5::extensionSV5);

        try
        {
            auto autosaveDirectory = Environment::getPath(Environment::PathId::autosave);
            Environment::autoCreateDirectory(autosaveDirectory);

            auto autosaveFullPath = autosaveDirectory / filename;

            auto autosaveFullPath8 = autosaveFullPath.u8string();
            Logging::info("Autosaving game to {}", autosaveFullPath8.c_str());
            S5::exportGameStateToFile(autosaveFullPath, S5::SaveFlags::isAutosave | S5::SaveFlags::noWindowClose);
        }
        catch (const std::exception& e)
        {
            Logging::error("Unable to autosave game: {}", e.what());
        }
    }

    static void autosaveCheck()
    {
        _monthsSinceLastAutosave++;

        if (!SceneManager::isTitleMode())
        {
            auto freq = Config::get().autosaveFrequency;
            if (freq > 0 && _monthsSinceLastAutosave >= freq)
            {
                autosave();
                autosaveClean();
                autosaveReset();
            }
        }
    }

    // 0x004968C7
    static void tickDate()
    {
        if (Game::hasFlags(GameStateFlags::tileManagerLoaded) && !SceneManager::isEditorMode())
        {
            if (updateDayCounter())
            {
                StationManager::updateDaily();
                VehicleManager::updateDaily();
                IndustryManager::updateDaily();
                MessageManager::updateDaily();
                WindowManager::updateDaily();

                auto yesterday = calcDate(getCurrentDay() - 1);
                auto today = calcDate(getCurrentDay());
                setDate(today);
                Scenario::updateSnowLine(today.dayOfYear);
                WindowManager::invalidate(WindowType::timeToolbar);

                if (today.month != yesterday.month)
                {
                    // End of every month
                    Scenario::getObjectiveProgress().monthsInChallenge++;
                    TownManager::updateMonthly();
                    IndustryManager::updateMonthly();
                    CompanyManager::updateMonthly1();
                    CompanyManager::updateMonthlyHeadquarters();
                    VehicleManager::updateMonthly();

                    if (today.year <= 2029)
                    {
                        Economy::updateMonthly();
                    }

                    // clang-format off
                    if (today.month == MonthId::january ||
                        today.month == MonthId::april ||
                        today.month == MonthId::july ||
                        today.month == MonthId::october)
                    // clang-format on
                    {
                        CompanyManager::updateQuarterly();
                    }

                    if (today.year != yesterday.year)
                    {
                        // End of every year
                        CompanyManager::updateYearly();
                        ObjectManager::updateDefaultLevelCrossingType();
                        ObjectManager::updateYearly2();
                        World::TileManager::updateYearly();
                    }

                    autosaveCheck();
                }

                CompanyManager::updateDaily();
            }
        }
    }

    // 0x0046ABCB
    void tick()
    {
        if (!Network::shouldProcessTick(ScenarioManager::getScenarioTicks() + 1))
        {
            return;
        }

        ScenarioManager::setScenarioTicks(ScenarioManager::getScenarioTicks() + 1);
        ScenarioManager::setScenarioTicks2(ScenarioManager::getScenarioTicks2() + 1);
        Network::processGameCommands(ScenarioManager::getScenarioTicks());

        recordTickStartPrng();
        World::TileManager::defragmentTilePeriodic();

        // Back up the `madeAnyChanges` variable to ensure we only capture user changes
        bool userMadeAnyChanges = Scenario::getOptions().madeAnyChanges;

        tickDate();
        World::TileManager::tick();
        World::WaveManager::tick();
        TownManager::tick();
        IndustryManager::tick();
        VehicleManager::tick();
        StationManager::tick();
        EffectsManager::tick();
        CompanyManager::tick();
        World::AnimationManager::tick();
        Audio::tick();

        Scenario::getOptions().madeAnyChanges = userMadeAnyChanges;

        auto& lastLoadError = S5::getLastLoadError();
        if (lastLoadError.errorCode != 0)
        {
            if (lastLoadError.errorCode != -3)
            {
                StringId title = lastLoadError.errorMessage;
                StringId message = StringIds::null;
                Ui::Windows::Error::open(title, message);
            }
            else
            {
                Ui::Windows::ObjectLoadError::open(lastLoadError.objectList);
            }
            S5::resetLastLoadError();
        }
    }

    void tickInterface()
    {
        getGameState().var_014A++;

        Audio::playBackgroundMusic();

        sub_431695(getNumFrameUpdates());
    }
}
