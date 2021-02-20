#pragma once

#include <cstdint>

namespace OpenLoco::EditorController
{

    enum class Step : int8_t
    {
        null = -1,
        step0 = 0,
        step1 = 1,
        step2 = 2,
        step3 = 3,
    };

    void init();

    Step getCurrentStep();
    Step getPreviousStep();
    Step getNextStep();
    bool canGoBack();
    void goToPreviousStep();
    void goToNextStep();

    void tick();
}
