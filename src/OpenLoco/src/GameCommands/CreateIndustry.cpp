#include "Date.h"
#include "Economy/Economy.h"
#include "GameCommands.h"
#include "Graphics/Colour.h"
#include "Localisation/FormatArguments.hpp"
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
#include "SceneManager.h"
#include "ViewportManager.h"
#include "World/IndustryManager.h"
#include "World/TownManager.h"
#include <OpenLoco/Core/Numerics.hpp>

namespace OpenLoco::GameCommands
{
    static loco_global<IndustryId, 0x00E0C3C9> _industryLastPlacedId;

    // 0x00454C91
    static IndustryId sub_454C91(uint8_t type, const World::Pos2& pos, const Core::Prng& prng)
    {
        const auto* indObj = ObjectManager::get<IndustryObject>(type);

        // TODO: MAKE THIS A FUNCTION
        // 0x00E0C38C
        uint32_t producedCargoTypes = 0;
        for (auto& cargoType : indObj->producedCargoType)
        {
            if (cargoType != 0xFFU)
            {
                producedCargoTypes |= (1U << cargoType);
            }
        }

        // 0x00E0C390
        uint32_t requiredCargoTypes = 0;
        for (auto& cargoType : indObj->requiredCargoType)
        {
            if (cargoType != 0xFFU)
            {
                requiredCargoTypes |= (1U << cargoType);
            }
        }

        for (const auto& ind : IndustryManager::industries())
        {
            const auto distance = Math::Vector::manhattanDistance(World::Pos2{ ind.x, ind.y }, pos);

            const auto* indObj2 = ind.getObject();
            uint32_t producedCargoTypes2 = 0;
            for (auto& cargoType : indObj2->producedCargoType)
            {
                if (cargoType != 0xFFU)
                {
                    producedCargoTypes2 |= (1U << cargoType);
                }
            }

            uint32_t requiredCargoTypes2 = 0;
            for (auto& cargoType : indObj2->requiredCargoType)
            {
                if (cargoType != 0xFFU)
                {
                    requiredCargoTypes2 |= (1U << cargoType);
                }
            }

            const auto hasAtLeast1CargoRequirement = ((requiredCargoTypes & producedCargoTypes2) != 0)
                || ((producedCargoTypes & requiredCargoTypes2) != 0);

            const auto tooClose = 32 * (hasAtLeast1CargoRequirement ? 24 : 9);

            if (distance < tooClose)
            {
                GameCommands::setErrorText(StringIds::too_close_to_another_industry);
                return IndustryId::null;
            }
        }

        // Find free industry slot (MOVE TO INDUSTRY MANAGER)
        for (IndustryId id = static_cast<IndustryId>(0); enumValue(id) < Limits::kMaxIndustries; id = static_cast<IndustryId>(enumValue(id) + 1))
        {
            auto* industry = IndustryManager::get(id);
            if (!industry->empty())
            {
                continue;
            }

            industry->prng = prng;
            industry->flags = IndustryFlags::none;
            industry->objectId = type;
            industry->x = pos.x;
            industry->y = pos.y;
            industry->numTiles = 0;
            industry->under_construction = 0;
            industry->tileLoop = World::TileLoop{};
            industry->var_DB = 0;
            industry->var_DD = 0;
            industry->var_DF = 25;
            industry->foundingYear = getCurrentYear();
            industry->var_E1 = {};
            for (auto& stats : industry->producedCargoStatsStation)
            {
                std::fill(std::begin(stats), std::end(stats), StationId::null);
            }
            std::fill(std::begin(industry->var_17D), std::end(industry->var_17D), 0);
            std::fill(std::begin(industry->var_181), std::end(industry->var_181), 0);
            std::fill(std::begin(industry->producedCargoQuantityMonthlyTotal), std::end(industry->producedCargoQuantityMonthlyTotal), 0);
            std::fill(std::begin(industry->producedCargoQuantityPreviousMonth), std::end(industry->producedCargoQuantityPreviousMonth), 0);
            std::fill(std::begin(industry->receivedCargoQuantityMonthlyTotal), std::end(industry->receivedCargoQuantityMonthlyTotal), 0);
            std::fill(std::begin(industry->receivedCargoQuantityPreviousMonth), std::end(industry->receivedCargoQuantityPreviousMonth), 0);
            std::fill(std::begin(industry->receivedCargoQuantityDailyTotal), std::end(industry->receivedCargoQuantityDailyTotal), 0);
            std::fill(std::begin(industry->producedCargoQuantityDeliveredMonthlyTotal), std::end(industry->producedCargoQuantityDeliveredMonthlyTotal), 0);
            std::fill(std::begin(industry->producedCargoQuantityDeliveredPreviousMonth), std::end(industry->producedCargoQuantityDeliveredPreviousMonth), 0);
            std::fill(std::begin(industry->producedCargoPercentTransportedPreviousMonth), std::end(industry->producedCargoPercentTransportedPreviousMonth), 0);
            std::fill(std::begin(industry->producedCargoMonthlyHistorySize), std::end(industry->producedCargoMonthlyHistorySize), 1);
            // Note: vanilla just set to 0 first entry
            std::fill(std::begin(industry->producedCargoMonthlyHistory1), std::end(industry->producedCargoMonthlyHistory1), 0);
            std::fill(std::begin(industry->producedCargoMonthlyHistory2), std::end(industry->producedCargoMonthlyHistory2), 0);
            std::fill(std::begin(industry->history_min_production), std::end(industry->history_min_production), 0);

            const auto res = TownManager::getClosestTownAndDensity(pos);
            if (!res.has_value())
            {
                GameCommands::setErrorText(StringIds::town_must_be_built_nearby_first);
                return IndustryId::null;
            }
            industry->town = res->first;
            industry->name = indObj->var_02;

            for (auto& innerInd : IndustryManager::industries())
            {
                if (innerInd.name != industry->name)
                {
                    continue;
                }
                if (&innerInd == industry)
                {
                    continue;
                }
                if (innerInd.town != industry->town)
                {
                    continue;
                }

                for (auto unique = 1; unique < 0xFFF; ++unique)
                {
                    FormatArguments args{};
                    args.push<uint16_t>(unique);
                    char buffer[512]{};
                    StringManager::formatString(buffer, indObj->var_02 + 1, &args);
                    const auto newName = StringManager::userStringAllocate(buffer, 0);
                    if (newName == StringIds::empty)
                    {
                        continue;
                    }
                    industry->name = newName;
                    break;
                }
            }
            return id;
        }

        GameCommands::setErrorText(StringIds::too_many_industries);
        return IndustryId::null;
    }

    // 0x0045572D
    static World::TileClearance::ClearFuncResult tileClearFunction(World::TileElement& el, const World::Pos2 pos, const uint8_t flags, currency32_t& cost)
    {
        // TODO: This is a copy of parts of TileClearance.cpp
        auto* elTree = el.as<World::TreeElement>();
        if (elTree == nullptr)
        {
            return World::TileClearance::ClearFuncResult::collision;
        }
        auto* treeObj = ObjectManager::get<TreeObject>(elTree->treeObjectId());
        cost += Economy::getInflationAdjustedCost(treeObj->clearCostFactor, treeObj->costIndex, 12);

        if ((flags & GameCommands::Flags::flag_6) || !(flags & GameCommands::Flags::apply))
        {
            return World::TileClearance::ClearFuncResult::noCollision;
        }

        World::TileManager::setRemoveElementPointerChecker(el);
        World::TileManager::removeTree(*elTree, GameCommands::Flags::apply, pos);
        // S5::getOptions().madeAnyChanges = 1;
        if (World::TileManager::wasRemoveOnLastElement())
        {
            return World::TileClearance::ClearFuncResult::allCollisionsRemoved;
        }
        return World::TileClearance::ClearFuncResult::collisionRemoved;
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
            auto* surface = tile.surface();

            auto baseZ = World::TileManager::getSurfaceCornerHeight(surface);
            highestBaseZ = std::max<World::SmallZ>(highestBaseZ, baseZ);
            if (surface->water())
            {
                highestBaseZ = std::max<World::SmallZ>(highestBaseZ, surface->water() * World::kMicroToSmallZStep);
            }
        }

        // Workout clearance height of building (including scaffolding if required)
        const auto buildingParts = indObj->getBuildingParts(buildingType);
        // 0x00E0C3BC (note this is bigZ and does not include the base height)
        auto clearHeight = 0;
        for (auto part : buildingParts)
        {
            clearHeight += indObj->buildingPartHeight[part];
        }
        if (!buildImmediate && indObj->scaffoldingSegmentType != 0xFF)
        {
            auto* scaffObj = ObjectManager::get<ScaffoldingObject>();
            bool requiresMore = clearHeight % scaffObj->segmentHeights[indObj->scaffoldingSegmentType];
            const auto numSegments = (clearHeight / scaffObj->segmentHeights[indObj->scaffoldingSegmentType]) + (requiresMore ? 1 : 0);
            clearHeight = (numSegments * scaffObj->segmentHeights[indObj->scaffoldingSegmentType]) + scaffObj->roofHeights[indObj->scaffoldingSegmentType];
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

            if ((flags & Flags::apply) && !(flags & Flags::flag_6))
            {
                World::TileManager::removeAllWallsOnTileBelow(tilePos, highestBaseZ + clearHeight / World::kSmallZStep);
            }

            {
                auto tile = World::TileManager::get(tilePos);
                auto* surface = tile.surface();

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

            // Perform additional validation ?for ghosts? and flatten surfaces
            if (!(flags & Flags::flag_6) && !indObj->hasFlags(IndustryObjectFlags::builtOnWater))
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
                                if (elTrack != nullptr && !elTrack->isGhost() && elTrack->hasBridge())
                                {
                                    setErrorText(StringIds::empty);
                                    return FAILURE;
                                }
                                else if (elRoad != nullptr && !elRoad->isGhost() && elRoad->hasBridge())
                                {
                                    setErrorText(StringIds::empty);
                                    return FAILURE;
                                }
                                else if (elStation != nullptr && elStation->unk5SLR5() == 2)
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
                        surface->setVar4SLR5(0);
                        surface->setVar6SLR5(0);
                    }
                }
            }

            // Create new tile
            if (flags & Flags::apply)
            {
                if (!(flags & Flags::flag_6))
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
                elIndustry->setGhost(flags & Flags::flag_6);
                Ui::ViewportManager::invalidate(World::toWorldSpace(tilePos), elIndustry->baseHeight(), elIndustry->clearHeight());
            }
        }

        auto& tileRef = industry->tiles[industry->numTiles];
        tileRef = World::Pos3(pos, (highestBaseZ * World::kSmallZStep | (isMultiTile ? 0x8000 : 0)));
        industry->numTiles++;
        return totalCost;
    }

    // 0x0045436B
    static currency32_t createIndustry(const IndustryPlacementArgs& args, const uint8_t flags)
    {
        GameCommands::setExpenditureType(ExpenditureType::Miscellaneous);
        {
            const auto centrePos = World::Pos2(args.pos.x + 16, args.pos.y + 16);
            const auto centreHeight = World::TileManager::getHeight(centrePos);
            GameCommands::setPosition({ args.pos.x, args.pos.y, centreHeight.landHeight });
        }

        // Vanilla bug args.srand0 not set. Change this when we can diverge
        Core::Prng prng{ enumValue(GameCommand::createIndustry), args.srand1 };
        const auto newIndustryId = sub_454C91(args.type, args.pos, prng);
        if (newIndustryId == IndustryId::null)
        {
            return FAILURE;
        }
        _industryLastPlacedId = newIndustryId;
        auto* newIndustry = IndustryManager::get(newIndustryId);
        auto* indObj = newIndustry->getObject();
        if (args.buildImmediately)
        {
            newIndustry->under_construction = 0xFF;
        }
        if (flags & Flags::flag_6)
        {
            newIndustry->flags |= IndustryFlags::isGhost;
        }
        if (!isEditorMode() && getUpdatingCompanyId() != CompanyId::neutral)
        {
            newIndustry->flags |= IndustryFlags::flag_04;
            newIndustry->owner = getUpdatingCompanyId();
        }

        // Note: Could use a fixed size vector as max size is Colour::max
        std::vector<Colour> availableColours;
        auto colourBitSet = indObj->availableColours;
        for (auto colourI32 = Numerics::bitScanForward(colourBitSet); colourI32 != -1; colourI32 = Numerics::bitScanForward(colourBitSet))
        {
            colourBitSet &= ~(1ULL << colourI32);
            availableColours.push_back(static_cast<Colour>(colourI32));
        }

        // 0x00E0C3D3
        const auto randColour = [&availableColours, &newIndustry]() {
            if (availableColours.empty())
            {
                return Colour::black;
            }
            return availableColours[newIndustry->prng.randNext(availableColours.size() - 1)];
        }();

        // 0x00E0C3BE - C0
        auto lastPlacedBuildingPos = World::Pos2{ newIndustry->x, newIndustry->y };

        // used also for 0x00454552 break up into two when function allowed to diverge
        const auto randVal = newIndustry->prng.randNext() & 0xFF;

        auto prodRateRand = randVal;
        for (auto i = 0; i < 2; ++i)
        {
            newIndustry->var_17D[i] = 0;
            newIndustry->productionRate[i] = (((indObj->initialProductionRate[i].max - indObj->initialProductionRate[i].min) * prodRateRand) / 256) + indObj->initialProductionRate[i].min;
            if (isEditorMode())
            {
                newIndustry->var_17D[i] = newIndustry->productionRate[i];
                newIndustry->producedCargoQuantityPreviousMonth[i] = newIndustry->var_17D[i] * 30;
            }
            // This is odd but follows vanilla
            prodRateRand = newIndustry->productionRate[i] & 0xFF;
        }

        currency32_t totalCost = 0;

        // 0x00454552
        const auto numBuildings = (((indObj->maxNumBuildings - indObj->minNumBuildings + 1) * randVal) / 256) + indObj->minNumBuildings;
        for (auto i = 0U; i < numBuildings; ++i)
        {
            const auto building = indObj->buildings[i];
            // 0x00E0C3D2 (bit 0)
            const bool isMultiTile = indObj->buildingSizeFlags & (1ULL << building);

            bool hasBuildingPlaced = false;
            // Attempt to place
            for (auto j = 0U; j < 25; ++j)
            {
                const auto randVal2 = newIndustry->prng.randNext();
                // bh
                const uint8_t direction = indObj->hasFlags(IndustryObjectFlags::notRotatable) ? 0 : randVal2 & 0x3;

                const auto [distanceRange, minDistance] = [j]() -> std::pair<tile_coord_t, tile_coord_t> {
                    if (j == 0)
                    {
                        return { 1, 0 };
                    }
                    else if (j < 8)
                    {
                        return { 3, -1 };
                    }
                    else
                    {
                        return { 5, -2 };
                    }
                }();

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
        if ((flags & Flags::apply) && !(flags & Flags::flag_6) && newIndustry->numTiles != 0)
        {
            if (indObj->var_EA != 0xFF)
            {
                uint32_t buildingWallEntranceMask = 0;
                if (indObj->buildingWallEntrance != 0xFF)
                {
                    const auto randWallVal = newIndustry->prng.srand_0();
                    buildingWallEntranceMask |= 1ULL << (randWallVal & 0xF);
                    buildingWallEntranceMask |= 1ULL << ((randWallVal >> 4) & 0xF);
                    // buildingWallEntranceMask |= 1ULL << ((randWallVal >> 8) & 0xF); CS meant to do this but made a mistake
                }

                // Claim surrounding surfaces and place perimiter fences
                for (auto i = 0; i < newIndustry->numTiles; ++i)
                {
                    const auto& tile = newIndustry->tiles[i];
                    const bool isMultiTile = tile.z & 0x8000;
                    const auto bottomLeft = World::toTileSpace(tile) - World::TilePos2{ 1, 1 };
                    const auto topRight = bottomLeft + (isMultiTile ? World::TilePos2{ 3, 3 } : World::TilePos2{ 2, 2 });
                    for (const auto& tilePos : World::TilePosRangeView(bottomLeft, topRight))
                    {
                        claimSurfaceForIndustry(tilePos, newIndustry->id(), indObj->var_EA);
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
                                buildingWallEntranceMask = Numerics::ror(buildingWallEntranceMask, 1);
                                wallArgs.type = placeEntrance ? indObj->buildingWallEntrance : indObj->buildingWall;
                                doCommand(wallArgs, Flags::apply);
                            }
                            if (tilePos.x == topRight.x)
                            {
                                wallArgs.rotation = 2;
                                bool placeEntrance = buildingWallEntranceMask & (1ULL << 0);
                                buildingWallEntranceMask = Numerics::ror(buildingWallEntranceMask, 1);
                                wallArgs.type = placeEntrance ? indObj->buildingWallEntrance : indObj->buildingWall;
                                doCommand(wallArgs, Flags::apply);
                            }
                            if (tilePos.y == bottomLeft.y)
                            {
                                wallArgs.rotation = 3;
                                bool placeEntrance = buildingWallEntranceMask & (1ULL << 0);
                                buildingWallEntranceMask = Numerics::ror(buildingWallEntranceMask, 1);
                                wallArgs.type = placeEntrance ? indObj->buildingWallEntrance : indObj->buildingWall;
                                doCommand(wallArgs, Flags::apply);
                            }
                            if (tilePos.y == topRight.y)
                            {
                                wallArgs.rotation = 1;
                                bool placeEntrance = buildingWallEntranceMask & (1ULL << 0);
                                buildingWallEntranceMask = Numerics::ror(buildingWallEntranceMask, 1);
                                wallArgs.type = placeEntrance ? indObj->buildingWallEntrance : indObj->buildingWall;
                                doCommand(wallArgs, Flags::apply);
                            }
                        }
                    }
                }
            }

            // Expand grounds
            if (indObj->var_EC != 0)
            {
                const auto numExpands = (((indObj->var_EB * newIndustry->prng.randNext(0xFF)) / 256) + 1) * 4;
                for (auto i = 0; i < numExpands; ++i)
                {
                    const auto randExpandVal = newIndustry->prng.randNext();
                    // dl
                    const auto surfaceUnk = (((randExpandVal & 0xFF) * indObj->var_EC) / 256) | (((randExpandVal >> 8) & 0x7) << 5);

                    const World::TilePos2 randOffset(
                        ((randExpandVal >> 11) & 0x1F) - 15,
                        ((randExpandVal >> 16) & 0x1F) - 15);

                    const World::Pos2 randPos = World::Pos2{ newIndustry->x, newIndustry->y } + World::toWorldSpace(randOffset);

                    bool useSecondWallType = ((randExpandVal >> 21) & 1) && indObj->wallTypes[2] != 0xFF;

                    const auto wallType = useSecondWallType ? indObj->wallTypes[2] : indObj->wallTypes[0];
                    const auto wallEntranceType = useSecondWallType ? indObj->wallTypes[3] : indObj->wallTypes[1];
                    newIndustry->expandGrounds(randPos, wallType, wallEntranceType, surfaceUnk);
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
        if ((flags & Flags::apply) && !(flags & Flags::flag_6) && !args.buildImmediately)
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
