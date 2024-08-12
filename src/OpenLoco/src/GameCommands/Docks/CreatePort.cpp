#include "CreatePort.h"
#include "Economy/Economy.h"
#include "Economy/Expenditures.h"
#include "Localisation/StringIds.h"
#include "Map/AnimationManager.h"
#include "Map/BuildingElement.h"
#include "Map/IndustryElement.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileClearance.h"
#include "Map/TileManager.h"
#include "Map/TreeElement.h"
#include "Objects/DockObject.h"
#include "Objects/IndustryObject.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "S5/S5.h"
#include "ViewportManager.h"
#include "World/CompanyManager.h"
#include "World/Industry.h"
#include "World/StationManager.h"

namespace OpenLoco::GameCommands
{
    // Used by company ai
    static loco_global<StationId, 0x0112C748> _lastPlacedDockStationId;
    static loco_global<uint32_t, 0x00112C734> _lastConstructedAdjoiningStationId;           // Can be 0xFFFF'FFFFU for no adjoining station
    static loco_global<World::Pos2, 0x00112C792> _lastConstructedAdjoiningStationCentrePos; // Can be x = -1 for no adjoining station

    struct NearbyStation
    {
        StationId id;
        bool isPhysicallyAttached;
    };

    // 0x0049060C
    static NearbyStation sub_49060C(World::Pos3 pos, uint8_t dockObjectId, uint8_t rotation)
    {
        registers regs;
        regs.eax = (pos.x & 0xFFFFU); // eax as we need to empty upper portion of eax
        regs.cx = pos.y;
        regs.dx = pos.z;
        regs.bl = dockObjectId;
        regs.bh = rotation;
        call(0x0049060C, regs);
        NearbyStation result{};
        result.id = static_cast<StationId>(regs.bx);
        result.isPhysicallyAttached = regs.eax & (1U << 31);
        return result;
    }

    // 0x004906AC
    static NearbyStation sub_4906AC(World::Pos3 pos, uint8_t dockObjectId, uint8_t rotation)
    {
        // This one is for ai preview allocated docks
        registers regs;
        regs.eax = (pos.x & 0xFFFFU); // eax as we need to empty upper portion of eax
        regs.cx = pos.y;
        regs.dx = pos.z;
        regs.bl = dockObjectId;
        regs.bh = rotation;
        call(0x004906AC, regs);
        NearbyStation result{};
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

    // 0x0048BDCE & 0x0048BD40
    static std::pair<NearbyStationValidation, StationId> validateNearbyStation(const World::Pos3 pos, const uint8_t docksObjectId, const uint8_t rotation, const uint8_t flags)
    {
        auto func = (flags & Flags::aiAllocated) ? &sub_4906AC : &sub_49060C;
        auto nearbyStation = func(pos, docksObjectId, rotation);
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

    // 0x004FEBD0
    // Where:
    // - P is the port
    // - X represents the border offsets
    //
    //    X X
    //  X P P X
    //  X P P X
    //    X X
    static constexpr std::array<World::Pos2, 8> kPortBorderOffsets = {
        World::Pos2{ -32, 0 },
        World::Pos2{ -32, 32 },
        World::Pos2{ 0, 64 },
        World::Pos2{ 32, 64 },
        World::Pos2{ 64, 32 },
        World::Pos2{ 64, 0 },
        World::Pos2{ 32, -32 },
        World::Pos2{ 0, -32 },
    };

    static bool isValidWaterIndustryPort(World::Pos2 pos)
    {
        for (auto& offset : kPortBorderOffsets)
        {
            const auto testPos = pos + offset;
            if (!World::validCoords(testPos))
            {
                continue;
            }

            auto tile = World::TileManager::get(testPos);
            for (auto& el : tile)
            {
                auto* elIndustry = el.as<World::IndustryElement>();
                if (elIndustry == nullptr)
                {
                    continue;
                }
                if (elIndustry->isGhost())
                {
                    continue;
                }
                auto* industry = elIndustry->industry();
                auto* industryObj = ObjectManager::get<IndustryObject>(industry->objectId);
                if (industryObj->hasFlags(IndustryObjectFlags::builtOnWater))
                {
                    return true;
                }
            }
        }
        return false;
    }

    // 0x00493F0E
    static uint32_t createBuilding(const PortPlacementArgs& args, const uint8_t flags, std::set<World::Pos3, World::LessThanPos3>& removedBuildings, const uint8_t buildingType)
    {
        // 0x00112C80B
        bool isWaterIndustryPort = false;
        if (World::validCoords(args.pos))
        {
            auto* elSurface = World::TileManager::get(args.pos).surface();
            if (elSurface->water())
            {
                isWaterIndustryPort = isValidWaterIndustryPort(args.pos);
                if (!isWaterIndustryPort)
                {
                    setErrorText(StringIds::can_only_be_built_on_water_next_to_water_based_industry);
                    return FAILURE;
                }
            }
        }

        auto* dockObj = ObjectManager::get<DockObject>(args.type);
        // This is identical to createIndustry but with a DockObject
        // TODO: look into making some sort of common version
        auto clearHeight = 0;
        for (auto part : dockObj->getBuildingParts(buildingType))
        {
            clearHeight += dockObj->partHeights[part];
        }
        // ceil to 4
        clearHeight += 3;
        clearHeight &= ~3;

        currency32_t totalCost = 0;
        const auto buildingFootprint = getBuildingTileOffsets(true);
        for (auto& offset : buildingFootprint)
        {
            const auto tilePos = World::toTileSpace(World::Pos2(args.pos) + offset.pos);
            if (!World::validCoords(tilePos))
            {
                setErrorText(StringIds::off_edge_of_map);
                return FAILURE;
            }

            if ((flags & Flags::apply) && !(flags & Flags::ghost))
            {
                World::TileManager::removeAllWallsOnTileBelow(tilePos, (args.pos.z + clearHeight) / World::kSmallZStep);
            }

            // 0x0049434F same as TileClearance::tileClearFunction but collides on surfaces as well
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
            if (isWaterIndustryPort)
            {
                auto tile = World::TileManager::get(tilePos);
                auto* surface = tile.surface();
                if (surface->waterHeight() != args.pos.z)
                {
                    // TODO: Give a better message
                    setErrorText(StringIds::empty);
                    return FAILURE;
                }
                if (surface->hasType6Flag())
                {
                    setErrorText(StringIds::water_channel_currently_needed_by_ships);
                    return FAILURE;
                }
                const auto baseZ = args.pos.z / World::kSmallZStep;
                const auto clearZ = (args.pos.z + clearHeight) / World::kSmallZStep;

                World::QuarterTile qt(0xF, 0xF);
                if (!World::TileClearance::applyClearAtStandardHeight(World::toWorldSpace(tilePos), baseZ, clearZ, qt, clearFunc))
                {
                    return FAILURE;
                }
            }
            else
            {
                auto tile = World::TileManager::get(tilePos);
                auto* surface = tile.surface();
                const auto argBaseZ = args.pos.z / World::kSmallZStep;
                const auto argClearZ = (args.pos.z + clearHeight) / World::kSmallZStep;
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

            if (!isWaterIndustryPort)
            {
                auto tile = World::TileManager::get(tilePos);
                auto* surface = tile.surface();
                auto baseZDiff = std::abs((args.pos.z / World::kSmallZStep) - surface->baseZ());
                if (surface->slope())
                {
                    baseZDiff++;
                }

                auto* landObj = ObjectManager::get<LandObject>(surface->terrain());
                totalCost += Economy::getInflationAdjustedCost(landObj->costFactor, landObj->costIndex, 10) * baseZDiff;
                // Flatten surfaces
                if (!(flags & Flags::ghost) && (flags & Flags::apply))
                {
                    if (surface->slope() || args.pos.z != surface->baseHeight())
                    {
                        if (flags & Flags::aiAllocated)
                        {
                            surface->setAiAllocated(true);
                        }
                        else
                        {
                            World::TileManager::mapInvalidateTileFull(World::toWorldSpace(tilePos));
                            surface->setBaseZ(args.pos.z / World::kSmallZStep);
                            surface->setClearZ(args.pos.z / World::kSmallZStep);
                            surface->setSlope(0);
                            surface->setSnowCoverage(0);
                            surface->setVar6SLR5(0);
                        }
                    }
                }
            }

            // Create new tile
            if (flags & Flags::apply)
            {
                if (!(flags & Flags::ghost))
                {
                    World::TileManager::removeSurfaceIndustry(World::toWorldSpace(tilePos));
                    World::TileManager::setTerrainStyleAsCleared(World::toWorldSpace(tilePos));
                }
                auto* elStation = World::TileManager::insertElement<World::StationElement>(World::toWorldSpace(tilePos), args.pos.z / World::kSmallZStep, 0xF);
                if (elStation == nullptr)
                {
                    return FAILURE;
                }
                elStation->setClearZ((clearHeight / World::kSmallZStep) + elStation->baseZ());
                elStation->setRotation(args.rotation);
                elStation->setObjectId(args.type);
                elStation->setStationType(StationType::docks);
                elStation->setOwner(getUpdatingCompanyId());
                elStation->setUnk4SLR4(0);
                elStation->setBuildingType(buildingType);
                if (!(flags & Flags::ghost))
                {
                    elStation->setStationId(_lastPlacedDockStationId);
                }
                else
                {
                    elStation->setStationId(static_cast<StationId>(0));
                }
                elStation->setGhost(flags & Flags::ghost);
                elStation->setSequenceIndex(offset.index);
                World::AnimationManager::createAnimation(8, World::toWorldSpace(tilePos), elStation->baseZ());

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

    // 0x004FEB80 & 0x004FEB90
    static constexpr std::array<std::array<World::Pos2, 2>, 4> kRotationToBuildingFront = {
        std::array<World::Pos2, 2>{
            World::Pos2{ 64, 0 },
            World::Pos2{ 64, 32 },
        },
        std::array<World::Pos2, 2>{
            World::Pos2{ 0, -32 },
            World::Pos2{ 32, -32 },
        },
        std::array<World::Pos2, 2>{
            World::Pos2{ -32, 32 },
            World::Pos2{ -32, 0 },
        },
        std::array<World::Pos2, 2>{
            World::Pos2{ 32, 64 },
            World::Pos2{ 0, 64 },
        },
    };

    static bool tileHasWater(World::Pos2 pos)
    {
        if (!World::validCoords(pos))
        {
            return false;
        }
        auto* elSurface = World::TileManager::get(pos).surface();
        if (elSurface->water() == 0)
        {
            return false;
        }
        return true;
    }

    static currency32_t createPort(const PortPlacementArgs& args, const uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Construction);
        setPosition(args.pos + World::Pos3(16, 16, 0));

        _lastPlacedDockStationId = StationId::null;

        if ((flags & Flags::apply) && !(flags & Flags::aiAllocated))
        {
            companySetObservation(getUpdatingCompanyId(), ObservationStatus::buildingDock, World::Pos2(args.pos) + World::Pos2{ 16, 16 }, EntityId::null, args.type);
        }

        _lastConstructedAdjoiningStationCentrePos = World::Pos2(-1, -1);
        _lastConstructedAdjoiningStationId = 0xFFFFFFFFU;

        if (!World::TileManager::checkFreeElementsAndReorganise())
        {
            return FAILURE;
        }

        for (auto& frontTile : kRotationToBuildingFront[args.rotation])
        {
            const auto frontPos = frontTile + args.pos;
            if (!tileHasWater(frontPos))
            {
                setErrorText(StringIds::requires_water_in_front_of_dock);
                return FAILURE;
            }
        }

        if ((flags & Flags::ghost) && (flags & Flags::apply))
        {
            _lastConstructedAdjoiningStationCentrePos = args.pos;
            auto nearbyStation = flags & Flags::aiAllocated ? sub_4906AC(args.pos, args.type, args.rotation) : sub_49060C(args.pos, args.type, args.rotation);
            _lastConstructedAdjoiningStationId = static_cast<int16_t>(nearbyStation.id);
        }

        if (!(flags & Flags::ghost))
        {
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
                        const auto newStationId = StationManager::allocateNewStation(args.pos, getUpdatingCompanyId(), 3);
                        if (newStationId != StationId::null)
                        {
                            _lastPlacedDockStationId = newStationId;
                            auto* station = StationManager::get(newStationId);
                            station->updateLabel();
                        }
                    }
                    break;
                    case NearbyStationValidation::okay:
                        _lastPlacedDockStationId = nearbyStationId;
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
                        const auto newStationId = StationManager::allocateNewStation(args.pos, getUpdatingCompanyId(), 3);
                        if (newStationId == StationId::null)
                        {
                            return FAILURE;
                        }
                        StationManager::deallocateStation(newStationId);
                        // _lastPlacedDockStationId not set but that's fine since this is the no apply side
                    }
                    break;
                    case NearbyStationValidation::okay:
                        _lastPlacedDockStationId = nearbyStationId;
                        break;
                }
            }
        }

        auto* dockObj = ObjectManager::get<DockObject>(args.type);
        currency32_t totalCost = Economy::getInflationAdjustedCost(dockObj->buildCostFactor, dockObj->costIndex, 7);

        std::set<World::Pos3, World::LessThanPos3> removedBuildings{};
        const auto buildingCost = createBuilding(args, flags, removedBuildings, 0);
        if (buildingCost == FAILURE)
        {
            return FAILURE;
        }
        totalCost += buildingCost;

        if (!(flags & Flags::ghost) && (flags & Flags::apply))
        {
            addTileToStation(_lastPlacedDockStationId, args.pos, args.rotation);

            auto* station = StationManager::get(_lastPlacedDockStationId);
            station->invalidate();
            recalculateStationModes(_lastPlacedDockStationId);
            recalculateStationCenter(_lastPlacedDockStationId);
            station->updateLabel();
            station->invalidate();
            sub_48D794(*station);
        }
        if (!(flags & (Flags::ghost | Flags::aiAllocated)) && (flags & Flags::apply))
        {
            playConstructionPlacementSound(args.pos);
        }

        // Vanilla did this check wrong
        if ((flags & Flags::apply) && CompanyManager::isPlayerCompany(getUpdatingCompanyId()))
        {
            companyEmotionEvent(getUpdatingCompanyId(), Emotion::thinking);
        }
        return totalCost;
    }

    // 0x00493AA7
    void createPort(registers& regs)
    {
        regs.ebx = createPort(PortPlacementArgs(regs), regs.bl);
    }
}
