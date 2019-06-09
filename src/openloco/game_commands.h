#pragma once

#include "interop/interop.hpp"
#include "map/tile.h"
#include "things/thing.h"

using namespace openloco::interop;

namespace openloco::game_commands
{
    inline void do_command(int esi, registers& registers);

    inline void do_20()
    {
        registers regs;
        regs.bl = 1;
        do_command(20, regs);
    }

    inline void do_21(uint8_t bl, uint8_t dl, uint8_t di)
    {
        registers regs;
        regs.bl = bl; // [ 1 ]
        regs.dl = dl; // [ 0, 2 ]
        regs.di = di; // [ 0 = load game, 1 = return to title screen, 2 = quit to desktop ]
        do_command(21, regs);
    }

    inline void do_46(uint8_t bl, uint16_t cx, uint16_t ax, uint32_t edx, uint32_t ebp, uint32_t edi)
    {
        registers regs;
        regs.bl = bl;   // [ 1 ]
        regs.cx = cx;   // town number or 0
        regs.ax = ax;   // [ 0, 1, 2]
        regs.edx = edx; // part of name buffer
        regs.ebp = ebp; // part of name buffer
        regs.edi = edi; // part of name buffer
        do_command(46, regs);
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

    inline bool do_50(uint8_t bl, uint8_t townId)
    {
        registers regs;
        regs.bl = bl; // [ 1 = remove town]
        regs.edi = townId;
        do_command(50, regs);
        return (uint32_t)regs.ebx != 0x80000000;
    }

    inline void do_73(thing_id_t id)
    {
        registers regs;
        regs.bl = 1;
        regs.ax = -2;
        regs.cx = id;
        do_command(73, regs);
    }

    inline void do_73(map::map_pos position)
    {
        registers regs;
        regs.bl = 1;
        regs.ax = position.x;
        regs.cx = position.y;
        do_command(73, regs);
    }

    inline void do_command(int esi, registers& registers)
    {
        registers.esi = esi;
        call(0x00431315, registers);
    }
}
