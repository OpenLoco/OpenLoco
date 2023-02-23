#include "BuildingElement.h"
#include "AnimationManager.h"
#include "CompanyManager.h"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "GameStateFlags.h"
#include "Objects/BuildingObject.h"
#include "Objects/ObjectManager.h"
#include "StationManager.h"
#include "Tile.h"
#include "TileManager.h"
#include "TownManager.h"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"

namespace OpenLoco::World
{
    const BuildingObject* BuildingElement::getObject() const
    {
        return ObjectManager::get<BuildingObject>(objectId());
    }

    // 0x0042DF8B
    bool BuildingElement::update(const World::Pos2& loc)
    {
        // Only update from tile index 0 of multi tile buildings
        if (multiTileIndex())
        {
            return true;
        }

        if (isGhost())
        {
            return true;
        }

        const auto* buildingObj = getObject();
        if (!isConstructed())
        {
            auto newUnk5u = unk5u();
            auto newAge = age();
            auto isConstructed = false;
            if (unk5u() != 7)
            {
                newUnk5u++;
            }
            else
            {
                auto* unkVariation = buildingObj->variationsArr10[variation()];
                if (unkVariation[age() + 1] != 0xFF)
                {
                    newUnk5u = 0;
                    newAge++;
                }
                else
                {
                    auto totalHeight = 3;
                    for (; *unkVariation != 0xFF; unkVariation++)
                    {
                        totalHeight += buildingObj->varationHeights[*unkVariation];
                    }
                    Ui::ViewportManager::invalidate(loc, baseHeight(), clearHeight(), ZoomLevel::quarter);

                    const auto newClearHeight = baseZ() + totalHeight / 4;
                    setClearZ(newClearHeight);
                    if (buildingObj->var_AD != 0)
                    {
                        AnimationManager::createAnimation(5, loc, baseZ());
                    }
                    if (buildingObj->hasFlags(BuildingObjectFlags::largeTile))
                    {
                        for (auto i = 1; i < 4; ++i)
                        {
                            const auto pos = loc + World::offsets[i];
                            auto tile = TileManager::get(pos);
                            for (auto& el : tile)
                            {
                                auto* elBuilding2 = el.as<BuildingElement>();
                                if (elBuilding2 == nullptr)
                                {
                                    continue;
                                }
                                if (elBuilding2->baseZ() != baseZ())
                                {
                                    continue;
                                }

                                Ui::ViewportManager::invalidate(pos, elBuilding2->baseHeight(), elBuilding2->clearHeight(), ZoomLevel::quarter);
                                elBuilding2->setClearZ(newClearHeight);
                            }
                        }
                    }

                    TownManager::sub_497DC1(loc, buildingObj->producedQuantity[0], 0, 0, 0);

                    newUnk5u = 0;
                    newAge = 0;
                    isConstructed = true;
                }
            }
            setConstructed(isConstructed);
            setUnk5u(newUnk5u);
            setAge(newAge);
            Ui::ViewportManager::invalidate(loc, baseHeight(), clearHeight(), ZoomLevel::quarter);
            if (buildingObj->hasFlags(BuildingObjectFlags::largeTile))
            {
                for (auto i = 1; i < 4; ++i)
                {
                    const auto pos = loc + World::offsets[i];
                    auto tile = TileManager::get(pos);
                    for (auto& el : tile)
                    {
                        auto* elBuilding2 = el.as<BuildingElement>();
                        if (elBuilding2 == nullptr)
                        {
                            continue;
                        }
                        if (elBuilding2->baseZ() != baseZ())
                        {
                            continue;
                        }
                        elBuilding2->setConstructed(isConstructed);
                        elBuilding2->setUnk5u(newUnk5u);
                        elBuilding2->setAge(newAge);
                        Ui::ViewportManager::invalidate(pos, elBuilding2->baseHeight(), elBuilding2->clearHeight(), ZoomLevel::quarter);
                    }
                }
            }
        }

        if (has_40())
        {
            return true;
        }

        auto res = TownManager::getClosestTownAndDensity(loc);
        if (!res)
        {
            return true;
        }

        auto* town = TownManager::get(res->first);
        if (isConstructed())
        {
            setUnk5u(unk5u() + 1);
            if (!unk5u() && age() != 63)
            {
                const auto newAge = age() + 1;

                setAge(newAge);
                if (buildingObj->hasFlags(BuildingObjectFlags::largeTile))
                {
                    for (auto i = 1; i < 4; ++i)
                    {
                        const auto pos = loc + World::offsets[i];
                        auto tile = TileManager::get(pos);
                        for (auto& el : tile)
                        {
                            auto* elBuilding2 = el.as<BuildingElement>();
                            if (elBuilding2 == nullptr)
                            {
                                continue;
                            }
                            if (elBuilding2->baseZ() != baseZ())
                            {
                                continue;
                            }
                            elBuilding2->setAge(newAge);
                        }
                    }
                }
            }
        }

        if (isConstructed() && age() >= 40)
        {
            if (town->prng.randNext(0xFFFF) <= 16)
            {
                GameCommands::BuildingRemovalArgs args;
                args.pos = World::Pos3(loc.x, loc.y, baseHeight());
                GameCommands::doCommand(args, GameCommands::Flags::apply);
                return false;
            }
        }

        if (isConstructed())
        {
            // This is purely to keep things in sync when confirmed no desync move
            // rand call into loop and force a desync.
            const uint16_t randVal = town->prng.randNext(0xFFFF);
            const uint8_t randArr[2] = { static_cast<uint8_t>(randVal), static_cast<uint8_t>(randVal >> 8) };
            for (auto i = 0; i < 2; ++i)
            {
                if (randArr[i] >= buildingObj->producedQuantity[i])
                {
                    continue;
                }

                auto producedAmount = randArr[i] / 4 + 1;
                if (Game::hasFlags(GameStateFlags::unk2))
                {
                    producedAmount = (producedAmount + 1) / 2;
                }
                town->var_19C[i][0] += producedAmount;

                const auto size = buildingObj->hasFlags(BuildingObjectFlags::largeTile) ? World::TilePos2(2, 2) : World::TilePos2(1, 1);
                town->var_19C[i][1] += StationManager::deliverCargoToNearbyStations(buildingObj->producedCargoType[i], producedAmount, loc, size) & 0xFF;
            }
        }
        return true;
    }
}
