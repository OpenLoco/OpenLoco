#include "FPSCounter.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Localisation/StringManager.h"
#include "../Ui.h"

#include <SDL2/SDL.h>
#include <stdio.h>

namespace OpenLoco::Drawing
{
    static uint32_t _lastFPSUpdateTicks;
    static uint32_t _lastFPSTicks;
    static float _currentFPS;

    static float measureFPS()
    {
        const uint32_t currentTicks = SDL_GetTicks();
        if (currentTicks - _lastFPSUpdateTicks > 500)
        {
            _lastFPSUpdateTicks = currentTicks;

            const uint32_t frameDelta = currentTicks - _lastFPSTicks;
            _currentFPS = 1000.0f / frameDelta;
        }
        _lastFPSTicks = currentTicks;
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
