#include "Scenario.h"
#include "Audio/Audio.h"
#include "CompanyManager.h"
#include "Date.h"
#include "Economy/Economy.h"
#include "Entities/EntityManager.h"
#include "Environment.h"
#include "Game.h"
#include "GameException.hpp"
#include "GameState.h"
#include "Graphics/Gfx.h"
#include "Gui.h"
#include "IndustryManager.h"
#include "Interop/Interop.hpp"
#include "Localisation/StringIds.h"
#include "Map/AnimationManager.h"
#include "Map/MapGenerator.h"
#include "Map/TileManager.h"
#include "Map/WaveManager.h"
#include "MultiPlayer.h"
#include "Objects/CargoObject.h"
#include "Objects/ClimateObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/ScenarioTextObject.h"
#include "Platform/Platform.h"
#include "S5/S5.h"
#include "StationManager.h"
#include "Title.h"
#include "TownManager.h"
#include "Ui/WindowManager.h"
#include "Windows/Construction/Construction.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;
using namespace OpenLoco::Ui;
using namespace OpenLoco::Literals;

namespace OpenLoco::Scenario
{
    static loco_global<CargoObject*, 0x0050D15C> _50D15C;

    static loco_global<uint32_t, 0x00525F5E> _scenario_ticks;

    static loco_global<uint8_t, 0x00525FB4> _currentSnowLine;
    static loco_global<Season, 0x00525FB5> _currentSeason;

    static loco_global<uint16_t, 0x0052622E> _52622E; // tick-related?

    static loco_global<ObjectiveType, 0x00526230> objectiveType;
    static loco_global<uint8_t, 0x00526231> objectiveFlags;
    static loco_global<uint32_t, 0x00526232> objectiveCompanyValue;
    static loco_global<uint32_t, 0x00526236> objectiveMonthlyVehicleProfit;
    static loco_global<uint8_t, 0x0052623A> objectivePerformanceIndex;
    static loco_global<uint8_t, 0x0052623B> objectiveDeliveredCargoType;
    static loco_global<uint32_t, 0x0052623C> objectiveDeliveredCargoAmount;
    static loco_global<uint8_t, 0x00526240> objectiveTimeLimitYears;
    static loco_global<uint16_t, 0x00526241> objectiveTimeLimitUntilYear;

    static loco_global<char[256], 0x0050B745> _currentScenarioFilename;
    static loco_global<uint16_t, 0x0050C19A> _50C19A;

    // 0x0046115C
    void sub_46115C()
    {
        Game::setFlags(0u);
        AnimationManager::reset();
        addr<0x0052624C, uint16_t>() = S5::S5FixFlags::fixFlag0 | S5::S5FixFlags::fixFlag1;
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
    static void updateSeason(int32_t currentDayOfYear, const ClimateObject* climateObj)
    {
        Season season = static_cast<Season>(climateObj->firstSeason);

        int32_t dayOffset = currentDayOfYear;
        for (size_t i = 0; i < std::size(climateObj->seasonLength); i++)
        {
            dayOffset -= climateObj->seasonLength[i];
            if (dayOffset < 0)
                break;

            season = nextSeason(season);
        }

        _currentSeason = season;
    }

    // 0x00496A18
    void initialiseSnowLine()
    {
        auto today = calcDate(getCurrentDay());
        int32_t currentDayOfYear = today.dayOfOlympiad;

        auto* climateObj = ObjectManager::get<ClimateObject>();
        if (climateObj == nullptr)
            return;

        updateSeason(currentDayOfYear, climateObj);

        if (_currentSeason == Season::winter)
        {
            _currentSnowLine = climateObj->winterSnowLine;
        }
        else
        {
            _currentSnowLine = climateObj->summerSnowLine;
        }
    }

    // 0x00496A84
    void updateSnowLine(int32_t currentDayOfYear)
    {
        auto* climateObj = ObjectManager::get<ClimateObject>();
        if (climateObj == nullptr)
            return;

        updateSeason(currentDayOfYear, climateObj);

        if (_currentSeason == Season::winter)
        {
            if (_currentSnowLine != climateObj->winterSnowLine)
                _currentSnowLine--;
        }
        else
        {
            if (_currentSnowLine != climateObj->summerSnowLine)
                _currentSnowLine++;
        }
    }

    // 0x00475988
    static void sub_475988()
    {
        call(0x00475988);
    }

    // 0x004A8810
    static void sub_4A8810()
    {
        call(0x004A8810);
    }

    // TODO: Move to OrderManager::reset
    // 0x004702EC
    static void sub_4702EC()
    {
        call(0x004702EC);
    }

    // TODO: Move to Terraform::reset
    // 0x004BAEC4
    static void sub_4BAEC4()
    {
        call(0x004BAEC4);
    }

    // 0x0043C8FD
    static void sub_43C8FD()
    {
        call(0x0043C8FD);
    }

    // 0x0043C88C
    void reset()
    {
        WindowManager::closeConstructionWindows();

        CompanyManager::setUpdatingCompanyId(CompanyId::neutral);
        WindowManager::setCurrentRotation(0);

        CompanyManager::reset();
        StringManager::reset();
        EntityManager::reset();

        Ui::Windows::Construction::Construction::reset();
        sub_46115C();
        WaveManager::reset();

        initialiseDate(1900);
        initialiseSnowLine();
        sub_475988();
        TownManager::reset();
        IndustryManager::reset();
        StationManager::reset();

        sub_4A8810();
        sub_4702EC();
        sub_4BAEC4();
        sub_43C8FD();
        Title::sub_4284C8();
    }

    // 0x004748D4
    // ?Set default types for building. Like the initial track type and vehicle type and such.?
    void sub_4748D4()
    {
        call(0x004748D4);
    }

    // 0x0043EDAD
    void eraseLandscape()
    {
        S5::getOptions().scenarioFlags &= ~(Scenario::Flags::landscapeGenerationDone);
        Ui::WindowManager::invalidate(Ui::WindowType::landscapeGeneration, 0);
        reset();
        S5::getOptions().madeAnyChanges = 0;
        addr<0x00F25374, uint8_t>() = 0;
        Gfx::invalidateScreen();
    }

    // 0x0043C90C
    void generateLandscape()
    {
        auto& options = S5::getOptions();
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

        _scenario_ticks = 0;
        _52622E = 0;
        _currentSeason = Season::winter;

        CompanyManager::determineAvailableVehicles();

        Economy::sub_46E2C0(getCurrentYear());
    }

    void registerHooks()
    {
        registerHook(
            0x0043C88C,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                reset();
                regs = backup;
                return 0;
            });

        registerHook(
            0x0043C90C,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                generateLandscape();
                regs = backup;
                return 0;
            });

        registerHook(
            0x0049685C,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                initialiseDate(regs.ax);
                regs = backup;
                return 0;
            });
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
        static loco_global<char[512], 0x00112CE04> scenarioFilename;
        std::strncpy(&*scenarioFilename, fullPath.u8string().c_str(), std::size(scenarioFilename));
        auto flags = call(0x00442837);
        return (flags & Interop::X86_FLAG_CARRY) == 0;
    }

    void start()
    {
        WindowManager::closeConstructionWindows();
        WindowManager::closeAllFloatingWindows();

        clearScreenFlag(ScreenFlags::title);
        initialiseViewports();
        Gui::init();
        Audio::resetMusic();

        auto& gameState = getGameState();
        if (gameState.flags & Flags::landscapeGenerationDone)
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
        sub_4BAEC4();
        Title::sub_4284C8();

        std::memcpy(gameState.scenarioDetails, S5::getOptions().scenarioDetails, sizeof(gameState.scenarioDetails));
        std::memcpy(gameState.scenarioName, S5::getOptions().scenarioName, sizeof(gameState.scenarioName));

        const auto* stexObj = ObjectManager::get<ScenarioTextObject>();
        if (stexObj != nullptr)
        {
            char buffer[256];
            StringManager::formatString(buffer, sizeof(buffer), stexObj->details);
            std::strncpy(gameState.scenarioDetails, buffer, sizeof(gameState.scenarioDetails));
        }

        auto savePath = Environment::getPath(Environment::PathId::save);
        savePath /= std::string(S5::getOptions().scenarioName) + S5::extensionSV5;
        std::strncpy(_currentScenarioFilename, savePath.u8string().c_str(), std::size(_currentScenarioFilename));

        call(0x004C159C);
        call(0x0046E07B); // load currency gfx
        CompanyManager::reset();
        CompanyManager::createPlayerCompany();
        initialiseDate(S5::getOptions().scenarioStartYear);
        initialiseSnowLine();
        sub_4748D4();
        std::fill(std::begin(gameState.recordSpeed), std::end(gameState.recordSpeed), 0_mph);
        gameState.objectiveTimeLimitUntilYear = gameState.objectiveTimeLimitYears - 1 + gameState.currentYear;
        gameState.objectiveMonthsInChallenge = 0;
        call(0x0049B546);
        gameState.lastMapWindowFlags = 0;

        TownManager::updateLabels();
        StationManager::updateLabels();
        Gfx::loadPalette();
        Gfx::invalidateScreen();
        resetScreenAge();
        _50C19A = 62000;
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
            return false;

        gameState.rng = Utility::prng(Platform::getTime() ^ oldRng.srand_0(), oldRng.srand_1());
        std::strncpy(gameState.scenarioFileName, path.u8string().c_str(), std::size(gameState.scenarioFileName) - 1);
        start();
    }

    // this will prepare _commonFormatArgs array before drawing the StringIds::challenge_value
    // after that for example it will draw this string: Achieve a performance index of 10.0% ("Engineer")
    // Note: no input and output parameters are in the original assembly, update is done in the memory
    // in this implementation we return FormatArguments so in the future it will be not depending on global variables
    // 0x004384E9
    void formatChallengeArguments(FormatArguments& args)
    {
        switch (objectiveType)
        {
            case Scenario::ObjectiveType::companyValue:
                args.push(StringIds::achieve_a_company_value_of);
                args.push(*objectiveCompanyValue);
                break;

            case Scenario::ObjectiveType::vehicleProfit:
                args.push(StringIds::achieve_a_monthly_profit_from_vehicles_of);
                args.push(*objectiveMonthlyVehicleProfit);
                break;

            case Scenario::ObjectiveType::performanceIndex:
            {
                args.push(StringIds::achieve_a_performance_index_of);
                int16_t performanceIndex = objectivePerformanceIndex * 10;
                formatPerformanceIndex(performanceIndex, args);
                break;
            }

            case Scenario::ObjectiveType::cargoDelivery:
            {
                args.push(StringIds::deliver);
                CargoObject* cargoObject = _50D15C;
                if (objectiveDeliveredCargoType != 0xFF)
                {
                    cargoObject = ObjectManager::get<CargoObject>(objectiveDeliveredCargoType);
                }
                args.push(cargoObject->unit_name_plural);
                args.push(*objectiveDeliveredCargoAmount);
                break;
            }
        }

        if ((objectiveFlags & Scenario::ObjectiveFlags::beTopCompany) != 0)
        {
            args.push(StringIds::and_be_the_top_performing_company);
        }
        if ((objectiveFlags & Scenario::ObjectiveFlags::beWithinTopThreeCompanies) != 0)
        {
            args.push(StringIds::and_be_one_of_the_top_3_performing_companies);
        }
        if ((objectiveFlags & Scenario::ObjectiveFlags::withinTimeLimit) != 0)
        {
            if (isTitleMode() || isEditorMode())
            {
                args.push(StringIds::within_years);
                args.push<uint16_t>(*objectiveTimeLimitYears);
            }
            else
            {
                args.push(StringIds::by_the_end_of);
                args.push(*objectiveTimeLimitUntilYear);
            }
        }

        args.push<uint16_t>(0);
        args.push<uint16_t>(0);
    }
}
