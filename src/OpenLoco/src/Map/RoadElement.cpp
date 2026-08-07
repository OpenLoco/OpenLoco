#include "Map/RoadElement.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Road/RemoveRoad.h"
#include "GameState.h"
#include "Map/Tile.h"
#include "Map/TileManager.h"
#include "Objects/LevelCrossingObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Scenario/ScenarioManager.h"
#include "ViewportManager.h"
#include "World/CompanyManager.h"

namespace OpenLoco::World
{
    RoadElement::RoadElement(World::SmallZ baseZ, World::SmallZ clearZ)
        : _4(0)
        , _5(0)
        , _6(0)
        , _7(0)
    {
        _0 = 0;
        _flags = 0;
        setBaseZ(baseZ);
        setClearZ(clearZ);
    }

    // 0x00477FC2
    bool RoadElement::tick(const World::Pos2& loc)
    {
        if (owner() == CompanyId::neutral || CompanyManager::isPlayerCompany(owner()))
        {
            return true;
        }

        if (!(getGameState().roadObjectIdIsAnyRoadTypeCompatible & (1 << roadObjectId())))
        {
            return true;
        }

        if (sequenceIndex())
        {
            return true;
        }

        if (isLevelCrossingClosed() || hasLevelCrossing() || hasUnk7_40() || hasUnk7_80())
        {
            return true;
        }

        if (isGhost() || isAiAllocated())
        {
            return true;
        }

        if (hasStationElement())
        {
            return true;
        }

        // Verify there are no other conflicting tile elements on the current tile either.
        // This probably duplicates the above series of checks as well?
        auto tile = TileManager::get(loc);
        for (auto& el : tile)
        {
            auto* roadEl = el.as<RoadElement>();
            if (roadEl == nullptr)
            {
                continue;
            }

            if (roadEl->baseZ() != baseZ())
            {
                continue;
            }

            if (roadEl->owner() == CompanyId::neutral || CompanyManager::isPlayerCompany(roadEl->owner()))
            {
                continue;
            }

            if (!(getGameState().roadObjectIdIsAnyRoadTypeCompatible & (1 << roadEl->roadObjectId())))
            {
                continue;
            }

            if (roadEl->sequenceIndex())
            {
                return true;
            }

            if (isLevelCrossingClosed() || hasLevelCrossing() || hasUnk7_40() || hasUnk7_80())
            {
                return true;
            }

            if (roadEl->isGhost() || roadEl->isAiAllocated())
            {
                return true;
            }

            if (roadEl->hasStationElement())
            {
                return true;
            }
        }

        CompanyId backup = GameCommands::getUpdatingCompanyId();
        GameCommands::setUpdatingCompanyId(owner());

        GameCommands::RoadRemovalArgs args;
        args.pos = World::Pos3(loc.x, loc.y, baseHeight());
        args.rotation = rotation();
        args.roadId = roadId();
        args.sequenceIndex = sequenceIndex();
        args.objectId = roadObjectId();
        GameCommands::doCommand(args, GameCommands::Flags::apply);

        GameCommands::setUpdatingCompanyId(backup);

        return false;
    }

    // 0x00479413
    bool updateLevelCrossingAnimation(const Animation& anim)
    {
        auto tile = TileManager::get(anim.pos);
        bool shouldInvalidate = false;
        bool hasAnimation = false;

        // It's possible to have multiple level crossing elements on the same tile/baseZ
        // this happens if you have tram tracks on road on track.
        for (auto& el : tile)
        {
            auto* elRoad = el.as<RoadElement>();
            if (elRoad == nullptr)
            {
                continue;
            }
            if (elRoad->baseZ() != anim.baseZ)
            {
                continue;
            }
            if (!elRoad->hasLevelCrossing())
            {
                continue;
            }

            const auto* levelCrossingObj = ObjectManager::get<LevelCrossingObject>(elRoad->levelCrossingObjectId());

            if (ScenarioManager::getScenarioTicks() & levelCrossingObj->transitionAnimationDelayBitmask)
            {
                hasAnimation = true; // skip this tick - don't advance the transition
            }
            else
            {
                // advance the transition animation by 1 frame
                shouldInvalidate = true;
                auto newFrame = elRoad->levelCrossingAnimationFrame();

                if (elRoad->isLevelCrossingClosed()) // closing
                {
                    if (newFrame != 15)
                    {
                        newFrame++;
                        if (newFrame > levelCrossingObj->transitionAnimationFrameCount)
                        {
                            newFrame = 15; // transition complete; fully closed
                        }
                    }
                    hasAnimation = true;
                }
                else // opening
                {
                    if (newFrame != 0)
                    {
                        newFrame--;
                        if (newFrame == 14)
                        {
                            newFrame = levelCrossingObj->transitionAnimationFrameCount;
                        }
                        hasAnimation = true;
                    }
                    // Doesn't set hasAnimation = true on else branch!
                    // when newFrame == 0, hasAnimation is false and thus transition is complete; fully open
                }
                elRoad->setLevelCrossingAnimationFrame(newFrame);
            }
        }
        if (shouldInvalidate)
        {
            Ui::ViewportManager::invalidate(anim.pos, anim.baseZ * kSmallZStep, anim.baseZ * kSmallZStep + 32, ZoomLevel::half);
        }

        return !hasAnimation;
    }
}
