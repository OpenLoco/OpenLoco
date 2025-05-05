#include "CreateRoadMod.h"
#include "Map/RoadElement.h"
#include "Map/TileManager.h"
#include "Map/Track/TrackData.h"

namespace OpenLoco::GameCommands
{
    /*
    struct RoadModsPlacementArgs
    {
        World::Pos3 pos;
        uint8_t rotation;
        uint8_t roadId;
        uint8_t index;
        uint8_t type;
        uint8_t roadObjType;
        World::Track::ModSection modSection;

        registers regs;
        regs.ax = pos.x;
        regs.cx = pos.y;
        regs.bh = rotation;
        regs.dl = roadId;
        regs.dh = index;
        regs.edi = pos.z | (type << 16);
        regs.ebp = roadObjType | (enumValue(modSection) << 16);
    };
    */

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
        uint32_t totalCost = 0;

        auto* roadEl = getRoadElement(args);
        if (roadEl == nullptr || !sub_431E6A(roadEl->owner(), reinterpret_cast<const World::TileElement*>(roadEl)))
        {
            return FAILURE;
        }

        auto& piece = World::TrackData::getRoadPiece(args.roadId)[roadEl->sequenceIndex()];
        const auto roadLoc = args.pos + World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, args.rotation), piece.z };

        // 0x0047A34C

        return totalCost;
    }

    void createRoadMod(registers& regs)
    {
        regs.ebx = createRoadMod(RoadModsPlacementArgs(regs), regs.bl);
    }
}
