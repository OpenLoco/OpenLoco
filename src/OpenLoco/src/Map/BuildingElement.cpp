#include "BuildingElement.h"
#include "Animation.h"
#include "AnimationManager.h"
#include "Game.h"
#include "GameCommands/Buildings/RemoveBuilding.h"
#include "GameCommands/GameCommands.h"
#include "GameStateFlags.h"
#include "Objects/BuildingObject.h"
#include "Objects/ObjectManager.h"
#include "ScenarioManager.h"
#include "Tile.h"
#include "TileManager.h"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"
#include "World/CompanyManager.h"
#include "World/Industry.h"
#include "World/StationManager.h"
#include "World/TownManager.h"

namespace OpenLoco::World
{
    const BuildingObject* BuildingElement::getObject() const
    {
        return ObjectManager::get<BuildingObject>(objectId());
    }

    template<typename TFunction>
    static void applyToMultiTile(BuildingElement& el0, const World::Pos2& loc, bool isMultiTile, TFunction&& func)
    {
        for (auto& offset : getBuildingTileOffsets(isMultiTile))
        {
            auto* elBuilding = &el0;
            const auto pos = loc + offset.pos;
            if (offset.index != 0)
            {
                auto tile = World::TileManager::get(pos);
                for (auto& el : tile)
                {
                    elBuilding = el.as<BuildingElement>();
                    if (elBuilding == nullptr)
                    {
                        continue;
                    }
                    if (elBuilding->baseZ() != el0.baseZ())
                    {
                        elBuilding = nullptr;
                        continue;
                    }
                    break;
                }
            }
            if (elBuilding != nullptr)
            {
                func(*elBuilding, pos);
            }
        }
    }

    // 0x0042DF8B
    bool BuildingElement::update(const World::Pos2& loc)
    {
        // Only update from tile index 0 of multi tile buildings
        if (sequenceIndex())
        {
            return true;
        }

        if (isGhost())
        {
            return true;
        }

        const auto* buildingObj = getObject();
        const auto isMultiTile = buildingObj->hasFlags(BuildingObjectFlags::largeTile);

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
                auto parts = buildingObj->getBuildingParts(variation());
                if (static_cast<uint32_t>(age() + 1) != parts.size())
                {
                    newUnk5u = 0;
                    newAge++;
                }
                else
                {
                    auto totalHeight = 3;
                    for (auto part : parts)
                    {
                        totalHeight += buildingObj->partHeights[part];
                    }

                    const auto newClearHeight = baseZ() + totalHeight / 4;

                    applyToMultiTile(*this, loc, isMultiTile, [newClearHeight](World::BuildingElement& elBuilding2, const World::Pos2& pos) {
                        Ui::ViewportManager::invalidate(pos, elBuilding2.baseHeight(), elBuilding2.clearHeight(), ZoomLevel::quarter);
                        elBuilding2.setClearZ(newClearHeight);
                    });

                    if (buildingObj->numElevatorSequences != 0)
                    {
                        AnimationManager::createAnimation(5, loc, baseZ());
                    }

                    TownManager::updateTownInfo(loc, buildingObj->producedQuantity[0], 0, 0, 0);

                    newUnk5u = 0;
                    newAge = 0;
                    isConstructed = true;
                }
            }

            applyToMultiTile(*this, loc, isMultiTile, [isConstructed, newUnk5u, newAge](World::BuildingElement& elBuilding2, const World::Pos2& pos) {
                elBuilding2.setConstructed(isConstructed);
                elBuilding2.setUnk5u(newUnk5u);
                elBuilding2.setAge(newAge);
                Ui::ViewportManager::invalidate(pos, elBuilding2.baseHeight(), elBuilding2.clearHeight(), ZoomLevel::quarter);
            });
        }

        if (isMiscBuilding())
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
                // Why are we not invalidating??
                applyToMultiTile(*this, loc, isMultiTile, [newAge](World::BuildingElement& elBuilding2, const World::Pos2&) {
                    elBuilding2.setAge(newAge);
                });
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

    // 0x0042E4D4
    bool updateBuildingAnimation1(const Animation& anim)
    {
        auto tile = TileManager::get(anim.pos);
        BuildingElement* elBuilding = nullptr;
        for (auto& el : tile)
        {
            elBuilding = el.as<BuildingElement>();
            if (elBuilding == nullptr)
            {
                continue;
            }
            if (elBuilding->baseZ() != anim.baseZ)
            {
                continue;
            }
            break;
        }

        if (elBuilding == nullptr)
        {
            return true;
        }
        // Animations are controlled from index 0
        if (elBuilding->sequenceIndex() != 0)
        {
            return true;
        }

        auto* buildingObj = elBuilding->getObject();
        if (buildingObj->numElevatorSequences == 0)
        {
            return true;
        }

        if (ScenarioManager::getScenarioTicks() & 0b1)
        {
            return false;
        }

        bool isMultiTile = buildingObj->hasFlags(BuildingObjectFlags::largeTile);

        applyToMultiTile(*elBuilding, anim.pos, isMultiTile, [](World::BuildingElement& elBuilding, const World::Pos2& pos) {
            Ui::ViewportManager::invalidate(pos, elBuilding.baseHeight(), elBuilding.clearHeight(), ZoomLevel::half);
        });
        return false;
    }

    // 0x0042E646
    bool updateBuildingAnimation2(const Animation& anim)
    {
        auto tile = TileManager::get(anim.pos);
        BuildingElement* elBuilding = nullptr;
        for (auto& el : tile)
        {
            elBuilding = el.as<BuildingElement>();
            if (elBuilding == nullptr)
            {
                continue;
            }
            if (elBuilding->baseZ() != anim.baseZ)
            {
                continue;
            }
            break;
        }

        if (elBuilding == nullptr)
        {
            return true;
        }

        auto* buildingObj = elBuilding->getObject();
        auto parts = buildingObj->getBuildingParts(elBuilding->variation());
        uint8_t slowestSpeed = 0xFF;
        uint8_t numFrames = 0;
        for (auto part : parts)
        {
            auto& partAnim = buildingObj->partAnimations[part];
            const uint8_t animSpeed = partAnim.animationSpeed & ~(1U << 7);
            if (partAnim.numFrames == 0)
            {
                continue;
            }
            numFrames |= partAnim.numFrames;
            if (partAnim.numFrames == 1)
            {
                continue;
            }
            slowestSpeed = std::min(slowestSpeed, animSpeed);
        }
        if (numFrames <= 1)
        {
            return true;
        }
        const auto speedMask = ((1 << slowestSpeed) - 1);
        if (!(ScenarioManager::getScenarioTicks() & speedMask))
        {
            Ui::ViewportManager::invalidate(anim.pos, elBuilding->baseHeight(), elBuilding->clearHeight(), ZoomLevel::quarter);
        }
        return false;
    }
}
