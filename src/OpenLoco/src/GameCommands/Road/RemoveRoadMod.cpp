#include "RemoveRoadMod.h"
#include "Economy/Expenditures.h"
#include "Map/RoadElement.h"
#include "Map/TileManager.h"
#include "Map/Track/TrackData.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include <OpenLoco/Math/Vector.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    // 0x0047A42F
    static currency32_t removeRoadMod(const RoadModsRemovalArgs& args, uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Construction);
        setPosition(args.pos + World::Pos3(16, 16, 0));

        auto* elRoad = [&args]() -> const World::RoadElement* {
            const auto tile = World::TileManager::get(args.pos);
            for (const auto& el : tile)
            {
                auto* elRoad = el.as<World::RoadElement>();
                if (elRoad == nullptr)
                {
                    continue;
                }

                if (elRoad->baseHeight() != args.pos.z)
                {
                    continue;
                }

                if (elRoad->rotation() != args.rotation)
                {
                    continue;
                }

                if (elRoad->sequenceIndex() != args.index)
                {
                    continue;
                }

                if (elRoad->roadObjectId() != args.roadObjType)
                {
                    continue;
                }

                if (elRoad->roadId() != args.roadId)
                {
                    continue;
                }

                return elRoad;
            }
            return nullptr;
        }();

        if (elRoad == nullptr)
        {
            return FAILURE;
        }

        if (!sub_431E6A(elRoad->owner(), reinterpret_cast<const World::TileElement*>(elRoad)))
        {
            return FAILURE;
        }

        const auto& piece = World::TrackData::getRoadPiece(elRoad->roadId())[elRoad->sequenceIndex()];
        const auto offsetToFirstTile = World::Pos3{
            Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, elRoad->rotation()),
            piece.z
        };
        const auto firstTilePos = args.pos - offsetToFirstTile;
        const auto tad = Vehicles::TrackAndDirection::_RoadAndDirection(elRoad->roadId(), elRoad->rotation());

        auto cost = Vehicles::removeRoadModsToTrackNetwork(firstTilePos, tad, elRoad->owner(), args.roadObjType, flags, args.modSection, args.type);

        return cost;
    }

    void removeRoadMod(registers& regs)
    {
        regs.ebx = removeRoadMod(RoadModsRemovalArgs(regs), regs.bl);
    }
}
