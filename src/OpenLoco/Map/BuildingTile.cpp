#include "../GameCommands/GameCommands.h"
#include "../Objects/BuildingObject.h"
#include "../StationManager.h"
#include "../TownManager.h"
#include "../ViewportManager.h"
#include "AnimationManager.h"
#include "Tile.h"
#include "TileManager.h"

namespace OpenLoco::Map
{
    static void sub_497DC1(const Map::Pos2& loc, uint32_t population, uint32_t unk1, uint16_t rating, uint16_t unk3)
    {
        registers regs;
        regs.edi = population;
        regs.esi = unk1;
        regs.ebp = rating | (unk3 << 16);
        regs.ax = loc.x;
        regs.cx = loc.y;
        call(0x00497DC1, regs);
    }

    // 0x0042DF8B
    bool BuildingElement::update(const Map::Pos2& loc)
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

        auto* buildingObj = object();
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
                    Ui::ViewportManager::invalidate(loc, baseZ() * 4, clearZ() * 4, ZoomLevel::quarter);

                    const auto newClearHeight = baseZ() + totalHeight / 4;
                    setClearZ(newClearHeight);
                    if (buildingObj->var_AD != 0)
                    {
                        AnimationManager::createAnimation(5, loc, baseZ());
                    }
                    if (buildingObj->flags & BuildingObjectFlags::large_tile)
                    {
                        for (auto i = 1; i < 4; ++i)
                        {
                            const auto pos = loc + Map::offsets[i];
                            auto tile = TileManager::get(pos);
                            for (auto& el : tile)
                            {
                                auto* elBuilding2 = el.asBuilding();
                                if (elBuilding2 == nullptr)
                                {
                                    continue;
                                }
                                if (elBuilding2->baseZ() != baseZ())
                                {
                                    continue;
                                }

                                Ui::ViewportManager::invalidate(pos, elBuilding2->baseZ() * 4, elBuilding2->clearZ() * 4, ZoomLevel::quarter);
                                elBuilding2->setClearZ(newClearHeight);
                            }
                        }
                    }

                    sub_497DC1(loc, buildingObj->producedQuantity[0], 0, 0, 0);

                    newUnk5u = 0;
                    newAge = 0;
                    isConstructed = true;
                }
            }
            setConstructed(isConstructed);
            setUnk5u(newUnk5u);
            setAge(newAge);
            Ui::ViewportManager::invalidate(loc, baseZ() * 4, clearZ() * 4, ZoomLevel::quarter);
            if (buildingObj->flags & BuildingObjectFlags::large_tile)
            {
                for (auto i = 1; i < 4; ++i)
                {
                    const auto pos = loc + Map::offsets[i];
                    auto tile = TileManager::get(pos);
                    for (auto& el : tile)
                    {
                        auto* elBuilding2 = el.asBuilding();
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
                        Ui::ViewportManager::invalidate(pos, elBuilding2->baseZ() * 4, elBuilding2->clearZ() * 4, ZoomLevel::quarter);
                    }
                }
            }
        }

        if (has_40())
        {
            return true;
        }

        auto res = TownManager::getClosestTownAndUnk(loc);
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
                if (buildingObj->flags & BuildingObjectFlags::large_tile)
                {
                    for (auto i = 1; i < 4; ++i)
                    {
                        const auto pos = loc + Map::offsets[i];
                        auto tile = TileManager::get(pos);
                        for (auto& el : tile)
                        {
                            auto* elBuilding2 = el.asBuilding();
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
                args.pos = Map::Pos3(loc.x, loc.y, baseZ() * 4);
                GameCommands::do_45(GameCommands::Flags::apply, args);
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
                if ((addr<0x00525E28, uint32_t>() & (1 << 1)) != 0)
                {
                    producedAmount = (producedAmount + 1) / 2;
                }
                town->var_19C[i][0] += producedAmount;

                const auto size = (buildingObj->flags & BuildingObjectFlags::large_tile) ? Map::TilePos2(2, 2) : Map::TilePos2(1, 1);
                town->var_19C[i][1] += StationManager::deliverCargoToNearbyStations(buildingObj->producedCargoType[i], producedAmount, loc, size) & 0xFF;
            }
        }
        return true;
    }
}
