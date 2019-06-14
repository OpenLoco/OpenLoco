#include "scenario.h"
#include "graphics/gfx.h"
#include "interop/interop.hpp"
#include "s5/s5.h"
#include "ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::scenario
{
    // 0x0043EDAD
    void eraseLandscape()
    {
        s5::getOptions().scenarioFlags &= ~(scenario::flags::landscape_generation_done);
        ui::WindowManager::invalidate(ui::WindowType::landscapeGeneration, 0);
        call(0x0043C88C);
        s5::getOptions().madeAnyChanges = 0;
        addr<0x00F25374, uint8_t>() = 0;
        gfx::invalidate_screen();
    }

    void generateLandscape()
    {
        call(0x0043C90C);
    }
}
