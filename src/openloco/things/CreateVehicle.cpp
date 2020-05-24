#include "../company.h"
#include "../companymgr.h"
#include "../core/Optional.hpp"
#include "../date.h"
#include "../game_commands.h"
#include "../management/Expenditures.h"
#include "../map/tile.h"
#include "../objects/objectmgr.h"
#include "../objects/road_object.h"
#include "../objects/track_object.h"
#include "../objects/vehicle_object.h"
#include "../ui/WindowManager.h"
#include "thingmgr.h"
#include "vehicle.h"
#include <numeric>
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
    static loco_global<int32_t, 0x011360FC> _11360FC;
    static loco_global<openloco::vehicle*, 0x01136240> _backupVeh0;
    static loco_global<int16_t, 0x01136248> _backup2E;
    static loco_global<int16_t, 0x0113624C> _backup2C;
    static loco_global<int16_t, 0x01136250> _backupX;
    static loco_global<int16_t, 0x01136254> _backupY;
    static loco_global<uint8_t, 0x01136258> _backupZ;
    static loco_global<uint16_t, 0x0113642A> _113642A; // used by build window and others
    static loco_global<uint32_t[32], 0x00525E5E> currencyMultiplicationFactor;
    static loco_global<uint8_t, 0x00525FC5> _525FC5;
    static loco_global<uint32_t, 0x00525FB8> _525FB8;
    static loco_global<thing_id_t[64 * 1000], 0x0096885C> _96885C;
    static loco_global<uint8_t[256000], 0x00987C5C> _987C5C;

    // 0x004B1D96
    static bool checkMaxAIVehicles()
    {
        if (is_player_company(_updating_company_id))
        {
            return true;
        }

        const auto& companies = companymgr::companies();
        auto totalAiVehicles = std::accumulate(companies.begin(), companies.end(), 0, [](int32_t& total, const auto& company) {
            if (is_player_company(company.id()))
                return 0;
            return std::accumulate(std::begin(company.transportTypeCount), std::end(company.transportTypeCount), 0);
        });

        if (totalAiVehicles > 500)
        {
            gGameCommandErrorText = string_ids::too_many_vehicles;
            return false;
        }

        return true;
    }

    // 0x004B1E44
    static bool checkMaxVehicles()
    {
        if (!checkMaxAIVehicles())
        {
            return false;
        }

        for (auto i = 0; i < 64 * 1000; i += 64)
        {
            auto id = _96885C[i];
            if (id == 0xFFFF)
            {
                return true;
            }
        }
        gGameCommandErrorText = string_ids::too_many_vehicles;
        return false;
    }

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
                auto veh2 = veh1->next_car()->as_vehicle_2();
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
            if (vehObject->var_24[i].body_sprite_ind == 0xFF)
            {
                continue;
            }

            auto unk = vehObject->var_24[i].body_sprite_ind & 0x7F;
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
        if (veh0 == nullptr)
        {
            return false;
        }
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

    static void sub_4B08DD(openloco::vehicle* const veh0)
    {
        registers regs{};
        regs.esi = reinterpret_cast<uint32_t>(veh0);
        call(0x004B08DD, regs);
    }

    // 0x00470039
    static openloco::vehicle_base* createVehicleThing()
    {
        registers regs{};
        call(0x00470039, regs);
        return reinterpret_cast<openloco::vehicle_base*>(regs.esi);
    }

    template<typename T>
    static T* createVehicleThing()
    {
        auto* const base = createVehicleThing();
        base->base_type = thing_base_type::vehicle;
        base->type = T::VehicleThingType;
        return reinterpret_cast<T*>(base);
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
            vehBogie->var_68 = static_cast<uint16_t>(std::max(4, reliabilityFactor));
        }
    }

    // 0x4AE8F1
    static openloco::vehicle_bogie* createBogie(const thing_id_t head, const uint16_t vehicleTypeId, const vehicle_object& vehObject, const uint8_t bodyNumber, openloco::vehicle* const lastVeh, const ColourScheme colourScheme)
    {
        auto newBogie = createVehicleThing<vehicle_bogie>();
        newBogie->owner = _updating_company_id;
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

        newBogie->var_14 = 1;
        newBogie->var_09 = 1;
        newBogie->var_15 = 1;

        newBogie->colour_scheme = colourScheme;
        lastVeh->next_car_id = newBogie->id;
        return newBogie;
    }

    // 0x4AE8F1
    static openloco::vehicle_bogie* createFirstBogie(const thing_id_t head, const uint16_t vehicleTypeId, const vehicle_object& vehObject, const uint8_t bodyNumber, openloco::vehicle* const lastVeh, const ColourScheme colourScheme)
    {
        auto newBogie = createBogie(head, vehicleTypeId, vehObject, bodyNumber, lastVeh, colourScheme);
        if (newBogie == nullptr) // Can never happen
        {
            return nullptr;
        }
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

        newBogie->object_sprite_type = vehObject.var_24[bodyNumber].front_bogie_sprite_ind;
        if (newBogie->object_sprite_type != 0xFF)
        {
            newBogie->var_14 = vehObject.var_B4[newBogie->object_sprite_type].var_02;
            newBogie->var_09 = vehObject.var_B4[newBogie->object_sprite_type].var_03;
            newBogie->var_15 = vehObject.var_B4[newBogie->object_sprite_type].var_04;
        }
        return newBogie;
    }

    // 0x004AEA9E
    static openloco::vehicle_bogie* createSecondBogie(const thing_id_t head, const uint16_t vehicleTypeId, const vehicle_object& vehObject, const uint8_t bodyNumber, openloco::vehicle* const lastVeh, const ColourScheme colourScheme)
    {
        auto newBogie = createBogie(head, vehicleTypeId, vehObject, bodyNumber, lastVeh, colourScheme);
        if (newBogie == nullptr) // Can never happen
        {
            return nullptr;
        }
        newBogie->var_38 = (1 << 1);
        newBogie->object_sprite_type = vehObject.var_24[bodyNumber].back_bogie_sprite_ind;
        if (newBogie->object_sprite_type != 0xFF)
        {
            newBogie->var_14 = vehObject.var_B4[newBogie->object_sprite_type].var_02;
            newBogie->var_09 = vehObject.var_B4[newBogie->object_sprite_type].var_03;
            newBogie->var_15 = vehObject.var_B4[newBogie->object_sprite_type].var_04;
        }
        return newBogie;
    }

    // 0x004AEA9E
    static openloco::vehicle_body* createBody(const thing_id_t head, const uint16_t vehicleTypeId, const vehicle_object& vehObject, const uint8_t bodyNumber, openloco::vehicle* const lastVeh, const ColourScheme colourScheme)
    {
        auto newBody = createVehicleThing<vehicle_body>();
        // TODO: move this into the create function somehow
        newBody->type = bodyNumber == 0 ? vehicle_thing_type::vehicle_body_end : vehicle_thing_type::vehicle_body_cont;
        newBody->owner = _updating_company_id;
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
        auto spriteType = vehObject.var_24[bodyNumber].body_sprite_ind;
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
        //if (veh0->mode == TransportMode::road)
        //{
        //    auto trackType = veh0->track_type;
        //    if (veh0->track_type == 0xFF)
        //    {
        //        trackType = _525FC5;
        //    }

        //    const auto roadObj = objectmgr::get<road_object>(trackType);
        //    auto* const veh3 = veh0->next_car()->next_car()->next_car(); // bogie or 6
        //    _11360FC = 0;
        //    if (veh3->type == vehicle_thing_type::vehicle_6)
        //    {
        //        // 0x4B7F16
        //    }
        //    else
        //    {
        //        // 0x4B7E57
        //        bool unk_ah = false;
        //        const auto vehicleObj = veh3->object();
        //        if (vehicleObj->sprites[0].var_05)
        //        {
        //            unk_ah = true;
        //        }

        //        if (vehicleObj->flags & flags_E0::rack_rail)
        //        {
        //            const auto rackRail = vehicleObj->rack_rail_type;
        //        }
        //    }
        //}
        registers regs{};
        regs.esi = reinterpret_cast<int32_t>(veh0);
        call(0x004B7CC3, regs);
    }

    // 0x004AE86D
    static bool createBody(openloco::vehicle* const veh0, const uint16_t vehicleTypeId)
    {
        if (!thingmgr::checkNumFreeThings(12))
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

        if (lastVeh == nullptr) // can never happen
        {
            return false;
        }
        lastVeh->next_car_id = endVeh->id;
        sub_4B7CC3(veh0);
        return true;
    }

    static std::optional<uint16_t> sub_4B1E00()
    {
        if (!checkMaxAIVehicles())
        {
            return {};
        }

        for (auto i = 0; i < 64 * 1000; i += 64)
        {
            auto id = _96885C[i];
            if (id == thing_id::null)
            {
                for (auto j = 0; j < 64; ++j)
                {
                    _96885C[i + j] = -2;
                }
                return { i };
            }
        }
        gGameCommandErrorText = string_ids::too_many_vehicles;
        return {};
    }

    static void sub_470312(vehicle_head* const newHead)
    {
        _987C5C[_525FB8] = 0;
        newHead->var_46 = _525FB8;
        _525FB8++;
        newHead->var_4A = 0;
        newHead->var_4C = 1;
    }

    // 0x004B64F9
    static uint16_t createUniqueTypeNumber(const VehicleType type)
    {
        std::array<bool, 1000> _unkArr{};
        auto v = thingmgr::first<openloco::vehicle>();
        while (v != nullptr)
        {
            auto next = v->next_vehicle();

            if (v->owner == _updating_company_id && v->vehicleType == type)
            {
                if (v->var_44 != 0)
                {
                    _unkArr[v->var_44 - 1] = true;
                }
            }
            v = next;
        }

        uint16_t newNum = 0;
        for (; newNum < _unkArr.size(); ++newNum)
        {
            if (!_unkArr[newNum])
                break;
        }
        return newNum + 1;
    }

    // 0x004AE34B
    static openloco::vehicle_head* createHead(const uint8_t trackType, const TransportMode mode, const uint16_t orderId, const VehicleType vehicleType)
    {
        auto* const newHead = createVehicleThing<vehicle_head>();
        thingmgr::moveSpriteToList(newHead, thingmgr::thing_list::vehicle_head);
        newHead->owner = _updating_company_id;
        newHead->head = newHead->id;
        newHead->var_0C |= (1 << 1);
        newHead->track_type = trackType;
        newHead->mode = mode;
        newHead->tile_x = -1;
        newHead->tile_y = 0;
        newHead->tile_base_z = 0;
        newHead->var_28 = 0;
        newHead->var_2E = 0;
        newHead->var_2C = 0;
        newHead->var_36 = orderId;
        newHead->var_14 = 0;
        newHead->var_09 = 0;
        newHead->var_15 = 0;
        newHead->var_38 = 0;
        newHead->var_3C = 0;
        newHead->vehicleType = vehicleType;
        newHead->var_22 = static_cast<uint8_t>(vehicleType) + 4;
        newHead->var_44 = createUniqueTypeNumber(vehicleType);
        newHead->var_5D = 0;
        newHead->var_54 = -1;
        newHead->var_5F = 0;
        newHead->var_60 = -1;
        newHead->var_61 = -1;
        newHead->var_69 = 0;
        newHead->var_77 = 0;
        newHead->var_79 = 0;
        sub_470312(newHead);
        return newHead;
    }

    // 0x004AE40E
    static openloco::vehicle_1* createVehicle1(const thing_id_t head, openloco::vehicle* const lastVeh)
    {
        auto* const newVeh1 = createVehicleThing<vehicle_1>();
        newVeh1->owner = _updating_company_id;
        newVeh1->head = head;
        newVeh1->track_type = lastVeh->track_type;
        newVeh1->mode = lastVeh->mode;
        newVeh1->tile_x = -1;
        newVeh1->tile_y = 0;
        newVeh1->tile_base_z = 0;
        newVeh1->var_28 = 0;
        newVeh1->var_2E = 0;
        newVeh1->var_2C = 0;
        newVeh1->var_36 = lastVeh->var_36;
        newVeh1->var_14 = 0;
        newVeh1->var_09 = 0;
        newVeh1->var_15 = 0;
        newVeh1->var_38 = 0;
        newVeh1->var_3C = 0;
        newVeh1->var_44 = 0;
        newVeh1->var_46 = 0;
        newVeh1->var_48 = 0;
        newVeh1->var_52 = 0;
        newVeh1->var_4E = 0;
        newVeh1->var_50 = 0;
        newVeh1->var_53 = -1;
        lastVeh->next_car_id = newVeh1->id;
        return newVeh1;
    }

    // 0x004AE4A0
    static openloco::vehicle_2* createVehicle2(const thing_id_t head, openloco::vehicle* const lastVeh)
    {
        auto* const newVeh2 = createVehicleThing<vehicle_2>();
        newVeh2->owner = _updating_company_id;
        newVeh2->head = head;
        newVeh2->track_type = lastVeh->track_type;
        newVeh2->mode = lastVeh->mode;
        newVeh2->tile_x = -1;
        newVeh2->tile_y = 0;
        newVeh2->tile_base_z = 0;
        newVeh2->var_28 = 0;
        newVeh2->var_2E = 0;
        newVeh2->var_2C = 0;
        newVeh2->var_36 = lastVeh->var_36;
        newVeh2->var_14 = 0;
        newVeh2->var_09 = 0;
        newVeh2->var_15 = 0;
        newVeh2->var_38 = 0;

        newVeh2->var_56 = 0;
        newVeh2->var_5A = 0;
        newVeh2->var_5B = 0;
        newVeh2->var_44 = -1;
        newVeh2->var_48 = -1;
        newVeh2->var_48 = 0;
        newVeh2->var_4A = 0;
        newVeh2->var_5E = 0;
        newVeh2->var_62 = 0;
        newVeh2->var_66 = 0;
        newVeh2->var_6A = 0;
        newVeh2->var_6E = 0;
        newVeh2->var_72 = 0;
        newVeh2->var_73 = 0;
        lastVeh->next_car_id = newVeh2->id;
        return newVeh2;
    }

    // 0x004AE54E
    static openloco::vehicle_tail* createVehicleTail(const thing_id_t head, openloco::vehicle* const lastVeh)
    {
        auto* const newTail = createVehicleThing<vehicle_tail>();
        newTail->owner = _updating_company_id;
        newTail->head = head;
        newTail->track_type = lastVeh->track_type;
        newTail->mode = lastVeh->mode;
        newTail->tile_x = -1;
        newTail->tile_y = 0;
        newTail->tile_base_z = 0;
        newTail->var_28 = 0;
        newTail->var_2E = 0;
        newTail->var_2C = 0;
        newTail->var_36 = lastVeh->var_36;
        newTail->var_14 = 0;
        newTail->var_09 = 0;
        newTail->var_15 = 0;
        newTail->var_38 = 0;

        newTail->var_44 = -1;
        newTail->var_48 = -1;
        newTail->var_4A = 0;
        lastVeh->next_car_id = newTail->id;
        newTail->next_car_id = thing_id::null;
        return newTail;
    }
    // 0x004AE318
    static std::optional<openloco::vehicle_head*> createBase(const TransportMode mode, const VehicleType type, const uint8_t trackType)
    {
        if (!thingmgr::checkNumFreeThings(4))
        {
            return {};
        }

        if (_525FB8 > 256000)
        {
            gGameCommandErrorText = string_ids::no_space_for_more_vehicle_orders;
            return {};
        }

        auto orderId = sub_4B1E00();
        if (!orderId)
        {
            return {};
        }

        auto* const head = createHead(trackType, mode, *orderId, type);
        openloco::vehicle* lastVeh = reinterpret_cast<openloco::vehicle*>(head);
        if (lastVeh == nullptr) // Can never happen
        {
            return {};
        }

        auto* const veh1 = createVehicle1(head->id, lastVeh);
        lastVeh = reinterpret_cast<openloco::vehicle*>(veh1);
        if (lastVeh == nullptr) // Can never happen
        {
            return {};
        }

        auto* const veh2 = createVehicle2(head->id, lastVeh);
        lastVeh = reinterpret_cast<openloco::vehicle*>(veh2);
        if (lastVeh == nullptr) // Can never happen
        {
            return {};
        }

        createVehicleTail(head->id, lastVeh);

        sub_4B7CC3(reinterpret_cast<openloco::vehicle*>(head));
        return { head };
    }

    static void sub_4AF7A4(openloco::vehicle* const veh0)
    {
        registers regs{};
        regs.esi = reinterpret_cast<int32_t>(veh0);
        call(0x004AF7A4, regs);
    }

    // 0x00437ED0
    static void companyRecalculateTransportCounts(const company_id_t companyId)
    {
        auto company = companymgr::get(companyId);
        for (auto& count : company->transportTypeCount)
        {
            count = 0;
        }

        auto v = thingmgr::first<openloco::vehicle>();
        while (v != nullptr)
        {
            auto next = v->next_vehicle();

            if (v->owner == companyId)
            {
                company->transportTypeCount[static_cast<uint8_t>(v->vehicleType)]++;
            }
            v = next;
        }

        ui::WindowManager::invalidate(ui::WindowType::company, companyId);
    }

    // 0x004B05E4
    static void sub_4B05E4(openloco::vehicle* const veh0, const coord_t x, const coord_t y, const uint8_t baseZ, const uint16_t unk1, const uint16_t unk2)
    {
        registers regs{};
        regs.esi = reinterpret_cast<int32_t>(veh0);
        regs.ax = x;
        regs.cx = y;
        regs.bx = unk2;
        regs.dl = baseZ;
        regs.ebp = unk1;
        call(0x004B05E4, regs);
    }

    // 0x004B1E77
    // Free orderId?
    static void sub_4B1E77(const uint16_t orderId)
    {
        uint16_t baseOrderId = orderId & ~(0x3F);
        for (auto i = 0; i < 64; ++i)
        {
            _96885C[i + baseOrderId] = -1;
        }
    }

    // 0x004AE74E
    static uint32_t create(const uint8_t flags, const uint16_t vehicleTypeId)
    {
        gameCommandMapX = location::null;
        if (!thingmgr::checkNumFreeThings(16))
        {
            return 0x80000000;
        }

        if (!checkMaxVehicles())
        {
            return 0x80000000;
        }

        if (flags & game_commands::GameCommandFlag::apply)
        {
            auto vehObject = objectmgr::get<vehicle_object>(vehicleTypeId);

            auto head = createBase(vehObject->mode, vehObject->type, vehObject->track_type);
            if (!head)
            {
                return 0x80000000;
            }

            auto _head = reinterpret_cast<openloco::vehicle*>(*head);
            _113642A = _head->id;
            if (createBody(_head, vehicleTypeId))
            {
                // 0x004AE6DE
                sub_4AF7A4(_head);
                companyRecalculateTransportCounts(_updating_company_id);

                if (_backupVeh0 != reinterpret_cast<openloco::vehicle*>(-1))
                {
                    sub_4B05E4(_backupVeh0, _backupX, _backupY, _backupZ, _backup2C, _backup2E);
                }

                ui::WindowManager::invalidate(ui::WindowType::vehicleList, _head->owner);
            }
            else
            {
                // Cleanup and delete base before exit.

                sub_4B1E77(_head->var_36);

                // 0x004AE7B2
                return 0x80000000;
            }
        }
        // 0x4AE733
        auto vehObject = objectmgr::get<vehicle_object>(vehicleTypeId);
        auto cost = (vehObject->cost_fact * currencyMultiplicationFactor[vehObject->cost_ind]) / 64;
        return cost;
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
            auto veh2 = veh0->next_car()->next_car()->as_vehicle_2();
            if (veh2 == nullptr)
            {
                return 0x80000000;
            }
            gameCommandMapX = veh2->x;
            gameCommandMapY = veh2->y;
            gameCommandMapZ = veh2->z;

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

            if (!thingmgr::checkNumFreeThings(12))
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
                    // 0x004AE6DE
                    sub_4AF7A4(veh0);
                    companyRecalculateTransportCounts(_updating_company_id);

                    if (_backupVeh0 != reinterpret_cast<openloco::vehicle*>(-1))
                    {
                        sub_4B05E4(_backupVeh0, _backupX, _backupY, _backupZ, _backup2C, _backup2E);
                    }

                    ui::WindowManager::invalidate(ui::WindowType::vehicleList, veh0->owner);
                }
                else
                {
                    if (_backupVeh0 == reinterpret_cast<openloco::vehicle*>(-1))
                    {
                        return 0x80000000;
                    }

                    openloco::vehicle* veh0backup = _backupVeh0;
                    // If it has an existing body
                    if (veh0backup->next_car()->next_car()->next_car()->type == vehicle_thing_type::vehicle_6)
                    {
                        sub_4B05E4(_backupVeh0, _backupX, _backupY, _backupZ, _backup2C, _backup2E);
                    }
                    return 0x80000000;
                }
            }
            // 0x4AE733
            auto vehObject = objectmgr::get<vehicle_object>(vehicleTypeId);
            auto cost = (vehObject->cost_fact * currencyMultiplicationFactor[vehObject->cost_ind]) / 64;
            return cost;
        }
    }
}
