#include "Graphics/FPSCounter.h"
#include "Config.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/TextRenderer.h"
#include "Localisation/Formatting.h"
#include "Map/TileManager.h"
#include "Ui.h"
#include "Ui/Window.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/ZoomLevel.hpp>

#include <chrono>
#include <optional>
#include <stdio.h>

namespace OpenLoco::Gfx
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

    static const char* zoomScaleName(const ZoomLevel zoom)
    {
        switch (static_cast<int8_t>(zoom))
        {
            case ZoomLevel::quadrupled: return "4x";
            case ZoomLevel::doubled: return "2x";
            case ZoomLevel::full: return "1x";
            case ZoomLevel::half: return "1/2x";
            case ZoomLevel::quarter: return "1/4x";
            case ZoomLevel::eighth: return "1/8x";
            case ZoomLevel::sixteenth: return "1/16x";
            case ZoomLevel::thirtySecond: return "1/32x";
            case ZoomLevel::sixtyFourth: return "1/64x";
            case ZoomLevel::hundredTwentyEighth: return "1/128x";
        }
        return "?";
    }

    static std::optional<ZoomLevel> getMainViewportZoom()
    {
        const auto* main = Ui::WindowManager::getMainWindow();
        if (main == nullptr || main->viewports[0] == nullptr)
        {
            return std::nullopt;
        }
        return main->viewports[0]->zoom;
    }

    void drawFPS(DrawingContext& drawingCtx)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        // Measure FPS
        const float fps = measureFPS();

        // Format string
        char fpsText[16];
        const char* formatString = (_currentFPS >= 10.0f ? "%.0f" : "%.1f");
        snprintf(fpsText, std::size(fpsText), formatString, fps);

        char buffer[96];
        buffer[0] = ControlCodes::Font::bold;
        buffer[1] = ControlCodes::Font::outline;
        buffer[2] = ControlCodes::Colour::white;

        // Show the zoom level and the map size of the main viewport next to the frame rate
        const auto zoom = getMainViewportZoom();
        if (zoom.has_value())
        {
            snprintf(&buffer[3], std::size(buffer) - 3, "%s  zoom %d, %s  map %dx%d", fpsText, static_cast<int32_t>(static_cast<int8_t>(*zoom)), zoomScaleName(*zoom), World::TileManager::getMapColumns(), World::TileManager::getMapRows());
        }
        else
        {
            snprintf(&buffer[3], std::size(buffer) - 3, "%s", fpsText);
        }

        // Draw text
        const int stringWidth = tr.getStringWidth(buffer);
        const bool inCentre = !Config::get().toolbarButtonsCentred;
        auto point = Ui::Point{ (inCentre ? (Ui::width() - stringWidth) / 2 : 4), 4 };
        tr.drawString(point, Colour::black, buffer);

        // Make area dirty so the text doesn't get drawn over the last
        invalidateRegion(point.x, point.y, point.x + stringWidth, point.y + 16);
    }
}
