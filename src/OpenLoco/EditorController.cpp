#include "EditorController.h"
#include "Audio/Audio.h"
#include "Config.h"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "GameState.h"
#include "Interop/Interop.hpp"
#include "Localisation/StringIds.h"
#include "Objects/CargoObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/ScenarioTextObject.h"
#include "S5/S5.h"
#include "Scenario.h"
#include "ScenarioManager.h"
#include "TownManager.h"
#include "Ui/WindowManager.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui;
using namespace OpenLoco::Ui::Windows;

namespace OpenLoco::EditorController
{
    static loco_global<uint8_t, 0x00508F17> paused_state;
    static loco_global<char[267], 0x00050B745> activeSavePath;
    static loco_global<char[512], 0x00112CE04> scenarioFilename;

    // 0x0043D7DC
    void init()
    {
        call(0x0043D7DC);
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

        options.scenarioFlags &= ~Scenario::Flags::landscapeGenerationDone;
        if (addr<0x00525E28, uint32_t>() & 1)
        {
            options.scenarioFlags |= Scenario::Flags::landscapeGenerationDone;
            Game::sub_46DB4C(); // draw preview map
        }

        options.maxCompetingCompanies = gameState.maxCompetingCompanies;
        options.competitorStartDelay = gameState.competitorStartDelay;
        gameState.numberOfIndustries = options.numberOfIndustries;
        options.objectiveType = gameState.objectiveType;
        options.objectiveFlags = gameState.objectiveFlags;
        options.objectiveCompanyValue = gameState.objectiveCompanyValue;
        options.objectiveMonthlyVehicleProfit = gameState.objectiveMonthlyVehicleProfit;
        options.objectivePerformanceIndex = gameState.objectivePerformanceIndex;
        options.objectiveDeliveredCargoType = gameState.objectiveDeliveredCargoType;
        options.objectiveDeliveredCargoAmount = gameState.objectiveDeliveredCargoAmount;
        options.objectiveTimeLimitYears = gameState.objectiveTimeLimitYears;

        options.objectiveDeliveredCargo = ObjectManager::getHeader(LoadedObjectHandle{ ObjectType::cargo, options.objectiveDeliveredCargoType });
        options.currency = ObjectManager::getHeader(LoadedObjectHandle{ ObjectType::currency, options.objectiveDeliveredCargoType });
    }

    // 0x0043EE25
    static bool validateStep1()
    {
        if ((addr<0x00525E28, uint32_t>() & 1) == 0)
        {
            return true;
        }

        if (!TownManager::towns().empty())
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
                LandscapeGeneration::open();
                break;

            case Step::saveScenario:
                break;
        }

        Gfx::invalidateScreen();
    }

    // 0x0043D15D
    void goToNextStep()
    {
        switch (getCurrentStep())
        {
            case Step::null:
                break;

            case Step::objectSelection:
                ObjectSelectionWindow::closeWindow();
                call(0x004747D4);
                Scenario::sub_4748D4();
                Scenario::initialiseSnowLine();
                S5::sub_4BAEC4();
                S5::getOptions().editorStep = Step::landscapeEditor;
                LandscapeGeneration::open();
                if (S5::getOptions().scenarioFlags & Scenario::Flags::landscapeGenerationDone)
                {
                    if ((addr<0x00525E28, uint32_t>() & 1) == 0)
                    {
                        Scenario::generateLandscape();
                    }
                }
                break;

            case Step::landscapeEditor:
            {
                if (!validateStep1())
                {
                    Windows::Error::open(StringIds::cant_advance_to_next_editor_stage, GameCommands::getErrorText());
                    break;
                }

                const auto cargoId = S5::getOptions().objectiveDeliveredCargoType;
                if (ObjectManager::get<CargoObject>(cargoId) == nullptr)
                {
                    for (size_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::cargo); i++)
                    {
                        if (ObjectManager::get<CargoObject>(i) != nullptr)
                        {
                            S5::getOptions().objectiveDeliveredCargoType = static_cast<uint8_t>(i);
                            break;
                        }
                    }
                }

                WindowManager::closeAllFloatingWindows();
                Scenario::initialiseDate(S5::getOptions().scenarioStartYear);
                Scenario::initialiseSnowLine();
                ScenarioOptions::open();
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

                auto path = fs::u8path(scenarioFilename.get());
                path.replace_extension(S5::extensionSC5);
                strncpy(activeSavePath, path.u8string().c_str(), 257); // Or 256?
                uint32_t saveFlags = S5::SaveFlags::scenario;
                if (Config::get().flags & Config::Flags::exportObjectsWithSaves)
                {
                    saveFlags |= S5::SaveFlags::packCustomObjects;
                }

                bool success = S5::save(path, saveFlags);

                if (!success)
                {
                    showError(StringIds::scenario_save_failed);
                    S5::getOptions().editorStep = Step::scenarioOptions;
                    break;
                }

                ScenarioManager::loadIndex(1);

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
                if (Game::hasFlags(1u << 0))
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
