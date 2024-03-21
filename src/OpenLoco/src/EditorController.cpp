#include "EditorController.h"
#include "Audio/Audio.h"
#include "Config.h"
#include "Date.h"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "GameException.hpp"
#include "GameState.h"
#include "GameStateFlags.h"
#include "Gui.h"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Localisation/StringManager.h"
#include "Objects/BuildingObject.h"
#include "Objects/CargoObject.h"
#include "Objects/ClimateObject.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectIndex.h"
#include "Objects/ObjectManager.h"
#include "Objects/ScenarioTextObject.h"
#include "OpenLoco.h"
#include "S5/S5.h"
#include "Scenario.h"
#include "ScenarioManager.h"
#include "ScenarioObjective.h"
#include "SceneManager.h"
#include "Title.h"
#include "Ui/WindowManager.h"
#include "World/CompanyManager.h"
#include "World/TownManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui;

namespace OpenLoco::EditorController
{
    static loco_global<char[267], 0x00050B745> _activeSavePath;
    static loco_global<char[512], 0x00112CE04> _scenarioFilename;

    // 0x00440297
    // TODO: only called from editor, but move to S5 namespace?
    static void resetLandDistributionPatterns()
    {
        auto& options = S5::getOptions();
        for (auto i = 0U; i < ObjectManager::getMaxObjects(ObjectType::land); i++)
        {
            options.landDistributionPatterns[i] = S5::LandDistributionPattern::everywhere;
            auto* landObj = ObjectManager::get<LandObject>(i);
            if (landObj == nullptr)
                continue;

            options.landDistributionPatterns[i] = S5::LandDistributionPattern(landObj->distributionPattern);
        }
    }

    // 0x0043D7DC
    void init()
    {
        // TODO: not sure why title flag is preserved?
        bool wasTitleMode = isTitleMode();
        setAllScreenFlags(ScreenFlags::editor);
        if (wasTitleMode)
            setScreenFlag(ScreenFlags::title);

        setGameSpeed(GameSpeed::Normal);

        auto& options = S5::getOptions();
        auto& gameState = getGameState();

        options.editorStep = Step::objectSelection;
        options.difficulty = 2;
        options.madeAnyChanges = 0;
        addr<0x00F25374, uint8_t>() = 0; // ?? backup for madeAnyChanges?
        options.scenarioFlags = Scenario::ScenarioFlags::landscapeGenerationDone;
        gameState.lastLandOption = 0xFF;
        gameState.lastMapWindowAttributes.flags = WindowFlags::none;

        WindowManager::closeAllFloatingWindows();
        initialiseViewports();
        Title::sub_4284C8();
        Audio::pauseSound();
        Audio::unpauseSound();
        ObjectManager::unloadAll();
        ObjectManager::prepareSelectionList(false);
        ObjectManager::loadSelectionListObjects(Scenario::getInUseSelectedObjectFlags());
        ObjectManager::freeSelectionList();
        ObjectManager::reloadAll();
        Scenario::sub_4748D4();

        options.scenarioStartYear = 1900;
        gameState.seaLevel = 4;
        options.minLandHeight = 2;
        options.topographyStyle = S5::TopographyStyle::mountains;
        options.hillDensity = 50;
        options.numberOfForests = 100;
        options.minForestRadius = 4;
        options.maxForestRadius = 40;
        options.minForestDensity = 1;
        options.maxForestDensity = 7;
        options.numberRandomTrees = 1000;
        options.minAltitudeForTrees = 0;
        options.maxAltitudeForTrees = 25;
        options.numberOfTowns = 55;
        options.maxTownSize = 3;
        options.numberOfIndustries = 1;

        resetLandDistributionPatterns();
        Scenario::reset();

        gameState.maxCompetingCompanies = 8;
        gameState.competitorStartDelay = 0;
        gameState.preferredAIIntelligence = 0;
        gameState.preferredAIAggressiveness = 0;
        gameState.preferredAICompetitiveness = 0;
        gameState.startingLoanSize = 1250;
        gameState.maxLoanSize = 3750;
        gameState.loanInterestRate = 10;
        gameState.industryFlags = IndustryManager::Flags::none;
        gameState.forbiddenVehiclesPlayers = 0;
        gameState.forbiddenVehiclesCompetitors = 0;
        gameState.scenarioObjective.type = Scenario::ObjectiveType::companyValue;
        gameState.scenarioObjective.flags = Scenario::ObjectiveFlags::none;
        gameState.scenarioObjective.companyValue = 1'000'000;
        gameState.scenarioObjective.monthlyVehicleProfit = 25'000;
        gameState.scenarioObjective.performanceIndex = 90;
        gameState.scenarioObjective.deliveredCargoType = 0;
        gameState.scenarioObjective.deliveredCargoAmount = 50'000;
        gameState.scenarioObjective.timeLimitYears = 20;

        Scenario::initialiseDate(options.scenarioStartYear);
        Scenario::updateSeason(getCurrentDay(), ObjectManager::get<ClimateObject>());

        StringManager::formatString(options.scenarioDetails, StringIds::no_details_yet);
        StringManager::formatString(options.scenarioName, StringIds::unnamed);

        showEditor();
        Audio::resetMusic();
        Gfx::loadPalette();
        Gfx::invalidateScreen();

        resetScreenAge();
        throw GameException::Interrupt;
    }

    // 0x0043CB9F
    void showEditor()
    {
        Windows::Main::open();

        Windows::Terraform::setAdjustLandToolSize(1);
        Windows::Terraform::setAdjustWaterToolSize(1);
        Windows::Terraform::setClearAreaToolSize(2);

        Windows::ToolbarTop::Editor::open();
        Windows::ToolbarBottom::Editor::open();
        Gui::resize();
    }

    Step getCurrentStep()
    {
        return S5::getOptions().editorStep;
    }

    Step getPreviousStep()
    {
        return Step(enumValue(getCurrentStep()) - 1);
    }

    Step getNextStep()
    {
        return Step(enumValue(getCurrentStep()) + 1);
    }

    bool canGoBack()
    {
        return getCurrentStep() != Step::objectSelection;
    }

    // 0x00440165
    static void setDefaultScenarioOptions()
    {
        S5::Options& options = S5::getOptions();
        auto& gameState = getGameState();
        // Sets the type of the scenario text to an invalid type
        options.scenarioText.flags = 0xFF | options.scenarioText.flags;

        auto* obj = ObjectManager::get<ScenarioTextObject>();
        if (obj != nullptr)
        {
            char buffer[256]{};
            StringManager::formatString(buffer, obj->name);
            strncpy(options.scenarioName, buffer, std::size(options.scenarioName) - 1);
            options.scenarioName[std::size(options.scenarioName) - 1] = '\0';
            // Copy the object
            options.scenarioText = ObjectManager::getHeader(LoadedObjectHandle{ ObjectType::scenarioText, 0 });
        }

        if (strnlen(options.scenarioName, sizeof(options.scenarioName)) == 0)
        {
            StringManager::formatString(options.scenarioName, StringIds::unnamed);
        }

        options.scenarioFlags &= ~Scenario::ScenarioFlags::landscapeGenerationDone;
        if (Game::hasFlags(GameStateFlags::tileManagerLoaded))
        {
            options.scenarioFlags |= Scenario::ScenarioFlags::landscapeGenerationDone;
            Game::sub_46DB4C(); // draw preview map
        }

        options.maxCompetingCompanies = CompanyManager::getMaxCompetingCompanies();
        options.competitorStartDelay = CompanyManager::getCompetitorStartDelay();
        gameState.numberOfIndustries = options.numberOfIndustries;
        options.objective = Scenario::getObjective();
        options.objectiveDeliveredCargo = ObjectManager::getHeader(LoadedObjectHandle{ ObjectType::cargo, options.objective.deliveredCargoType });
        options.currency = ObjectManager::getHeader(LoadedObjectHandle{ ObjectType::currency, 0 });
    }

    // 0x0043EE25
    static bool validateStep1()
    {
        if (!Game::hasFlags(GameStateFlags::tileManagerLoaded))
        {
            return true;
        }

        if (TownManager::towns().size() >= Limits::kMinTowns)
        {
            return true;
        }

        GameCommands::setErrorText(StringIds::at_least_one_town_be_built);
        return false;
    }

    // 0x0043D0FA
    void goToPreviousStep()
    {
        switch (getCurrentStep())
        {
            case Step::null:
            case Step::objectSelection:
                break;

            case Step::landscapeEditor:
                // 0x0043D119
                WindowManager::closeAllFloatingWindows();
                S5::getOptions().editorStep = Step::objectSelection;
                break;

            case Step::scenarioOptions:
                // 0x0043D12C
                WindowManager::closeAllFloatingWindows();
                S5::sub_4BAEC4();
                S5::getOptions().editorStep = Step::landscapeEditor;
                Windows::LandscapeGeneration::open();
                break;

            case Step::saveScenario:
                break;
        }

        Gfx::invalidateScreen();
    }

    // 0x004747D4
    static StringId validateHeadquarterBuilding()
    {
        size_t numHeadquarterTypes = 0;
        for (LoadedObjectId id = 0; id < ObjectManager::getMaxObjects(ObjectType::building); ++id)
        {
            auto* buildingObj = ObjectManager::get<BuildingObject>(id);
            if (buildingObj == nullptr)
            {
                continue;
            }
            if (buildingObj->hasFlags(BuildingObjectFlags::isHeadquarters))
            {
                numHeadquarterTypes++;
            }
        }
        if (numHeadquarterTypes == 0)
        {
            return StringIds::company_headquarter_building_type_must_be_selected;
        }
        else if (numHeadquarterTypes > 1)
        {
            return StringIds::only_one_company_headquarter_building_type_must_be_selected;
        }
        return StringIds::null;
    }

    // 0x0043D15D
    void goToNextStep()
    {
        switch (getCurrentStep())
        {
            case Step::null:
                break;

            case Step::objectSelection:
            {
                if (!Windows::ObjectSelectionWindow::tryCloseWindow())
                {
                    // Try close has failed so do not allow changing step!
                    return;
                }
                auto res = validateHeadquarterBuilding();
                if (res != StringIds::null)
                {
                    Windows::Error::open(StringIds::invalid_selection_of_objects, res);
                    return;
                }
                Scenario::sub_4748D4();
                Scenario::initialiseSnowLine();
                S5::sub_4BAEC4();
                S5::getOptions().editorStep = Step::landscapeEditor;
                Windows::LandscapeGeneration::open();
                if ((S5::getOptions().scenarioFlags & Scenario::ScenarioFlags::landscapeGenerationDone) != Scenario::ScenarioFlags::none)
                {
                    if (!Game::hasFlags(GameStateFlags::tileManagerLoaded))
                    {
                        Scenario::generateLandscape();
                    }
                }
                break;
            }
            case Step::landscapeEditor:
            {
                if (!validateStep1())
                {
                    Windows::Error::open(StringIds::cant_advance_to_next_editor_stage, GameCommands::getErrorText());
                    break;
                }

                const auto cargoId = S5::getOptions().objective.deliveredCargoType;
                if (ObjectManager::get<CargoObject>(cargoId) == nullptr)
                {
                    for (size_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::cargo); i++)
                    {
                        if (ObjectManager::get<CargoObject>(i) != nullptr)
                        {
                            S5::getOptions().objective.deliveredCargoType = static_cast<uint8_t>(i);
                            break;
                        }
                    }
                }

                WindowManager::closeAllFloatingWindows();
                Scenario::initialiseDate(S5::getOptions().scenarioStartYear);
                Scenario::initialiseSnowLine();
                Windows::ScenarioOptions::open();
                S5::getOptions().editorStep = Step::scenarioOptions;
                break;
            }

            case Step::scenarioOptions:
            {
                setDefaultScenarioOptions();
                // Original checks for a non-true response, and aborts saving with an error.

                WindowManager::closeAllFloatingWindows();

                if (!Game::saveScenarioOpen())
                    break;

                S5::getOptions().editorStep = Step::null;
                call(0x0046F910); // Sets up new multiplayer related rands

                auto path = fs::u8path(_scenarioFilename.get());
                path.replace_extension(S5::extensionSC5);
                strncpy(_activeSavePath, path.u8string().c_str(), 257); // Or 256?
                S5::SaveFlags saveFlags = S5::SaveFlags::scenario;
                if (Config::get().hasFlags(Config::Flags::exportObjectsWithSaves))
                {
                    saveFlags |= S5::SaveFlags::packCustomObjects;
                }

                bool success = S5::exportGameStateToFile(path, saveFlags);

                if (!success)
                {
                    Windows::Error::open(StringIds::scenario_save_failed);
                    S5::getOptions().editorStep = Step::scenarioOptions;
                    break;
                }

                ScenarioManager::loadIndex(true);

                // This ends with a premature tick termination
                Game::returnToTitle();
                return; // won't be reached
            }

            case Step::saveScenario:
                break;
        }

        Gfx::invalidateScreen();
    }

    // Part of 0x0043D9D4
    void tick()
    {
        switch (getCurrentStep())
        {
            case Step::null:
                break;

            case Step::objectSelection:
                if (WindowManager::find(WindowType::objectSelection) == nullptr)
                    Windows::ObjectSelectionWindow::open();
                break;

            case Step::landscapeEditor:
                // Scenario/landscape loaded?
                if (Game::hasFlags(GameStateFlags::tileManagerLoaded))
                    return;

                if (WindowManager::find(WindowType::landscapeGeneration) == nullptr)
                    Windows::LandscapeGeneration::open();
                break;

            case Step::scenarioOptions:
                if (WindowManager::find(WindowType::scenarioOptions) == nullptr)
                    Windows::ScenarioOptions::open();
                break;

            case Step::saveScenario:
                break;
        }
    }
}
