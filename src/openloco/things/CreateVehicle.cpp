#include "../company.h"
#include "../companymgr.h"
#include "../date.h"
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
    static loco_global<company_id_t, 0x009C68EB> _updating_company_id;
    static loco_global<uint16_t, 0x009C68E0> gameCommandMapX;
    static loco_global<uint16_t, 0x009C68E2> gameCommandMapY;
    static loco_global<uint16_t, 0x009C68E4> gameCommandMapZ;
    static loco_global<string_id, 0x009C68E6> gGameCommandErrorText;
    static loco_global<uint8_t, 0x009C68EA> gGameCommandExpenditureType; // multiplied by 4
    static loco_global<uint8_t, 0x009C68EE> _errorCompanyId;
    static loco_global<map::tile_element*, 0x009C68D0> _9C68D0;
    static loco_global<ColourScheme, 0x01136140> _1136140; // primary colour
    static loco_global<openloco::vehicle*, 0x01136240> _backupVeh0;
    static loco_global<int16_t, 0x01136248> _backup2E;
    static loco_global<int16_t, 0x0113624C> _backup2C;
    static loco_global<int16_t, 0x01136250> _backupX;
    static loco_global<int16_t, 0x01136254> _backupY;
    static loco_global<uint8_t, 0x01136258> _backupZ;
    static loco_global<uint32_t[32], 0x00525E5E> currencyMultiplicationFactor;

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

    // 0x004B8FA2
    bool isVehicleTypeCompatible(openloco::vehicle* const veh0, const uint16_t vehicleTypeId)
    {
        auto newObject = objectmgr::get<vehicle_object>(vehicleTypeId);
        if (newObject->mode == TransportMode::air || newObject->mode == TransportMode::water)
        {
            auto veh = veh0->next_car()->next_car()->next_car();
            if (veh->type != vehicle_thing_type::vehicle_6)
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

    static void sub_4B08DD(openloco::vehicle* const veh0)
    {
        registers regs{};
        regs.esi = reinterpret_cast<uint32_t>(veh0);
        call(0x004B08DD, regs);
    }

    // 0x00470039
    static openloco::vehicle* createVehicleThing()
    {
        registers regs{};
        call(0x00470039, regs);
        return reinterpret_cast<openloco::vehicle*>(regs.esi);
    }

    static void sub_4BA873(openloco::vehicle_bogie* const vehBogie)
    {
        vehBogie->var_68 = 0xFFFF;
        if (vehBogie->reliability != 0)
        {
            int32_t reliabilityFactor = vehBogie->reliability / 256;
            reliabilityFactor *= reliabilityFactor;
            reliabilityFactor /= 16;

            auto& prng = gprng();
            int32_t randVal = (prng.rand_next(65535) * reliabilityFactor / 2) / 256;
            reliabilityFactor -= reliabilityFactor / 4;
            reliabilityFactor += randVal;
            vehBogie->var_68 = static_cast<uint16_t>(std::min(4, reliabilityFactor));
        }
    }

    // 0x4AE8F1
    static openloco::vehicle_bogie* createBogie(const thing_id_t head, const uint16_t vehicleTypeId, const vehicle_object& vehObject, const uint8_t bodyNumber, openloco::vehicle* const lastVeh, const ColourScheme colourScheme)
    {
        auto newThing = createVehicleThing();
        newThing->base_type = thing_base_type::vehicle;
        newThing->type = vehicle_thing_type::vehicle_bogie;
        newThing->owner = _updating_company_id;

        auto newBogie = newThing->as_vehicle_bogie();

        newBogie->head = head;
        newBogie->body_index = bodyNumber;
        newBogie->track_type = lastVeh->track_type;
        newBogie->mode = lastVeh->mode;
        newBogie->tile_x = -1;
        newBogie->tile_y = 0;
        newBogie->tile_base_z = 0;
        newBogie->var_2E = 0;
        newBogie->var_2C = 0;
        newBogie->var_36 = lastVeh->var_36;
        newBogie->object_id = vehicleTypeId;

        auto& prng = gprng();
        newBogie->var_44 = prng.rand_next();
        newBogie->creation_day = current_day();
        newBogie->var_46 = 0;
        newBogie->var_47 = 0;
        newBogie->accepted_cargo_types = 0;
        newBogie->cargo_type = 0xFF;
        newBogie->var_51 = 0;
        newBogie->var_5E = 0;
        newBogie->var_5F = 0;
        newBogie->var_60 = 0; // different
        newBogie->var_61 = 0; // different

        newBogie->object_sprite_type = vehObject.var_24[bodyNumber].spriteType; //different
        newBogie->var_14 = 1;
        newBogie->var_09 = 1;
        newBogie->var_15 = 1;
        if (newBogie->object_sprite_type != 0xFF) // different
        {
            newBogie->var_14 = vehObject.var_B4[newBogie->object_sprite_type].var_02;
            newBogie->var_09 = vehObject.var_B4[newBogie->object_sprite_type].var_03;
            newBogie->var_15 = vehObject.var_B4[newBogie->object_sprite_type].var_04;
        }

        newBogie->colour_scheme = colourScheme;
        lastVeh->next_car_id = newBogie->id;
        return newBogie;
    }

    // 0x4AE8F1
    static openloco::vehicle_bogie* createFirstBogie(const thing_id_t head, const uint16_t vehicleTypeId, const vehicle_object& vehObject, const uint8_t bodyNumber, openloco::vehicle* const lastVeh, const ColourScheme colourScheme)
    {
        auto newBogie = createBogie(head, vehicleTypeId, vehObject, bodyNumber, lastVeh, colourScheme);
        newBogie->var_38 = 0;

        int32_t reliability = vehObject.reliability * 256;
        if (current_year() + 2 > vehObject.designed)
        {
            reliability -= reliability / 8;
            if (current_year() + 3 > vehObject.designed)
            {
                reliability -= reliability / 8;
            }
        }
        if (reliability != 0)
        {
            reliability += 255;
        }
        newBogie->reliability = reliability;
        sub_4BA873(newBogie);

        auto cost = (vehObject.cost_fact * currencyMultiplicationFactor[vehObject.cost_ind]) / 64;
        newBogie->var_62 = cost - cost / 8;

        if (bodyNumber == 0)
        {
            if (vehObject.num_simultaneous_cargo_types >= 2)
            {
                newBogie->max_cargo = vehObject.max_secondary_cargo;
                newBogie->accepted_cargo_types = vehObject.secondary_cargo_types;
                auto cargoType = utility::bitscanforward(newBogie->accepted_cargo_types);
                if (cargoType != -1)
                {
                    newBogie->cargo_type = cargoType;
                }
            }
        }
        return newBogie;
    }

    // 0x004AEA9E
    static openloco::vehicle_bogie* createSecondBogie(const thing_id_t head, const uint16_t vehicleTypeId, const vehicle_object& vehObject, const uint8_t bodyNumber, openloco::vehicle* const lastVeh, const ColourScheme colourScheme)
    {
        auto newBogie = createBogie(head, vehicleTypeId, vehObject, bodyNumber, lastVeh, colourScheme);
        newBogie->var_38 = (1 << 1);
        return newBogie;
    }

    // 0x004AEA9E
    static openloco::vehicle_body* createBody(const thing_id_t head, const uint16_t vehicleTypeId, const vehicle_object& vehObject, const uint8_t bodyNumber, openloco::vehicle* const lastVeh, const ColourScheme colourScheme)
    {
        auto newThing = createVehicleThing();
        newThing->base_type = thing_base_type::vehicle;
        newThing->type = bodyNumber == 0 ? vehicle_thing_type::vehicle_body_end : vehicle_thing_type::vehicle_body_cont;
        newThing->owner = _updating_company_id;

        auto newBody = newThing->as_vehicle_body();

        newBody->head = head;
        newBody->body_index = bodyNumber;
        newBody->track_type = lastVeh->track_type;
        newBody->mode = lastVeh->mode;
        newBody->tile_x = -1;
        newBody->tile_y = 0;
        newBody->tile_base_z = 0;
        newBody->var_2E = 0;
        newBody->var_2C = 0;
        newBody->var_36 = lastVeh->var_36;
        newBody->var_38 = (1 << 0); // different
        newBody->object_id = vehicleTypeId;

        auto& prng = gprng();
        newBody->var_44 = prng.rand_next();
        newBody->creation_day = current_day();
        newBody->var_46 = 0;
        newBody->var_47 = 0;
        newBody->accepted_cargo_types = 0;
        newBody->cargo_type = 0xFF;
        newBody->var_51 = 0;
        newBody->var_55 = 0; // different
        newBody->var_5E = 0;
        newBody->var_5F = 0;

        if (bodyNumber == 0)
        {
            if (vehObject.num_simultaneous_cargo_types >= 1)
            {
                newBody->max_cargo = vehObject.max_primary_cargo;
                newBody->accepted_cargo_types = vehObject.primary_cargo_types;
                auto cargoType = utility::bitscanforward(newBody->accepted_cargo_types);
                if (cargoType != -1)
                {
                    newBody->cargo_type = cargoType;
                }
            }
        }

        newBody->var_14 = 1;
        newBody->var_09 = 1;
        newBody->var_15 = 1;

        // different onwards
        auto spriteType = vehObject.var_24[bodyNumber].spriteType;
        if (spriteType != 0xFF)
        {
            if (spriteType & (1 << 7))
            {
                newBody->var_38 |= (1 << 1);
                spriteType &= ~(1 << 7);
            }
        }
        newBody->object_sprite_type = spriteType;

        if (newBody->object_sprite_type != 0xFF)
        {
            newBody->var_14 = vehObject.sprites[newBody->object_sprite_type].var_08;
            newBody->var_09 = vehObject.sprites[newBody->object_sprite_type].var_09;
            newBody->var_15 = vehObject.sprites[newBody->object_sprite_type].var_0A;
        }

        newBody->colour_scheme = colourScheme; // same

        if (bodyNumber == 0 && vehObject.flags & flags_E0::flag_02)
        {
            newBody->var_38 |= (1 << 3);
        }

        if (bodyNumber + 1 == vehObject.var_04 && vehObject.flags & flags_E0::flag_03)
        {
            newBody->var_38 |= (1 << 3);
        }

        lastVeh->next_car_id = newBody->id; // same
        return newBody;
    }

    static void sub_4B7CC3(openloco::vehicle* const veh0)
    {
        registers regs{};
        regs.esi = reinterpret_cast<int32_t>(veh0);
        call(0x004B7CC3, regs);
    }

    // 0x004AE86D
    static bool createBody(openloco::vehicle* const veh0, const uint16_t vehicleTypeId)
    {
        if (!checkNumFreeThings(12))
        {
            return false;
        }

        openloco::vehicle* lastVeh = veh0;   // will be of type vehicle_body_end at end of loop
        openloco::vehicle* endVeh = lastVeh; // will be of type vehicle_6 at end of loop
        for (; endVeh->type != vehicle_thing_type::vehicle_6; endVeh = lastVeh->next_car())
        {
            lastVeh = lastVeh->next_car();
        }

        const auto vehObject = objectmgr::get<vehicle_object>(vehicleTypeId);
        const auto company = companymgr::get(_updating_company_id);
        _1136140 = company->mainColours;
        auto colourScheme = company->mainColours;
        if (company->customVehicleColoursSet & (1 << vehObject->colour_type))
        {
            _1136140 = company->vehicleColours[vehObject->colour_type - 1];
            colourScheme = company->vehicleColours[vehObject->colour_type - 1];
        }

        openloco::vehicle_bogie* newCarStart = nullptr;
        for (auto bodyNumber = 0; bodyNumber < vehObject->var_04; ++bodyNumber)
        {
            auto* const firstBogie = createFirstBogie(veh0->id, vehicleTypeId, *vehObject, bodyNumber, lastVeh, colourScheme);

            lastVeh = reinterpret_cast<openloco::vehicle*>(firstBogie);

            auto* const secondBogie = createSecondBogie(veh0->id, vehicleTypeId, *vehObject, bodyNumber, lastVeh, colourScheme);
            lastVeh = reinterpret_cast<openloco::vehicle*>(secondBogie);

            auto* const body = createBody(veh0->id, vehicleTypeId, *vehObject, bodyNumber, lastVeh, colourScheme);
            lastVeh = reinterpret_cast<openloco::vehicle*>(body);

            if (newCarStart == nullptr)
            {
                newCarStart = firstBogie;
            }
        }

        lastVeh->next_car_id = endVeh->id;
        sub_4B7CC3(veh0);
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
        _backupVeh0 = reinterpret_cast<openloco::vehicle*>(-1);
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

            if (flags & game_commands::GameCommandFlag::apply)
            {
                if (veh0->tile_x != -1)
                {
                    _backupX = veh0->tile_x;
                    _backupY = veh0->tile_y;
                    _backupZ = veh0->tile_base_z;
                    _backup2C = veh0->var_2C;
                    _backup2E = veh0->var_2E;
                    _backupVeh0 = veh0;
                    sub_4B08DD(veh0);
                }

                if (createBody(veh0, vehicleTypeId))
                {
                    registers regs{};
                    regs.edx = vehicleTypeId;
                    regs.esi = reinterpret_cast<int32_t>(veh0);
                    call(0x004AE6DE, regs);
                    return regs.ebx;
                }
                else
                {
                    registers regs{};
                    regs.edx = vehicleTypeId;
                    regs.esi = reinterpret_cast<int32_t>(veh0);
                    call(0x004AE809, regs);
                    return regs.ebx;
                }
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
