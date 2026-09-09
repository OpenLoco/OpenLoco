#pragma once

#include "Types.hpp"
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

    StringId getCurrentStepString();
    StringId getPreviousStepString();
    StringId getNextStepString();

    bool canGoBack();
    void goToPreviousStep();
    void goToNextStep();

    void tick();
}
