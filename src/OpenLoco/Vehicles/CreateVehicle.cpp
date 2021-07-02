#include "../Audio/Audio.h"
#include "../CompanyManager.h"
#include "../Core/Optional.hpp"
#include "../Date.h"
#include "../Economy/Economy.h"
#include "../Economy/Expenditures.h"
#include "../Entities/EntityManager.h"
#include "../GameCommands/GameCommands.h"
#include "../Localisation/StringIds.h"
#include "../Map/Tile.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/RoadObject.h"
#include "../Objects/SoundObject.h"
#include "../Objects/TrackObject.h"
#include "../Objects/VehicleObject.h"
#include "../Station.h"
#include "../Types.hpp"
#include "../Ui/WindowManager.h"
#include "Vehicle.h"
#include <numeric>
#include <utility>

using namespace OpenLoco::Interop;
using namespace OpenLoco::GameCommands;
using namespace OpenLoco::Literals;

namespace OpenLoco::Vehicles
{
    constexpr uint32_t max_orders = 256000;
    constexpr auto max_num_vehicles = 1000;
    constexpr auto max_num_routing_steps = 64;
    constexpr auto max_ai_vehicles = 500;
    constexpr auto max_num_car_components_in_car = 4;           // TODO: Move to VehicleObject
    constexpr auto num_vehicle_components_in_car_component = 3; // Bogie bogie body
    constexpr auto num_vehicle_components_in_base = 4;          // head unk_1 unk_2 tail
    constexpr auto max_num_vehicle_components_in_car = num_vehicle_components_in_car_component * max_num_car_components_in_car;
    constexpr EntityId_t allocated_but_free_routing_station = -2; // Indicates that this array is allocated to a vehicle but no station has been set.

    static loco_global<CompanyId_t, 0x009C68EB> _updating_company_id;
    static loco_global<uint8_t, 0x009C68EE> _errorCompanyId;
    static loco_global<Map::TileElement*, 0x009C68D0> _9C68D0;
    static loco_global<ColourScheme, 0x01136140> _1136140; // primary colour
    static loco_global<int32_t, 0x011360FC> _11360FC;
    static loco_global<VehicleHead*, 0x01136240> _backupVeh0;
    static loco_global<int16_t, 0x01136248> _backup2E;
    static loco_global<TrackAndDirection, 0x0113624C> _backup2C;
    static loco_global<int16_t, 0x01136250> _backupX;
    static loco_global<int16_t, 0x01136254> _backupY;
    static loco_global<uint8_t, 0x01136258> _backupZ;
    static loco_global<uint16_t, 0x0113642A> _113642A; // used by build window and others
    static loco_global<uint8_t, 0x00525FC5> _525FC5;
    static loco_global<uint32_t, 0x00525FB8> _orderTableLength;                                  // total used length of _987C5C
    static loco_global<EntityId_t[max_num_vehicles][max_num_routing_steps], 0x0096885C> _96885C; // Likely routing related
    static loco_global<uint8_t[max_orders], 0x00987C5C> _987C5C;                                 // ?orders? ?routing related?

    // 0x004B1D96
    static bool aiIsBelowVehicleLimit()
    {
        if (isPlayerCompany(_updating_company_id))
        {
            return true;
        }

        const auto& companies = CompanyManager::companies();
        auto totalAiVehicles = std::accumulate(companies.begin(), companies.end(), 0, [](int32_t& total, const auto& company) {
            if (isPlayerCompany(company.id()))
                return total;
            return total + std::accumulate(std::begin(company.transportTypeCount), std::end(company.transportTypeCount), 0);
        });

        if (totalAiVehicles > max_ai_vehicles)
        {
            GameCommands::setErrorText(StringIds::too_many_vehicles);
            return false;
        }

        return true;
    }

    // 0x004B1E44
    static bool isEmptyVehicleSlotAvailable()
    {
        if (!aiIsBelowVehicleLimit())
        {
            return false;
        }

        for (auto i = 0; i < max_num_vehicles; i++)
        {
            auto id = _96885C[i][0];
            if (id == EntityId::null)
            {
                return true;
            }
        }
        GameCommands::setErrorText(StringIds::too_many_vehicles);
        return false;
    }

    template<typename T>
    static T* createVehicleThing()
    {
        auto* const base = EntityManager::createEntityVehicle();
        base->base_type = EntityBaseType::vehicle;
        auto* const vehicleBase = base->asVehicle();
        vehicleBase->setSubType(T::vehicleThingType);
        return static_cast<T*>(vehicleBase);
    }

    // 0x004BA873
    // esi : vehBogie
    static void sub_4BA873(VehicleBogie* const vehBogie)
    {
        vehBogie->var_68 = 0xFFFF;
        if (vehBogie->reliability != 0)
        {
            int32_t reliabilityFactor = vehBogie->reliability / 256;
            reliabilityFactor *= reliabilityFactor;
            reliabilityFactor /= 16;

            auto& prng = gPrng();
            int32_t randVal = (prng.randNext(65535) * reliabilityFactor / 2) / 65536;
            reliabilityFactor -= reliabilityFactor / 4;
            reliabilityFactor += randVal;
            vehBogie->var_68 = static_cast<uint16_t>(std::max(4, reliabilityFactor));
        }
    }

    // 0x004AE8F1, 0x004AEA9E
    static VehicleBogie* createBogie(const EntityId_t head, const uint16_t vehicleTypeId, const VehicleObject& vehObject, const uint8_t bodyNumber, VehicleBase* const lastVeh, const ColourScheme colourScheme)
    {
        auto newBogie = createVehicleThing<VehicleBogie>();
        newBogie->owner = _updating_company_id;
        newBogie->head = head;
        newBogie->body_index = bodyNumber;
        newBogie->track_type = lastVeh->getTrackType();
        newBogie->mode = lastVeh->getTransportMode();
        newBogie->tile_x = -1;
        newBogie->tile_y = 0;
        newBogie->tile_base_z = 0;
        newBogie->var_2E = 0;
        newBogie->var_2C = TrackAndDirection(0, 0);
        newBogie->var_36 = lastVeh->getVar36();
        newBogie->object_id = vehicleTypeId;

        auto& prng = gPrng();
        newBogie->var_44 = prng.randNext();
        newBogie->creation_day = getCurrentDay();
        newBogie->var_46 = 0;
        newBogie->var_47 = 0;
        newBogie->secondaryCargo.acceptedTypes = 0;
        newBogie->secondaryCargo.type = 0xFF;
        newBogie->secondaryCargo.qty = 0;
        newBogie->var_5E = 0;
        newBogie->var_5F = 0;
        newBogie->var_60 = 0; // different to createbody
        newBogie->var_61 = 0; // different to createbody

        newBogie->var_14 = 1;
        newBogie->var_09 = 1;
        newBogie->var_15 = 1;

        newBogie->colour_scheme = colourScheme;
        lastVeh->setNextCar(newBogie->id);
        return newBogie;
    }

    // 0x4AE8F1
    static VehicleBogie* createFirstBogie(const EntityId_t head, const uint16_t vehicleTypeId, const VehicleObject& vehObject, const uint8_t bodyNumber, VehicleBase* const lastVeh, const ColourScheme colourScheme)
    {
        auto newBogie = createBogie(head, vehicleTypeId, vehObject, bodyNumber, lastVeh, colourScheme);
        if (newBogie == nullptr) // Can never happen
        {
            return nullptr;
        }
        newBogie->var_38 = 0;

        int32_t reliability = vehObject.reliability * 256;
        if (getCurrentYear() + 2 > vehObject.designed)
        {
            // Reduce reliability by an eighth after 2 years past design
            reliability -= reliability / 8;
            if (getCurrentYear() + 3 > vehObject.designed)
            {
                // Reduce reliability by a further eighth (quarter total) after 3 years past design
                reliability -= reliability / 8;
            }
        }
        if (reliability != 0)
        {
            reliability += 255;
        }
        newBogie->reliability = reliability;
        sub_4BA873(newBogie);

        // Calculate refund cost == 7/8 * cost
        auto cost = Economy::getInflationAdjustedCost(vehObject.cost_factor, vehObject.cost_index, 6);
        newBogie->refund_cost = cost - cost / 8;

        if (bodyNumber == 0)
        {
            // Only front car components front bogie can have cargo
            // stores only secondary cargo presumably due to space constraints
            // in the front car component body
            if (vehObject.num_simultaneous_cargo_types > 1)
            {
                newBogie->secondaryCargo.maxQty = vehObject.max_secondary_cargo;
                newBogie->secondaryCargo.acceptedTypes = vehObject.secondary_cargo_types;
                auto cargoType = Utility::bitScanForward(newBogie->secondaryCargo.acceptedTypes);
                if (cargoType != -1)
                {
                    newBogie->secondaryCargo.type = cargoType;
                }
            }
        }

        newBogie->object_sprite_type = vehObject.var_24[bodyNumber].front_bogie_sprite_ind;
        if (newBogie->object_sprite_type != SpriteIndex::null)
        {
            newBogie->var_14 = vehObject.bogie_sprites[newBogie->object_sprite_type].var_02;
            newBogie->var_09 = vehObject.bogie_sprites[newBogie->object_sprite_type].var_03;
            newBogie->var_15 = vehObject.bogie_sprites[newBogie->object_sprite_type].var_04;
        }
        return newBogie;
    }

    // 0x004AEA9E
    static VehicleBogie* createSecondBogie(const EntityId_t head, const uint16_t vehicleTypeId, const VehicleObject& vehObject, const uint8_t bodyNumber, VehicleBase* const lastVeh, const ColourScheme colourScheme)
    {
        auto newBogie = createBogie(head, vehicleTypeId, vehObject, bodyNumber, lastVeh, colourScheme);
        if (newBogie == nullptr) // Can never happen
        {
            return nullptr;
        }
        newBogie->var_38 = Flags38::isReversed;
        newBogie->object_sprite_type = vehObject.var_24[bodyNumber].back_bogie_sprite_ind;
        if (newBogie->object_sprite_type != SpriteIndex::null)
        {
            newBogie->var_14 = vehObject.bogie_sprites[newBogie->object_sprite_type].var_02;
            newBogie->var_09 = vehObject.bogie_sprites[newBogie->object_sprite_type].var_03;
            newBogie->var_15 = vehObject.bogie_sprites[newBogie->object_sprite_type].var_04;
        }
        return newBogie;
    }

    // 0x004AEA9E
    static VehicleBody* createBody(const EntityId_t head, const uint16_t vehicleTypeId, const VehicleObject& vehObject, const uint8_t bodyNumber, VehicleBase* const lastVeh, const ColourScheme colourScheme)
    {
        auto newBody = createVehicleThing<VehicleBody>();
        // TODO: move this into the create function somehow
        newBody->setSubType(bodyNumber == 0 ? VehicleThingType::body_start : VehicleThingType::body_continued);
        newBody->owner = _updating_company_id;
        newBody->head = head;
        newBody->body_index = bodyNumber;
        newBody->track_type = lastVeh->getTrackType();
        newBody->mode = lastVeh->getTransportMode();
        newBody->tile_x = -1;
        newBody->tile_y = 0;
        newBody->tile_base_z = 0;
        newBody->var_2E = 0;
        newBody->var_2C = TrackAndDirection(0, 0);
        newBody->var_36 = lastVeh->getVar36();
        newBody->var_38 = Flags38::unk_0; // different to create bogie
        newBody->object_id = vehicleTypeId;

        auto& prng = gPrng();
        newBody->var_44 = prng.randNext();
        newBody->creation_day = getCurrentDay();
        newBody->var_46 = 0;
        newBody->var_47 = 0;
        newBody->primaryCargo.acceptedTypes = 0;
        newBody->primaryCargo.type = 0xFF;
        newBody->primaryCargo.qty = 0;
        newBody->var_55 = 0; // different to create bogie
        newBody->var_5E = 0;
        newBody->var_5F = 0;

        // different to create bogie
        if (bodyNumber == 0)
        {
            // If the car carries any type of cargo then it will have a primary
            // cargo in the first body of the first car component of the car.
            // Locomotives do not carry any cargo.
            if (vehObject.num_simultaneous_cargo_types != 0)
            {
                newBody->primaryCargo.maxQty = vehObject.max_primary_cargo;
                newBody->primaryCargo.acceptedTypes = vehObject.primary_cargo_types;
                auto cargoType = Utility::bitScanForward(newBody->primaryCargo.acceptedTypes);
                if (cargoType != -1)
                {
                    newBody->primaryCargo.type = cargoType;
                }
            }
        }

        newBody->var_14 = 1;
        newBody->var_09 = 1;
        newBody->var_15 = 1;

        // different onwards to create bogie
        auto spriteType = vehObject.var_24[bodyNumber].body_sprite_ind;
        if (spriteType != SpriteIndex::null)
        {
            if (spriteType & SpriteIndex::flag_unk7)
            {
                newBody->var_38 |= Flags38::isReversed;
                spriteType &= ~SpriteIndex::flag_unk7;
            }
        }
        newBody->object_sprite_type = spriteType;

        if (newBody->object_sprite_type != SpriteIndex::null)
        {
            newBody->var_14 = vehObject.bodySprites[newBody->object_sprite_type].var_08;
            newBody->var_09 = vehObject.bodySprites[newBody->object_sprite_type].var_09;
            newBody->var_15 = vehObject.bodySprites[newBody->object_sprite_type].var_0A;
        }

        newBody->colour_scheme = colourScheme; // same as create bogie

        if (bodyNumber == 0 && vehObject.flags & FlagsE0::flag_02)
        {
            newBody->var_38 |= Flags38::unk_3;
        }

        if (bodyNumber + 1 == vehObject.var_04 && vehObject.flags & FlagsE0::flag_03)
        {
            newBody->var_38 |= Flags38::unk_3;
        }

        lastVeh->setNextCar(newBody->id); // same as create bogie
        return newBody;
    }

    // 0x004AE86D
    static bool createCar(VehicleHead* head, const uint16_t vehicleTypeId)
    {
        if (!EntityManager::checkNumFreeEntities(max_num_vehicle_components_in_car))
        {
            return false;
        }

        // Get Car insertion location
        Vehicle train(head);
        // lastVeh will point to the vehicle component prior to the tail (head, unk_1, unk_2 *here*, tail) or (... bogie, bogie, body *here*, tail)
        VehicleBase* lastVeh = nullptr;
        if (!train.cars.empty())
        {
            for (auto& car : train.cars)
            {
                for (auto& carComponent : car)
                {
                    lastVeh = carComponent.body;
                }
            }
        }
        else
        {
            lastVeh = train.veh2;
        }

        const auto vehObject = ObjectManager::get<VehicleObject>(vehicleTypeId);
        const auto company = CompanyManager::get(_updating_company_id);
        _1136140 = company->mainColours; // Copy to global variable. Can be removed when all global uses confirmed
        auto colourScheme = company->mainColours;
        if (company->customVehicleColoursSet & (1 << vehObject->colour_type))
        {
            _1136140 = company->vehicleColours[vehObject->colour_type - 1]; // Copy to global variable. Can be removed when all global uses confirmed
            colourScheme = company->vehicleColours[vehObject->colour_type - 1];
        }

        VehicleBogie* newCarStart = nullptr;
        for (auto bodyNumber = 0; bodyNumber < vehObject->var_04; ++bodyNumber)
        {
            auto* const firstBogie = createFirstBogie(head->id, vehicleTypeId, *vehObject, bodyNumber, lastVeh, colourScheme);
            lastVeh = firstBogie;

            auto* const secondBogie = createSecondBogie(head->id, vehicleTypeId, *vehObject, bodyNumber, lastVeh, colourScheme);
            lastVeh = secondBogie;

            auto* const body = createBody(head->id, vehicleTypeId, *vehObject, bodyNumber, lastVeh, colourScheme);
            lastVeh = body;

            if (newCarStart == nullptr)
            {
                newCarStart = firstBogie;
            }
        }

        if (lastVeh == nullptr) // can never happen
        {
            return false;
        }
        lastVeh->setNextCar(train.tail->id);
        head->sub_4B7CC3();
        return true;
    }

    static std::optional<uint16_t> sub_4B1E00()
    {
        if (!aiIsBelowVehicleLimit())
        {
            return {};
        }

        // ?Routing? related. Max 64 routing stops.
        for (auto i = 0; i < max_num_vehicles; i++)
        {
            auto id = _96885C[i][0];
            if (id == EntityId::null)
            {
                for (auto j = 0; j < max_num_routing_steps; ++j)
                {
                    _96885C[i][j] = allocated_but_free_routing_station;
                }
                return { i };
            }
        }
        GameCommands::setErrorText(StringIds::too_many_vehicles);
        return {};
    }

    static void sub_470312(VehicleHead* const newHead)
    {
        _987C5C[_orderTableLength] = 0;
        newHead->orderTableOffset = _orderTableLength;
        _orderTableLength++;
        newHead->currentOrder = 0;
        newHead->sizeOfOrderTable = 1;
    }

    // 0x004B64F9
    static uint16_t createUniqueTypeNumber(const VehicleType type)
    {
        std::array<bool, max_num_vehicles> _unkArr{};
        for (auto v : EntityManager::VehicleList())
        {
            if (v->owner == _updating_company_id && v->vehicleType == type)
            {
                if (v->ordinalNumber != 0)
                {
                    _unkArr[v->ordinalNumber - 1] = true;
                }
            }
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
    static VehicleHead* createHead(const uint8_t trackType, const TransportMode mode, const uint16_t orderId, const VehicleType vehicleType)
    {
        auto* const newHead = createVehicleThing<VehicleHead>();
        EntityManager::moveEntityToList(newHead, EntityManager::EntityListType::vehicleHead);
        newHead->owner = _updating_company_id;
        newHead->head = newHead->id;
        newHead->var_0C |= Flags0C::commandStop;
        newHead->track_type = trackType;
        newHead->mode = mode;
        newHead->tile_x = -1;
        newHead->tile_y = 0;
        newHead->tile_base_z = 0;
        newHead->var_28 = 0;
        newHead->var_2E = 0;
        newHead->var_2C = TrackAndDirection(0, 0);
        newHead->var_36 = orderId * max_num_routing_steps;
        newHead->var_14 = 0;
        newHead->var_09 = 0;
        newHead->var_15 = 0;
        newHead->var_38 = 0;
        newHead->var_3C = 0;
        newHead->vehicleType = vehicleType;
        newHead->name = static_cast<uint8_t>(vehicleType) + 4;
        newHead->ordinalNumber = 0; // Reset to null value so ignored in next function
        newHead->ordinalNumber = createUniqueTypeNumber(vehicleType);
        newHead->var_52 = 0;
        newHead->var_5C = 0;
        newHead->status = Status::unk_0;
        newHead->stationId = StationId::null;
        newHead->var_5F = 0;
        newHead->var_60 = -1;
        newHead->var_61 = -1;
        newHead->var_69 = 0;
        newHead->lastAverageSpeed = 0;
        newHead->var_79 = 0;
        sub_470312(newHead);
        return newHead;
    }

    // 0x004AE40E
    static Vehicle1* createVehicle1(const EntityId_t head, VehicleBase* const lastVeh)
    {
        auto* const newVeh1 = createVehicleThing<Vehicle1>();
        newVeh1->owner = _updating_company_id;
        newVeh1->head = head;
        newVeh1->track_type = lastVeh->getTrackType();
        newVeh1->mode = lastVeh->getTransportMode();
        newVeh1->tile_x = -1;
        newVeh1->tile_y = 0;
        newVeh1->tile_base_z = 0;
        newVeh1->var_28 = 0;
        newVeh1->var_2E = 0;
        newVeh1->var_2C = TrackAndDirection(0, 0);
        newVeh1->var_36 = lastVeh->getVar36();
        newVeh1->var_14 = 0;
        newVeh1->var_09 = 0;
        newVeh1->var_15 = 0;
        newVeh1->var_38 = 0;
        newVeh1->var_3C = 0;
        newVeh1->var_44 = 0_mph;
        newVeh1->timeAtSignal = 0;
        newVeh1->var_48 = 0;
        newVeh1->var_52 = 0;
        newVeh1->var_4E = 0;
        newVeh1->var_50 = 0;
        newVeh1->lastIncome.day = -1;
        lastVeh->setNextCar(newVeh1->id);
        return newVeh1;
    }

    // 0x004AE4A0
    static Vehicle2* createVehicle2(const EntityId_t head, VehicleBase* const lastVeh)
    {
        auto* const newVeh2 = createVehicleThing<Vehicle2>();
        newVeh2->owner = _updating_company_id;
        newVeh2->head = head;
        newVeh2->track_type = lastVeh->getTrackType();
        newVeh2->mode = lastVeh->getTransportMode();
        newVeh2->tile_x = -1;
        newVeh2->tile_y = 0;
        newVeh2->tile_base_z = 0;
        newVeh2->var_28 = 0;
        newVeh2->var_2E = 0;
        newVeh2->var_2C = TrackAndDirection(0, 0);
        newVeh2->var_36 = lastVeh->getVar36();
        newVeh2->var_14 = 0;
        newVeh2->var_09 = 0;
        newVeh2->var_15 = 0;
        newVeh2->var_38 = 0;

        newVeh2->currentSpeed = 0.0_mph;
        newVeh2->var_5A = 0;
        newVeh2->var_5B = 0;
        newVeh2->drivingSoundId = SoundObjectId::null;
        newVeh2->objectId = -1;
        newVeh2->var_4A = 0;
        newVeh2->lifetimeProfit = 0;
        newVeh2->profit[0] = 0;
        newVeh2->profit[1] = 0;
        newVeh2->profit[2] = 0;
        newVeh2->profit[3] = 0;
        newVeh2->reliability = 0;
        newVeh2->var_73 = 0;
        lastVeh->setNextCar(newVeh2->id);
        return newVeh2;
    }

    // 0x004AE54E
    static VehicleTail* createVehicleTail(const EntityId_t head, VehicleBase* const lastVeh)
    {
        auto* const newTail = createVehicleThing<VehicleTail>();
        newTail->owner = _updating_company_id;
        newTail->head = head;
        newTail->track_type = lastVeh->getTrackType();
        newTail->mode = lastVeh->getTransportMode();
        newTail->tile_x = -1;
        newTail->tile_y = 0;
        newTail->tile_base_z = 0;
        newTail->var_28 = 0;
        newTail->var_2E = 0;
        newTail->var_2C = TrackAndDirection(0, 0);
        newTail->var_36 = lastVeh->getVar36();
        newTail->var_14 = 0;
        newTail->var_09 = 0;
        newTail->var_15 = 0;
        newTail->var_38 = 0;
        newTail->drivingSoundId = SoundObjectId::null;
        newTail->objectId = -1;
        newTail->var_4A = 0;
        lastVeh->setNextCar(newTail->id);
        newTail->next_car_id = EntityId::null;
        return newTail;
    }
    // 0x004AE318
    static std::optional<VehicleHead*> createBaseVehicle(const TransportMode mode, const VehicleType type, const uint8_t trackType)
    {
        if (!EntityManager::checkNumFreeEntities(num_vehicle_components_in_base))
        {
            return {};
        }

        if (_orderTableLength >= max_orders)
        {
            GameCommands::setErrorText(StringIds::no_space_for_more_vehicle_orders);
            return {};
        }

        auto orderId = sub_4B1E00();
        if (!orderId)
        {
            return {};
        }

        auto* head = createHead(trackType, mode, *orderId, type);
        VehicleBase* lastVeh = head;
        if (lastVeh == nullptr) // Can never happen
        {
            return {};
        }

        lastVeh = createVehicle1(head->id, lastVeh);
        if (lastVeh == nullptr) // Can never happen
        {
            return {};
        }

        lastVeh = createVehicle2(head->id, lastVeh);
        if (lastVeh == nullptr) // Can never happen
        {
            return {};
        }

        createVehicleTail(head->id, lastVeh);

        head->sub_4B7CC3();
        return { head };
    }

    static void sub_4AF7A4(VehicleHead* const veh0)
    {
        registers regs{};
        regs.esi = reinterpret_cast<int32_t>(veh0);
        call(0x004AF7A4, regs);
    }

    // 0x004B05E4
    static void placeDownVehicle(VehicleHead* const head, const coord_t x, const coord_t y, const uint8_t baseZ, const TrackAndDirection unk1, const uint16_t unk2)
    {
        registers regs{};
        regs.esi = reinterpret_cast<int32_t>(head);
        regs.ax = x;
        regs.cx = y;
        regs.bx = unk2;
        regs.dl = baseZ;
        regs.ebp = unk1.track._data;
        call(0x004B05E4, regs);
    }

    // 0x004B1E77
    // Free orderId?
    static void sub_4B1E77(const uint16_t orderId)
    {
        uint16_t baseOrderId = (orderId & ~(0x3F)) / max_num_routing_steps;
        for (auto i = 0; i < max_num_routing_steps; ++i)
        {
            _96885C[baseOrderId][i] = EntityId::null;
        }
    }

    static void sub_470795(const uint32_t removeOrderTableOffset, const int16_t sizeOfRemovedOrderTable)
    {
        for (auto head : EntityManager::VehicleList())
        {
            if (head->orderTableOffset >= removeOrderTableOffset)
            {
                head->orderTableOffset += sizeOfRemovedOrderTable;
            }
        }
    }

    // 0x00470334
    // Remove vehicle ?orders?
    static void sub_470334(VehicleHead* const head)
    {
        sub_470795(head->orderTableOffset, head->sizeOfOrderTable * -1);
        auto length = _orderTableLength - head->orderTableOffset - head->sizeOfOrderTable;
        memmove(&_987C5C[head->orderTableOffset], &_987C5C[head->sizeOfOrderTable + head->orderTableOffset], length);

        _orderTableLength = _orderTableLength - head->sizeOfOrderTable;
    }

    // 0x0042851C
    // Delete related news items??
    static void sub_42851C(const EntityId_t id, const uint8_t type)
    {
        registers regs{};
        regs.al = type;
        regs.dx = id;
        call(0x0042851C, regs);
    }

    // 0x004AE6DE
    static void updateWholeVehicle(VehicleHead* const head)
    {
        sub_4AF7A4(head);
        auto company = CompanyManager::get(_updating_company_id);
        company->recalculateTransportCounts();

        if (_backupVeh0 != reinterpret_cast<VehicleHead*>(-1))
        {
            placeDownVehicle(_backupVeh0, _backupX, _backupY, _backupZ, _backup2C, _backup2E);
        }

        Ui::WindowManager::invalidate(Ui::WindowType::vehicleList, head->owner);
    }

    // 0x004AE74E
    static uint32_t createNewVehicle(const uint8_t flags, const uint16_t vehicleTypeId)
    {
        GameCommands::setPosition({ Location::null, 0, 0 });
        if (!EntityManager::checkNumFreeEntities(max_num_vehicle_components_in_car + num_vehicle_components_in_base))
        {
            return FAILURE;
        }

        if (!isEmptyVehicleSlotAvailable())
        {
            return FAILURE;
        }

        if (flags & GameCommands::Flags::apply)
        {
            auto vehObject = ObjectManager::get<VehicleObject>(vehicleTypeId);

            auto head = createBaseVehicle(vehObject->mode, vehObject->type, vehObject->track_type);
            if (!head)
            {
                return FAILURE;
            }

            auto _head = *head;
            _113642A = _head->id;
            if (createCar(_head, vehicleTypeId))
            {
                // 0x004AE6DE
                updateWholeVehicle(_head);
            }
            else
            {
                // Cleanup and delete base vehicle before exit.
                sub_4B1E77(_head->var_36);
                sub_470334(_head);
                sub_42851C(_head->id, 3);
                auto veh1 = _head->nextVehicleComponent();
                if (veh1 == nullptr)
                {
                    throw std::runtime_error("Bad vehicle structure");
                }
                auto veh2 = veh1->nextVehicleComponent();
                if (veh2 == nullptr)
                {
                    throw std::runtime_error("Bad vehicle structure");
                }
                auto tail = veh2->nextVehicleComponent();
                // Get all vehicles before freeing
                EntityManager::freeEntity(_head);
                EntityManager::freeEntity(veh1);
                EntityManager::freeEntity(veh2);
                EntityManager::freeEntity(tail);
                return FAILURE;
            }
        }
        // 0x4AE733
        auto vehObject = ObjectManager::get<VehicleObject>(vehicleTypeId);
        auto cost = Economy::getInflationAdjustedCost(vehObject->cost_factor, vehObject->cost_index, 6);
        return cost;
    }

    // 0x004AE5FF
    static uint32_t addCarToVehicle(const uint8_t flags, const uint16_t vehicleTypeId, const uint16_t vehicleThingId)
    {
        Vehicle train(vehicleThingId);
        GameCommands::setPosition(train.veh2->position);

        if (!sub_431E6A(train.head->owner))
        {
            return FAILURE;
        }

        if (!train.head->canBeModified())
        {
            return FAILURE;
        }

        if (!train.head->isVehicleTypeCompatible(vehicleTypeId))
        {
            return FAILURE;
        }

        if (!EntityManager::checkNumFreeEntities(max_num_vehicle_components_in_car))
        {
            return FAILURE;
        }

        if (flags & GameCommands::Flags::apply)
        {
            if (train.head->tile_x != -1)
            {
                _backupX = train.head->tile_x;
                _backupY = train.head->tile_y;
                _backupZ = train.head->tile_base_z;
                _backup2C = train.head->var_2C;
                _backup2E = train.head->var_2E;
                _backupVeh0 = train.head;
                train.head->liftUpVehicle();
            }

            if (createCar(train.head, vehicleTypeId))
            {
                // Note train.cars is no longer valid from after createCar
                updateWholeVehicle(train.head);
            }
            else
            {
                if (_backupVeh0 == reinterpret_cast<VehicleHead*>(-1))
                {
                    return FAILURE;
                }

                VehicleHead* veh0backup = _backupVeh0;
                // If it has an existing body
                Vehicle bkupTrain(veh0backup);
                if (!bkupTrain.cars.empty())
                {
                    placeDownVehicle(_backupVeh0, _backupX, _backupY, _backupZ, _backup2C, _backup2E);
                }
                return FAILURE;
            }
        }
        // 0x4AE733
        auto vehObject = ObjectManager::get<VehicleObject>(vehicleTypeId);
        auto cost = Economy::getInflationAdjustedCost(vehObject->cost_factor, vehObject->cost_index, 6);
        return cost;
    }

    // 0x004AE5E4
    static uint32_t create(const uint8_t flags, const uint16_t vehicleTypeId, const uint16_t vehicleThingId)
    {
        GameCommands::setExpenditureType(ExpenditureType::VehiclePurchases);
        _backupVeh0 = reinterpret_cast<VehicleHead*>(-1);
        if (vehicleThingId == (uint16_t)-1)
        {
            return createNewVehicle(flags, vehicleTypeId);
        }
        else
        {
            return addCarToVehicle(flags, vehicleTypeId, vehicleThingId);
        }
    }

    void create(registers& regs)
    {
        regs.ebx = create(regs.bl, regs.dx, regs.di);
    }
}
