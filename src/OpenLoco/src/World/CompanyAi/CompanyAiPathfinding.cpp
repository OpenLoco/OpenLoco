#include "CompanyAiPathfinding.h"
#include "CompanyAi.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackObject.h"
#include "World/Company.h"

#include <OpenLoco/Interop/Interop.hpp>

namespace OpenLoco::CompanyAi
{
    static Interop::loco_global<uint8_t, 0x0112C519> _trackType112C519;
    static Interop::loco_global<uint32_t, 0x0112C388> _createTrackRoadCommandMods;
    static Interop::loco_global<uint32_t, 0x0112C38C> _createTrackRoadCommandRackRail;
    static Interop::loco_global<uint32_t, 0x0112C374> _createTrackRoadCommandAiUnkFlags;

    // 0x00483A7E
    void sub_483A7E(Company& company, AiThought& thought)
    {
        // 0x0112C384
        uint32_t unkFlags = 0U;

        _trackType112C519 = thought.trackObjId;
        const bool isRoad = thought.trackObjId & (1U << 7);
        uint32_t unkMods = 0U;
        uint32_t unkRackRail = 0U;
        if (isRoad)
        {
            // 0x00483B38
            unkFlags |= (1U << 1);

            auto* roadObj = ObjectManager::get<RoadObject>(thought.trackObjId & ~(1U << 7));
            for (auto i = 0U; i < std::size(roadObj->mods); ++i)
            {
                if (roadObj->mods[i] == 0xFFU)
                {
                    continue;
                }
                if (thought.mods & (1U << i))
                {
                    unkMods |= (1U << (16 + i));
                }
                if (thought.rackRailType == roadObj->mods[i])
                {
                    unkRackRail |= (1U << (16 + i));
                }
            }
        }
        else
        {
            // 0x00483A98
            if (thought.hasPurchaseFlags(AiPurchaseFlags::unk0))
            {
                unkFlags |= (1U << 1);
            }
            auto* trackObj = ObjectManager::get<TrackObject>(thought.trackObjId);
            for (auto i = 0U; i < std::size(trackObj->mods); ++i)
            {
                if (trackObj->mods[i] == 0xFFU)
                {
                    continue;
                }
                if (thought.mods & (1U << i))
                {
                    unkMods |= (1U << (16 + i));
                }
                if (thought.rackRailType == trackObj->mods[i])
                {
                    unkRackRail |= (1U << (16 + i));
                }
            }
        }
        _createTrackRoadCommandMods = unkMods;
        _createTrackRoadCommandRackRail = unkRackRail;
        _createTrackRoadCommandAiUnkFlags = 1U << 22;

        if (company.var_85C3 & (1U << 3))
        {
            // 0x00483BAF
        }
    }
}
