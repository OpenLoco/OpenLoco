#include "VehiclePlace.h"
#include "Economy/Expenditures.h"
#include "GameState.h"
#include "Map/RoadElement.h"
#include "Map/TileManager.h"
#include "Map/Track/TrackData.h"
#include "Map/TrackElement.h"
#include "Objects/RoadExtraObject.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackExtraObject.h"
#include "Objects/TrackObject.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleManager.h"
#include "ViewportManager.h"
#include <OpenLoco/Core/Numerics.hpp>

namespace OpenLoco::GameCommands
{
    static bool validateRoadPlacement(World::Pos3 pos, uint16_t trackAndDirection, const Vehicles::VehicleHead& head)
    {
        Vehicles::TrackAndDirection::_RoadAndDirection rtad(0, 0);
        rtad._data = trackAndDirection;

        if (rtad.isReversed())
        {
            rtad.setReversed(false);
            auto& roadSize = World::TrackData::getUnkRoad(rtad._data);
            pos -= roadSize.pos;
            if (roadSize.rotationEnd < 12)
            {
                pos += World::Pos3{ World::kRotationOffset[roadSize.rotationEnd], 0 };
            }
        }
        pos.z += World::TrackData::getRoadPiece(rtad.id())[0].z;

        auto* elRoad = [&pos, &head, &rtad]() -> const World::RoadElement* {
            const auto tile = World::TileManager::get(pos);
            for (auto& el : tile)
            {
                auto* elRoad = el.as<World::RoadElement>();
                if (elRoad == nullptr)
                {
                    continue;
                }
                if (elRoad->baseHeight() != pos.z)
                {
                    continue;
                }
                if (elRoad->isAiAllocated() || elRoad->isGhost())
                {
                    continue;
                }
                if (elRoad->roadObjectId() != head.trackType)
                {
                    if (head.trackType != 0xFF)
                    {
                        continue;
                    }
                    if (!(getGameState().roadObjectIdIsNotTram & (1 << elRoad->roadObjectId())))
                    {
                        continue;
                    }
                }
                if (elRoad->roadId() != rtad.id())
                {
                    continue;
                }
                if (elRoad->rotation() != rtad.cardinalDirection())
                {
                    continue;
                }
                return elRoad;
            }
            return nullptr;
        }();

        if (elRoad == nullptr)
        {
            if (head.trackType != 0xFFU)
            {
                auto* trackObj = ObjectManager::get<RoadObject>(head.trackType);
                FormatArguments::common(trackObj->name);
            }
            else
            {
                FormatArguments::common(StringIds::road);
            }
            setErrorText(StringIds::can_only_be_placed_on_stringid);
            return false;
        }

        if (!(getGameState().roadObjectIdIsFlag7 & (1 << elRoad->roadObjectId())))
        {
            if (!sub_431E6A(elRoad->owner(), reinterpret_cast<const World::TileElement*>(elRoad)))
            {
                return false;
            }
        }

        if ((elRoad->mods() & head.var_53) != head.var_53)
        {
            const auto missingMods = (~elRoad->mods()) & head.var_53;
            const auto firstMissing = Numerics::bitScanForward(missingMods);
            auto* roadObj = ObjectManager::get<RoadObject>(elRoad->roadObjectId());
            auto* roadExtraObj = ObjectManager::get<RoadExtraObject>(roadObj->mods[firstMissing]);
            FormatArguments::common(roadExtraObj->name);
            setErrorText(StringIds::this_vehicle_requires_stringid);
            return false;
        }

        return true;
    }

    static bool validateTrackPlacement(World::Pos3 pos, uint16_t trackAndDirection, const Vehicles::VehicleHead& head)
    {
        Vehicles::TrackAndDirection::_TrackAndDirection ttad(0, 0);
        ttad._data = trackAndDirection;

        if (ttad.isReversed())
        {
            ttad.setReversed(false);
            auto& trackSize = World::TrackData::getUnkTrack(ttad._data);
            pos -= trackSize.pos;
            if (trackSize.rotationEnd < 12)
            {
                pos += World::Pos3{ World::kRotationOffset[trackSize.rotationEnd], 0 };
            }
        }
        pos.z += World::TrackData::getTrackPiece(ttad.id())[0].z;

        auto* elTrack = [&pos, &head, &ttad]() -> const World::TrackElement* {
            const auto tile = World::TileManager::get(pos);
            for (auto& el : tile)
            {
                auto* elTrack = el.as<World::TrackElement>();
                if (elTrack == nullptr)
                {
                    continue;
                }
                if (elTrack->baseHeight() != pos.z)
                {
                    continue;
                }
                if (elTrack->isAiAllocated() || elTrack->isGhost())
                {
                    continue;
                }
                if (elTrack->trackObjectId() != head.trackType)
                {
                    continue;
                }
                if (elTrack->trackId() != ttad.id())
                {
                    continue;
                }
                if (elTrack->rotation() != ttad.cardinalDirection())
                {
                    continue;
                }
                return elTrack;
            }
            return nullptr;
        }();

        if (elTrack == nullptr)
        {
            auto* trackObj = ObjectManager::get<TrackObject>(head.trackType);
            FormatArguments::common(trackObj->name);
            setErrorText(StringIds::can_only_be_placed_on_stringid);
            return false;
        }

        if (!sub_431E6A(elTrack->owner(), reinterpret_cast<const World::TileElement*>(elTrack)))
        {
            return false;
        }

        if ((elTrack->mods() & head.var_53) != head.var_53)
        {
            const auto missingMods = (~elTrack->mods()) & head.var_53;
            const auto firstMissing = Numerics::bitScanForward(missingMods);
            auto* trackObj = ObjectManager::get<TrackObject>(elTrack->trackObjectId());
            auto* trackExtraObj = ObjectManager::get<TrackExtraObject>(trackObj->mods[firstMissing]);
            FormatArguments::common(trackExtraObj->name);
            setErrorText(StringIds::this_vehicle_requires_stringid);
            return false;
        }
        return true;
    }

    // 0x004B01B6
    static currency32_t vehiclePlace(const VehiclePlacementArgs& args, const uint8_t flags)
    {
        if (args.head == EntityId::null)
        {
            return FAILURE;
        }

        try
        {
            Vehicles::Vehicle train(args.head);
            // TODO: Deduplicate
            constexpr std::array<ExpenditureType, 6> vehTypeToCost = {
                ExpenditureType::TrainRunningCosts,
                ExpenditureType::BusRunningCosts,
                ExpenditureType::TruckRunningCosts,
                ExpenditureType::TramRunningCosts,
                ExpenditureType::AircraftRunningCosts,
                ExpenditureType::ShipRunningCosts,
            };
            setExpenditureType(vehTypeToCost[enumValue(train.head->vehicleType)]);
            setPosition(args.pos);

            if (!sub_431E6A(train.head->owner))
            {
                return FAILURE;
            }
            if (!args.convertGhost)
            {
                if (train.head->tileX != -1)
                {
                    setErrorText(StringIds::empty);
                    return FAILURE;
                }
                if (train.cars.empty())
                {
                    setErrorText(StringIds::empty);
                    return FAILURE;
                }
            }

            if (!(flags & Flags::apply))
            {
                return 0;
            }

            if (args.convertGhost)
            {
                train.applyToComponents([](auto& component) {
                    component.var_38 &= ~Vehicles::Flags38::isGhost;
                    Ui::ViewportManager::invalidate(&component, ZoomLevel::eighth);
                });
            }
            else
            {
                World::Pos3 pos = args.pos;

                if (train.head->mode == TransportMode::road)
                {
                    if (!validateRoadPlacement(pos, args.trackAndDirection, *train.head))
                    {
                        return FAILURE;
                    }
                }
                else
                {
                    if (!validateTrackPlacement(pos, args.trackAndDirection, *train.head))
                    {
                        return FAILURE;
                    }
                }
                Vehicles::TrackAndDirection tad(0, 0);
                tad.track._data = args.trackAndDirection;
                const auto res = VehicleManager::placeDownVehicle(train.head, args.pos.x, args.pos.y, args.pos.z / World::kSmallZStep, tad, args.trackProgress);
                if (res != VehicleManager::PlaceDownResult::Okay)
                {
                    setErrorText(
                        res == VehicleManager::PlaceDownResult::Unk0
                            ? StringIds::not_enough_space_or_vehicle_in_the_way
                            : StringIds::vehicle_approaching_or_in_the_way);
                    return FAILURE;
                }

                train.head->vehicleFlags |= VehicleFlags::commandStop;
                train.head->manualPower = -40;
                if (flags & Flags::ghost)
                {
                    train.applyToComponents([](auto& component) {
                        component.var_38 |= Vehicles::Flags38::isGhost;
                    });
                }
            }
        }
        catch (Exception::RuntimeError&)
        {
            return FAILURE;
        }
        if ((flags & Flags::apply) && !(flags & Flags::ghost))
        {
            Vehicles::playPlacedownSound(args.pos);
        }
        return 0;
    }

    void vehiclePlace(registers& regs)
    {
        regs.ebx = vehiclePlace(VehiclePlacementArgs(regs), regs.bl);
    }
}
