#include "scenario.h"
#include "graphics/gfx.h"
#include "interop/interop.hpp"
#include "ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::scenario
{
    // 0x0043EDAD
    void eraseLandscape()
    {
        *scenarioFlags &= ~(scenario::flags::landscape_generation_done);
        ui::WindowManager::invalidate(ui::WindowType::landscapeGeneration, 0);
        call(0x0043C88C);
        _9C871C = 0;
        addr<0x00F25374, uint8_t>() = 0;
        gfx::invalidate_screen();
    }

    void generateLandscape()
    {
        call(0x0043C90C);
    }
}
