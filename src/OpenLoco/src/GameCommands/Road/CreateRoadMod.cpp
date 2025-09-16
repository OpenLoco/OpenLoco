#include "CreateRoadMod.h"
#include "Map/RoadElement.h"
#include "Map/TileManager.h"
#include "Map/Track/TrackData.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"

namespace OpenLoco::GameCommands
{
    static World::RoadElement* getRoadElement(const RoadModsPlacementArgs& args)
    {
        auto tile = World::TileManager::get(args.pos);
        for (auto& element : tile)
        {
            auto* roadEl = element.as<World::RoadElement>();
            if (roadEl == nullptr)
            {
                continue;
            }
            if (roadEl->baseHeight() != args.pos.z)
            {
                continue;
            }
            if (roadEl->rotation() != args.rotation)
            {
                continue;
            }
            if (roadEl->rotation() != args.rotation)
            {
                continue;
            }
            if (roadEl->sequenceIndex() != args.index)
            {
                continue;
            }
            if (roadEl->roadObjectId() != args.roadObjType)
            {
                continue;
            }
            if (roadEl->roadId() != args.roadId)
            {
                continue;
            }
            return roadEl;
        }
        return nullptr;
    }

    // 0x0047A21E
    static uint32_t createRoadMod(const RoadModsPlacementArgs& args, uint8_t flags)
    {
        auto* roadEl = getRoadElement(args);
        if (roadEl == nullptr || !sub_431E6A(roadEl->owner(), reinterpret_cast<const World::TileElement*>(roadEl)))
        {
            return FAILURE;
        }

        auto& piece = World::TrackData::getRoadPiece(args.roadId)[roadEl->sequenceIndex()];
        const auto roadLoc = World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, args.rotation), piece.z };

        // 0x0047A34C
        const auto firstTilePos = args.pos - roadLoc;
        const auto rad = Vehicles::TrackAndDirection::_RoadAndDirection(roadEl->roadId(), roadEl->rotation());

        auto result = Vehicles::applyRoadModsToTrackNetwork(firstTilePos, rad, roadEl->owner(), args.roadObjType, flags, args.modSection, args.type);
        if (result.allPlacementsFailed)
        {
            setErrorText(StringIds::track_road_unsuitable);
            return FAILURE;
        }
        if (result.networkTooComplex && (flags & Flags::apply) && !(flags & Flags::ghost))
        {
            Ui::Windows::Error::open(StringIds::null, StringIds::too_much_track_some_track_not_upgraded);
        }

        return result.cost;
    }

    void createRoadMod(registers& regs)
    {
        regs.ebx = createRoadMod(RoadModsPlacementArgs(regs), regs.bl);
    }
}
