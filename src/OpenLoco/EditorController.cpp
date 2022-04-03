#include "EditorController.h"
#include "Game.h"
#include "S5/S5.h"
#include "Ui/WindowManager.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui;
using namespace OpenLoco::Ui::Windows;

namespace OpenLoco::EditorController
{
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

    // 0x0043D0FA
    void goToPreviousStep()
    {
        call(0x0043D0FA);
    }

    // 0x0043D15D
    void goToNextStep()
    {
        call(0x0043D15D);
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
