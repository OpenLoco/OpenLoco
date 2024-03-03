#include "CreateTrack.h"
#include "Economy/Economy.h"
#include "Map/TileManager.h"
#include "Map/Track/TrackData.h"
#include "Objects/ObjectManager.h"
#include "Objects/TrackExtraObject.h"
#include "Objects/TrackObject.h"
#include "World/StationManager.h"

namespace OpenLoco::GameCommands
{
    static loco_global<uint8_t, 0x01136072> _byte_1136072;
    static loco_global<uint8_t, 0x01136073> _byte_1136073;
    static loco_global<uint8_t, 0x01136074> _byte_1136074;
    static loco_global<uint8_t, 0x01136075> _byte_1136075;
    static loco_global<uint16_t[44], 0x004F8764> _4F8764;

    // 0x0049BB98
    uint32_t createTrack(const TrackPlacementArgs& args, uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Construction);
        setPosition(args.pos + World::Pos3{ 16, 16, 0 });
        _byte_1136072 = 0; // Tunnel related
        _byte_1136073 = 0; // Bridge related
        _byte_1136074 = 0;
        _byte_1136075 = 0xFFU;
        // 0x01135C68 = unkFlags

        if (flags & Flags::apply && !(flags & Flags::aiAllocated))
        {
            companySetObservation(getUpdatingCompanyId(), ObservationStatus::buildingTrackRoad, args.pos, EntityId::null, args.trackObjectId);
        }

        if (World::TileManager::checkFreeElementsAndReorganise())
        {
            return FAILURE;
        }

        auto* trackObj = ObjectManager::get<TrackObject>(args.trackObjectId);
        const auto compatFlags = World::TrackData::getTrackCompatibleFlags(args.trackId);
        uint8_t validMods = args.mods;

        for (auto i = 0U; i < 4; ++i)
        {
            if (args.mods & (1U << i))
            {
                auto* extraObj = ObjectManager::get<TrackExtraObject>(trackObj->mods[i]);
                if ((compatFlags & extraObj->trackPieces) != compatFlags)
                {
                    validMods &= ~(1U << i);
                }
            }
        }

        // This is a check from RCT2 and is not used in loco TRACK_ELEM_FLAG_STARTS_AT_HALF_HEIGHT
        if (_4F8764[args.trackId] & (1U << 10))
        {
            if ((args.pos.z & 0xF) != 8)
            {
                return FAILURE;
            }
        }
        else
        {
            if (args.pos.z & 0xF)
            {
                return FAILURE;
            }
        }

        currency32_t totalCost = 0;

        {
            const auto trackBaseCost = Economy::getInflationAdjustedCost(trackObj->buildCostFactor, trackObj->costIndex, 10);
            const auto cost = (trackBaseCost * World::TrackData::getTrackCostFactor(args.trackId)) / 256;
            totalCost += cost;
        }
        for (auto i = 0U; i < 4; ++i)
        {
            if (validMods & (1U << i))
            {
                auto* extraObj = ObjectManager::get<TrackExtraObject>(trackObj->mods[i]);
                const auto trackExtraBaseCost = Economy::getInflationAdjustedCost(extraObj->buildCostFactor, extraObj->costIndex, 10);
                const auto cost = (trackExtraBaseCost * World::TrackData::getTrackCostFactor(args.trackId)) / 256;
                totalCost += cost;
            }
        }

        auto& trackPieces = World::TrackData::getTrackPiece(args.trackId);
        for (auto& piece : trackPieces)
        {
        }

        return totalCost;
    }

    void createTrack(registers& regs)
    {
        regs.ebx = createTrack(TrackPlacementArgs(regs), regs.bl);
    }
}
