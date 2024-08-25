#include "CreateAirport.h"
#include "Economy/Economy.h"
#include "Localisation/StringIds.h"
#include "Map/AnimationManager.h"
#include "Map/BuildingElement.h"
#include "Map/QuarterTile.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileClearance.h"
#include "Map/TileManager.h"
#include "Map/TreeElement.h"
#include "Objects/AirportObject.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "S5/S5.h"
#include "World/CompanyManager.h"
#include "World/Industry.h"
#include "World/StationManager.h"
#include "World/TownManager.h"

namespace OpenLoco::GameCommands
{
    static loco_global<StationId, 0x0112C744> _lastPlacedAirportStationId;
    static loco_global<uint32_t, 0x00112C734> _lastConstructedAdjoiningStationId;           // Can be 0xFFFF'FFFFU for no adjoining station
    static loco_global<World::Pos2, 0x00112C792> _lastConstructedAdjoiningStationCentrePos; // Can be x = -1 for no adjoining station

    // 0x00490372
    static StationManager::NearbyStation sub_490372(World::Pos3 pos, uint8_t airportObjectId, uint8_t rotation)
    {
        registers regs;
        regs.eax = (pos.x & 0xFFFFU); // eax as we need to empty upper portion of eax
        regs.cx = pos.y;
        regs.dx = pos.z;
        regs.bl = airportObjectId;
        regs.bh = rotation;
        call(0x00490372, regs);
        StationManager::NearbyStation result{};
        result.id = static_cast<StationId>(regs.bx);
        result.isPhysicallyAttached = regs.eax & (1U << 31);
        return result;
    }

    enum class NearbyStationValidation
    {
        okay,
        requiresNewStation,
        failure,
    };

    // 0x00492E48 & 0x00492DBA
    static std::pair<NearbyStationValidation, StationId> validateNearbyStation(const World::Pos3 pos, const uint8_t airportObjectId, const uint8_t rotation, const uint8_t flags)
    {
        auto nearbyStation = sub_490372(pos, airportObjectId, rotation);
        if (nearbyStation.id == StationId::null)
        {
            return std::make_pair(NearbyStationValidation::requiresNewStation, StationId::null);
        }

        // _lastPlacedTrackStationId = nearbyStation.id; set in callers
        auto* station = StationManager::get(nearbyStation.id);
        if (station->stationTileSize >= std::size(station->stationTiles))
        {
            if (nearbyStation.isPhysicallyAttached)
            {
                setErrorText(StringIds::station_too_large);
                return std::make_pair(NearbyStationValidation::failure, StationId::null);
            }
            return std::make_pair(NearbyStationValidation::requiresNewStation, StationId::null);
        }
        else
        {
            if (!(flags & Flags::aiAllocated))
            {
                if (StationManager::exceedsStationSize(*station, pos))
                {
                    if (nearbyStation.isPhysicallyAttached)
                    {
                        setErrorText(StringIds::station_too_spread_out);
                        return std::make_pair(NearbyStationValidation::failure, StationId::null);
                    }
                    return std::make_pair(NearbyStationValidation::requiresNewStation, StationId::null);
                }
            }
        }
        return std::make_pair(NearbyStationValidation::okay, nearbyStation.id);
    }

    // 0x004930E1
    static uint32_t createBuilding(const World::TilePos2 pos, const int16_t baseHeight, const uint8_t rotation, const uint8_t variation, const uint8_t airportObjectId, std::set<World::Pos3, World::LessThanPos3>& removedBuildings, const uint8_t flags)
    {
        auto* airportObj = ObjectManager::get<AirportObject>(airportObjectId);

        // This is identical to createIndustry, createDock but with an airportObject
        // TODO: look into making some sort of common version
        auto clearHeight = 0;
        for (auto part : airportObj->getBuildingParts(variation))
        {
            clearHeight += airportObj->buildingPartHeights[part];
        }
        // ceil to 4
        clearHeight += 3;
        clearHeight &= ~3;

        // Unsure why?
        clearHeight = std::max(clearHeight, 128);

        const auto is2x2 = airportObj->largeTiles & (1U << variation);
        currency32_t totalCost = 0;
        const auto buildingFootprint = getBuildingTileOffsets(is2x2);
        for (auto& offset : buildingFootprint)
        {
            const auto tilePos = pos + World::toTileSpace(offset.pos);
            if (!World::validCoords(tilePos))
            {
                setErrorText(StringIds::off_edge_of_map);
                return FAILURE;
            }

            if ((flags & Flags::apply) && !(flags & Flags::ghost) && !(flags & Flags::aiAllocated))
            {
                World::TileManager::removeAllWallsOnTileBelow(tilePos, (baseHeight + clearHeight) / World::kSmallZStep);
            }

            // 0x004933B2 same as TileClearance::tileClearFunction but collides on surfaces as well
            // identical to createPort version
            auto clearFunc = [pos = World::toWorldSpace(tilePos), &removedBuildings, flags, &totalCost](World::TileElement& el) {
                switch (el.type())
                {
                    case World::ElementType::tree:
                    {
                        auto* elTree = el.as<World::TreeElement>();
                        if (elTree == nullptr)
                        {
                            return World::TileClearance::ClearFuncResult::noCollision;
                        }
                        return World::TileClearance::clearTreeCollision(*elTree, pos, flags, totalCost);
                    }
                    case World::ElementType::building:
                    {
                        auto* elBuilding = el.as<World::BuildingElement>();
                        if (elBuilding == nullptr)
                        {
                            return World::TileClearance::ClearFuncResult::noCollision;
                        }
                        return World::TileClearance::clearBuildingCollision(*elBuilding, pos, removedBuildings, flags, totalCost);
                    }
                    default:
                        return World::TileClearance::ClearFuncResult::collision;
                }
            };

            // This is similar but not the same as createIndustry/createBuilding
            {
                auto tile = World::TileManager::get(tilePos);
                auto* surface = tile.surface();
                const auto argBaseZ = baseHeight / World::kSmallZStep;
                const auto argClearZ = (baseHeight + clearHeight) / World::kSmallZStep;
                const auto minBaseZ = std::min<World::SmallZ>(surface->baseZ(), argBaseZ);
                const auto maxBaseZ = std::max<World::SmallZ>(surface->baseZ(), argBaseZ);
                const auto baseZ = std::min<World::SmallZ>(minBaseZ, argClearZ);
                const auto clearZ = std::max<World::SmallZ>(maxBaseZ, argClearZ);

                World::QuarterTile qt(0xF, 0xF);
                if (!World::TileClearance::applyClearAtStandardHeight(World::toWorldSpace(tilePos), baseZ, clearZ, qt, clearFunc))
                {
                    return FAILURE;
                }
            }

            auto tile = World::TileManager::get(tilePos);
            auto* surface = tile.surface();
            auto baseZDiff = std::abs((baseHeight / World::kSmallZStep) - surface->baseZ());
            if (surface->slope())
            {
                baseZDiff++;
            }

            auto* landObj = ObjectManager::get<LandObject>(surface->terrain());
            totalCost += Economy::getInflationAdjustedCost(landObj->costFactor, landObj->costIndex, 10) * baseZDiff;
            // Flatten surfaces
            if (!(flags & Flags::ghost) && (flags & Flags::apply))
            {
                if (surface->slope() || baseHeight != surface->baseHeight())
                {
                    if (flags & Flags::aiAllocated)
                    {
                        surface->setAiAllocated(true);
                    }
                    else
                    {
                        World::TileManager::mapInvalidateTileFull(World::toWorldSpace(tilePos));
                        surface->setBaseZ(baseHeight / World::kSmallZStep);
                        surface->setClearZ(baseHeight / World::kSmallZStep);
                        surface->setSlope(0);
                        surface->setSnowCoverage(0);
                        surface->setVar6SLR5(0);
                    }
                }
            }

            // Create new tile
            if (flags & Flags::apply)
            {
                if (!(flags & Flags::ghost) && !(flags & Flags::aiAllocated))
                {
                    World::TileManager::removeSurfaceIndustry(World::toWorldSpace(tilePos));
                    World::TileManager::setTerrainStyleAsCleared(World::toWorldSpace(tilePos));
                }
                auto* elStation = World::TileManager::insertElement<World::StationElement>(World::toWorldSpace(tilePos), baseHeight / World::kSmallZStep, 0xF);
                if (elStation == nullptr)
                {
                    return FAILURE;
                }
                elStation->setClearZ((clearHeight / World::kSmallZStep) + elStation->baseZ());
                elStation->setRotation(rotation);
                elStation->setObjectId(airportObjectId);
                elStation->setStationType(StationType::airport);
                elStation->setOwner(getUpdatingCompanyId());
                elStation->setUnk4SLR4(0);
                elStation->setBuildingType(variation);
                if (!(flags & Flags::ghost))
                {
                    elStation->setStationId(_lastPlacedAirportStationId);
                }
                else
                {
                    elStation->setStationId(static_cast<StationId>(0));
                }
                elStation->setGhost(flags & Flags::ghost);
                elStation->setSequenceIndex(offset.index);
                World::AnimationManager::createAnimation(7, World::toWorldSpace(tilePos), elStation->baseZ());

                elStation->setAiAllocated(flags & Flags::aiAllocated);
                if (!(flags & Flags::aiAllocated))
                {
                    World::TileManager::mapInvalidateTileFull(World::toWorldSpace(tilePos));
                }

                S5::getOptions().madeAnyChanges = 1;
            }
        }
        return totalCost;
    }

    // 0x00492C41
    static currency32_t createAirport(const AirportPlacementArgs& args, const uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Construction);
        setPosition(args.pos + World::Pos3(16, 16, 0));

        _lastPlacedAirportStationId = StationId::null;

        if ((flags & Flags::apply) && !(flags & Flags::aiAllocated))
        {
            companySetObservation(getUpdatingCompanyId(), ObservationStatus::buildingAirport, World::Pos2(args.pos) + World::Pos2{ 16, 16 }, EntityId::null, args.type);
        }

        _lastConstructedAdjoiningStationCentrePos = World::Pos2(-1, -1);
        _lastConstructedAdjoiningStationId = 0xFFFFFFFFU;

        if (!World::TileManager::checkFreeElementsAndReorganise())
        {
            return FAILURE;
        }

        const auto closestTown = TownManager::getClosestTownAndDensity(args.pos);
        if (!closestTown.has_value())
        {
            return FAILURE;
        }

        auto* town = TownManager::get(closestTown->first);
        if (town->numberOfAirports >= 4)
        {
            setErrorText(StringIds::town_will_not_allow_airport_to_be_built_here);
            return FAILURE;
        }

        if (!CompanyManager::isPlayerCompany(getUpdatingCompanyId()))
        {
            if (town->numberOfAirports >= 2)
            {
                setErrorText(StringIds::town_will_not_allow_airport_to_be_built_here);
                return FAILURE;
            }
        }

        if ((flags & Flags::apply) && !(flags & Flags::aiAllocated) && !(flags & Flags::ghost))
        {
            town->numberOfAirports++;
        }

        if ((flags & Flags::ghost) && (flags & Flags::apply))
        {
            _lastConstructedAdjoiningStationCentrePos = args.pos;
            auto nearbyStation = sub_490372(args.pos, args.type, args.rotation);
            _lastConstructedAdjoiningStationId = static_cast<int16_t>(nearbyStation.id);
        }

        auto* airportObj = ObjectManager::get<AirportObject>(args.type);

        if (!(flags & Flags::ghost))
        {
            const auto nameMode = airportObj->hasFlags(AirportObjectFlags::isHelipad) ? 2 : 1;
            if (flags & Flags::apply)
            {
                auto [result, nearbyStationId] = validateNearbyStation(args.pos, args.type, args.rotation, flags);
                switch (result)
                {
                    case NearbyStationValidation::failure:
                        // Odd???
                        break;
                    case NearbyStationValidation::requiresNewStation:
                    {
                        const auto newStationId = StationManager::allocateNewStation(args.pos, getUpdatingCompanyId(), nameMode);
                        if (newStationId != StationId::null)
                        {
                            _lastPlacedAirportStationId = newStationId;
                            auto* station = StationManager::get(newStationId);
                            station->updateLabel();
                        }
                    }
                    break;
                    case NearbyStationValidation::okay:
                        _lastPlacedAirportStationId = nearbyStationId;
                        break;
                }
            }
            else
            {
                // Same as the other branch but deallocate after allocating and return failure on failure
                auto [result, nearbyStationId] = validateNearbyStation(args.pos, args.type, args.rotation, flags);
                switch (result)
                {
                    case NearbyStationValidation::failure:
                        return FAILURE;
                    case NearbyStationValidation::requiresNewStation:
                    {
                        const auto newStationId = StationManager::allocateNewStation(args.pos, getUpdatingCompanyId(), nameMode);
                        if (newStationId == StationId::null)
                        {
                            return FAILURE;
                        }
                        StationManager::deallocateStation(newStationId);
                        // _lastPlacedAirportStationId not set but that's fine since this is the no apply side
                    }
                    break;
                    case NearbyStationValidation::okay:
                        _lastPlacedAirportStationId = nearbyStationId;
                        break;
                }
            }
        }

        currency32_t totalCost = Economy::getInflationAdjustedCost(airportObj->buildCostFactor, airportObj->costIndex, 6);
        std::set<World::Pos3, World::LessThanPos3> removedBuildings{};
        for (auto& buildingPosition : airportObj->getBuildingPositions())
        {
            auto buildingTilePos = Math::Vector::rotate(World::TilePos2(buildingPosition.x, buildingPosition.y), args.rotation) + World::toTileSpace(args.pos);
            const bool is2x2 = airportObj->largeTiles & (1U << buildingPosition.index);
            if (is2x2)
            {
                buildingTilePos -= World::toTileSpace(getBuildingTileOffsets(true)[args.rotation].pos);
            }

            const auto buildingRotation = (args.rotation + buildingPosition.rotation) & 0x3;
            const auto cost = createBuilding(buildingTilePos, args.pos.z, buildingRotation, buildingPosition.index, args.type, removedBuildings, flags);
            if (cost != FAILURE)
            {
                totalCost += cost;
            }
            else
            {
                return FAILURE;
            }
        }
        if (!(flags & Flags::ghost) && (flags & Flags::apply))
        {
            addTileToStation(_lastPlacedAirportStationId, args.pos, args.rotation);

            auto* station = StationManager::get(_lastPlacedAirportStationId);
            station->flags |= StationFlags::flag_6;
            station->airportStartPos = args.pos;
            station->airportRotation = args.rotation;
            station->airportMovementOccupiedEdges = 0;
            station->invalidate();
            recalculateStationModes(_lastPlacedAirportStationId);
            recalculateStationCenter(_lastPlacedAirportStationId);
            station->updateLabel();
            station->invalidate();
            sub_48D794(*station);
        }
        if (!(flags & (Flags::ghost | Flags::aiAllocated)) && (flags & Flags::apply))
        {
            playConstructionPlacementSound(args.pos);
        }

        if ((flags & Flags::apply) && CompanyManager::isPlayerCompany(getUpdatingCompanyId()))
        {
            companyEmotionEvent(getUpdatingCompanyId(), Emotion::thinking);
        }
        return totalCost;
    }

    void createAirport(registers& regs)
    {
        regs.ebx = createAirport(AirportPlacementArgs(regs), regs.bl);
    }
}
