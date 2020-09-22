#include "Scenario.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"
#include "S5/S5.h"
#include "Ui/WindowManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Scenario
{
    // 0x0043EDAD
    void eraseLandscape()
    {
        S5::getOptions().scenarioFlags &= ~(Scenario::flags::landscape_generation_done);
        Ui::WindowManager::invalidate(Ui::WindowType::landscapeGeneration, 0);
        call(0x0043C88C);
        S5::getOptions().madeAnyChanges = 0;
        addr<0x00F25374, uint8_t>() = 0;
        Gfx::invalidateScreen();
    }

    void generateLandscape()
    {
        call(0x0043C90C);
    }

    // 0x0044400C
    void start(const char* ebx)
    {
        registers regs;
        regs.ebx = reinterpret_cast<int32_t>(ebx);
        call(0x0044400C, regs);
    }
}
