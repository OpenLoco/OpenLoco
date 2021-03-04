#pragma once

#include "Interop/Interop.hpp"
#include "Map/Tile.h"
#include "Objects/ObjectManager.h"
#include "Ptr.h"
#include "Things/Thing.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Vehicles
{
    struct VehicleHead;
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
        vehicle_pass_signal = 4,
        vehicle_create = 5,
        vehicle_sell = 6,
        gc_unk_7 = 7,
        gc_unk_8 = 8,
        change_loan = 9,
        vehicle_rename = 10,
        change_station_name = 11,
        vehicle_local_express = 12,
        gc_unk_13 = 13,
        gc_unk_14 = 14,
        gc_unk_15 = 15,
        gc_unk_16 = 16,
        gc_unk_17 = 17,
        gc_unk_18 = 18,
        change_company_colour_scheme = 19,
        pause_game = 20,
        load_save_quit_game = 21,
        gc_unk_22 = 22,
        gc_unk_23 = 23,
        change_land_material = 24,
        raise_land = 25,
        lower_land = 26,
        lower_raise_land_mountain = 27,
        raise_water = 28,
        lower_water = 29,
        change_company_name = 30,
        change_company_owner_name = 31,
        gc_unk_32 = 32,
        gc_unk_33 = 33,
        gc_unk_34 = 34,
        vehicle_order_insert = 35,
        vehicle_order_delete = 36,
        vehicle_order_skip = 37,
        gc_unk_38 = 38,
        gc_unk_39 = 39,
        gc_unk_40 = 40,
        gc_unk_41 = 41,
        gc_unk_42 = 42,
        gc_unk_43 = 43,
        gc_unk_44 = 44,
        gc_unk_45 = 45,
        change_town_name = 46,
        gc_unk_47 = 47,
        remove_industry = 48,
        gc_unk_49 = 49,
        remove_town = 50,
        gc_unk_51 = 51,
        gc_unk_52 = 52,
        gc_unk_53 = 53,
        remove_company_headquarters = 54,
        build_company_headquarters = 55,
        gc_unk_56 = 56,
        gc_unk_57 = 57,
        gc_unk_58 = 58,
        vehicle_abort_pickup_air = 59,
        gc_unk_60 = 60,
        gc_unk_61 = 61,
        gc_unk_62 = 62,
        vehicle_abort_pickup_water = 63,
        vehicle_refit = 64,
        change_company_face = 65,
        clear_land = 66,
        load_multiplayer_map = 67,
        gc_unk_68 = 68,
        gc_unk_69 = 69,
        gc_unk_70 = 70,
        send_chat_message = 71,
        multiplayer_save = 72,
        update_owner_status = 73,
        vehicle_speed_control = 74,
        vehicle_order_up = 75,
        vehicle_order_down = 76,
        vehicle_apply_shunt_cheat = 77,
        apply_free_cash_cheat = 78,
        rename_industry = 79,
        vehicle_clone = 80,
    };

    constexpr uint32_t FAILURE = 0x80000000;

    void registerHooks();
    uint32_t doCommand(GameCommand command, const registers& registers);
    bool sub_431E6A(const company_id_t company, Map::tile_element* const tile = nullptr);

    inline void do_0(thing_id_t source, thing_id_t dest)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.dx = source;
        regs.di = dest;
        doCommand(GameCommand::vehicle_rearrange, regs);
    }

    inline bool do_2(thing_id_t head)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply | GameCommandFlag::flag_3 | GameCommandFlag::flag_6;
        regs.di = head;
        return doCommand(GameCommand::vehicle_pickup, regs) != FAILURE;
    }

    // Reverse (vehicle)
    inline void do_3(thing_id_t vehicleHead, Vehicles::VehicleHead* const head)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.dx = vehicleHead;
        // Bug in game command 3 requires to set edi to a vehicle prior to calling
        regs.edi = ToInt(head);

        doCommand(GameCommand::vehicle_reverse, regs);
    }

    // Pass signal (vehicle)
    inline void do_4(thing_id_t vehicleHead)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.di = vehicleHead;

        doCommand(GameCommand::vehicle_pass_signal, regs);
    }

    // Build vehicle
    inline uint32_t do_5(uint16_t vehicle_type, uint16_t vehicle_id = 0xFFFF)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.di = vehicle_id;
        regs.edx = vehicle_type;

        return doCommand(GameCommand::vehicle_create, regs);
    }

    // Build vehicle
    inline uint32_t queryDo_5(uint16_t vehicle_type, uint16_t vehicle_id = 0xFFFF)
    {
        registers regs;
        regs.bl = 0;
        regs.di = vehicle_id;
        regs.edx = vehicle_type;

        return doCommand(GameCommand::vehicle_create, regs);
    }

    inline void do_6(thing_id_t car)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.dx = car;
        doCommand(GameCommand::vehicle_sell, regs);
    }

    // Change loan
    inline void do_9(currency32_t newLoan)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.edx = newLoan;
        doCommand(GameCommand::change_loan, regs);
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
        doCommand(GameCommand::vehicle_rename, regs);
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
        doCommand(GameCommand::change_station_name, regs);
    }

    inline void do12(thing_id_t head, uint8_t bh)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.bh = bh;
        regs.dx = head;
        doCommand(GameCommand::vehicle_local_express, regs);
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

        doCommand(GameCommand::change_company_colour_scheme, regs);
    }

    // Pause game
    inline void do_20()
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        doCommand(GameCommand::pause_game, regs);
    }

    // Load/save/quit game
    inline void do_21(uint8_t dl, uint8_t di)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.dl = dl; // [ 0, 2 ]
        regs.di = di; // [ 0 = load game, 1 = return to title screen, 2 = quit to desktop ]
        doCommand(GameCommand::load_save_quit_game, regs);
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
        doCommand(GameCommand::change_land_material, regs);
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
        return doCommand(GameCommand::raise_land, regs);
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
        return doCommand(GameCommand::lower_land, regs);
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
        return doCommand(GameCommand::lower_raise_land_mountain, regs);
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
        return doCommand(GameCommand::raise_water, regs);
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
        return doCommand(GameCommand::lower_water, regs);
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
        return doCommand(GameCommand::change_company_name, regs) != FAILURE;
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
        return doCommand(GameCommand::change_company_owner_name, regs) != FAILURE;
    }

    inline bool do_35(thing_id_t head, uint64_t rawOrder, uint32_t orderOffset)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.eax = rawOrder & 0xFFFFFFFF;
        regs.cx = rawOrder >> 32;
        regs.di = head;
        regs.edx = orderOffset;
        return doCommand(GameCommand::vehicle_order_insert, regs);
    }

    inline bool do_36(thing_id_t head, uint32_t orderOffset)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.di = head;
        regs.edx = orderOffset;
        return doCommand(GameCommand::vehicle_order_delete, regs);
    }

    inline bool do_37(thing_id_t head)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.di = head;
        return doCommand(GameCommand::vehicle_order_skip, regs);
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
        doCommand(GameCommand::change_town_name, regs);
    }

    // Remove industry
    inline bool do_48(uint8_t industryId)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.dx = industryId;
        return doCommand(GameCommand::remove_industry, regs) != FAILURE;
    }

    // Remove town
    inline bool do_50(uint8_t townId)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.edi = townId;
        return doCommand(GameCommand::remove_town, regs) != FAILURE;
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
        return doCommand(GameCommand::remove_company_headquarters, regs);
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
        doCommand(GameCommand::build_company_headquarters, regs);
    }

    inline bool do_59(thing_id_t head)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply | GameCommandFlag::flag_3 | GameCommandFlag::flag_6;
        regs.di = head;
        return doCommand(GameCommand::vehicle_abort_pickup_air, regs) != FAILURE;
    }

    inline bool do_63(thing_id_t head)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply | GameCommandFlag::flag_3 | GameCommandFlag::flag_6;
        regs.di = head;
        return doCommand(GameCommand::vehicle_abort_pickup_water, regs) != FAILURE;
    }

    // Refit vehicle
    inline void do_64(thing_id_t vehicleHead, uint16_t option)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.di = vehicleHead;
        regs.dx = option;
        doCommand(GameCommand::vehicle_refit, regs);
    }

    // Change company face
    inline bool do_65(const ObjectHeader& object, uint8_t company)
    {
        auto objPtr = reinterpret_cast<const int32_t*>(&object);
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.eax = *objPtr++;
        regs.ecx = *objPtr++;
        regs.edx = *objPtr++;
        regs.edi = *objPtr;
        regs.bh = company;
        return doCommand(GameCommand::change_company_face, regs) != FAILURE;
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
        doCommand(GameCommand::clear_land, regs);
    }

    // Load multiplayer map
    inline void do_67(const char* filename)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.ebp = ToInt(filename);
        doCommand(GameCommand::load_multiplayer_map, regs);
    }

    // Multiplayer-related
    inline void do_69()
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        doCommand(GameCommand::gc_unk_69, regs);
    }

    // Multiplayer-related
    inline void do_70()
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        doCommand(GameCommand::gc_unk_70, regs);
    }

    // Send chat message
    inline void do_71(int32_t ax, const char* string)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.ax = ax;
        memcpy(&regs.ecx, &string[0], 4);
        memcpy(&regs.edx, &string[4], 4);
        memcpy(&regs.ebp, &string[8], 4);
        memcpy(&regs.edi, &string[12], 4);
        doCommand(GameCommand::send_chat_message, regs);
    }

    // Multiplayer save
    inline void do_72()
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        doCommand(GameCommand::multiplayer_save, regs);
    }

    // Update owner status
    inline void do_73(thing_id_t id)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.ax = -2;
        regs.cx = id;
        doCommand(GameCommand::update_owner_status, regs);
    }

    // Update owner status
    inline void do_73(Map::map_pos position)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.ax = position.x;
        regs.cx = position.y;
        doCommand(GameCommand::update_owner_status, regs);
    }

    inline uint32_t do_74(thing_id_t head, int16_t speed)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.cx = head;
        regs.dx = speed;
        return doCommand(GameCommand::vehicle_speed_control, regs);
    }

    inline uint32_t do_75(thing_id_t head, uint32_t orderOffset)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.di = head;
        regs.edx = orderOffset;
        return doCommand(GameCommand::vehicle_order_up, regs);
    }

    inline uint32_t do_76(thing_id_t head, uint32_t orderOffset)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.di = head;
        regs.edx = orderOffset;
        return doCommand(GameCommand::vehicle_order_down, regs);
    }

    inline void do_77(thing_id_t head)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.cx = head;
        doCommand(GameCommand::vehicle_apply_shunt_cheat, regs);
    }

    inline void do_78()
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        GameCommands::doCommand(GameCommand::apply_free_cash_cheat, regs);
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
        doCommand(GameCommand::rename_industry, regs);
    }

    inline bool do_80(uint16_t head)
    {
        registers regs;
        regs.bl = GameCommandFlag::apply;
        regs.ax = head;
        return GameCommands::doCommand(GameCommand::vehicle_clone, regs) != FAILURE;
    }
}
