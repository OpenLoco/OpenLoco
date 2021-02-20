#include "EditorController.h"
#include "Ui/WindowManager.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui;
using namespace OpenLoco::Ui::Windows;

namespace OpenLoco::EditorController
{
    static loco_global<Step, 0x009C8714> _editorStep;

    // 0x0043D7DC
    void init()
    {
        call(0x0043D7DC);
    }

    Step getCurrentStep()
    {
        return (Step)*_editorStep;
    }

    Step getPreviousStep()
    {
        return (Step)((uint8_t)(*_editorStep) - 1);
    }

    Step getNextStep()
    {
        return (Step)((uint8_t)(*_editorStep) + 1);
    }

    bool canGoBack()
    {
        return getCurrentStep() != Step::step0;
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
            case Step::step0:
                if (WindowManager::find(WindowType::objectSelection) == nullptr)
                    Windows::ObjectSelectionWindow::open();
                break;

            case Step::step1:
                // Scenario/landscape loaded?
                if ((addr<0x00525E28, uint32_t>() & 1) != 0)
                    return;

                if (WindowManager::find(WindowType::landscapeGeneration) == nullptr)
                    Windows::LandscapeGeneration::open();
                break;

            case Step::step2:
                if (WindowManager::find(WindowType::scenarioOptions) == nullptr)
                    Windows::ScenarioOptions::open();
                break;

            case Step::step3:
                break;
        }
    }
}
