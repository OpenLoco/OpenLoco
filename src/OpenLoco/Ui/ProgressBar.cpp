#include "ProgressBar.h"
#include "../Interop/Interop.hpp"
#include "../OpenLoco.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::ProgressBar
{
    static uint32_t _1136590;

    // 0x004CF5C5
    void begin(string_id stringId, int32_t edx)
    {
        _1136590 = edx;
        if (isInitialised())
        {
            _1136590 &= (1 << 31);
            registers regs;
            regs.eax = stringId;
            call(0x004CF6B0, regs);
        }
        else
        {
            registers regs;
            regs.eax = stringId;
            call(0x004CF5DA, regs);
        }
    }

    // 0x004CF621
    // eax: value
    void setProgress(int32_t value)
    {
        registers regs;
        regs.eax = value;
        call(0x004CF621, regs);
    }

    // 0x00408163
    static void destroyLoadingWindow()
    {
        call(0x00408163);
    }

    // 0x004CF60B
    void end()
    {
        if (_1136590 & (1 << 31))
        {
            call(0x004CF74E);
        }
        else
        {
            destroyLoadingWindow();
        }
    }
}
