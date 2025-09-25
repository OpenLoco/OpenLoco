#include "CreateBuilding.h"
#include "Economy/Economy.h"
#include "Localisation/StringIds.h"
#include "Map/AnimationManager.h"
#include "Map/BuildingElement.h"
#include "Map/IndustryElement.h"
#include "Map/QuarterTile.h"
#include "Map/RoadElement.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileClearance.h"
#include "Map/TileManager.h"
#include "Map/TrackElement.h"
#include "Map/TreeElement.h"
#include "Objects/BuildingObject.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/ScaffoldingObject.h"
#include "ScenarioOptions.h"
#include "ViewportManager.h"
#include "World/Industry.h"
#include "World/Station.h"
#include "World/TownManager.h"

namespace OpenLoco::GameCommands
{
    // 0x0042D133
    static uint32_t createBuilding(const BuildingPlacementArgs& args, uint8_t flags)
    {
        GameCommands::setExpenditureType(ExpenditureType::Construction);
        GameCommands::setPosition(args.pos + World::Pos3{ 16, 16, 0 });

        // 0x00525D2C
        currency32_t totalCost = 0;

        auto* buildingObj = ObjectManager::get<BuildingObject>(args.type);

        if (!World::TileManager::checkFreeElementsAndReorganise())
        {
            return FAILURE;
        }

        if (!buildingObj->hasFlags(BuildingObjectFlags::miscBuilding))
        {
            auto nearest = TownManager::getClosestTownAndDensity(args.pos);
            if (!nearest.has_value())
            {
                setErrorText(StringIds::town_must_be_built_first);
                return FAILURE;
            }
        }

        // This is identical to createIndustry but with a BuildingObject
        // TODO: look into making some sort of common version
        auto clearHeight = 0;
        const auto partHeights = buildingObj->getBuildingPartHeights();
        for (auto part : buildingObj->getBuildingParts(args.variation))
        {
            clearHeight += partHeights[part];
        }
        if (!args.buildImmediately && buildingObj->scaffoldingSegmentType != 0xFF)
        {
            auto* scaffObj = ObjectManager::get<ScaffoldingObject>();
            const auto segmentHeight = scaffObj->segmentHeights[buildingObj->scaffoldingSegmentType];
            const bool requiresMore = clearHeight % segmentHeight;
            const auto numSegments = (clearHeight / segmentHeight) + (requiresMore ? 1 : 0);
            clearHeight = (numSegments * segmentHeight) + scaffObj->roofHeights[buildingObj->scaffoldingSegmentType];
        }
        // ceil to 4
        clearHeight += 3;
        clearHeight &= ~3;

        World::TileClearance::RemovedBuildings removedBuildings;

        const auto buildingFootprint = getBuildingTileOffsets(buildingObj->hasFlags(BuildingObjectFlags::largeTile));

        for (auto& offset : buildingFootprint)
        {
            const auto tilePos = World::toTileSpace(World::Pos2(args.pos) + offset.pos);
            if (!World::TileManager::validCoords(tilePos))
            {
                return FAILURE;
            }

            if ((flags & Flags::apply) && !(flags & Flags::ghost))
            {
                World::TileManager::removeAllWallsOnTileBelow(tilePos, (args.pos.z + clearHeight) / World::kSmallZStep);
            }

            {
                auto tile = World::TileManager::get(tilePos);
                const auto* surface = tile.surface();

                // Perform clearance checks
                if (surface->water())
                {
                    setErrorText(StringIds::cant_build_this_underwater);
                    return FAILURE;
                }

                const auto baseZ = std::min<World::SmallZ>(surface->baseZ(), (args.pos.z / World::kSmallZStep));
                const auto clearZ = (args.pos.z + clearHeight) / World::kSmallZStep;

                World::QuarterTile qt(0xF, 0xF);
                auto clearFunc = [tilePos, &removedBuildings, flags, &totalCost](World::TileElement& el) {
                    switch (el.type())
                    {
                        case World::ElementType::tree:
                        {
                            auto* elTree = el.as<World::TreeElement>();
                            if (elTree == nullptr)
                            {
                                return World::TileClearance::ClearFuncResult::noCollision;
                            }
                            return World::TileClearance::clearTreeCollision(*elTree, World::toWorldSpace(tilePos), flags, totalCost);
                        }
                        case World::ElementType::building:
                        {
                            auto* elBuilding = el.as<World::BuildingElement>();
                            if (elBuilding == nullptr)
                            {
                                return World::TileClearance::ClearFuncResult::noCollision;
                            }
                            // This is used to stop creating a building ghost
                            // even if a clear could succeed here. This is
                            // because if it did place a ghost the ghost cleanup
                            // function might remove the wrong building!
                            if (flags & Flags::preventBuildingClearing)
                            {
                                return World::TileClearance::ClearFuncResult::collision;
                            }
                            return World::TileClearance::clearBuildingCollision(*elBuilding, World::toWorldSpace(tilePos), removedBuildings, flags, totalCost);
                        }
                        default:
                            return World::TileClearance::ClearFuncResult::collision;
                    }
                };
                if (!World::TileClearance::applyClearAtStandardHeight(World::toWorldSpace(tilePos), baseZ, clearZ, qt, clearFunc))
                {
                    return FAILURE;
                }
                // TODO: This is dangerous pointer might be invalid?
                if (surface->slope() || surface->baseHeight() != args.pos.z)
                {
                    const auto* landObj = ObjectManager::get<LandObject>(surface->terrain());
                    totalCost += Economy::getInflationAdjustedCost(landObj->costFactor, landObj->costIndex, 10);
                }
            }

            // Flatten surfaces (also checks if other elements will cause issues due to the flattening of the surface)
            if (!(flags & Flags::ghost))
            {
                auto tile = World::TileManager::get(tilePos);
                auto* surface = tile.surface();

                // TODO: This is identical to CreateIndustry
                if (surface->slope() || args.pos.z != surface->baseHeight())
                {
                    if (args.pos.z < surface->baseHeight())
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
                                else if (elTree != nullptr && args.pos.z + clearHeight <= elTree->baseHeight())
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
                        surface->setBaseZ(args.pos.z / World::kSmallZStep);
                        surface->setClearZ(args.pos.z / World::kSmallZStep);
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
                auto* elBuilding = World::TileManager::insertElement<World::BuildingElement>(World::toWorldSpace(tilePos), args.pos.z / World::kSmallZStep, 0xF);
                if (elBuilding == nullptr)
                {
                    return FAILURE;
                }
                elBuilding->setClearZ((clearHeight / World::kSmallZStep) + elBuilding->baseZ());
                elBuilding->setRotation(args.rotation);
                elBuilding->setConstructed(args.buildImmediately);
                if (args.buildImmediately && offset.index == 0 && buildingObj->numElevatorSequences != 0)
                {
                    World::AnimationManager::createAnimation(5, World::toWorldSpace(tilePos), elBuilding->baseZ());
                }
                elBuilding->setObjectId(args.type);
                elBuilding->setSequenceIndex(offset.index);
                elBuilding->setUnk5u(0);
                elBuilding->setColour(args.colour);
                elBuilding->setVariation(args.variation);
                elBuilding->setAge(0);
                elBuilding->setIsMiscBuilding(buildingObj->hasFlags(BuildingObjectFlags::miscBuilding));

                bool hasFrames = false;
                const auto partAnimations = buildingObj->getBuildingPartAnimations();
                for (auto part : buildingObj->getBuildingParts(args.variation))
                {
                    if (partAnimations[part].numFrames > 1)
                    {
                        hasFrames = true;
                    }
                }
                if (hasFrames)
                {
                    World::AnimationManager::createAnimation(6, World::toWorldSpace(tilePos), elBuilding->baseZ());
                }

                elBuilding->setGhost(flags & Flags::ghost);
                Ui::ViewportManager::invalidate(World::toWorldSpace(tilePos), elBuilding->baseHeight(), elBuilding->clearHeight());
                Scenario::getOptions().madeAnyChanges = 1;
            }
        }

        if ((flags & Flags::apply) && !(flags & Flags::ghost) && !buildingObj->hasFlags(BuildingObjectFlags::miscBuilding))
        {
            const auto populationCapacity = buildingObj->producedQuantity[0];
            const auto population = args.buildImmediately ? populationCapacity : 0;

            auto* town = TownManager::updateTownInfo(args.pos, population, populationCapacity, 0, 1);
            if (town != nullptr)
            {
                if (buildingObj->var_AC != 0xFF)
                {
                    town->var_150[buildingObj->var_AC]++;
                }
            }
        }
        return totalCost;
    }

    void createBuilding(registers& regs)
    {
        BuildingPlacementArgs args(regs);
        regs.ebx = createBuilding(args, regs.bl);
    }
}
