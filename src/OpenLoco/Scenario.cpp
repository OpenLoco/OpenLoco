#include "Scenario.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"
#include "S5/S5.h"
#include "Ui/WindowManager.h"

using namespace OpenLoco::interop;

namespace OpenLoco::scenario
{
    // 0x0043EDAD
    void eraseLandscape()
    {
        s5::getOptions().scenarioFlags &= ~(scenario::flags::landscape_generation_done);
        ui::WindowManager::invalidate(ui::WindowType::landscapeGeneration, 0);
        call(0x0043C88C);
        s5::getOptions().madeAnyChanges = 0;
        addr<0x00F25374, uint8_t>() = 0;
        Gfx::invalidateScreen();
    }

    void generateLandscape()
    {
        call(0x0043C90C);
    }

    // 0x0044400C
    void start(int32_t ebx)
    {
        registers regs;
        regs.ebx = ebx;
        call(0x0044400C, regs);
    }
}
