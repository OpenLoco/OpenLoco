#pragma once

#include "interop/interop.hpp"

using namespace openloco::interop;

namespace openloco::game_commands
{
    inline void do_21(uint8_t bl, uint8_t dl, uint8_t di)
    {
        registers regs;
        regs.bl = bl; // [ 1 ]
        regs.dl = dl; // [ 0, 2 ]
        regs.di = di; // [ 0, 1, 2 ]
        regs.esi = 21;
        call(0x00431315, regs);
    }
}
