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
            setVar6SLR5(0);
            setIndustry(IndustryId(0));
            auto z = baseHeight();
            Ui::ViewportManager::invalidate(pos, z, z + 32, ZoomLevel::eighth);
            TownManager::updateTownInfo(pos, 0, 0, -30, 0);
        }
    }

    static void updateNonIndustrialSurface(SurfaceElement& elSurface, const World::Pos2 loc)
    {
        auto* landObj = ObjectManager::get<LandObject>(elSurface.terrain());
        if (landObj->hasFlags(LandObjectFlags::unk0))
        {
            return;
        }
        elSurface.setVar5SLR5(elSurface.getVar5SLR5() + 1);
        if (elSurface.getVar5SLR5() != 0)
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
                if (elSurface.var_6_SLR5())
                {
                    elSurface.setVar6SLR5(0);
                    Ui::ViewportManager::invalidate(loc, elSurface.baseHeight(), elSurface.baseHeight());
                }
                return;
            }
        }

        if (elSurface.var_6_SLR5() + 1 < landObj->var_03)
        {
            elSurface.setVar6SLR5(elSurface.var_6_SLR5() + 1);
            Ui::ViewportManager::invalidate(loc, elSurface.baseHeight(), elSurface.baseHeight());
        }
    }

    WallElement* sub_4C49E9(const World::Pos2 loc, SmallZ baseZ, SmallZ clearZ, uint8_t direction)
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

    static void updateIndustrialSurface(SurfaceElement& elSurface, const World::Pos2 loc)
    {
        auto* industry = IndustryManager::get(elSurface.industryId());
        auto* industryObj = industry->getObject();
        if (industryObj->hasFlags(IndustryObjectFlags::builtOnSnow) && elSurface.snowCoverage() < 4)
        {
            TileManager::removeSurfaceIndustry(loc);
            return;
        }

        if (elSurface.var_6_SLR5() != industryObj->var_EA || elSurface.var_6_SLR5() == 0)
        {
            elSurface.setVar5SLR5(elSurface.getVar5SLR5() + 1);
            if (elSurface.getVar5SLR5() == 0)
            {
                uint8_t newVar6SLR5 = elSurface.var_6_SLR5() + 1;
                if (newVar6SLR5 > industryObj->var_EC)
                {
                    newVar6SLR5 = 0;
                }
                elSurface.setVar6SLR5(newVar6SLR5);
                Ui::ViewportManager::invalidate(loc, elSurface.baseHeight(), elSurface.baseHeight());
            }
        }

        for (auto i = 0; i < 4; ++i)
        {
            auto* elWall = sub_4C49E9(loc, elSurface.baseZ(), elSurface.baseZ() + 16, i);
            if (elWall)
            {
                if (elSurface.var_6_SLR5() != industryObj->var_EA)
                {
                    continue;
                }
                const auto nextLoc = loc + kRotationOffset[i];
                if (!validCoords(nextLoc))
                {
                    continue;
                }

                auto* nextSurface = TileManager::get(nextLoc).surface();
                if (!nextSurface->isIndustrial() || nextSurface->industryId() != elSurface.industryId())
                {
                    continue;
                }
                if (nextSurface->var_6_SLR5() != industryObj->var_EA)
                {
                    continue;
                }
                GameCommands::WallRemovalArgs args{};
                args.pos = Pos3(loc, elWall->baseHeight());
                args.rotation = i;
                GameCommands::doCommand(args, GameCommands::Flags::apply);
                continue;
            }
            else
            {
                const auto nextLoc = loc + kRotationOffset[i];
                if (!validCoords(nextLoc))
                {
                    continue;
                }

                auto* elWall2 = sub_4C49E9(nextLoc, elSurface.baseZ(), elSurface.baseZ() + 16, i);
                if (elWall2)
                {
                    continue;
                }
                auto* nextSurface = TileManager::get(nextLoc).surface();
                if (nextSurface->isIndustrial())
                {
                    if (nextSurface->industryId() == elSurface.industryId())
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
                if (elSurface.var_6_SLR5() != industryObj->var_EA || industryObj->buildingWall != 0xFFU)
                {
                    if (elSurface.var_6_SLR5() == industryObj->var_EA)
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

                    // elSurface is invalid! Need to get it again!
                }
            }
        }
    }

    // 0x004691FA
    void updateSurface(SurfaceElement& elSurface, const World::Pos2 loc)
    {
        if (elSurface.isIndustrial())
        {
            updateIndustrialSurface(elSurface, loc);
        }
        else
        {
            updateNonIndustrialSurface(elSurface, loc);
        }

        const MicroZ snow = Scenario::getCurrentSnowLine() / kMicroToSmallZStep;
        uint8_t targetCoverage = std::clamp(((elSurface.baseZ() / kMicroToSmallZStep) + 1) - snow, 0, 5);
        if (elSurface.snowCoverage() != targetCoverage)
        {
            if (elSurface.snowCoverage() > targetCoverage)
            {
                elSurface.setSnowCoverage(elSurface.snowCoverage() - 1);
            }
            else
            {
                elSurface.setSnowCoverage(elSurface.snowCoverage() + 1);
            }
            Ui::ViewportManager::invalidate(loc, elSurface.baseHeight(), elSurface.baseHeight());
        }

        if (!elSurface.water())
        {
            return;
        }
        if (elSurface.hasType6Flag())
        {
            elSurface.setVariation(elSurface.variation() + 1);
            if (elSurface.variation() >= 64)
            {
                elSurface.setVariation(0);
                elSurface.setType6Flag(false);
            }
        }
        if (elSurface.isFlag6())
        {
            return;
        }
        WaveManager::createWave(elSurface, loc);
    }
}
