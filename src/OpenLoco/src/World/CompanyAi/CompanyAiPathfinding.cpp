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
    static Interop::loco_global<World::Pos2, 0x0112C3C2> _unk1Pos112C3C2;
    static Interop::loco_global<World::SmallZ, 0x0112C515> _unk1PosBaseZ112C515;
    static Interop::loco_global<World::Pos2, 0x0112C3C6> _unk2Pos112C3C6;
    static Interop::loco_global<World::SmallZ, 0x0112C3C6> _unk2PosBaseZ112C517;
    static Interop::loco_global<World::Pos2, 0x0112C3CC> _unk3Pos112C3CC;
    static Interop::loco_global<World::SmallZ, 0x0112C59C> _unk3PosBaseZ112C59C;
    static Interop::loco_global<uint32_t, 0x0112C388> _createTrackRoadCommandMods;
    static Interop::loco_global<uint32_t, 0x0112C38C> _createTrackRoadCommandRackRail;
    static Interop::loco_global<uint32_t, 0x0112C374> _createTrackRoadCommandAiUnkFlags;
    static Interop::loco_global<uint8_t, 0x0112C59F> _createTrackRoadCommandBridge0;
    static Interop::loco_global<uint8_t, 0x0112C5A0> _createTrackRoadCommandBridge1;
    static Interop::loco_global<uint8_t, 0x0112C5A1> _createTrackRoadCommandBridge2;

    // 0x00483A7E
    sfl::static_vector<uint8_t, 64> sub_483A7E(Company& company, AiThought& thought)
    {
        // 0x0112C384
        bool allowSteepSlopes = false;

        _trackType112C519 = thought.trackObjId;
        const bool isRoad = thought.trackObjId & (1U << 7);
        uint32_t unkMods = 0U;
        uint32_t unkRackRail = 0U;
        if (isRoad)
        {
            // 0x00483B38
            allowSteepSlopes = true;

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
            allowSteepSlopes = thought.hasPurchaseFlags(AiPurchaseFlags::unk0);

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
            if (_unk2PosBaseZ112C517 == _unk3PosBaseZ112C59C)
            {
                const auto diff = *_unk2Pos112C3C6 - *_unk3Pos112C3CC;
                const auto absDiff = World::Pos2(std::abs(diff.x), std::abs(diff.y));
                if (absDiff.x <= 3 * World::kTileSize && absDiff.y <= 3 * World::kTileSize)
                {
                    _createTrackRoadCommandAiUnkFlags = *_createTrackRoadCommandAiUnkFlags & ~(1U << 22);
                }
            }
            if (_unk2PosBaseZ112C517 == _unk1PosBaseZ112C515)
            {
                const auto diff = *_unk2Pos112C3C6 - *_unk1Pos112C3C2;
                const auto absDiff = World::Pos2(std::abs(diff.x), std::abs(diff.y));
                if (absDiff.x <= 3 * World::kTileSize && absDiff.y <= 3 * World::kTileSize)
                {
                    _createTrackRoadCommandAiUnkFlags = *_createTrackRoadCommandAiUnkFlags & ~(1U << 22);
                }
            }
        }

        if (isRoad)
        {
            _createTrackRoadCommandAiUnkFlags = *_createTrackRoadCommandAiUnkFlags & ~(1U << 22);

            auto* roadObj = ObjectManager::get<RoadObject>(thought.trackObjId & ~(1U << 7));
            _createTrackRoadCommandAiUnkFlags |= (1U << 21);
            if (roadObj->hasFlags(RoadObjectFlags::unk_03))
            {
                _createTrackRoadCommandAiUnkFlags = *_createTrackRoadCommandAiUnkFlags & ~(1U << 21);
                _createTrackRoadCommandAiUnkFlags |= (1U << 20);
            }
        }

        _createTrackRoadCommandBridge0 = company.var_259A;
        _createTrackRoadCommandBridge1 = company.var_259B;
        _createTrackRoadCommandBridge2 = company.var_259C;

        if (isRoad)
        {
            // 0x00483DAA
            auto* roadObj = ObjectManager::get<RoadObject>(thought.trackObjId & ~(1U << 7));

            sfl::static_vector<uint8_t, 64> validRoadIds;
            validRoadIds.push_back(0U); // straight

            using enum World::Track::RoadTraitFlags;

            if (roadObj->hasTraitFlags(turnaround))
            {
                validRoadIds.push_back(9U); // turnaround
            }
            if (roadObj->hasTraitFlags(smallCurve))
            {
                validRoadIds.push_back(3U); // leftCurveSmall
                validRoadIds.push_back(4U); // rightCurveSmall
            }
            if (roadObj->hasTraitFlags(verySmallCurve))
            {
                validRoadIds.push_back(1U); // leftCurveVerySmall
                validRoadIds.push_back(2U); // rightCurveVerySmall
            }
            if (roadObj->hasTraitFlags(slope))
            {
                validRoadIds.push_back(5U); // straightSlopeUp
                validRoadIds.push_back(6U); // straightSlopeDown
                if (roadObj->hasTraitFlags(steepSlope) && allowSteepSlopes)
                {
                    validRoadIds.push_back(7U); // straightSteepSlopeUp
                    validRoadIds.push_back(8U); // straightSteepSlopeDown
                }
            }
        }
        else
        {
            // 0x00483CB6
            auto* trackObj = ObjectManager::get<TrackObject>(thought.trackObjId);

            sfl::static_vector<uint8_t, 64> validTrackIds;
            validTrackIds.push_back(0U); // straight

            using enum World::Track::TrackTraitFlags;
            if (trackObj->hasTraitFlags(diagonal))
            {
                validTrackIds.push_back(1U); // diagonal
            }
            if (trackObj->hasTraitFlags(sBend))
            {
                validTrackIds.push_back(12U); // sBendLeft
                validTrackIds.push_back(13U); // sBendRight
            }
            if (trackObj->hasTraitFlags(largeCurve))
            {
                validTrackIds.push_back(8U);  // leftCurveLarge
                validTrackIds.push_back(9U);  // rightCurveLarge
                validTrackIds.push_back(10U); // diagonalLeftCurveLarge
                validTrackIds.push_back(11U); // diagonalRightCurveLarge
            }
            if (trackObj->hasTraitFlags(normalCurve))
            {
                validTrackIds.push_back(6U); // leftCurve
                validTrackIds.push_back(7U); // rightCurve
            }
            if (trackObj->hasTraitFlags(smallCurve))
            {
                validTrackIds.push_back(4U); // leftCurveSmall
                validTrackIds.push_back(5U); // rightCurveSmall
                if (trackObj->hasTraitFlags(slopedCurve))
                {
                    validTrackIds.push_back(18U); // leftCurveSmallSlopeUp
                    validTrackIds.push_back(19U); // rightCurveSmallSlopeUp
                    validTrackIds.push_back(20U); // leftCurveSmallSlopeDown
                    validTrackIds.push_back(21U); // rightCurveSmallSlopeDown
                    if (trackObj->hasTraitFlags(steepSlope) && allowSteepSlopes)
                    {
                        validTrackIds.push_back(22U); // leftCurveSmallSteepSlopeUp
                        validTrackIds.push_back(23U); // rightCurveSmallSteepSlopeUp
                        validTrackIds.push_back(24U); // leftCurveSmallSteepSlopeDown
                        validTrackIds.push_back(25U); // rightCurveSmallSteepSlopeDown
                    }
                }
            }
            if (trackObj->hasTraitFlags(verySmallCurve))
            {
                validTrackIds.push_back(2U); // leftCurveVerySmall
                validTrackIds.push_back(3U); // rightCurveVerySmall
            }
            if (trackObj->hasTraitFlags(slope))
            {
                validTrackIds.push_back(14U); // straightSlopeUp
                validTrackIds.push_back(15U); // straightSlopeDown
                if (trackObj->hasTraitFlags(steepSlope) && allowSteepSlopes)
                {
                    validTrackIds.push_back(16U); // straightSteepSlopeUp
                    validTrackIds.push_back(17U); // straightSteepSlopeDown
                }
            }
        }
    }
}
