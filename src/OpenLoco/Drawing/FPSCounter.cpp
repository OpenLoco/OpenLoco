#include "FPSCounter.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Localisation/StringManager.h"
#include "../Ui.h"

#include <chrono>
#include <stdio.h>

namespace OpenLoco::Drawing
{
    using Clock_t = std::chrono::high_resolution_clock;
    using Timepoint_t = Clock_t::time_point;

    static Timepoint_t _referenceTime;
    static uint32_t _currentFrameCount;
    static float _currentFPS;

    static float measureFPS()
    {
        _currentFrameCount++;

        auto currentTime = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - _referenceTime).count() / 1000.0;

        if (elapsed > 1000)
        {
            _currentFPS = _currentFrameCount;
            _currentFrameCount = 0;
            _referenceTime = currentTime;
        }

        return _currentFPS;
    }

    void drawFPS()
    {
        // Measure FPS
        const float fps = measureFPS();

        // Format string
        char buffer[64];
        buffer[0] = ControlCodes::font_bold;
        buffer[1] = ControlCodes::outline;
        buffer[2] = ControlCodes::colour_white;

        const char* formatString = (_currentFPS >= 10.0f ? "%.0f" : "%.1f");
        snprintf(&buffer[3], std::size(buffer) - 3, formatString, fps);

        auto& dpi = Gfx::screenDpi();

        // Draw text
        const int stringWidth = Gfx::getStringWidth(buffer);
        const auto x = Ui::width() / 2 - (stringWidth / 2);
        const auto y = 2;
        Gfx::drawString(&dpi, x, y, Colour::black, buffer);

        // Make area dirty so the text doesn't get drawn over the last
        Gfx::setDirtyBlocks(x - 16, y - 4, x + 16, 16);
    }
}
