#include "../company.h"
#include "../game_commands.h"
#include "../management/Expenditures.h"
#include "../map/tile.h"
#include "../objects/objectmgr.h"
#include "../objects/vehicle_object.h"
#include "thingmgr.h"
#include "vehicle.h"
#include <utility>

using namespace openloco;
using namespace openloco::interop;
using namespace openloco::objectmgr;

namespace openloco::things::vehicle
{
    static loco_global<uint32_t, 0x01136240> _1136240;
    static loco_global<company_id_t, 0x009C68EB> _updating_company_id;
    static loco_global<uint16_t, 0x009C68E0> gameCommandMapX;
    static loco_global<uint16_t, 0x009C68E2> gameCommandMapY;
    static loco_global<uint16_t, 0x009C68E4> gameCommandMapZ;
    static loco_global<string_id, 0x009C68E6> gGameCommandErrorText;
    static loco_global<uint8_t, 0x009C68EA> gGameCommandExpenditureType; // multiplied by 4
    static loco_global<uint8_t, 0x009C68EE> _errorCompanyId;
    static loco_global<map::tile_element*, 0x009C68D0> _9C68D0;

    static bool sub_431E6A(const company_id_t company, map::tile_element* const tile)
    {
        if (company == company_id::neutral)
        {
            return true;
        }
        if (_updating_company_id == company || _updating_company_id == company_id::neutral)
        {
            return true;
        }
        gGameCommandErrorText = -2;
        _errorCompanyId = company;
        _9C68D0 = tile == nullptr ? reinterpret_cast<map::tile_element*>(-1) : tile;
        return false;
    }

    static bool sub_4B0BDD(openloco::vehicle* const veh0)
    {
        switch (veh0->var_5D)
        {
            case 8:
                gGameCommandErrorText = string_ids::vehicle_has_crashed;
                return false;
            case 9:
                gGameCommandErrorText = string_ids::vehicle_is_stuck;
                return false;
            case 7:
                gGameCommandErrorText = string_ids::vehicle_has_broken_down;
                return false;
            default:
            {
                auto veh1 = veh0->next_car();
                auto veh2 = veh1->next_car()->as_vehicle_2or6();
                if (veh0->vehicleType == VehicleType::plane || veh0->vehicleType == VehicleType::ship)
                {
                    if (veh2->var_73 & (1 << 0))
                    {
                        gGameCommandErrorText = string_ids::vehicle_has_broken_down;
                        return false;
                    }

                    if (veh0->tile_x == -1)
                    {
                        return true;
                    }

                    if (veh0->var_5D != 6 && veh0->var_5D != 1)
                    {
                        gGameCommandErrorText = string_ids::vehicle_must_be_stopped;
                        return false;
                    }
                    if (veh2->var_56 == 0)
                    {
                        return true;
                    }
                    gGameCommandErrorText = string_ids::vehicle_must_be_stopped;
                    return false;
                }
                else
                {

                    if (veh0->tile_x == -1)
                    {
                        return true;
                    }
                    if (veh2->var_56 == 0)
                    {
                        return true;
                    }
                    if (veh1->var_3C <= 13961)
                    {
                        return true;
                    }
                    gGameCommandErrorText = string_ids::vehicle_must_be_stopped;
                    return false;
                }
            }
        }
    }

    static bool sub_4B90F0(const uint16_t newVehicleTypeId, const uint16_t sourceVehicleTypeId)
    {
        auto newObject = objectmgr::get<vehicle_object>(newVehicleTypeId);       //edi
        auto sourceObject = objectmgr::get<vehicle_object>(sourceVehicleTypeId); // esi

        if ((newObject->flags & flags_E0::can_couple) && (sourceObject->flags & flags_E0::can_couple))
        {
            gGameCommandErrorText = string_ids::incompatible_vehicle;
            return false;
        }

        if (newVehicleTypeId == sourceVehicleTypeId)
        {
            return true;
        }

        if (newObject->num_compat != 0)
        {
            for (auto i = 0; i < newObject->num_compat; ++i)
            {
                if (newObject->compatible_vehicles[i] == sourceVehicleTypeId)
                {
                    return true;
                }
            }
        }

        if (sourceObject->num_compat != 0)
        {
            for (auto i = 0; i < sourceObject->num_compat; ++i)
            {
                if (sourceObject->compatible_vehicles[i] == newVehicleTypeId)
                {
                    return true;
                }
            }
        }

        if ((newObject->num_compat != 0) && (sourceObject->num_compat != 0))
        {
            gGameCommandErrorText = string_ids::incompatible_vehicle;
            return false;
        }

        return true;
    }

    // 0x004B9780
    // used by road vehicles only maybe??
    static uint32_t getVehicleTypeLength(const uint16_t vehicleTypeId)
    {
        auto vehObject = objectmgr::get<vehicle_object>(vehicleTypeId);
        auto length = 0;
        for (auto i = 0; i < vehObject->var_04; ++i)
        {
            if (vehObject->var_24[i].sprite_ind == 0xFF)
            {
                continue;
            }

            auto unk = vehObject->var_24[i].sprite_ind & 0x7F;
            length += vehObject->sprites[unk].bogey_position * 2;
        }
        return length;
    }

    // 0x004B97B7
    // used by road vehicles only maybe??
    static uint32_t getVehicleTotalLength(openloco::vehicle* const veh0)
    {
        auto totalLength = 0;
        auto veh = veh0->next_car()->next_car()->next_car();
        while (veh->type != vehicle_thing_type::vehicle_6)
        {
            if (veh->type == vehicle_thing_type::vehicle_body_end)
            {
                totalLength += getVehicleTypeLength(veh->object_id);
            }
            veh = veh->next_car();
        }
        return totalLength;
    }

    bool isVehicleTypeCompatible(openloco::vehicle* const veh0, const uint16_t vehicleTypeId)
    {
        auto newObject = objectmgr::get<vehicle_object>(vehicleTypeId);
        if (newObject->mode == TransportMode::air || newObject->mode == TransportMode::water)
        {
            auto veh26 = veh0->next_car()->next_car()->next_car()->as_vehicle_2or6();
            if (veh26->type != vehicle_thing_type::vehicle_6)
            {
                gGameCommandErrorText = string_ids::incompatible_vehicle;
                return false;
            }
        }
        else
        {
            if (newObject->track_type != veh0->track_type)
            {
                gGameCommandErrorText = string_ids::incompatible_vehicle;
                return false;
            }
        }

        if (newObject->mode != veh0->mode)
        {
            gGameCommandErrorText = string_ids::incompatible_vehicle;
            return false;
        }

        if (newObject->type != veh0->vehicleType)
        {
            gGameCommandErrorText = string_ids::incompatible_vehicle;
            return false;
        }

        {
            auto veh = veh0->next_car()->next_car()->next_car();
            if (veh->type != vehicle_thing_type::vehicle_6)
            {
                while (veh->type != vehicle_thing_type::vehicle_6)
                {
                    if (!sub_4B90F0(vehicleTypeId, veh->object_id))
                    {
                        return false;
                    }

                    vehicle_body* vehUnk = nullptr;
                    do
                    {
                        veh = veh->next_car()->next_car()->next_car();
                        if (veh->type == vehicle_thing_type::vehicle_6)
                        {
                            break;
                        }

                        vehUnk = veh->next_car()->next_car()->as_vehicle_body();
                    } while (vehUnk != nullptr && vehUnk->type == vehicle_thing_type::vehicle_body_cont);
                }
            }
        }
        if (veh0->mode != TransportMode::road)
        {
            return true;
        }

        if (veh0->track_type != 0xFF)
        {
            return true;
        }

        auto curTotalLength = getVehicleTotalLength(veh0);
        auto additionalNewLength = getVehicleTypeLength(vehicleTypeId);
        if (curTotalLength + additionalNewLength > 176)
        {
            gGameCommandErrorText = string_ids::vehicle_too_long;
            return false;
        }
        return true;
    }

    bool checkNumFreeThings(const size_t numNewThings)
    {
        if (thingmgr::getListCount(thingmgr::thing_list::null) <= numNewThings)
        {
            gGameCommandErrorText = string_ids::too_many_objects_in_game;
            return false;
        }
        return true;
    }

    // 0x004AE74E
    static uint32_t create(const uint8_t flags, const uint16_t vehicleTypeId)
    {
        registers regs{};
        regs.bl = game_commands::GameCommandFlag::apply;
        regs.edx = vehicleTypeId;
        call(0x4AE74E, regs);
        return regs.ebx;
    }

    // 0x004AE5E4
    uint32_t create(const uint8_t flags, const uint16_t vehicleTypeId, const uint16_t vehicleThingId)
    {
        gGameCommandExpenditureType = static_cast<uint8_t>(ExpenditureType::VehiclePurchases) * 4;
        _1136240 = -1;
        if (vehicleThingId == (uint16_t)-1)
        {
            return create(flags, vehicleTypeId);
        }
        else
        {
            auto veh0 = thingmgr::get<openloco::vehicle>(vehicleThingId);
            auto veh2 = veh0->next_car()->next_car()->as_vehicle_2or6();
            gameCommandMapX = veh2->x;
            gameCommandMapY = veh2->y;
            gameCommandMapZ = veh2->z;

            veh0->owner;
            if (!sub_431E6A(veh0->owner, nullptr))
            {
                return 0x80000000;
            }

            if (!sub_4B0BDD(veh0))
            {
                return 0x80000000;
            }

            if (!isVehicleTypeCompatible(veh0, vehicleTypeId))
            {
                return 0x80000000;
            }

            if (!checkNumFreeThings(12))
            {
                return 0x80000000;
            }
            // 0x4AE665
            registers regs{};
            regs.bl = flags;
            regs.di = vehicleThingId;
            regs.edx = vehicleTypeId;
            regs.esi = reinterpret_cast<uint32_t>(veh0);
            call(0x004AE665, regs);
            return regs.ebx;
        }
    }
}
