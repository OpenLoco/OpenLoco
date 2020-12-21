#pragma once

#include "Interop/Interop.hpp"
#include "Map/Tile.h"
#include "Objects/ObjectManager.h"
#include "Things/Thing.h"

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    struct vehicle_head;
}

namespace OpenLoco::GameCommands
{
    enum GameCommandFlag : uint8_t
    {
        apply = 1 << 0,  // 0x01
        flag_1 = 1 << 1, // 0x02
        flag_2 = 1 << 2, // 0x04
        flag_3 = 1 << 3, // 0x08
        flag_4 = 1 << 4, // 0x10
        flag_5 = 1 << 5, // 0x20
        flag_6 = 1 << 6, // 0x40
    };

    enum class GameCommand : uint8_t
    {
        vehicle_rearrange = 0,
        vehicle_place = 1,
        vehicle_pickup = 2,
        vehicle_reverse = 3,
        vehicle_create = 5,
        vehicle_sell = 6,
        build_vehicle = 9,
        vehicle_rename = 10,
        change_station_name = 11,
        vehicle_local_express = 12,
        change_company_colour_scheme = 19,
        pause_game = 20,
        load_save_quit_game = 21,
        change_land_material = 24,
        raise_land = 25,
        lower_land = 26,
        lower_raise_land_mountain = 27,
        raise_water = 28,
        lower_water = 29,
        vehicle_order_insert = 35,
        vehicle_order_delete = 36,
        vehicle_order_skip = 37,
        change_company_name = 46,
        remove_industry = 54,
        build_company_headquarters = 55,
        vehicle_abort_pickup_air = 59,
        vehicle_abort_pickup_water = 63,
        change_company_face = 66,
        load_multiplayer_map = 67,
        send_chat_message = 71,
        update_owner_status = 73,
        vehicle_speed_control = 74,
        vehicle_order_up = 75,
        vehicle_order_down = 76,
        vehicle_apply_shunt_cheat = 77,
        apply_free_cash_cheat = 78,
        rename_industry = 79,
    };

    constexpr uint32_t FAILURE = 0x80000000;

    void registerHooks();
    uint32_t doCommand(int esi, const registers& registers);
    bool sub_431E6A(const company_id_t company, Map::tile_element* const tile = nullptr);

    inline void do_0(thing_id_t source, thing_id_t dest)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.dx = source;
        regs.di = dest;
        doCommand(static_cast<int32_t>(GameCommand::vehicle_rearrange), regs);
    }

    inline bool do_2(thing_id_t head)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply | GameCommandFlag::flag_3 | GameCommandFlag::flag_6;
        regs.di = head;
        return doCommand(static_cast<int32_t>(GameCommand::vehicle_pickup), regs) != FAILURE;
    }

    // Reverse (vehicle)
    inline void do_3(thing_id_t vehicleHead, OpenLoco::vehicle_head* const head)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.dx = vehicleHead;
        // Bug in game command 3 requires to set edi to a vehicle prior to calling
        regs.edi = reinterpret_cast<uint32_t>(head);

        doCommand(static_cast<int32_t>(GameCommand::vehicle_reverse), regs);
    }

    // Pass signal (vehicle)
    inline void do_4(thing_id_t vehicleHead)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.di = vehicleHead;

        doCommand(4, regs);
    }

    // Build vehicle
    inline bool do_5(uint16_t vehicle_type, uint16_t vehicle_id = 0xFFFF)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.di = vehicle_id;
        regs.edx = vehicle_type;

        return doCommand(5, regs) != FAILURE;
    }

    inline void do_6(thing_id_t car)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.dx = car;
        doCommand(static_cast<int32_t>(GameCommand::vehicle_sell), regs);
    }

    // Change loan
    inline void do_9(currency32_t newLoan)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.edx = newLoan;
        doCommand(9, regs);
    }

    // Change vehicle name
    inline void do_10(thing_id_t head, uint16_t i, uint32_t edx, uint32_t ebp, uint32_t edi)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.cx = head; // vehicle head id
        regs.ax = i;    // [ 0, 1, 2]
        regs.edx = edx; // part of name buffer
        regs.ebp = ebp; // part of name buffer
        regs.edi = edi; // part of name buffer
        doCommand(static_cast<int32_t>(GameCommand::vehicle_rename), regs);
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
        doCommand(11, regs);
    }

    inline void do12(thing_id_t head, uint8_t bh)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.bh = bh;
        regs.dx = head;
        doCommand(12, regs);
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

        doCommand(19, regs);
    }

    // Pause game
    inline void do_20()
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        doCommand(20, regs);
    }

    // Load/save/quit game
    inline void do_21(uint8_t dl, uint8_t di)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.dl = dl; // [ 0, 2 ]
        regs.di = di; // [ 0 = load game, 1 = return to title screen, 2 = quit to desktop ]
        doCommand(21, regs);
    }

    // Change Land Material
    inline void do_24(Map::map_pos pointA, Map::map_pos pointB, uint8_t landType, uint8_t flags)
    {
        registers regs;
        regs.ax = pointA.x;
        regs.cx = pointA.y;
        regs.di = pointB.x;
        regs.bp = pointB.y;
        regs.dl = landType;
        regs.bl = flags;
        doCommand(24, regs);
    }

    // Raise Land
    inline uint32_t do_25(Map::map_pos centre, Map::map_pos pointA, Map::map_pos pointB, uint16_t di, uint8_t flags)
    {
        registers regs;
        regs.ax = centre.x;
        regs.cx = centre.y;
        regs.edx = pointB.x << 16 | pointA.x;
        regs.ebp = pointB.y << 16 | pointA.y;
        regs.bl = flags;
        regs.di = di;
        return doCommand(25, regs);
    }

    // Lower Land
    inline uint32_t do_26(Map::map_pos centre, Map::map_pos pointA, Map::map_pos pointB, uint16_t di, uint8_t flags)
    {
        registers regs;
        regs.ax = centre.x;
        regs.cx = centre.y;
        regs.edx = pointB.x << 16 | pointA.x;
        regs.ebp = pointB.y << 16 | pointA.y;
        regs.bl = flags;
        regs.di = di;
        return doCommand(26, regs);
    }

    // Lower/Raise Land Mountain
    inline uint32_t do_27(Map::map_pos centre, Map::map_pos pointA, Map::map_pos pointB, uint16_t di, uint8_t flags)
    {
        registers regs;
        regs.ax = centre.x;
        regs.cx = centre.y;
        regs.edx = pointB.x << 16 | pointA.x;
        regs.ebp = pointB.y << 16 | pointA.y;
        regs.bl = flags;
        regs.di = di;
        return doCommand(27, regs);
    }

    // Raise Water
    inline uint32_t do_28(Map::map_pos pointA, Map::map_pos pointB, uint8_t flags)
    {
        registers regs;
        regs.ax = pointA.x;
        regs.cx = pointA.y;
        regs.di = pointB.x;
        regs.bp = pointB.y;
        regs.bl = flags;
        return doCommand(28, regs);
    }

    // Lower Water
    inline uint32_t do_29(Map::map_pos pointA, Map::map_pos pointB, uint8_t flags)
    {
        registers regs;
        regs.ax = pointA.x;
        regs.cx = pointA.y;
        regs.di = pointB.x;
        regs.bp = pointB.y;
        regs.bl = flags;
        return doCommand(29, regs);
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
        return doCommand(30, regs) != FAILURE;
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
        return doCommand(31, regs) != FAILURE;
    }

    inline bool do_35(thing_id_t head, uint8_t order, uint64_t orderArgument, uint32_t orderOffset)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.eax = order | (orderArgument << 3);
        regs.cx = orderArgument >> 32;
        regs.di = head;
        regs.edx = orderOffset;
        return doCommand(static_cast<int32_t>(GameCommand::vehicle_order_insert), regs);
    }

    inline bool do_36(thing_id_t head, uint32_t orderOffset)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.di = head;
        regs.edx = orderOffset;
        return doCommand(static_cast<int32_t>(GameCommand::vehicle_order_delete), regs);
    }

    inline bool do_37(thing_id_t head)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.di = head;
        return doCommand(static_cast<int32_t>(GameCommand::vehicle_order_skip), regs);
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
        doCommand(46, regs);
    }

    // Remove industry
    inline bool do_48(uint8_t industryId)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.dx = industryId;
        return doCommand(48, regs) != FAILURE;
    }

    // Remove town
    inline bool do_50(uint8_t townId)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.edi = townId;
        return doCommand(50, regs) != FAILURE;
    }

    // Remove company headquarters (or build - needs to be checked)
    // Note: The game seems to call do_55, then do_54 in success case and returns 0x0 code
    //       if we try to build over the existing only do_54 is called and it returns 0x80000000 (in regs.ebx)
    inline uint32_t do_54(uint8_t bl, uint16_t ax, uint16_t cx, uint16_t di, uint16_t dx)
    {
        registers regs;
        regs.bl = bl; // flags
        regs.cx = cx; // x
        regs.ax = ax; // y
        regs.di = di; // z
        regs.dx = dx; // company index (value 1 in testing case)
        return doCommand(54, regs);
    }

    // Build company headquarters (or remove - needs to be checked)
    // Note: The game seems to call do_55, then do_54 in success case
    //       if we try to build over the existing one do_55 is not called
    inline void do_55(uint8_t bl, uint16_t ax, uint16_t cx, uint16_t di)
    {
        registers regs;
        regs.bl = bl; // flags
        regs.cx = cx; // x?
        regs.ax = ax; // y?
        regs.di = di; // z?
        doCommand(55, regs);
    }

    inline bool do_59(thing_id_t head)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply | GameCommandFlag::flag_3 | GameCommandFlag::flag_6;
        regs.di = head;
        return doCommand(static_cast<int32_t>(GameCommand::vehicle_abort_pickup_air), regs) != FAILURE;
    }

    inline bool do_63(thing_id_t head)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply | GameCommandFlag::flag_3 | GameCommandFlag::flag_6;
        regs.di = head;
        return doCommand(static_cast<int32_t>(GameCommand::vehicle_abort_pickup_water), regs) != FAILURE;
    }

    // Refit vehicle
    inline void do_64(thing_id_t vehicleHead, uint16_t option)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.di = vehicleHead;
        regs.dx = option;
        doCommand(64, regs);
    }

    // Change company face
    inline bool do_65(const ObjectManager::header& object, uint8_t company)
    {
        auto objPtr = reinterpret_cast<const int32_t*>(&object);
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.eax = *objPtr++;
        regs.ecx = *objPtr++;
        regs.edx = *objPtr++;
        regs.edi = *objPtr;
        regs.bh = company;
        return doCommand(65, regs) != FAILURE;
    }

    // Clear Land
    inline void do_66(Map::map_pos centre, Map::map_pos pointA, Map::map_pos pointB, uint8_t flags)
    {
        registers regs;
        regs.ax = centre.x;
        regs.cx = centre.y;
        regs.edx = pointB.x << 16 | pointA.x;
        regs.ebp = pointB.y << 16 | pointA.y;
        regs.bl = flags;
        doCommand(66, regs);
    }

    // Load multiplayer map
    inline void do_67(const char* filename)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.ebp = reinterpret_cast<int32_t>(filename);
        doCommand(67, regs);
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
        doCommand(71, regs);
    }

    // Update owner status
    inline void do_73(thing_id_t id)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.ax = -2;
        regs.cx = id;
        doCommand(73, regs);
    }

    // Update owner status
    inline void do_73(Map::map_pos position)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.ax = position.x;
        regs.cx = position.y;
        doCommand(73, regs);
    }

    inline uint32_t do_74(thing_id_t head, int16_t speed)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.cx = head;
        regs.dx = speed;
        return doCommand(static_cast<int32_t>(GameCommand::vehicle_speed_control), regs);
    }

    inline uint32_t do_75(thing_id_t head, uint32_t orderOffset)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.di = head;
        regs.edx = orderOffset;
        return doCommand(static_cast<int32_t>(GameCommand::vehicle_order_up), regs);
    }

    inline uint32_t do_76(thing_id_t head, uint32_t orderOffset)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.di = head;
        regs.edx = orderOffset;
        return doCommand(static_cast<int32_t>(GameCommand::vehicle_order_down), regs);
    }

    inline void do_77(thing_id_t head)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.cx = head;
        doCommand(static_cast<int32_t>(GameCommand::vehicle_apply_shunt_cheat), regs);
    }

    inline void do_78()
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        GameCommands::doCommand(static_cast<int32_t>(GameCommand::apply_free_cash_cheat), regs);
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
        doCommand(79, regs);
    }
}
