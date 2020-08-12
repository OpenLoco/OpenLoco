#include "EditorController.h"
#include "Audio/Audio.h"
#include "Config.h"
#include "Game.h"
#include "Interop/Interop.hpp"
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
    static loco_global<string_id, 0x009C68E6> gGameCommandErrorText;
    static loco_global<Step, 0x009C8714> _editorStep;
    static loco_global<char[267], 0x00050B745> activeSavePath;
    static loco_global<uint8_t, 0x00525FB7> maxCompetingCompanies;
    static loco_global<uint8_t, 0x00526214> competitorStartDelay;
    static loco_global<uint8_t, 0x0052622D> _52622D;
    static loco_global<uint8_t, 0x00526230> objectiveType;
    static loco_global<uint8_t, 0x00526231> objectiveFlags;
    static loco_global<uint32_t, 0x00526232> objectiveCompanyValue;
    static loco_global<uint32_t, 0x00526236> objectiveMonthlyVehicleProfit;
    static loco_global<uint8_t, 0x0052623A> objectivePerformanceIndex;
    static loco_global<uint8_t, 0x0052623B> objectiveDeliveredCargoType;
    static loco_global<uint32_t, 0x0052623C> objectiveDeliveredCargoAmount;
    static loco_global<uint8_t, 0x00526240> objectiveTimeLimitYears;
    static loco_global<char[512], 0x00112CE04> scenarioFilename;

    // 0x0043D7DC
    void init()
    {
        call(0x0043D7DC);
    }

    Step getCurrentStep()
    {
        return (Step)S5::getOptions().editorStep;
    }

    Step getPreviousStep()
    {
        return Step(static_cast<uint8_t>(S5::getOptions().editorStep) - 1);
    }

    Step getNextStep()
    {
        return Step(static_cast<uint8_t>(S5::getOptions().editorStep) + 1);
    }

    bool canGoBack()
    {
        return getCurrentStep() != Step::objectSelection;
    }

    static void sub_440165()
    {
        S5::Options& options = S5::getOptions();
        //        options.scenarioText.type = 0xFF;

        auto obj = ObjectManager::get<ScenarioTextObject>();
        if (obj != nullptr)
        {
            // TODO: copy text object string + header
        }

        if (strnlen(options.scenarioName, sizeof(options.scenarioName)) == 0)
        {
            StringManager::formatString(options.scenarioName, StringIds::unnamed);
        }

        options.scenarioFlags &= ~Scenario::flags::landscape_generation_done;
        if (addr<0x00525E28, uint32_t>() & 1)
        {
            options.scenarioFlags |= Scenario::flags::landscape_generation_done;
            call(0x46DB4C); // draw preview map
        }

        options.maxCompetingCompanies = maxCompetingCompanies;
        options.competitorStartDelay = competitorStartDelay;
        _52622D = options.numberOfIndustries;
        options.objectiveType = objectiveType;
        options.objectiveFlags = objectiveFlags;
        options.objectiveCompanyValue = objectiveCompanyValue;
        options.objectiveMonthlyVehicleProfit = objectiveMonthlyVehicleProfit;
        options.objectivePerformanceIndex = objectivePerformanceIndex;
        options.objectiveDeliveredCargoType = objectiveDeliveredCargoType;
        options.objectiveDeliveredCargoAmount = objectiveDeliveredCargoAmount;
        options.objectiveTimeLimitYears = objectiveTimeLimitYears;

        // TODO: copy object headers
        options.objectiveDeliveredCargo = ObjectHeader();
        options.currency = ObjectHeader();
    }

    // 0x0043EE25
    static bool validateStep1()
    {
        if ((addr<0x00525E28, uint32_t>() & 1) == 0)
        {
            return true;
        }

        for (auto& town : TownManager::towns())
        {
            if (!town.empty())
                return true;
        }

        gGameCommandErrorText = StringIds::str_1671;
        return false;
    }

    static bool sub_4418DB()
    {
        char title[128];
        StringManager::formatString(title, StringIds::str_366); // Save Scenario

        // TODO: build default scenario path
        char path[128];

        // Unused? Maybe for CJK versions?
        char buffer2[128];
        StringManager::formatString(buffer2, StringIds::str_368); // OpenLoco Scenario File

        Audio::pauseSound();
        paused_state = paused_state ^ (1 << 2);
        Gfx::invalidateScreen();
        Gfx::render();

        // blocking
        auto result = PromptBrowse::open(PromptBrowse::browse_type::save, path, S5::filterSC5, title);

        Audio::unpauseSound();
        call(0x004072EC);
        paused_state = paused_state ^ (1 << 2);
        Gfx::invalidateScreen();
        Gfx::render();
        return result;
    }

    // 0x004BAEC4
    void TerraformConfig()
    {
        call(0x004BAEC4);
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
                _editorStep = Step::objectSelection;
                break;

            case Step::scenarioOptions:
                // 0x0043D12C
                WindowManager::closeAllFloatingWindows();
                TerraformConfig();
                _editorStep = Step::landscapeEditor;
                LandscapeGeneration::open();
                break;

            case Step::saveScenario:
                break;
        }

        Gfx::invalidateScreen();
    }

    void ClimateInit()
    {
        call(0x00496A18);
    }

    // 0x0043D15D
    void goToNextStep()
    {
        switch (getCurrentStep())
        {
            case Step::null:
                break;

            case Step::objectSelection:
                call(0x473A13);
                call(0x4747D4);
                call(0x4748D4);
                ClimateInit();
                TerraformConfig();
                _editorStep = Step::landscapeEditor;
                LandscapeGeneration::open();
                if (S5::getOptions().scenarioFlags & Scenario::landscape_generation_done)
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
                    Windows::Error::open(StringIds::str_1672, gGameCommandErrorText);
                    break;
                }

                int eax = S5::getOptions().objectiveDeliveredCargoType;
                if (ObjectManager::get<CargoObject>(eax) == nullptr)
                {
                    for (size_t i = 0; i < ObjectManager::getMaxObjects(object_type::cargo); i++)
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
                ClimateInit();
                ScenarioOptions::open();
                _editorStep = Step::scenarioOptions;
                break;
            }

            case Step::scenarioOptions:
            {
                sub_440165();
                // Original checks for a non-true response, and aborts saving with an error.

                WindowManager::closeAllFloatingWindows();

                if (!sub_4418DB())
                    break;

                _editorStep = Step::null;
                call(0x0046F910);

                auto path = fs::path(scenarioFilename.get());
                path.replace_extension(S5::extensionSC5);
                strncpy(activeSavePath, path.u8string().c_str(), 257); // Or 256?
                uint32_t saveFlags = S5::SaveFlags::scenario;
                if (Config::get().flags & Config::flags::export_objects_with_saves)
                {
                    saveFlags |= S5::SaveFlags::packCustomObjects;
                }

                bool success = S5::save(path, saveFlags);

                if (!success)
                {
                    showError(StringIds::str_1711);
                    _editorStep = Step::scenarioOptions;
                    break;
                }

                ScenarioManager::loadIndex(1);

                // This ends with a premature tick termination
                call(0x0043C0FD);
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
