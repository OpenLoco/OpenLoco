#include "RoadElement.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Road/RemoveRoad.h"
#include "GameState.h"
#include "Objects/LevelCrossingObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "ScenarioManager.h"
#include "Tile.h"
#include "TileManager.h"
#include "ViewportManager.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::World
{
    RoadElement::RoadElement(World::SmallZ baseZ, World::SmallZ clearZ)
        : _4(0)
        , _5(0)
        , _6(0)
        , _7(0)
    {
        _type = 0;
        _flags = 0;
        setType(World::ElementType::road);
        setBaseZ(baseZ);
        setClearZ(clearZ);
    }

    // 0x00477FC2
    bool RoadElement::update(const World::Pos2& loc)
    {
        if (owner() == CompanyId::neutral || CompanyManager::isPlayerCompany(owner()))
            return true;

        if (!(getGameState().roadObjectIdIsNotTram & (1 << roadObjectId())))
            return true;

        if (sequenceIndex())
            return true;

        if (hasUnk7_10() || hasLevelCrossing() || hasUnk7_40() || hasUnk7_80())
            return true;

        if (isGhost() || isAiAllocated())
            return true;

        if (hasStationElement())
            return true;

        // Verify there are no other conflicting tile elements on the current tile either.
        // This probably duplicates the above series of checks as well?
        auto tile = TileManager::get(loc);
        for (auto& el : tile)
        {
            auto* roadEl = el.as<RoadElement>();
            if (roadEl == nullptr)
                continue;

            if (roadEl->baseZ() != baseZ())
                continue;

            if (roadEl->owner() == CompanyId::neutral || CompanyManager::isPlayerCompany(roadEl->owner()))
                continue;

            if (!(getGameState().roadObjectIdIsNotTram & (1 << roadEl->roadObjectId())))
                continue;

            if (roadEl->sequenceIndex())
                return true;

            if (hasUnk7_10() || hasLevelCrossing() || hasUnk7_40() || hasUnk7_80())
                return true;

            if (roadEl->isGhost() || roadEl->isAiAllocated())
                return true;

            if (roadEl->hasStationElement())
                return true;
        }

        CompanyId backup = CompanyManager::getUpdatingCompanyId();
        CompanyManager::setUpdatingCompanyId(owner());

        GameCommands::RoadRemovalArgs args;
        args.pos = World::Pos3(loc.x, loc.y, baseHeight());
        args.rotation = rotation();
        args.roadId = roadId();
        args.sequenceIndex = sequenceIndex();
        args.objectId = roadObjectId();
        GameCommands::doCommand(args, GameCommands::Flags::apply);

        CompanyManager::setUpdatingCompanyId(backup);

        return false;
    }

    // 0x00479413
    bool updateLevelCrossingAnimation(const Animation& anim)
    {
        auto tile = TileManager::get(anim.pos);
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
            bool shouldInvalidate = false;
            bool hasAnimation = false;

            const auto* levelCrossingObj = ObjectManager::get<LevelCrossingObject>(elRoad->levelCrossingObjectId());

            if (ScenarioManager::getScenarioTicks() & levelCrossingObj->var_0A)
            {
                hasAnimation = true;
            }
            else
            {
                shouldInvalidate = true;
                auto newFrame = elRoad->unk6l();
                if (elRoad->hasUnk7_10())
                {
                    if (newFrame != 15)
                    {
                        newFrame++;
                        if (newFrame > levelCrossingObj->closedFrames)
                        {
                            newFrame = 15;
                        }
                    }
                    hasAnimation = true;
                }
                else
                {
                    if (newFrame != 0)
                    {
                        newFrame--;
                        if (newFrame == 14)
                        {
                            newFrame = levelCrossingObj->closedFrames;
                        }
                        hasAnimation = true;
                    }
                    // Doesn't set hasAnimation = true on else branch!
                }
                elRoad->setUnk6l(newFrame);
            }

            if (shouldInvalidate)
            {
                Ui::ViewportManager::invalidate(anim.pos, el.baseHeight(), el.clearHeight(), ZoomLevel::half);
            }

            return !hasAnimation;
        }
        return true;
    }
}
