#include "Scenario.h"
#include "Audio/Audio.h"
#include "Config.h"
#include "Date.h"
#include "Economy/Economy.h"
#include "Entities/EntityManager.h"
#include "Environment.h"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "GameException.hpp"
#include "GameState.h"
#include "GameStateFlags.h"
#include "Graphics/Gfx.h"
#include "Graphics/PaletteMap.h"
#include "Gui.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Map/AnimationManager.h"
#include "Map/MapGenerator/MapGenerator.h"
#include "Map/TileManager.h"
#include "Map/WaveManager.h"
#include "MessageManager.h"
#include "MultiPlayer.h"
#include "Objects/CargoObject.h"
#include "Objects/ClimateObject.h"
#include "Objects/ObjectIndex.h"
#include "Objects/ObjectManager.h"
#include "Objects/ScenarioTextObject.h"
#include "OpenLoco.h"
#include "S5/S5.h"
#include "ScenarioConstruction.h"
#include "ScenarioManager.h"
#include "ScenarioObjective.h"
#include "ScenarioOptions.h"
#include "SceneManager.h"
#include "Title.h"
#include "Ui/WindowManager.h"
#include "Ui/Windows/Construction/Construction.h"
#include "Vehicles/OrderManager.h"
#include "Vehicles/RoutingManager.h"
#include "World/CompanyManager.h"
#include "World/CompanyRecords.h"
#include "World/IndustryManager.h"
#include "World/StationManager.h"
#include "World/TownManager.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <OpenLoco/Platform/Platform.h>

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;
using namespace OpenLoco::Ui;
using namespace OpenLoco::Literals;

namespace OpenLoco::Scenario
{
    // 0x0046115C
    void sub_46115C()
    {
        Game::setFlags(GameStateFlags::none);
        AnimationManager::reset();
        getGameState().fixFlags = enumValue(S5::S5FixFlags::fixFlag0 | S5::S5FixFlags::fixFlag1);
    }

    Season nextSeason(Season season)
    {
        switch (season)
        {
            case Season::autumn:
                return Season::winter;
            case Season::winter:
                return Season::spring;
            case Season::spring:
                return Season::summer;
            case Season::summer:
            default:
                return Season::autumn;
        }
    }

    // 0x00496A18, 0x00496A84 (adapted)
    void updateSeason(int32_t currentDayOfYear, const ClimateObject* climateObj)
    {
        Season season = static_cast<Season>(climateObj->firstSeason);

        int32_t dayOffset = currentDayOfYear;
        for (size_t i = 0; i < std::size(climateObj->seasonLength); i++)
        {
            dayOffset -= climateObj->seasonLength[i];
            if (dayOffset < 0)
            {
                break;
            }

            season = nextSeason(season);
        }

        setCurrentSeason(season);
    }

    // 0x00496A18
    void initialiseSnowLine()
    {
        auto today = calcDate(getCurrentDay());
        int32_t currentDayOfYear = today.dayOfYear;

        auto* climateObj = ObjectManager::get<ClimateObject>();
        if (climateObj == nullptr)
        {
            return;
        }

        updateSeason(currentDayOfYear, climateObj);

        if (getCurrentSeason() == Season::winter)
        {
            setCurrentSnowLine(climateObj->winterSnowLine);
        }
        else
        {
            setCurrentSnowLine(climateObj->summerSnowLine);
        }
    }

    // 0x00496A84
    void updateSnowLine(int32_t currentDayOfYear)
    {
        auto* climateObj = ObjectManager::get<ClimateObject>();
        if (climateObj == nullptr)
        {
            return;
        }

        updateSeason(currentDayOfYear, climateObj);

        if (getCurrentSeason() == Season::winter)
        {
            if (getCurrentSnowLine() != climateObj->winterSnowLine)
            {
                setCurrentSnowLine(getCurrentSnowLine() - 1);
            }
        }
        else
        {
            if (getCurrentSnowLine() != climateObj->summerSnowLine)
            {
                setCurrentSnowLine(getCurrentSnowLine() + 1);
            }
        }
    }

    // 0x00525FB4
    World::SmallZ getCurrentSnowLine()
    {
        return getGameState().currentSnowLine;
    }
    void setCurrentSnowLine(World::SmallZ snowline)
    {
        getGameState().currentSnowLine = snowline;
    }

    // 0x00525FB5
    Season getCurrentSeason()
    {
        return getGameState().currentSeason;
    }
    void setCurrentSeason(Season season)
    {
        getGameState().currentSeason = season;
    }

    // 0x0043C8FD
    static void sub_43C8FD()
    {
        auto& gameState = getGameState();
        gameState.currentSnowLine = 0xFF;
        sub_4969E0(1);
    }

    // 0x004969E0
    void sub_4969E0(uint8_t al)
    {
        auto& gameState = getGameState();
        gameState.var_B94C = al;
        gameState.var_B950 = 1;
        gameState.var_B952 = 0;
        gameState.var_B954 = 0;
        gameState.var_B956 = 0;
        gameState.currentRainLevel = 0;
    }

    // 0x0043C88C
    void reset()
    {
        WindowManager::closeConstructionWindows();

        GameCommands::setUpdatingCompanyId(CompanyId::neutral);
        WindowManager::setCurrentRotation(0);

        CompanyManager::reset();
        StringManager::reset();
        EntityManager::reset();

        Ui::Windows::Construction::Construction::reset();
        sub_46115C();
        WaveManager::reset();

        initialiseDate(1900);
        initialiseSnowLine();
        resetRoadObjects();
        TownManager::reset();
        IndustryManager::reset();
        StationManager::reset();

        Vehicles::RoutingManager::resetRoutingTable();
        Vehicles::OrderManager::reset();
        Ui::Windows::Terraform::resetLastSelections();
        sub_43C8FD();
        MessageManager::reset();
    }

    // 0x004748D4
    // ?Set default types for building. Like the initial track type and vehicle type and such.?
    void sub_4748D4()
    {
        ObjectManager::updateRoadObjectIdFlags();
        ObjectManager::updateDefaultLevelCrossingType();
        ObjectManager::updateLastTrackTypeOption();
        CompanyManager::updatePlayerInfrastructureOptions();
        Gfx::invalidateScreen();
        ObjectManager::sub_4748FA();
        Gfx::loadCurrency();
    }

    // 0x0043EDAD
    void eraseLandscape()
    {
        Scenario::getOptions().scenarioFlags &= ~(ScenarioFlags::landscapeGenerationDone);
        Ui::WindowManager::invalidate(Ui::WindowType::landscapeGeneration, 0);
        reset();
        Scenario::getOptions().madeAnyChanges = 0;
        addr<0x00F25374, uint8_t>() = 0;
        Gfx::invalidateScreen();
    }

    // 0x0043C90C
    void generateLandscape()
    {
        auto& options = Scenario::getOptions();
        MapGenerator::generate(options);
        options.madeAnyChanges = 0;
        addr<0x00F25374, uint8_t>() = 0;
    }

    // 0x0049685C
    void initialiseDate(uint16_t year)
    {
        initialiseDate(year, MonthId::january, 1);
    }

    void initialiseDate(uint16_t year, MonthId month, uint8_t day)
    {
        // NB: this base value was already 1800 in Locomotion.
        auto date = Date(year, month, day);
        uint32_t dayCount = calcDays(date);

        setCurrentDay(dayCount);
        setDate(date);

        setDayProgression(0);

        ScenarioManager::setScenarioTicks(0);
        Ui::WindowManager::setVehiclePreviewRotationFrame(0);
        setCurrentSeason(Season::winter);

        CompanyManager::determineAvailableVehicles();

        Economy::setInflationForYear(getCurrentYear());
    }

    // 0x00442837
    bool load(const fs::path& path)
    {
        WindowManager::closeConstructionWindows();
        WindowManager::closeAllFloatingWindows();

        // Resolve filenames to base game scenario directory
        fs::path fullPath = path;
        if (!fullPath.has_root_path())
        {
            auto scenarioPath = Environment::getPath(Environment::PathId::scenarios);
            fullPath = scenarioPath / path;
        }

        Audio::pauseSound();

        auto result = S5::importSaveToGameState(fullPath, S5::LoadFlags::scenario);
        Audio::unpauseSound();
        return result;
    }

    void start()
    {
        WindowManager::closeConstructionWindows();
        WindowManager::closeAllFloatingWindows();

        SceneManager::removeSceneFlags(SceneManager::Flags::title);
        initialiseViewports();
        Gui::init();
        Audio::resetMusic();

        auto& gameState = getGameState();
        if ((gameState.flags & GameStateFlags::tileManagerLoaded) != GameStateFlags::none)
        {
            auto mainWindow = WindowManager::getMainWindow();
            if (mainWindow != nullptr)
            {
                SavedViewSimple savedView;
                savedView.viewX = gameState.savedViewX;
                savedView.viewY = gameState.savedViewY;
                savedView.zoomLevel = static_cast<ZoomLevel>(gameState.savedViewZoom);
                savedView.rotation = gameState.savedViewRotation;
                mainWindow->viewportFromSavedView(savedView);
                mainWindow->invalidate();
            }
        }
        else
        {
            generateLandscape();
            WindowManager::setCurrentRotation(0);
        }

        EntityManager::updateSpatialIndex();
        addr<0x0052334E, uint16_t>() = 0; // _thousandthTickCounter
        Ui::Windows::Terraform::resetLastSelections();
        MessageManager::reset();

        std::memcpy(gameState.scenarioDetails, Scenario::getOptions().scenarioDetails, sizeof(gameState.scenarioDetails));
        std::memcpy(gameState.scenarioName, Scenario::getOptions().scenarioName, sizeof(gameState.scenarioName));

        const auto* stexObj = ObjectManager::get<ScenarioTextObject>();
        if (stexObj != nullptr)
        {
            char buffer[256];
            StringManager::formatString(buffer, sizeof(buffer), stexObj->details);
            std::strncpy(gameState.scenarioDetails, buffer, sizeof(gameState.scenarioDetails));
        }

        auto savePath = Environment::getPath(Environment::PathId::save);
        savePath /= std::string(Scenario::getOptions().scenarioName) + S5::extensionSV5;
        Game::setActiveSavePath(savePath.u8string());

        loadPreferredCurrencyNewGame();
        Gfx::loadCurrency();
        CompanyManager::reset();
        CompanyManager::createPlayerCompany();
        initialiseDate(Scenario::getOptions().scenarioStartYear);
        initialiseSnowLine();
        sub_4748D4();

        CompanyManager::setRecords(CompanyManager::kZeroRecords);
        getObjectiveProgress().timeLimitUntilYear = getObjective().timeLimitYears - 1 + gameState.currentYear;
        getObjectiveProgress().monthsInChallenge = 0;
        initialiseDefaultTrackRoadMods();
        gameState.lastMapWindowAttributes.flags = WindowFlags::none;

        TownManager::updateLabels();
        StationManager::updateLabels();
        Gfx::loadDefaultPalette();
        Gfx::invalidateScreen();
        SceneManager::resetSceneAge();
        MultiPlayer::setFlag(MultiPlayer::flags::flag_10);
        throw GameException::Interrupt;
    }

    // 0x0044400C
    bool loadAndStart(const fs::path& path)
    {
        // Set new rng seed based on current rng and current time
        // TODO do we really need to use the current rng? seems unnecessary, keeping with vanilla logic for now
        auto& gameState = getGameState();
        auto oldRng = gameState.rng;

        if (!load(path))
        {
            return false;
        }

        gameState.rng = Core::Prng(Platform::getTime() ^ oldRng.srand_0(), oldRng.srand_1());
        std::strncpy(gameState.scenarioFileName, path.u8string().c_str(), std::size(gameState.scenarioFileName) - 1);
        start();
    }

    // this will prepare _commonFormatArgs array before drawing the StringIds::challenge_value
    // after that for example it will draw this string: Achieve a performance index of 10.0% ("Engineer")
    // Note: no input and output parameters are in the original assembly, update is done in the memory
    // in this implementation we return FormatArguments so in the future it will be not depending on global variables
    // 0x004384E9
    void formatChallengeArguments(const Objective& objective, const ObjectiveProgress& progress, FormatArguments& args)
    {
        switch (objective.type)
        {
            case Scenario::ObjectiveType::companyValue:
                args.push(StringIds::achieve_a_company_value_of);
                args.push(objective.companyValue);
                break;

            case Scenario::ObjectiveType::vehicleProfit:
                args.push(StringIds::achieve_a_monthly_profit_from_vehicles_of);
                args.push(objective.monthlyVehicleProfit);
                break;

            case Scenario::ObjectiveType::performanceIndex:
            {
                args.push(StringIds::achieve_a_performance_index_of);
                int16_t performanceIndex = objective.performanceIndex * 10;
                formatPerformanceIndex(performanceIndex, args);
                break;
            }

            case Scenario::ObjectiveType::cargoDelivery:
            {
                args.push(StringIds::deliver);
                const CargoObject* cargoObject = reinterpret_cast<CargoObject*>(ObjectManager::getTemporaryObject());
                if (objective.deliveredCargoType != 0xFF)
                {
                    cargoObject = ObjectManager::get<CargoObject>(objective.deliveredCargoType);
                }
                if (cargoObject == nullptr)
                {
                    args.push(StringIds::empty);
                }
                else
                {
                    args.push(cargoObject->unitNamePlural);
                }
                args.push(objective.deliveredCargoAmount);
                break;
            }
        }

        if ((objective.flags & ObjectiveFlags::beTopCompany) != ObjectiveFlags::none)
        {
            args.push(StringIds::and_be_the_top_performing_company);
        }
        if ((objective.flags & ObjectiveFlags::beWithinTopThreeCompanies) != ObjectiveFlags::none)
        {
            args.push(StringIds::and_be_one_of_the_top_3_performing_companies);
        }
        if ((objective.flags & ObjectiveFlags::withinTimeLimit) != ObjectiveFlags::none)
        {
            if (SceneManager::isTitleMode() || SceneManager::isEditorMode())
            {
                args.push(StringIds::within_years);
                args.push<uint16_t>(objective.timeLimitYears);
            }
            else
            {
                args.push(StringIds::by_the_end_of);
                args.push(progress.timeLimitUntilYear);
            }
        }

        args.push<uint16_t>(0);
        args.push<uint16_t>(0);
    }

    static loco_global<ObjectManager::SelectedObjectsFlags*, 0x50D144> _inUseobjectSelection;
    static loco_global<ObjectManager::ObjectSelectionMeta, 0x0112C1C5> _objectSelectionMeta;

    static std::span<ObjectManager::SelectedObjectsFlags> getInUseSelectedObjectFlags()
    {
        return std::span<ObjectManager::SelectedObjectsFlags>(*_inUseobjectSelection, ObjectManager::getNumInstalledObjects());
    }

    static void loadPreferredCurrency()
    {
        const auto& preferredCurreny = Config::get().preferredCurrency;

        if (preferredCurreny.isEmpty())
        {
            return;
        }

        ObjectManager::prepareSelectionList(true);
        const auto oldCurrency = ObjectManager::getActiveObject(ObjectType::currency, getInUseSelectedObjectFlags());
        if (oldCurrency.index != ObjectManager::kNullObjectIndex)
        {
            if (oldCurrency.object._header == preferredCurreny)
            {
                ObjectManager::freeSelectionList();
                return;
            }
            ObjectManager::selectObjectFromIndex(ObjectManager::SelectObjectModes::defaultDeselect, oldCurrency.object._header, getInUseSelectedObjectFlags(), _objectSelectionMeta);
        }
        if (!ObjectManager::selectObjectFromIndex(ObjectManager::SelectObjectModes::defaultSelect, preferredCurreny, getInUseSelectedObjectFlags(), _objectSelectionMeta))
        {
            // Failed so reselect the old currency and give up
            ObjectManager::selectObjectFromIndex(ObjectManager::SelectObjectModes::defaultSelect, oldCurrency.object._header, getInUseSelectedObjectFlags(), _objectSelectionMeta);
            ObjectManager::freeSelectionList();
            return;
        }
        ObjectManager::unloadUnselectedSelectionListObjects(getInUseSelectedObjectFlags());
        ObjectManager::loadSelectionListObjects(getInUseSelectedObjectFlags());
        ObjectManager::reloadAll();
        Gfx::loadCurrency();
        ObjectManager::freeSelectionList();
    }

    // 0x004C153B
    void loadPreferredCurrencyAlways()
    {
        if (!Config::get().usePreferredCurrencyAlways)
        {
            return;
        }

        loadPreferredCurrency();
    }

    // 0x004C159C
    void loadPreferredCurrencyNewGame()
    {
        if (!Config::get().usePreferredCurrencyForNewGames)
        {
            loadPreferredCurrencyAlways();
            return;
        }

        loadPreferredCurrency();
    }
}
