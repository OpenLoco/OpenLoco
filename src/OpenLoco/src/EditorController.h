#pragma once

#include <cstdint>

namespace OpenLoco::EditorController
{

    enum class Step : int8_t
    {
        null = -1,
        objectSelection = 0,
        landscapeEditor = 1,
        scenarioOptions = 2,
        saveScenario = 3,
    };

    void init();
    void showEditor();

    Step getCurrentStep();
    Step getPreviousStep();
    Step getNextStep();
    bool canGoBack();
    void goToPreviousStep();
    void goToNextStep();

    void tick();
}
