#include "CreateVehicle.h"
#include "Audio/Audio.h"
#include "Config.h"
#include "Date.h"
#include "Economy/Economy.h"
#include "Economy/Expenditures.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "LastGameOptionManager.h"
#include "Localisation/StringIds.h"
#include "Map/Tile.h"
#include "MessageManager.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Objects/SoundObject.h"
#include "Objects/TrackObject.h"
#include "Objects/VehicleObject.h"
#include "Random.h"
#include "Types.hpp"
#include "Ui/WindowManager.h"
#include "Vehicles/OrderManager.h"
#include "Vehicles/RoutingManager.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleManager.h"
#include "World/CompanyManager.h"
#include "World/Station.h"
#include <numeric>
#include <optional>
#include <utility>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Literals;
using namespace OpenLoco::Vehicles;

namespace OpenLoco::GameCommands
{
    constexpr auto kMaxAiVehicles = 500;
    constexpr auto kMaxNumCarComponentsInCar = 4;           // TODO: Move to VehicleObject
    constexpr auto kNumVehicleComponentsInCarComponent = 3; // Bogie body
    constexpr auto kNumVehicleComponentsInBase = 4;         // head unk_1 unk_2 tail
    constexpr auto kMaxNumVehicleComponentsInCar = kNumVehicleComponentsInCarComponent * kMaxNumCarComponentsInCar;
    static loco_global<CompanyId, 0x009C68EB> _updatingCompanyId;
    static loco_global<uint8_t, 0x009C68EE> _errorCompanyId;
    static loco_global<const World::TileElement*, 0x009C68D0> _9C68D0;
    static loco_global<ColourScheme, 0x01136140> _1136140; // primary colour
    static loco_global<int32_t, 0x011360FC> _11360FC;
    static loco_global<VehicleHead*, 0x01136240> _backupVeh0;
    static loco_global<int16_t, 0x01136248> _backup2E;
    static loco_global<TrackAndDirection, 0x0113624C> _backup2C;
    static loco_global<int16_t, 0x01136250> _backupX;
    static loco_global<int16_t, 0x01136254> _backupY;
    static loco_global<uint8_t, 0x01136258> _backupZ;
    static loco_global<EntityId, 0x0113642A> _113642A; // used by build window and others

    // 0x004B1D96
    static bool aiIsBelowVehicleLimit()
    {
        if (CompanyManager::isPlayerCompany(_updatingCompanyId))
        {
            return true;
        }

        const auto& companies = CompanyManager::companies();
        auto totalAiVehicles = std::accumulate(companies.begin(), companies.end(), 0, [](const int32_t total, const auto& company) {
            if (CompanyManager::isPlayerCompany(company.id()))
                return total;
            return total + std::accumulate(std::begin(company.transportTypeCount), std::end(company.transportTypeCount), 0);
        });

        if (totalAiVehicles > kMaxAiVehicles)
        {
            setErrorText(StringIds::too_many_vehicles);
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

        if (RoutingManager::isEmptyRoutingSlotAvailable())
        {
            return true;
        }
        setErrorText(StringIds::too_many_vehicles);
        return false;
    }

    template<typename T>
    static T* createVehicleBaseEntity()
    {
        auto* const base = EntityManager::createEntityVehicle();
        base->baseType = EntityBaseType::vehicle;
        auto* const vehicleBase = base->asBase<Vehicles::VehicleBase>();
        vehicleBase->setSubType(T::kVehicleThingType);
        return static_cast<T*>(vehicleBase);
    }

    // 0x004AE8F1, 0x004AEA9E
    static VehicleBogie* createBogie(const EntityId head, const uint16_t vehicleTypeId, [[maybe_unused]] const VehicleObject& vehObject, const uint8_t bodyNumber, VehicleBase* const lastVeh, const ColourScheme colourScheme)
    {
        auto newBogie = createVehicleBaseEntity<VehicleBogie>();
        newBogie->owner = _updatingCompanyId;
        newBogie->head = head;
        newBogie->bodyIndex = bodyNumber;
        newBogie->trackType = lastVeh->getTrackType();
        newBogie->mode = lastVeh->getTransportMode();
        newBogie->tileX = -1;
        newBogie->tileY = 0;
        newBogie->tileBaseZ = 0;
        newBogie->subPosition = 0;
        newBogie->trackAndDirection = TrackAndDirection(0, 0);
        newBogie->routingHandle = lastVeh->getRoutingHandle();
        newBogie->objectId = vehicleTypeId;

        auto& prng = gPrng1();
        newBogie->var_44 = prng.randNext();
        newBogie->creationDay = getCurrentDay();
        newBogie->var_46 = 0;
        newBogie->var_47 = 0;
        newBogie->secondaryCargo.acceptedTypes = 0;
        newBogie->secondaryCargo.type = 0xFF;
        newBogie->secondaryCargo.qty = 0;
        newBogie->var_5E = 0;
        newBogie->breakdownFlags = BreakdownFlags::none;
        newBogie->var_60 = 0; // different to createbody
        newBogie->var_61 = 0; // different to createbody

        newBogie->spriteWidth = 1;
        newBogie->spriteHeightNegative = 1;
        newBogie->spriteHeightPositive = 1;

        newBogie->colourScheme = colourScheme;
        lastVeh->setNextCar(newBogie->id);
        return newBogie;
    }

    // 0x4AE8F1
    static VehicleBogie* createFirstBogie(const EntityId head, const uint16_t vehicleTypeId, const VehicleObject& vehObject, const uint8_t bodyNumber, VehicleBase* const lastVeh, const ColourScheme colourScheme)
    {
        auto newBogie = createBogie(head, vehicleTypeId, vehObject, bodyNumber, lastVeh, colourScheme);
        if (newBogie == nullptr) // Can never happen
        {
            return nullptr;
        }
        newBogie->var_38 = Flags38::none;

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
        sub_4BA873(*newBogie);

        // Calculate refund cost == 7/8 * cost
        auto cost = Economy::getInflationAdjustedCost(vehObject.costFactor, vehObject.costIndex, 6);
        newBogie->refundCost = cost - cost / 8;

        if (bodyNumber == 0)
        {
            // Only front car components front bogie can have cargo
            // stores only secondary cargo presumably due to space constraints
            // in the front car component body
            if (vehObject.numSimultaneousCargoTypes > 1)
            {
                newBogie->secondaryCargo.maxQty = vehObject.maxCargo[1];
                newBogie->secondaryCargo.acceptedTypes = vehObject.compatibleCargoCategories[1];
                auto cargoType = Numerics::bitScanForward(newBogie->secondaryCargo.acceptedTypes);
                if (cargoType != -1)
                {
                    newBogie->secondaryCargo.type = cargoType;
                }
            }
        }

        newBogie->objectSpriteType = vehObject.var_24[bodyNumber].frontBogieSpriteInd;
        if (newBogie->objectSpriteType != SpriteIndex::null)
        {
            newBogie->spriteWidth = vehObject.bogieSprites[newBogie->objectSpriteType].width;
            newBogie->spriteHeightNegative = vehObject.bogieSprites[newBogie->objectSpriteType].heightNegative;
            newBogie->spriteHeightPositive = vehObject.bogieSprites[newBogie->objectSpriteType].heightPositive;
        }
        return newBogie;
    }

    // 0x004AEA9E
    static VehicleBogie* createSecondBogie(const EntityId head, const uint16_t vehicleTypeId, const VehicleObject& vehObject, const uint8_t bodyNumber, VehicleBase* const lastVeh, const ColourScheme colourScheme)
    {
        auto newBogie = createBogie(head, vehicleTypeId, vehObject, bodyNumber, lastVeh, colourScheme);
        if (newBogie == nullptr) // Can never happen
        {
            return nullptr;
        }
        newBogie->var_38 = Flags38::isReversed;
        newBogie->objectSpriteType = vehObject.var_24[bodyNumber].backBogieSpriteInd;
        if (newBogie->objectSpriteType != SpriteIndex::null)
        {
            newBogie->spriteWidth = vehObject.bogieSprites[newBogie->objectSpriteType].width;
            newBogie->spriteHeightNegative = vehObject.bogieSprites[newBogie->objectSpriteType].heightNegative;
            newBogie->spriteHeightPositive = vehObject.bogieSprites[newBogie->objectSpriteType].heightPositive;
        }
        return newBogie;
    }

    // 0x004AEA9E
    static VehicleBody* createBody(const EntityId head, const uint16_t vehicleTypeId, const VehicleObject& vehObject, const uint8_t bodyNumber, VehicleBase* const lastVeh, const ColourScheme colourScheme)
    {
        auto newBody = createVehicleBaseEntity<VehicleBody>();
        // TODO: move this into the create function somehow
        newBody->setSubType(bodyNumber == 0 ? VehicleEntityType::body_start : VehicleEntityType::body_continued);
        newBody->owner = _updatingCompanyId;
        newBody->head = head;
        newBody->bodyIndex = bodyNumber;
        newBody->trackType = lastVeh->getTrackType();
        newBody->mode = lastVeh->getTransportMode();
        newBody->tileX = -1;
        newBody->tileY = 0;
        newBody->tileBaseZ = 0;
        newBody->subPosition = 0;
        newBody->trackAndDirection = TrackAndDirection(0, 0);
        newBody->routingHandle = lastVeh->getRoutingHandle();
        newBody->var_38 = Flags38::unk_0; // different to create bogie
        newBody->objectId = vehicleTypeId;

        auto& prng = gPrng1();
        newBody->var_44 = prng.randNext();
        newBody->creationDay = getCurrentDay();
        newBody->var_46 = 0;
        newBody->var_47 = 0;
        newBody->primaryCargo.acceptedTypes = 0;
        newBody->primaryCargo.type = 0xFF;
        newBody->primaryCargo.qty = 0;
        newBody->var_55 = 0; // different to create bogie
        newBody->var_5E = 0;
        newBody->breakdownFlags = BreakdownFlags::none;

        // different to create bogie
        if (bodyNumber == 0)
        {
            // If the car carries any type of cargo then it will have a primary
            // cargo in the first body of the first car component of the car.
            // Locomotives do not carry any cargo.
            if (vehObject.numSimultaneousCargoTypes != 0)
            {
                newBody->primaryCargo.maxQty = vehObject.maxCargo[0];
                newBody->primaryCargo.acceptedTypes = vehObject.compatibleCargoCategories[0];
                auto cargoType = Numerics::bitScanForward(newBody->primaryCargo.acceptedTypes);
                if (cargoType != -1)
                {
                    newBody->primaryCargo.type = cargoType;
                }
            }
        }

        newBody->spriteWidth = 1;
        newBody->spriteHeightNegative = 1;
        newBody->spriteHeightPositive = 1;

        // different onwards to create bogie
        auto spriteType = vehObject.var_24[bodyNumber].bodySpriteInd;
        if (spriteType != SpriteIndex::null)
        {
            if (spriteType & SpriteIndex::flag_unk7)
            {
                newBody->var_38 |= Flags38::isReversed;
                spriteType &= ~SpriteIndex::flag_unk7;
            }
        }
        newBody->objectSpriteType = spriteType;

        if (newBody->objectSpriteType != SpriteIndex::null)
        {
            newBody->spriteWidth = vehObject.bodySprites[newBody->objectSpriteType].width;
            newBody->spriteHeightNegative = vehObject.bodySprites[newBody->objectSpriteType].heightNegative;
            newBody->spriteHeightPositive = vehObject.bodySprites[newBody->objectSpriteType].heightPositive;
        }

        newBody->colourScheme = colourScheme; // same as create bogie

        if (bodyNumber == 0 && vehObject.hasFlags(VehicleObjectFlags::flag_02))
        {
            newBody->var_38 |= Flags38::unk_3;
        }

        if (bodyNumber + 1 == vehObject.var_04 && vehObject.hasFlags(VehicleObjectFlags::flag_03))
        {
            newBody->var_38 |= Flags38::unk_3;
        }

        lastVeh->setNextCar(newBody->id); // same as create bogie
        return newBody;
    }

    // 0x004AE86D
    static bool createCar(VehicleHead* head, const uint16_t vehicleTypeId)
    {
        if (!EntityManager::checkNumFreeEntities(kMaxNumVehicleComponentsInCar))
        {
            return false;
        }

        // Get Car insertion location
        Vehicle train(*head);
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
        const auto company = CompanyManager::get(_updatingCompanyId);
        _1136140 = company->mainColours; // Copy to global variable. Can be removed when all global uses confirmed
        auto colourScheme = company->mainColours;
        if (company->customVehicleColoursSet & (1 << vehObject->colourType))
        {
            _1136140 = company->vehicleColours[vehObject->colourType - 1]; // Copy to global variable. Can be removed when all global uses confirmed
            colourScheme = company->vehicleColours[vehObject->colourType - 1];
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

    // 0x004B64F9
    static uint16_t createUniqueTypeNumber(const VehicleType type)
    {
        std::array<bool, Limits::kMaxVehicles> _unkArr{};
        for (auto* v : VehicleManager::VehicleList())
        {
            if (v->owner == _updatingCompanyId && v->vehicleType == type)
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
    static VehicleHead* createHead(const uint8_t trackType, const TransportMode mode, const RoutingHandle routingHandle, const VehicleType vehicleType)
    {
        auto* const newHead = createVehicleBaseEntity<VehicleHead>();
        EntityManager::moveEntityToList(newHead, EntityManager::EntityListType::vehicleHead);
        newHead->owner = _updatingCompanyId;
        newHead->head = newHead->id;
        newHead->vehicleFlags |= VehicleFlags::commandStop;
        newHead->trackType = trackType;
        newHead->mode = mode;
        newHead->tileX = -1;
        newHead->tileY = 0;
        newHead->tileBaseZ = 0;
        newHead->remainingDistance = 0;
        newHead->subPosition = 0;
        newHead->trackAndDirection = TrackAndDirection(0, 0);
        newHead->routingHandle = routingHandle;
        newHead->spriteWidth = 0;
        newHead->spriteHeightNegative = 0;
        newHead->spriteHeightPositive = 0;
        newHead->var_38 = Flags38::none;
        newHead->var_3C = 0;
        newHead->vehicleType = vehicleType;
        newHead->name = static_cast<uint8_t>(vehicleType) + 4;
        newHead->ordinalNumber = 0; // Reset to null value so ignored in next function
        newHead->ordinalNumber = createUniqueTypeNumber(vehicleType);
        newHead->var_52 = 0;
        newHead->var_5C = 0;
        newHead->status = Status::unk_0;
        newHead->stationId = StationId::null;
        newHead->breakdownFlags = BreakdownFlags::none;
        newHead->var_60 = 0xFFU;
        newHead->var_61 = 0xFFU;
        newHead->totalRefundCost = 0;
        newHead->lastAverageSpeed = 0_mph;
        newHead->restartStoppedCarsTimeout = 0;
        OrderManager::allocateOrders(*newHead);
        return newHead;
    }

    // 0x004AE40E
    static Vehicle1* createVehicle1(const EntityId head, VehicleBase* const lastVeh)
    {
        auto* const newVeh1 = createVehicleBaseEntity<Vehicle1>();
        newVeh1->owner = _updatingCompanyId;
        newVeh1->head = head;
        newVeh1->trackType = lastVeh->getTrackType();
        newVeh1->mode = lastVeh->getTransportMode();
        newVeh1->tileX = -1;
        newVeh1->tileY = 0;
        newVeh1->tileBaseZ = 0;
        newVeh1->remainingDistance = 0;
        newVeh1->subPosition = 0;
        newVeh1->trackAndDirection = TrackAndDirection(0, 0);
        newVeh1->routingHandle = lastVeh->getRoutingHandle();
        newVeh1->spriteWidth = 0;
        newVeh1->spriteHeightNegative = 0;
        newVeh1->spriteHeightPositive = 0;
        newVeh1->var_38 = Flags38::none;
        newVeh1->var_3C = 0;
        newVeh1->targetSpeed = 0_mph;
        newVeh1->timeAtSignal = 0;
        newVeh1->var_48 = Flags48::none;
        newVeh1->var_52 = 0;
        newVeh1->var_4E = 0;
        newVeh1->var_50 = 0;
        newVeh1->lastIncome.day = -1;
        lastVeh->setNextCar(newVeh1->id);
        return newVeh1;
    }

    // 0x004AE4A0
    static Vehicle2* createVehicle2(const EntityId head, VehicleBase* const lastVeh)
    {
        auto* const newVeh2 = createVehicleBaseEntity<Vehicle2>();
        newVeh2->owner = _updatingCompanyId;
        newVeh2->head = head;
        newVeh2->trackType = lastVeh->getTrackType();
        newVeh2->mode = lastVeh->getTransportMode();
        newVeh2->tileX = -1;
        newVeh2->tileY = 0;
        newVeh2->tileBaseZ = 0;
        newVeh2->remainingDistance = 0;
        newVeh2->subPosition = 0;
        newVeh2->trackAndDirection = TrackAndDirection(0, 0);
        newVeh2->routingHandle = lastVeh->getRoutingHandle();
        newVeh2->spriteWidth = 0;
        newVeh2->spriteHeightNegative = 0;
        newVeh2->spriteHeightPositive = 0;
        newVeh2->var_38 = Flags38::none;
        newVeh2->currentSpeed = 0.0_mph;
        newVeh2->var_5A = 0;
        newVeh2->var_5B = 0;
        newVeh2->drivingSoundId = SoundObjectId::null;
        newVeh2->objectId = 0xFFFFU;
        newVeh2->soundFlags = Vehicles::SoundFlags::none;
        newVeh2->curMonthRevenue = 0;
        newVeh2->profit[0] = 0;
        newVeh2->profit[1] = 0;
        newVeh2->profit[2] = 0;
        newVeh2->profit[3] = 0;
        newVeh2->reliability = 0;
        newVeh2->var_73 = Flags73::none;
        lastVeh->setNextCar(newVeh2->id);
        return newVeh2;
    }

    // 0x004AE54E
    static VehicleTail* createVehicleTail(const EntityId head, VehicleBase* const lastVeh)
    {
        auto* const newTail = createVehicleBaseEntity<VehicleTail>();
        newTail->owner = _updatingCompanyId;
        newTail->head = head;
        newTail->trackType = lastVeh->getTrackType();
        newTail->mode = lastVeh->getTransportMode();
        newTail->tileX = -1;
        newTail->tileY = 0;
        newTail->tileBaseZ = 0;
        newTail->remainingDistance = 0;
        newTail->subPosition = 0;
        newTail->trackAndDirection = TrackAndDirection(0, 0);
        newTail->routingHandle = lastVeh->getRoutingHandle();
        newTail->spriteWidth = 0;
        newTail->spriteHeightNegative = 0;
        newTail->spriteHeightPositive = 0;
        newTail->var_38 = Flags38::none;
        newTail->drivingSoundId = SoundObjectId::null;
        newTail->objectId = 0xFFFFU;
        newTail->soundFlags = Vehicles::SoundFlags::none;
        newTail->trainDanglingTimeout = 0;
        lastVeh->setNextCar(newTail->id);
        newTail->nextCarId = EntityId::null;
        return newTail;
    }

    static std::optional<RoutingHandle> getAndAllocateFreeRoutingHandle()
    {
        if (!aiIsBelowVehicleLimit())
        {
            return std::nullopt;
        }
        auto res = RoutingManager::getAndAllocateFreeRoutingHandle();
        if (res.has_value())
        {
            return res;
        }

        setErrorText(StringIds::too_many_vehicles);
        return std::nullopt;
    }

    // 0x004AE318
    static std::optional<VehicleHead*> createBaseVehicle(const TransportMode mode, const VehicleType type, const uint8_t trackType)
    {
        if (!EntityManager::checkNumFreeEntities(kNumVehicleComponentsInBase))
        {
            return {};
        }

        if (OrderManager::orderTableLength() >= Limits::kMaxOrders)
        {
            setErrorText(StringIds::no_space_for_more_vehicle_orders);
            return {};
        }

        auto routingHandle = getAndAllocateFreeRoutingHandle();
        if (!routingHandle)
        {
            return {};
        }

        auto* head = createHead(trackType, mode, *routingHandle, type);
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

    // 0x004AE6DE
    static void updateWholeVehicle(VehicleHead* const head)
    {
        head->sub_4AF7A4();
        auto company = CompanyManager::get(_updatingCompanyId);
        company->recalculateTransportCounts();

        if (_backupVeh0 != reinterpret_cast<VehicleHead*>(-1))
        {
            VehicleManager::placeDownVehicle(_backupVeh0, _backupX, _backupY, _backupZ, _backup2C, _backup2E);
        }

        Ui::WindowManager::invalidate(Ui::WindowType::vehicleList, enumValue(head->owner));
    }

    // 0x004AE74E
    static uint32_t createNewVehicle(const uint8_t flags, const uint16_t vehicleTypeId)
    {
        setPosition({ Location::null, 0, 0 });
        if (!EntityManager::checkNumFreeEntities(kMaxNumVehicleComponentsInCar + kNumVehicleComponentsInBase))
        {
            return FAILURE;
        }

        if (!isEmptyVehicleSlotAvailable())
        {
            return FAILURE;
        }

        if (flags & Flags::apply)
        {
            auto vehObject = ObjectManager::get<VehicleObject>(vehicleTypeId);

            auto head = createBaseVehicle(vehObject->mode, vehObject->type, vehObject->trackType);
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
                RoutingManager::freeRoutingHandle(_head->routingHandle);
                OrderManager::freeOrders(_head);
                MessageManager::removeAllSubjectRefs(enumValue(_head->id), MessageItemArgumentType::vehicle);
                auto veh1 = _head->nextVehicleComponent();
                if (veh1 == nullptr)
                {
                    throw Exception::RuntimeError("Bad vehicle structure");
                }
                auto veh2 = veh1->nextVehicleComponent();
                if (veh2 == nullptr)
                {
                    throw Exception::RuntimeError("Bad vehicle structure");
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
        auto cost = Economy::getInflationAdjustedCost(vehObject->costFactor, vehObject->costIndex, 6);
        return cost;
    }

    // 0x004AE5FF
    static uint32_t addCarToVehicle(const uint8_t flags, const uint16_t vehicleTypeId, const EntityId headId)
    {
        Vehicle train(headId);
        setPosition(train.veh2->position);

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

        if (!EntityManager::checkNumFreeEntities(kMaxNumVehicleComponentsInCar))
        {
            return FAILURE;
        }

        if (flags & Flags::apply)
        {
            if (train.head->tileX != -1)
            {
                _backupX = train.head->tileX;
                _backupY = train.head->tileY;
                _backupZ = train.head->tileBaseZ;
                _backup2C = train.head->trackAndDirection;
                _backup2E = train.head->subPosition;
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
                Vehicle bkupTrain(*veh0backup);
                if (!bkupTrain.cars.empty())
                {
                    VehicleManager::placeDownVehicle(_backupVeh0, _backupX, _backupY, _backupZ, _backup2C, _backup2E);
                }
                return FAILURE;
            }
        }
        // 0x4AE733
        auto vehObject = ObjectManager::get<VehicleObject>(vehicleTypeId);
        auto cost = Economy::getInflationAdjustedCost(vehObject->costFactor, vehObject->costIndex, 6);
        return cost;
    }

    // 0x004AE5E4
    static uint32_t createVehicle(const uint8_t flags, const uint16_t vehicleTypeId, const EntityId headId)
    {
        setExpenditureType(ExpenditureType::VehiclePurchases);
        _backupVeh0 = reinterpret_cast<VehicleHead*>(-1);

        const auto* company = CompanyManager::get(CompanyManager::getUpdatingCompanyId());
        auto vehicleIsLocked = !company->isVehicleIndexUnlocked(static_cast<uint16_t>(vehicleTypeId));

        if (vehicleIsLocked && !Config::get().buildLockedVehicles)
        {
            setErrorText(StringIds::vehicle_is_locked);
            return FAILURE;
        }

        if (headId == EntityId::null)
        {
            return createNewVehicle(flags, vehicleTypeId);
        }
        else
        {
            return addCarToVehicle(flags, vehicleTypeId, headId);
        }
    }

    void createVehicle(registers& regs)
    {
        regs.ebx = createVehicle(regs.bl, regs.dx, EntityId(regs.di));
    }
}
