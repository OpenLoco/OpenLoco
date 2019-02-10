#pragma once

#include "interop/interop.hpp"

using namespace openloco::interop;

namespace openloco::game_commands
{
    inline void do_command(int esi, registers& registers);

    inline void do_21(uint8_t bl, uint8_t dl, uint8_t di)
    {
        registers regs;
        regs.bl = bl; // [ 1 ]
        regs.dl = dl; // [ 0, 2 ]
        regs.di = di; // [ 0, 1, 2 ]
        do_command(21, regs);
    }

    inline void do_71(int32_t ax, char* string)
    {

        registers regs;
        regs.bl = 1;
        regs.ax = ax;
        memcpy(&regs.ecx, &string[0], 4);
        memcpy(&regs.edx, &string[4], 4);
        memcpy(&regs.ebp, &string[8], 4);
        memcpy(&regs.edi, &string[12], 4);
        do_command(71, regs);
    }

    inline void do_command(int esi, registers& registers)
    {
        registers.esi = esi;
        call(0x00431315, registers);
    }
}
