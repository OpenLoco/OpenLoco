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
#include "ScenarioOptions.h"
#include "World/CompanyManager.h"
#include "World/Industry.h"
#include "World/StationManager.h"
#include "World/TownManager.h"

namespace OpenLoco::GameCommands
{
    // 0x00490372
    static StationManager::NearbyStation findNearbyStationAirport(World::Pos3 pos, uint8_t airportObjectId, uint8_t rotation)
    {
        // This function is very similar to StationManager::findNearbyStation differences are marked

        const auto companyId = getUpdatingCompanyId();
        // Change from StationManager::findNearbyStation
        auto* airportObj = ObjectManager::get<AirportObject>(airportObjectId);
        const auto [minExtent, maxExtent] = airportObj->getAirportExtents(World::toTileSpace(pos), rotation);

        // Check area including a 2 tile border around the airport
        const auto tilePosA = minExtent - World::TilePos2(2, 2);
        const auto tilePosB = maxExtent + World::TilePos2(2, 2);

        auto minDistanceStation = StationId::null;
        auto minDistance = std::numeric_limits<int16_t>::max();
        bool isPhysicallyAttached = false;
        for (const auto& tilePos : World::getClampedRange(tilePosA, tilePosB))
        {
            const auto tile = World::TileManager::get(tilePos);
            for (const auto& el : tile)
            {
                auto* elStation = el.as<World::StationElement>();
                if (elStation == nullptr)
                {
                    continue;
                }
                if (elStation->isGhost())
                {
                    continue;
                }
                auto* station = StationManager::get(elStation->stationId());
                if (station->owner != companyId)
                {
                    continue;
                }

                const auto distance = Math::Vector::chebyshevDistance2D(World::toWorldSpace(tilePos), pos);
                if (distance < minDistance)
                {
                    auto distDiffZ = std::abs(elStation->baseHeight() - pos.z);
                    if (distDiffZ > StationManager::kMaxStationNearbyDistance)
                    {
                        continue;
                    }
                    // Change from StationManager::findNearbyStation
                    if ((station->flags & StationFlags::flag_6) != StationFlags::none)
                    {
                        continue;
                    }

                    minDistance = distance + distDiffZ / 2;
                    if (minDistance <= StationManager::kMaxStationNearbyDistance)
                    {
                        isPhysicallyAttached = true;
                    }
                    minDistanceStation = elStation->stationId();
                }
            }
        }

        const auto nearbyEmptyStation = StationManager::findNearbyEmptyStation(pos, companyId, minDistance);
        if (nearbyEmptyStation != StationId::null)
        {
            return StationManager::NearbyStation{ nearbyEmptyStation, isPhysicallyAttached };
        }
        else
        {
            return StationManager::NearbyStation{ minDistanceStation, isPhysicallyAttached };
        }
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
        auto nearbyStation = findNearbyStationAirport(pos, airportObjectId, rotation);
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
    static uint32_t createBuilding(const StationId stationId, const World::TilePos2 pos, const int16_t baseHeight, const uint8_t rotation, const uint8_t variation, const uint8_t airportObjectId, World::TileClearance::RemovedBuildings& removedBuildings, const uint8_t flags)
    {
        auto* airportObj = ObjectManager::get<AirportObject>(airportObjectId);

        // This is identical to createIndustry, createDock but with an airportObject
        // TODO: look into making some sort of common version
        auto clearHeight = 0;
        const auto heights = airportObj->getBuildingPartHeights();
        for (auto part : airportObj->getBuildingParts(variation))
        {
            clearHeight += heights[part];
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
                        surface->setBaseZ(baseHeight / World::kSmallZStep);
                        surface->setClearZ(baseHeight / World::kSmallZStep);
                        surface->setSlope(0);
                        surface->setSnowCoverage(0);
                        surface->setGrowthStage(0);
                    }

                    if (shouldInvalidateTile(flags))
                    {
                        World::TileManager::mapInvalidateTileFull(World::toWorldSpace(tilePos));
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
                    elStation->setStationId(stationId);
                }
                else
                {
                    elStation->setStationId(static_cast<StationId>(0));
                }
                elStation->setGhost(flags & Flags::ghost);
                elStation->setSequenceIndex(offset.index);
                World::AnimationManager::createAnimation(7, World::toWorldSpace(tilePos), elStation->baseZ());

                elStation->setAiAllocated(flags & Flags::aiAllocated);
                if (shouldInvalidateTile(flags))
                {
                    World::TileManager::mapInvalidateTileFull(World::toWorldSpace(tilePos));
                }

                Scenario::getOptions().madeAnyChanges = 1;
            }
        }
        return totalCost;
    }

    // 0x00492C41
    static currency32_t createAirport(const AirportPlacementArgs& args, const uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Construction);
        setPosition(args.pos + World::Pos3(16, 16, 0));

        auto& returnState = getLegacyReturnState();
        returnState.lastPlacedAirport = StationId::null;
        returnState.lastConstructedAdjoiningStation = StationId::null;
        returnState.lastConstructedAdjoiningStationPos = World::Pos2(-1, -1);

        if ((flags & Flags::apply) && !(flags & Flags::aiAllocated))
        {
            companySetObservation(getUpdatingCompanyId(), ObservationStatus::buildingAirport, World::Pos2(args.pos) + World::Pos2{ 16, 16 }, EntityId::null, args.type);
        }

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
            returnState.lastConstructedAdjoiningStationPos = args.pos;
            auto nearbyStation = findNearbyStationAirport(args.pos, args.type, args.rotation);
            returnState.lastConstructedAdjoiningStation = nearbyStation.id;
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
                            returnState.lastPlacedAirport = newStationId;
                            auto* station = StationManager::get(newStationId);
                            station->updateLabel();
                        }
                    }
                    break;
                    case NearbyStationValidation::okay:
                        returnState.lastPlacedAirport = nearbyStationId;
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
                        // returnState.lastPlacedAirport not set but that's fine since this is the no apply side
                    }
                    break;
                    case NearbyStationValidation::okay:
                        returnState.lastPlacedAirport = nearbyStationId;
                        break;
                }
            }
        }

        currency32_t totalCost = Economy::getInflationAdjustedCost(airportObj->buildCostFactor, airportObj->costIndex, 6);

        World::TileClearance::RemovedBuildings removedBuildings{};
        for (auto& buildingPosition : airportObj->getBuildingPositions())
        {
            auto buildingTilePos = Math::Vector::rotate(World::TilePos2(buildingPosition.x, buildingPosition.y), args.rotation) + World::toTileSpace(args.pos);
            const bool is2x2 = airportObj->largeTiles & (1U << buildingPosition.index);
            if (is2x2)
            {
                buildingTilePos -= World::toTileSpace(getBuildingTileOffsets(true)[args.rotation].pos);
            }

            const auto buildingRotation = (args.rotation + buildingPosition.rotation) & 0x3;
            const auto cost = createBuilding(returnState.lastPlacedAirport, buildingTilePos, args.pos.z, buildingRotation, buildingPosition.index, args.type, removedBuildings, flags);
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
            addTileToStation(returnState.lastPlacedAirport, args.pos, args.rotation);

            auto* station = StationManager::get(returnState.lastPlacedAirport);
            station->flags |= StationFlags::flag_6;
            station->airportStartPos = args.pos;
            station->airportRotation = args.rotation;
            station->airportMovementOccupiedEdges = 0;
            station->invalidate();
            recalculateStationModes(returnState.lastPlacedAirport);
            recalculateStationCenter(returnState.lastPlacedAirport);
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
