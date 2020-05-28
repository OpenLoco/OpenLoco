#pragma once

#include "interop/interop.hpp"
#include "map/tile.h"
#include "objects/objectmgr.h"
#include "things/thing.h"

using namespace openloco::interop;

namespace openloco::game_commands
{
    enum GameCommandFlag : uint8_t
    {
        apply = 1 << 0,  // 0x01
        flag_2 = 1 << 2, // 0x04
        flag_3 = 1 << 3, // 0x08
        flag_4 = 1 << 4, // 0x10
        flag_5 = 1 << 5, // 0x20
        flag_6 = 1 << 6, // 0x40
    };

    constexpr uint32_t FAILURE = 0x80000000;

    void registerHooks();
    uint32_t do_command(int esi, const registers& registers);

    // Build vehicle
    inline bool do_5(uint16_t vehicle_type, uint16_t vehicle_id = 0xFFFF)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.di = vehicle_id;
        regs.edx = vehicle_type;

        return do_command(5, regs) != FAILURE;
    }

    // Change loan
    inline void do_9(currency32_t newLoan)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.edx = newLoan;
        do_command(9, regs);
    }

    // Change station name
    inline void do_11(uint16_t cx, uint16_t ax, uint32_t edx, uint32_t ebp, uint32_t edi)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.cx = cx;   // station number or 0
        regs.ax = ax;   // [ 0, 1, 2]
        regs.edx = edx; // part of name buffer
        regs.ebp = ebp; // part of name buffer
        regs.edi = edi; // part of name buffer
        do_command(11, regs);
    }

    // Change company colour scheme
    inline void do_19(int8_t isPrimary, int8_t value, int8_t colourType, int8_t setColourMode, uint8_t companyId)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.cl = colourType;    // vehicle type or main
        regs.dh = setColourMode; // [ 0, 1 ] -- 0 = set colour, 1 = toggle enabled/disabled;
        regs.dl = companyId;     // company id

        if (setColourMode == 0)
        {
            // cl is divided by 2 when used
            regs.ah = isPrimary; // [ 0, 1 ] -- primary or secondary palette
            regs.al = value;     // new colour
        }
        else if (setColourMode == 1)
        {
            regs.al = value; // [ 0, 1 ] -- off or on
        }

        do_command(19, regs);
    }

    // Pause game
    inline void do_20()
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        do_command(20, regs);
    }

    // Load/save/quit game
    inline void do_21(uint8_t dl, uint8_t di)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.dl = dl; // [ 0, 2 ]
        regs.di = di; // [ 0 = load game, 1 = return to title screen, 2 = quit to desktop ]
        do_command(21, regs);
    }

    // Change Land Material
    inline void do_24(map::map_pos pointA, map::map_pos pointB, uint8_t landType, uint8_t flags)
    {
        registers regs;
        regs.ax = pointA.x;
        regs.cx = pointA.y;
        regs.di = pointB.x;
        regs.bp = pointB.y;
        regs.dl = landType;
        regs.bl = flags;
        do_command(24, regs);
    }

    // Raise Land
    inline uint32_t do_25(map::map_pos centre, map::map_pos pointA, map::map_pos pointB, uint16_t di, uint8_t flags)
    {
        registers regs;
        regs.ax = centre.x;
        regs.cx = centre.y;
        regs.edx = pointB.x << 16 | pointA.x;
        regs.ebp = pointB.y << 16 | pointA.y;
        regs.bl = flags;
        regs.di = di;
        return do_command(25, regs);
    }

    // Lower Land
    inline uint32_t do_26(map::map_pos centre, map::map_pos pointA, map::map_pos pointB, uint16_t di, uint8_t flags)
    {
        registers regs;
        regs.ax = centre.x;
        regs.cx = centre.y;
        regs.edx = pointB.x << 16 | pointA.x;
        regs.ebp = pointB.y << 16 | pointA.y;
        regs.bl = flags;
        regs.di = di;
        return do_command(26, regs);
    }

    // Lower/Raise Land Mountain
    inline uint32_t do_27(map::map_pos centre, map::map_pos pointA, map::map_pos pointB, uint16_t di, uint8_t flags)
    {
        registers regs;
        regs.ax = centre.x;
        regs.cx = centre.y;
        regs.edx = pointB.x << 16 | pointA.x;
        regs.ebp = pointB.y << 16 | pointA.y;
        regs.bl = flags;
        regs.di = di;
        return do_command(27, regs);
    }

    // Raise Water
    inline uint32_t do_28(map::map_pos pointA, map::map_pos pointB, uint8_t flags)
    {
        registers regs;
        regs.ax = pointA.x;
        regs.cx = pointA.y;
        regs.di = pointB.x;
        regs.bp = pointB.y;
        regs.bl = flags;
        return do_command(28, regs);
    }

    // Lower Water
    inline uint32_t do_29(map::map_pos pointA, map::map_pos pointB, uint8_t flags)
    {
        registers regs;
        regs.ax = pointA.x;
        regs.cx = pointA.y;
        regs.di = pointB.x;
        regs.bp = pointB.y;
        regs.bl = flags;
        return do_command(29, regs);
    }

    // Change company name
    inline bool do_30(uint16_t cx, uint16_t ax, uint32_t edx, uint32_t ebp, uint32_t edi)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.cx = cx;   // company id
        regs.ax = ax;   // [ 0, 1, 2]
        regs.edx = edx; // part of name buffer
        regs.ebp = ebp; // part of name buffer
        regs.edi = edi; // part of name buffer
        return do_command(30, regs) != FAILURE;
    }

    // Change company owner name
    inline bool do_31(uint16_t cx, uint16_t ax, uint32_t edx, uint32_t ebp, uint32_t edi)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.cx = cx;   // company id
        regs.ax = ax;   // [ 0, 1, 2]
        regs.edx = edx; // part of name buffer
        regs.ebp = ebp; // part of name buffer
        regs.edi = edi; // part of name buffer
        return do_command(31, regs) != FAILURE;
    }

    // Rename town
    inline void do_46(uint16_t cx, uint16_t ax, uint32_t edx, uint32_t ebp, uint32_t edi)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.cx = cx;   // town number or 0
        regs.ax = ax;   // [ 0, 1, 2]
        regs.edx = edx; // part of name buffer
        regs.ebp = ebp; // part of name buffer
        regs.edi = edi; // part of name buffer
        do_command(46, regs);
    }

    // Remove industry
    inline bool do_48(uint8_t industryId)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.dx = industryId;
        return do_command(48, regs) != FAILURE;
    }

    // Remove town
    inline bool do_50(uint8_t townId)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.edi = townId;
        return do_command(50, regs) != FAILURE;
    }

    // Build company headquarters
    inline void do_55(uint8_t bl, uint16_t ax, uint16_t cx, uint16_t di)
    {
        registers regs;
        regs.bl = bl; // flags
        regs.cx = cx; // x?
        regs.ax = ax; // y?
        regs.di = di; // z?
        do_command(55, regs);
    }

    // Change company face
    inline bool do_65(const objectmgr::header& object, uint8_t company)
    {
        auto objPtr = reinterpret_cast<const int32_t*>(&object);
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.eax = *objPtr++;
        regs.ecx = *objPtr++;
        regs.edx = *objPtr++;
        regs.edi = *objPtr;
        regs.bh = company;
        return do_command(65, regs) != FAILURE;
    }

    // Clear Land
    inline void do_66(map::map_pos centre, map::map_pos pointA, map::map_pos pointB, uint8_t flags)
    {
        registers regs;
        regs.ax = centre.x;
        regs.cx = centre.y;
        regs.edx = pointB.x << 16 | pointA.x;
        regs.ebp = pointB.y << 16 | pointA.y;
        regs.bl = flags;
        do_command(66, regs);
    }

    // Send chat message
    inline void do_71(int32_t ax, char* string)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.ax = ax;
        memcpy(&regs.ecx, &string[0], 4);
        memcpy(&regs.edx, &string[4], 4);
        memcpy(&regs.ebp, &string[8], 4);
        memcpy(&regs.edi, &string[12], 4);
        do_command(71, regs);
    }

    // Update owner status
    inline void do_73(thing_id_t id)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.ax = -2;
        regs.cx = id;
        do_command(73, regs);
    }

    // Update owner status
    inline void do_73(map::map_pos position)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.ax = position.x;
        regs.cx = position.y;
        do_command(73, regs);
    }

    // Rename industry
    inline void do_79(uint16_t cx, uint16_t ax, uint32_t edx, uint32_t ebp, uint32_t edi)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.cx = cx;   // industry number or 0
        regs.ax = ax;   // [ 0, 1, 2]
        regs.edx = edx; // part of name buffer
        regs.ebp = ebp; // part of name buffer
        regs.edi = edi; // part of name buffer
        do_command(79, regs);
    }
}
