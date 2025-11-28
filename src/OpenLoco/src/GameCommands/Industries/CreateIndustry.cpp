#include "CreateIndustry.h"
#include "Date.h"
#include "Economy/Economy.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Terraform/CreateWall.h"
#include "Graphics/Colour.h"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Map/AnimationManager.h"
#include "Map/BuildingElement.h"
#include "Map/IndustryElement.h"
#include "Map/RoadElement.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileClearance.h"
#include "Map/TileManager.h"
#include "Map/TrackElement.h"
#include "Map/TreeElement.h"
#include "MessageManager.h"
#include "Objects/IndustryObject.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/ScaffoldingObject.h"
#include "Objects/TreeObject.h"
#include "ScenarioOptions.h"
#include "SceneManager.h"
#include "ViewportManager.h"
#include "World/IndustryManager.h"
#include "World/TownManager.h"
#include <OpenLoco/Core/Numerics.hpp>

namespace OpenLoco::GameCommands
{
    // Convert the cargo id's into a bitset
    static uint32_t getProducedCargoBitSet(const IndustryObject& indObj)
    {
        uint32_t producedCargoTypes = 0;
        for (auto& cargoType : indObj.producedCargoType)
        {
            if (cargoType != 0xFFU)
            {
                producedCargoTypes |= (1U << cargoType);
            }
        }
        return producedCargoTypes;
    }

    // Convert the cargo id's into a bitset
    static uint32_t getRequiredCargoBitSet(const IndustryObject& indObj)
    {
        uint32_t requiredCargoTypes = 0;
        for (auto& cargoType : indObj.requiredCargoType)
        {
            if (cargoType != 0xFFU)
            {
                requiredCargoTypes |= (1U << cargoType);
            }
        }
        return requiredCargoTypes;
    }

    // 0x00454C91
    static IndustryId sub_454C91(uint8_t type, const World::Pos2& pos, const Core::Prng& prng)
    {
        const auto* indObj = ObjectManager::get<IndustryObject>(type);

        // 0x00E0C38C
        const uint32_t producedCargoTypes = getProducedCargoBitSet(*indObj);

        // 0x00E0C390
        const uint32_t requiredCargoTypes = getRequiredCargoBitSet(*indObj);

        for (const auto& ind : IndustryManager::industries())
        {
            const auto distance = Math::Vector::manhattanDistance2D(World::Pos2{ ind.x, ind.y }, pos);

            const auto* indObj2 = ind.getObject();
            const uint32_t producedCargoTypes2 = getProducedCargoBitSet(*indObj2);
            const uint32_t requiredCargoTypes2 = getRequiredCargoBitSet(*indObj2);

            const auto hasAtLeast1CargoRequirement = ((requiredCargoTypes & producedCargoTypes2) != 0)
                || ((producedCargoTypes & requiredCargoTypes2) != 0);

            const auto minIndustryDistance = 32 * (hasAtLeast1CargoRequirement ? 24 : 9);

            if (distance < minIndustryDistance)
            {
                GameCommands::setErrorText(StringIds::too_close_to_another_industry);
                return IndustryId::null;
            }
        }

        const auto res = TownManager::getClosestTownAndDensity(pos);
        if (!res.has_value())
        {
            GameCommands::setErrorText(StringIds::town_must_be_built_nearby_first);
            return IndustryId::null;
        }

        const auto id = IndustryManager::allocateNewIndustry(type, pos, prng, res->first);
        if (id != IndustryId::null)
        {
            return id;
        }
        GameCommands::setErrorText(StringIds::too_many_industries);
        return IndustryId::null;
    }

    // 0x0045572D
    static World::TileClearance::ClearFuncResult tileClearFunction(World::TileElement& el, const World::Pos2 pos, const uint8_t flags, currency32_t& cost)
    {
        auto* elTree = el.as<World::TreeElement>();
        if (elTree == nullptr)
        {
            return World::TileClearance::ClearFuncResult::collision;
        }
        return World::TileClearance::clearTreeCollision(*elTree, pos, flags, cost);
    }

    /* 0x004551CC
       bh:7 = buildImmediately
       bh:0-3 = rotation
       edi = randColour << 16
       dl = buildingType
       dh = industryId
       ax = pos.x
       cx = pos.y
       bl = flags
    */
    static currency32_t placeIndustryBuilding(const IndustryId industryId, const World::Pos2& pos, const uint8_t direction, const uint8_t buildingType, const Colour colour, const bool buildImmediate, const uint8_t flags)
    {
        auto* industry = IndustryManager::get(industryId);
        auto* indObj = industry->getObject();
        if (!World::TileManager::checkFreeElementsAndReorganise())
        {
            return FAILURE;
        }

        const bool isMultiTile = indObj->buildingSizeFlags & (1U << buildingType);

        // Workout the max surface height for building footprint
        const auto buildingFootprint = getBuildingTileOffsets(isMultiTile);

        // 0x00E0C3CB (note this is smallZ)
        World::SmallZ highestBaseZ = 0;
        for (const auto& offset : buildingFootprint)
        {
            const auto tilePos = World::toTileSpace(pos + offset.pos);
            if (!World::validCoords(tilePos))
            {
                continue;
            }
            auto tile = World::TileManager::get(tilePos);
            const auto* surface = tile.surface();

            auto baseZ = World::TileManager::getSurfaceCornerHeight(*surface);
            highestBaseZ = std::max<World::SmallZ>(highestBaseZ, baseZ);
            if (surface->water())
            {
                highestBaseZ = std::max<World::SmallZ>(highestBaseZ, surface->water() * World::kMicroToSmallZStep);
            }
        }

        // Workout clearance height of building (including scaffolding if required)
        const auto buildingParts = indObj->getBuildingParts(buildingType);
        const auto partHeights = indObj->getBuildingPartHeights();
        // 0x00E0C3BC (note this is bigZ and does not include the base height)
        auto clearHeight = 0;
        for (auto part : buildingParts)
        {
            clearHeight += partHeights[part];
        }
        if (!buildImmediate && indObj->scaffoldingSegmentType != 0xFF)
        {
            auto* scaffObj = ObjectManager::get<ScaffoldingObject>();
            const auto segmentHeight = scaffObj->segmentHeights[indObj->scaffoldingSegmentType];
            const bool requiresMore = clearHeight % segmentHeight;
            const auto numSegments = (clearHeight / segmentHeight) + (requiresMore ? 1 : 0);
            clearHeight = (numSegments * segmentHeight) + scaffObj->roofHeights[indObj->scaffoldingSegmentType];
        }
        // ceil to 4
        clearHeight += 3;
        clearHeight &= ~3;

        const auto clearZ = (clearHeight / World::kSmallZStep) + highestBaseZ;

        currency32_t totalCost = 0;

        // Loop over footprint
        for (const auto& offset : buildingFootprint)
        {
            const auto tilePos = World::toTileSpace(pos + offset.pos);
            if (!World::validCoords(tilePos))
            {
                return FAILURE;
            }

            if ((flags & Flags::apply) && !(flags & Flags::ghost))
            {
                World::TileManager::removeAllWallsOnTileBelow(tilePos, highestBaseZ + clearHeight / World::kSmallZStep);
            }

            {
                auto tile = World::TileManager::get(tilePos);
                const auto* surface = tile.surface();

                auto clearFunc = [tilePos, flags, &totalCost](World::TileElement& el) {
                    return tileClearFunction(el, World::toWorldSpace(tilePos), flags, totalCost);
                };

                // Perform clearance checks
                if (indObj->hasFlags(IndustryObjectFlags::builtOnWater))
                {
                    if (surface->water() * World::kMicroToSmallZStep != highestBaseZ)
                    {
                        setErrorText(StringIds::can_only_be_built_on_water);
                        return FAILURE;
                    }

                    if (surface->hasType6Flag())
                    {
                        setErrorText(StringIds::water_channel_currently_needed_by_ships);
                        return FAILURE;
                    }
                    World::QuarterTile qt(0xF, 0xF);
                    if (!World::TileClearance::applyClearAtStandardHeight(World::toWorldSpace(tilePos), highestBaseZ, clearZ, qt, clearFunc))
                    {
                        return FAILURE;
                    }
                }
                else
                {
                    if (surface->water())
                    {
                        setErrorText(StringIds::cant_build_this_underwater);
                        return FAILURE;
                    }
                    World::QuarterTile qt(0xF, 0xF);
                    if (!World::TileClearance::applyClearAtStandardHeight(World::toWorldSpace(tilePos), surface->baseZ(), clearZ, qt, clearFunc))
                    {
                        return FAILURE;
                    }
                    // TODO: This is dangerous pointer might be invalid?
                    if (surface->slope() || surface->baseZ() != highestBaseZ)
                    {
                        const auto* landObj = ObjectManager::get<LandObject>(surface->terrain());
                        totalCost += Economy::getInflationAdjustedCost(landObj->costFactor, landObj->costIndex, 10);
                    }
                }
            }

            // Flatten surfaces (also checks if other elements will cause issues due to the flattening of the surface)
            if (!(flags & Flags::ghost) && !indObj->hasFlags(IndustryObjectFlags::builtOnWater))
            {
                auto tile = World::TileManager::get(tilePos);
                auto* surface = tile.surface();

                if (surface->slope() || highestBaseZ != surface->baseZ())
                {
                    if (highestBaseZ < surface->baseZ())
                    {
                        bool hasPassedSurface = false;
                        for (auto& el : tile)
                        {
                            if (el.type() == World::ElementType::surface)
                            {
                                hasPassedSurface = true;
                                continue;
                            }

                            if (hasPassedSurface)
                            {
                                auto* elTrack = el.as<World::TrackElement>();
                                auto* elRoad = el.as<World::RoadElement>();
                                auto* elStation = el.as<World::StationElement>();
                                auto* elBuilding = el.as<World::BuildingElement>();
                                auto* elIndustry = el.as<World::IndustryElement>();
                                auto* elTree = el.as<World::TreeElement>();
                                if (elTrack != nullptr && !elTrack->isGhost() && !elTrack->hasBridge())
                                {
                                    setErrorText(StringIds::empty);
                                    return FAILURE;
                                }
                                else if (elRoad != nullptr && !elRoad->isGhost() && !elRoad->hasBridge())
                                {
                                    setErrorText(StringIds::empty);
                                    return FAILURE;
                                }
                                else if (elStation != nullptr && elStation->stationType() == StationType::airport)
                                {
                                    setErrorText(StringIds::empty);
                                    return FAILURE;
                                }
                                else if (elBuilding != nullptr)
                                {
                                    setErrorText(StringIds::empty);
                                    return FAILURE;
                                }
                                else if (elIndustry != nullptr)
                                {
                                    setErrorText(StringIds::empty);
                                    return FAILURE;
                                }
                                else if (elTree != nullptr && clearZ <= elTree->baseZ())
                                {
                                    setErrorText(StringIds::empty);
                                    return FAILURE;
                                }
                            }
                        }
                    }
                    if (flags & Flags::apply)
                    {
                        World::TileManager::mapInvalidateTileFull(World::toWorldSpace(tilePos));
                        surface->setBaseZ(highestBaseZ);
                        surface->setClearZ(highestBaseZ);
                        surface->setSlope(0);
                        surface->setSnowCoverage(0);
                        surface->setGrowthStage(0);
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
                auto* elIndustry = World::TileManager::insertElement<World::IndustryElement>(World::toWorldSpace(tilePos), highestBaseZ, 0xF);
                if (elIndustry == nullptr)
                {
                    return FAILURE;
                }
                elIndustry->setClearZ(clearZ);
                elIndustry->setRotation(direction);
                elIndustry->setIsConstructed(buildImmediate);
                elIndustry->setIndustryId(industryId);
                elIndustry->setSequenceIndex(offset.index);
                // TODO: If it turns out there are more vars in _5 they should be cleared here
                elIndustry->setSectionProgress(0);
                elIndustry->setColour(colour);
                elIndustry->setBuildingType(buildingType);
                elIndustry->setVar_6_003F(0);
                World::AnimationManager::createAnimation(3, World::toWorldSpace(tilePos), elIndustry->baseZ());
                elIndustry->setGhost(flags & Flags::ghost);
                Ui::ViewportManager::invalidate(World::toWorldSpace(tilePos), elIndustry->baseHeight(), elIndustry->clearHeight());
            }
        }

        auto& tileRef = industry->tiles[industry->numTiles];
        tileRef = World::Pos3(pos, (highestBaseZ * World::kSmallZStep | (isMultiTile ? 0x8000 : 0)));
        industry->numTiles++;
        return totalCost;
    }

    // 0x0045442A
    static Colour getRandomAvailableColour(Core::Prng& prng, const IndustryObject& indObj)
    {
        // Could use a fixed size vector here as its never > Colour::max
        std::vector<Colour> availableColours;
        auto colourBitSet = indObj.availableColours;
        for (auto colourI32 = Numerics::bitScanForward(colourBitSet); colourI32 != -1; colourI32 = Numerics::bitScanForward(colourBitSet))
        {
            colourBitSet &= ~(1ULL << colourI32);
            availableColours.push_back(static_cast<Colour>(colourI32));
        }

        if (availableColours.empty())
        {
            return Colour::black;
        }
        // Note: Don't optimise for size 1 as randNext required to prevent divergence
        return availableColours[prng.randNext(availableColours.size() - 1)];
    }

    struct RangeAndMinDistance
    {
        tile_coord_t distanceRange;
        tile_coord_t minDistance;
    };

    // As more placement attempts happen the distance searched to try place increases
    constexpr std::array<RangeAndMinDistance, 25> kPlacementAttemptDistanceRangeAndMin = {
        RangeAndMinDistance{ 1, 0 }, // 0
        RangeAndMinDistance{ 3, -1 },
        RangeAndMinDistance{ 3, -1 },
        RangeAndMinDistance{ 3, -1 },
        RangeAndMinDistance{ 3, -1 },
        RangeAndMinDistance{ 3, -1 },
        RangeAndMinDistance{ 3, -1 },
        RangeAndMinDistance{ 3, -1 }, // 7
        RangeAndMinDistance{ 5, -2 },
        RangeAndMinDistance{ 5, -2 },
        RangeAndMinDistance{ 5, -2 },
        RangeAndMinDistance{ 5, -2 },
        RangeAndMinDistance{ 5, -2 },
        RangeAndMinDistance{ 5, -2 },
        RangeAndMinDistance{ 5, -2 },
        RangeAndMinDistance{ 5, -2 },
        RangeAndMinDistance{ 5, -2 },
        RangeAndMinDistance{ 5, -2 },
        RangeAndMinDistance{ 5, -2 },
        RangeAndMinDistance{ 5, -2 },
        RangeAndMinDistance{ 5, -2 },
        RangeAndMinDistance{ 5, -2 },
        RangeAndMinDistance{ 5, -2 },
        RangeAndMinDistance{ 5, -2 },
        RangeAndMinDistance{ 5, -2 },
    };

    // 0x0045436B
    static currency32_t createIndustry(const IndustryPlacementArgs& args, const uint8_t flags)
    {
        getLegacyReturnState().lastPlacedIndustryId = IndustryId::null;
        GameCommands::setExpenditureType(ExpenditureType::Miscellaneous);
        {
            const auto centrePos = World::Pos2(args.pos.x + 16, args.pos.y + 16);
            const auto centreHeight = World::TileManager::getHeight(centrePos);
            GameCommands::setPosition({ args.pos.x, args.pos.y, centreHeight.landHeight });
        }

        // TODO: Vanilla bug args.srand0 not set. Change this when we can diverge
        Core::Prng prng{ enumValue(GameCommand::createIndustry), args.srand1 };
        const auto newIndustryId = sub_454C91(args.type, args.pos, prng);
        if (newIndustryId == IndustryId::null)
        {
            return FAILURE;
        }
        getLegacyReturnState().lastPlacedIndustryId = newIndustryId;
        auto* newIndustry = IndustryManager::get(newIndustryId);
        auto* indObj = newIndustry->getObject();
        if (args.buildImmediately)
        {
            newIndustry->under_construction = 0xFF;
        }
        if (flags & Flags::ghost)
        {
            newIndustry->flags |= IndustryFlags::isGhost;
        }
        if (!SceneManager::isEditorMode() && getUpdatingCompanyId() != CompanyId::neutral)
        {
            newIndustry->flags |= IndustryFlags::flag_04;
            newIndustry->owner = getUpdatingCompanyId();
        }

        // 0x00E0C3D3
        const auto randColour = getRandomAvailableColour(newIndustry->prng, *indObj);

        // 0x00E0C3BE - C0
        auto lastPlacedBuildingPos = World::Pos2{ newIndustry->x, newIndustry->y };

        // TODO: used also for 0x00454552 break up into two when function allowed to diverge
        const auto randVal = newIndustry->prng.randNext() & 0xFF;

        auto prodRateRand = randVal;
        for (auto i = 0; i < 2; ++i)
        {
            newIndustry->dailyProduction[i] = 0;

            const auto& initalRate = indObj->initialProductionRate[i];
            newIndustry->dailyProductionTarget[i] = (((initalRate.max - initalRate.min) * prodRateRand) / 256) + initalRate.min;

            if (SceneManager::isEditorMode())
            {
                newIndustry->dailyProduction[i] = newIndustry->dailyProductionTarget[i];
                newIndustry->producedCargoQuantityPreviousMonth[i] = newIndustry->dailyProduction[i] * 30;
            }
            // This is odd but follows vanilla
            prodRateRand = newIndustry->dailyProductionTarget[i] & 0xFF;
        }

        currency32_t totalCost = 0;

        // 0x00454552
        const auto numBuildings = (((indObj->maxNumBuildings - indObj->minNumBuildings + 1) * randVal) / 256) + indObj->minNumBuildings;
        const auto buildings = indObj->getBuildings();
        for (auto i = 0U; i < numBuildings; ++i)
        {
            const auto building = buildings[i];
            // 0x00E0C3D2 (bit 0)
            const bool isMultiTile = indObj->buildingSizeFlags & (1ULL << building);

            bool hasBuildingPlaced = false;
            // Attempt to place
            for (auto j = 0U; j < 25; ++j)
            {
                const auto randVal2 = newIndustry->prng.randNext();
                // bh
                const uint8_t direction = indObj->hasFlags(IndustryObjectFlags::notRotatable) ? 0 : randVal2 & 0x3;

                const auto [distanceRange, minDistance] = kPlacementAttemptDistanceRangeAndMin[j];

                const auto randTileX = static_cast<tile_coord_t>((distanceRange * ((randVal2 >> 18) & 0xFF)) / 256) + minDistance;
                const auto randTileY = static_cast<tile_coord_t>((distanceRange * ((randVal2 >> 2) & 0xFF)) / 256) + minDistance;

                const auto randPos = lastPlacedBuildingPos + World::toWorldSpace(World::TilePos2(randTileX, randTileY));
                const auto minRandPos = randPos;
                const auto maxRandPos = isMultiTile ? randPos + World::toWorldSpace(World::TilePos2{ 1, 1 }) : randPos;

                // TODO: Yank this out into a function
                bool placementFailure = false;
                for (auto k = 0U; k < newIndustry->numTiles; ++k)
                {
                    const auto minTileLoc = World::Pos2{ newIndustry->tiles[k].x, newIndustry->tiles[k].y };
                    const auto maxTileLoc = (newIndustry->tiles[k].z & (1U << 15)) ? minTileLoc + World::toWorldSpace(World::TilePos2{ 1, 1 }) : minTileLoc;

                    if (minRandPos.x > maxTileLoc.x)
                    {
                        continue;
                    }
                    if (minRandPos.y > maxTileLoc.y)
                    {
                        continue;
                    }

                    if (maxRandPos.x < minTileLoc.x)
                    {
                        continue;
                    }
                    if (maxRandPos.y < minTileLoc.y)
                    {
                        continue;
                    }
                    placementFailure = true;
                    break;
                }
                if (placementFailure)
                {
                    continue;
                }

                // bh bit 7 is buildImmediately
                // edi = randColour << 16
                // dl = building
                // dh = industryId
                if (flags & Flags::apply)
                {
                    // do test placement
                    const uint32_t cost = placeIndustryBuilding(newIndustryId, randPos, direction, building, randColour, args.buildImmediately, flags & ~(Flags::apply));
                    if (cost == FAILURE)
                    {
                        continue;
                    }
                    Scenario::getOptions().madeAnyChanges = 1;
                    // Why are we incrementing this even on test?
                    newIndustry->numTiles--;
                }

                const uint32_t cost = placeIndustryBuilding(newIndustryId, randPos, direction, building, randColour, args.buildImmediately, flags);
                if (cost == FAILURE)
                {
                    continue;
                }
                totalCost += cost;
                lastPlacedBuildingPos = randPos;
                hasBuildingPlaced = true;
                break;
            }
            if (!hasBuildingPlaced)
            {
                StringManager::emptyUserString(newIndustry->name);
                // Free the industry slot
                newIndustry->name = StringIds::null;
                return FAILURE;
            }
        }

        // Find centre of tiles placed
        if (newIndustry->numTiles != 0)
        {
            // Use int32_t as this is a total and will be larger than 16bit
            int32_t totalX = 0;
            int32_t totalY = 0;
            for (auto i = 0; i < newIndustry->numTiles; ++i)
            {
                auto& tile = newIndustry->tiles[i];
                bool isMultiTile = tile.z & 0x8000;
                totalX += tile.x;
                totalY += tile.y;
                if (isMultiTile)
                {
                    totalX += 16;
                    totalY += 16;
                }
            }
            newIndustry->x = 0xFFFFFFE0 & ((totalX / newIndustry->numTiles) + 16);
            newIndustry->y = 0xFFFFFFE0 & ((totalY / newIndustry->numTiles) + 16);
        }

        // 0x00454745
        if ((flags & Flags::apply) && !(flags & Flags::ghost) && newIndustry->numTiles != 0)
        {
            if (indObj->farmTileGrowthStageNoProduction != 0xFF)
            {
                uint32_t buildingWallEntranceMask = 0;
                if (indObj->buildingWallEntrance != 0xFF)
                {
                    const auto randWallVal = newIndustry->prng.srand_0();
                    buildingWallEntranceMask |= 1ULL << (randWallVal & 0xF);
                    buildingWallEntranceMask |= 1ULL << ((randWallVal >> 4) & 0xF);
                    // buildingWallEntranceMask |= 1ULL << ((randWallVal >> 8) & 0xF); CS meant to do this but made a mistake
                }

                // Claim surrounding surfaces and place perimeter fences
                for (auto i = 0; i < newIndustry->numTiles; ++i)
                {
                    const auto& tile = newIndustry->tiles[i];
                    const bool isMultiTile = tile.z & 0x8000;
                    const auto bottomLeft = World::toTileSpace(tile) - World::TilePos2{ 1, 1 };
                    const auto topRight = bottomLeft + (isMultiTile ? World::TilePos2{ 3, 3 } : World::TilePos2{ 2, 2 });
                    for (const auto& tilePos : World::TilePosRangeView(bottomLeft, topRight))
                    {
                        claimSurfaceForIndustry(tilePos, newIndustry->id(), indObj->farmTileGrowthStageNoProduction, 0);
                        // TODO: This is very similar to expand grounds code
                        if (indObj->buildingWall != 0xFF)
                        {
                            GameCommands::WallPlacementArgs wallArgs;
                            wallArgs.pos = World::Pos3(World::toWorldSpace(tilePos), 0);
                            wallArgs.primaryColour = Colour::black;
                            wallArgs.secondaryColour = Colour::black;
                            wallArgs.tertiaryColour = Colour::black;
                            // Place fences
                            // Note: Order of this 0, 2, 3, 1 is to match vanilla
                            if (tilePos.x == bottomLeft.x)
                            {
                                wallArgs.rotation = 0;
                                bool placeEntrance = buildingWallEntranceMask & (1ULL << 0);
                                buildingWallEntranceMask = std::rotr(buildingWallEntranceMask, 1);
                                wallArgs.type = placeEntrance ? indObj->buildingWallEntrance : indObj->buildingWall;
                                doCommand(wallArgs, Flags::apply);
                            }
                            if (tilePos.x == topRight.x)
                            {
                                wallArgs.rotation = 2;
                                bool placeEntrance = buildingWallEntranceMask & (1ULL << 0);
                                buildingWallEntranceMask = std::rotr(buildingWallEntranceMask, 1);
                                wallArgs.type = placeEntrance ? indObj->buildingWallEntrance : indObj->buildingWall;
                                doCommand(wallArgs, Flags::apply);
                            }
                            if (tilePos.y == bottomLeft.y)
                            {
                                wallArgs.rotation = 3;
                                bool placeEntrance = buildingWallEntranceMask & (1ULL << 0);
                                buildingWallEntranceMask = std::rotr(buildingWallEntranceMask, 1);
                                wallArgs.type = placeEntrance ? indObj->buildingWallEntrance : indObj->buildingWall;
                                doCommand(wallArgs, Flags::apply);
                            }
                            if (tilePos.y == topRight.y)
                            {
                                wallArgs.rotation = 1;
                                bool placeEntrance = buildingWallEntranceMask & (1ULL << 0);
                                buildingWallEntranceMask = std::rotr(buildingWallEntranceMask, 1);
                                wallArgs.type = placeEntrance ? indObj->buildingWallEntrance : indObj->buildingWall;
                                doCommand(wallArgs, Flags::apply);
                            }
                        }
                    }
                }
            }

            // Expand grounds
            if (indObj->farmTileNumGrowthStages != 0)
            {
                const auto numExpands = (((indObj->farmNumFields * newIndustry->prng.randNext(0xFF)) / 256) + 1) * 4;
                for (auto i = 0; i < numExpands; ++i)
                {
                    const auto randExpandVal = newIndustry->prng.randNext();
                    // dl
                    const auto growthStage = ((randExpandVal & 0xFF) * indObj->farmTileNumGrowthStages) / 256;
                    const auto updateTimerVal = (randExpandVal >> 8) & 0x7;

                    const World::TilePos2 randOffset(
                        ((randExpandVal >> 11) & 0x1F) - 15,
                        ((randExpandVal >> 16) & 0x1F) - 15);

                    const World::Pos2 randPos = World::Pos2{ newIndustry->x, newIndustry->y } + World::toWorldSpace(randOffset);

                    bool useSecondWallType = ((randExpandVal >> 21) & 1) && indObj->wallTypes[2] != 0xFF;

                    const auto wallType = useSecondWallType ? indObj->wallTypes[2] : indObj->wallTypes[0];
                    const auto wallEntranceType = useSecondWallType ? indObj->wallTypes[3] : indObj->wallTypes[1];
                    newIndustry->expandGrounds(randPos, wallType, wallEntranceType, growthStage, updateTimerVal);
                }
            }
        }

        // Cleanup
        if (!(flags & Flags::apply))
        {
            StringManager::emptyUserString(newIndustry->name);
            // Free the industry slot
            newIndustry->name = StringIds::null;
        }
        totalCost += Economy::getInflationAdjustedCost(indObj->costFactor, indObj->costIndex, 3);

        // Send message post
        if ((flags & Flags::apply) && !(flags & Flags::ghost) && !args.buildImmediately)
        {
            MessageManager::post(MessageType::newIndustry, CompanyId::null, enumValue(newIndustry->id()), 0xFFFFU);
        }
        setExpenditureType(ExpenditureType::Miscellaneous);
        return totalCost;
    }

    void createIndustry(registers& regs)
    {
        IndustryPlacementArgs args(regs);
        regs.ebx = createIndustry(args, regs.bl);
    }
}
