#include "FPSCounter.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Localisation/Formatting.h"
#include "Ui.h"

#include <chrono>
#include <stdio.h>

namespace OpenLoco::Drawing
{
    using Clock_t = std::chrono::high_resolution_clock;
    using TimePoint_t = Clock_t::time_point;

    static TimePoint_t _referenceTime;
    static uint32_t _currentFrameCount;
    static float _currentFPS;

    static float measureFPS()
    {
        _currentFrameCount++;

        auto currentTime = Clock_t::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - _referenceTime).count() / 1000.0;

        if (elapsed > 1.0)
        {
            _currentFPS = _currentFrameCount / elapsed;
            _currentFrameCount = 0;
            _referenceTime = currentTime;
        }

        return _currentFPS;
    }

    void drawFPS()
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        // Measure FPS
        const float fps = measureFPS();

        // Format string
        char buffer[64];
        buffer[0] = ControlCodes::Font::bold;
        buffer[1] = ControlCodes::Font::outline;
        buffer[2] = ControlCodes::Colour::white;

        const char* formatString = (_currentFPS >= 10.0f ? "%.0f" : "%.1f");
        snprintf(&buffer[3], std::size(buffer) - 3, formatString, fps);

        auto& rt = Gfx::getScreenRT();

        // Draw text
        const int stringWidth = drawingCtx.getStringWidth(buffer);
        auto point = Ui::Point(Ui::width() / 2 - (stringWidth / 2), 2);
        drawingCtx.drawString(rt, point, Colour::black, buffer);

        // Make area dirty so the text doesn't get drawn over the last
        Gfx::invalidateRegion(point.x - 16, point.y - 4, point.x + 16, 16);
    }
}
