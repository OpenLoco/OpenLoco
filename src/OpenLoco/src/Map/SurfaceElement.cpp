#include "SurfaceElement.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Terraform/CreateWall.h"
#include "GameCommands/Terraform/RemoveWall.h"
#include "IndustryElement.h"
#include "Objects/IndustryObject.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "Scenario.h"
#include "TileManager.h"
#include "ViewportManager.h"
#include "WallElement.h"
#include "WaveManager.h"
#include "World/IndustryManager.h"
#include "World/TownManager.h"

namespace OpenLoco::World
{
    void SurfaceElement::removeIndustry(const World::Pos2& pos)
    {
        if (isIndustrial())
        {
            setIsIndustrialFlag(false);
            setGrowthStage(0);
            setVariation(0);
            auto z = baseHeight();
            Ui::ViewportManager::invalidate(pos, z, z + 32, ZoomLevel::eighth);
            TownManager::updateTownInfo(pos, 0, 0, -30, 0);
        }
    }

    static void updateNonIndustrialSurface(SurfaceElement& elSurface, const World::Pos2 loc)
    {
        auto* landObj = ObjectManager::get<LandObject>(elSurface.terrain());
        if (!landObj->hasFlags(LandObjectFlags::unk0))
        {
            return;
        }
        elSurface.setUpdateTimer(elSurface.getUpdateTimer() + 1);
        if (elSurface.getUpdateTimer() != 0)
        {
            return;
        }
        if (elSurface.water())
        {
            SmallZ waterZ = elSurface.water() * kMicroToSmallZStep;
            if (elSurface.slope())
            {
                waterZ -= 4;
            }
            if (waterZ > elSurface.baseZ())
            {
                if (elSurface.getGrowthStage())
                {
                    elSurface.setGrowthStage(0);
                    Ui::ViewportManager::invalidate(loc, elSurface.baseHeight(), elSurface.baseHeight());
                }
                return;
            }
        }

        if (elSurface.getGrowthStage() + 1 < landObj->numGrowthStages)
        {
            elSurface.setGrowthStage(elSurface.getGrowthStage() + 1);
            Ui::ViewportManager::invalidate(loc, elSurface.baseHeight(), elSurface.baseHeight());
        }
    }

    // 0x004C49E9
    static WallElement* getWallElement(const World::Pos2 loc, SmallZ baseZ, SmallZ clearZ, uint8_t direction)
    {
        auto tile = TileManager::get(loc);
        for (auto& el : tile)
        {
            auto* elWall = el.as<WallElement>();
            if (elWall == nullptr)
            {
                continue;
            }
            if (elWall->isGhost())
            {
                continue;
            }
            if (elWall->clearZ() < baseZ)
            {
                continue;
            }
            if (elWall->baseZ() > clearZ)
            {
                continue;
            }
            if (elWall->rotation() != direction)
            {
                continue;
            }
            return elWall;
        }
        return nullptr;
    }

    static bool updateIndustrialSurface(SurfaceElement& elSurface, const World::Pos2 loc)
    {
        auto* industry = IndustryManager::get(elSurface.industryId());
        auto* industryObj = industry->getObject();
        if (industryObj->hasFlags(IndustryObjectFlags::builtOnSnow) && elSurface.snowCoverage() < 4)
        {
            TileManager::removeSurfaceIndustry(loc);
            return false;
        }

        if (elSurface.getGrowthStage() != industryObj->farmTileGrowthStageNoProduction || elSurface.getGrowthStage() == 0)
        {
            elSurface.setUpdateTimer(elSurface.getUpdateTimer() + 1);
            if (elSurface.getUpdateTimer() == 0)
            {
                uint8_t newGrowthStage = elSurface.getGrowthStage() + 1;
                if (newGrowthStage >= industryObj->farmTileNumGrowthStages)
                {
                    newGrowthStage = 0;
                }
                elSurface.setGrowthStage(newGrowthStage);
                Ui::ViewportManager::invalidate(loc, elSurface.baseHeight(), elSurface.baseHeight());
            }
        }

        SurfaceElement* surf = &elSurface;
        bool tileAddedRemoved = false;
        for (auto i = 0; i < 4; ++i)
        {
            // This loop looks for walls on either sides of the surface tile being updated
            //
            //     |   N   |
            //     |   2   |
            // --- | ----- | ---
            //     |   1   |
            // N 2 | 1 X 1 | 2 N
            //     |   1   |
            // --- | ----- | ---
            //     |   2   |
            //     |   N   |
            //
            // N == Neighbour tile (this loop)
            // X == Surface being updated
            // 1 == Potential wall on surface being updated
            // 2 == Potential wall on Neighbour tile (this loop)

            auto* elWall = getWallElement(loc, surf->baseZ(), surf->baseZ() + 16, i);
            if (elWall)
            {
                // Wall has been found on this surface tile.
                // Validate if it should be here and if not
                // remove it
                if (surf->getGrowthStage() != industryObj->farmTileGrowthStageNoProduction)
                {
                    continue;
                }
                const auto nextLoc = loc + kRotationOffset[i];
                if (!TileManager::validCoords(nextLoc))
                {
                    continue;
                }

                auto* nextSurface = TileManager::get(nextLoc).surface();
                if (!nextSurface->isIndustrial() || nextSurface->industryId() != surf->industryId())
                {
                    continue;
                }
                if (nextSurface->getGrowthStage() != industryObj->farmTileGrowthStageNoProduction)
                {
                    continue;
                }
                GameCommands::WallRemovalArgs args{};
                args.pos = Pos3(loc, elWall->baseHeight());
                args.rotation = i;
                GameCommands::doCommand(args, GameCommands::Flags::apply);
                // tileAddedRemoved = true;
                // To be safe we should really re-get the tile and stop processing the tile (tileAddedRemoved = true)
                // but that subtly diverges from vanilla.
                // TODO: Change this when we want to diverge!

                // surf might be invalid! Re-get it just to be safe.
                surf = TileManager::get(loc).surface();
                continue;
            }
            else
            {
                const auto nextLoc = loc + kRotationOffset[i];
                if (!TileManager::validCoords(nextLoc))
                {
                    continue;
                }

                // ^ (1U << 1) as we need to look at reverse direction for the neighbours wall
                auto* elWall2 = getWallElement(nextLoc, surf->baseZ(), surf->baseZ() + 16, i ^ (1U << 1));
                if (elWall2)
                {
                    continue;
                }

                // Wall has *NOT* been found on neighbour surface tile.
                // Validate if it should exist and if it should create
                // it.
                auto* nextSurface = TileManager::get(nextLoc).surface();
                if (nextSurface->isIndustrial())
                {
                    if (nextSurface->industryId() == surf->industryId())
                    {
                        continue;
                    }
                }
                else
                {
                    bool hasIndustryBuilding = [&nextLoc]() {
                        auto tile2 = TileManager::get(nextLoc);
                        bool passedSurface = false;
                        for (auto& el : tile2)
                        {
                            if (el.as<SurfaceElement>() != nullptr)
                            {
                                passedSurface = true;
                                continue;
                            }
                            if (passedSurface)
                            {
                                auto* elIndustry = el.as<IndustryElement>();
                                if (elIndustry == nullptr)
                                {
                                    continue;
                                }
                                if (elIndustry->isGhost())
                                {
                                    continue;
                                }
                                return true;
                            }
                        }
                        return false;
                    }();
                    if (hasIndustryBuilding)
                    {
                        continue;
                    }
                }
                uint8_t wallType = industryObj->wallTypes[0];
                if (wallType == 0xFFU)
                {
                    continue;
                }
                if (surf->getGrowthStage() != industryObj->farmTileGrowthStageNoProduction || industryObj->buildingWall != 0xFFU)
                {
                    if (surf->getGrowthStage() == industryObj->farmTileGrowthStageNoProduction)
                    {
                        wallType = industryObj->buildingWall;
                    }
                    GameCommands::WallPlacementArgs args{};
                    args.pos = Pos3(loc, 0);
                    args.primaryColour = Colour::black;
                    args.secondaryColour = Colour::black;
                    args.tertiaryColour = Colour::black;
                    args.type = wallType;
                    args.rotation = i;
                    GameCommands::doCommand(args, GameCommands::Flags::apply);

                    tileAddedRemoved = true;
                    // surf is invalid! Need to re-get it.
                    surf = TileManager::get(loc).surface();
                }
            }
        }
        return tileAddedRemoved;
    }

    // 0x004691FA
    bool updateSurface(SurfaceElement& elSurface, const World::Pos2 loc)
    {
        bool elSurfaceValid = true;
        if (elSurface.isIndustrial())
        {
            elSurfaceValid = !updateIndustrialSurface(elSurface, loc);
        }
        else
        {
            updateNonIndustrialSurface(elSurface, loc);
        }

        auto* surf = elSurfaceValid ? &elSurface : TileManager::get(loc).surface();

        const MicroZ snow = Scenario::getCurrentSnowLine() / kMicroToSmallZStep;
        uint8_t targetCoverage = std::clamp(((surf->baseZ() / kMicroToSmallZStep) + 1) - snow, 0, 5);
        if (surf->snowCoverage() != targetCoverage)
        {
            if (surf->snowCoverage() > targetCoverage)
            {
                surf->setSnowCoverage(surf->snowCoverage() - 1);
            }
            else
            {
                surf->setSnowCoverage(surf->snowCoverage() + 1);
            }
            Ui::ViewportManager::invalidate(loc, surf->baseHeight(), surf->baseHeight());
        }

        if (!surf->water())
        {
            return elSurfaceValid;
        }
        if (surf->hasType6Flag())
        {
            surf->setVariation(surf->variation() + 1);
            if (surf->variation() >= 64)
            {
                surf->setVariation(0);
                surf->setType6Flag(false);
            }
        }
        if (surf->isFlag6())
        {
            return elSurfaceValid;
        }
        WaveManager::createWave(*surf, loc);
        return elSurfaceValid;
    }
}
