#include "RoadElement.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Road/RemoveRoad.h"
#include "GameState.h"
#include "Objects/RoadObject.h"
#include "Tile.h"
#include "TileManager.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::World
{
    // 0x00477FC2
    bool RoadElement::update(const World::Pos2& loc)
    {
        if (owner() == CompanyId::neutral || CompanyManager::isPlayerCompany(owner()))
            return true;

        if (!(getGameState().roadObjectIdIsTram & (1 << roadObjectId())))
            return true;

        if (sequenceIndex())
            return true;

        if (hasUnk7_10() || hasLevelCrossing() || mods())
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

            if (!(getGameState().roadObjectIdIsTram & (1 << roadEl->roadObjectId())))
                continue;

            if (roadEl->sequenceIndex())
                return true;

            if (hasUnk7_10() || hasLevelCrossing() || mods())
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
        args.unkDirection = unkDirection();
        args.roadId = roadId();
        args.sequenceIndex = sequenceIndex();
        args.objectId = roadObjectId();
        GameCommands::doCommand(args, GameCommands::Flags::apply);

        CompanyManager::setUpdatingCompanyId(backup);

        return false;
    }
}
