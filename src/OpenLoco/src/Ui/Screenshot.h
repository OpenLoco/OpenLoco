#pragma once

#include <cstdint>

namespace OpenLoco::Ui
{
    enum class ScreenshotType : uint8_t
    {
        regular = 0,
        giant = 1,
    };

    void triggerScreenshotCountdown(int8_t numTicks, ScreenshotType type);
    void handleScreenshotCountdown();
}
