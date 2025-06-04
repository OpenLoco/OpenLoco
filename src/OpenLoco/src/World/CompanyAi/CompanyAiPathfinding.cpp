#include "CompanyAiPathfinding.h"
#include "CompanyAi.h"
#include "GameCommands/Road/CreateRoad.h"
#include "GameCommands/Track/CreateTrack.h"
#include "GameCommands/Track/RemoveTrack.h"
#include "Map/Tile.h"
#include "Map/TileManager.h"
#include "Map/Track/TrackData.h"
#include "Map/TrackElement.h"
#include "Map/Track/Track.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackObject.h"
#include "World/Company.h"

#include <OpenLoco/Interop/Interop.hpp>

namespace OpenLoco::CompanyAi
{
    static Interop::loco_global<uint8_t, 0x0112C518> _112C518; // company 0x85EE
    static Interop::loco_global<int32_t, 0x0112C398> _112C398; // company 0x85DE
    static Interop::loco_global<uint8_t, 0x0112C519> _trackRoadObjType112C519;
    static Interop::loco_global<World::Pos2, 0x0112C3C2> _unk1Pos112C3C2;
    static Interop::loco_global<World::SmallZ, 0x0112C515> _unk1PosBaseZ112C515;
    static Interop::loco_global<World::Pos2, 0x0112C3C6> _unk2Pos112C3C6;
    static Interop::loco_global<World::SmallZ, 0x0112C517> _unk2PosBaseZ112C517;
    static Interop::loco_global<World::Pos2, 0x0112C3CC> _unk3Pos112C3CC;
    static Interop::loco_global<World::SmallZ, 0x0112C59C> _unk3PosBaseZ112C59C;
    static Interop::loco_global<uint8_t, 0x0112C59E> _unk3Rot112C59E;
    static Interop::loco_global<uint32_t, 0x0112C388> _createTrackRoadCommandMods;
    static Interop::loco_global<uint32_t, 0x0112C38C> _createTrackRoadCommandRackRail;
    static Interop::loco_global<Company*, 0x0112C390> _unk112C390;
    static Interop::loco_global<uint32_t, 0x0112C358> _maxTrackRoadWeightingLimit; // Limits the extent of the track/road placement search
    static Interop::loco_global<uint32_t, 0x0112C374> _createTrackRoadCommandAiUnkFlags;
    static Interop::loco_global<uint32_t, 0x0112C378> _trackRoadPlacementCurrentWeighting;
    static Interop::loco_global<uint32_t, 0x0112C37C> _trackRoadPlacementBridgeWeighting;
    static Interop::loco_global<uint32_t, 0x0112C380> _numBuildingRequiredDestroyed112C380;
    static Interop::loco_global<uint8_t, 0x0112C59B> _queryTrackRoadPlacementFlags;
    static Interop::loco_global<uint8_t, 0x0112C59F> _createTrackRoadCommandBridge0;
    static Interop::loco_global<uint8_t, 0x0112C5A0> _createTrackRoadCommandBridge1;
    static Interop::loco_global<uint8_t, 0x0112C5A1> _createTrackRoadCommandBridge2;
    static Interop::loco_global<uint16_t, 0x0112C4D4> _unkTad112C4D4;
    static Interop::loco_global<uint16_t, 0x0112C3CA> _unkTad112C3CA;
    static Interop::loco_global<uint16_t, 0x0112C3D0> _queryTrackRoadPlacementMinScore;
    static Interop::loco_global<uint16_t, 0x0112C3D2> _queryTrackRoadPlacementMinWeighting;
    static Interop::loco_global<uint8_t[65], 0x0112C51A> _validTrackRoadIds;
    static Interop::loco_global<uint8_t, 0x01136073> _byte_1136073;
    static Interop::loco_global<World::MicroZ, 0x01136074> _byte_1136074;
    static Interop::loco_global<uint8_t, 0x01136075> _byte_1136075;           // bridgeType of any overlapping track
    static Interop::loco_global<uint8_t, 0x0112C2E9> _alternateTrackObjectId; // set from GameCommands::createRoad

    // 0x00483A7E
    static sfl::static_vector<uint8_t, 64> sub_483A7E(const Company& company, const AiThought& thought)
    {
        // 0x0112C384
        bool allowSteepSlopes = false;

        _trackRoadObjType112C519 = thought.trackObjId;
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
                if (thought.mods & (1U << roadObj->mods[i]))
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
                if (thought.mods & (1U << trackObj->mods[i]))
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
            return validRoadIds;
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
            return validTrackIds;
        }
    }

    // 0x00483EF2
    static bool sub_483EF2(const Company& company)
    {
        const auto& road0Size = World::TrackData::getUnkRoad(company.var_85D5);
        const auto connectPos0 = World::Pos3(company.var_85D0, company.var_85D4 * World::kSmallZStep) + road0Size.pos - World::Pos3(World::kRotationOffset[road0Size.rotationEnd], 0);
        const auto rotationConnect0 = road0Size.rotationEnd ^ (1U << 1);

        const auto& road1Size = World::TrackData::getUnkRoad(company.var_85DC);
        const auto connectPos1 = World::Pos3(company.var_85D7, company.var_85DB * World::kSmallZStep) + road1Size.pos;
        const auto rotationConnect1 = road1Size.rotationEnd;

        return connectPos0 == connectPos1 && rotationConnect0 == rotationConnect1;
    }

    // 0x00483E2D
    static bool sub_483E2D(const Company& company)
    {
        const auto& track0Size = World::TrackData::getUnkTrack(company.var_85D5);
        auto connectPos0 = World::Pos3(company.var_85D0, company.var_85D4 * World::kSmallZStep) + track0Size.pos;
        if (track0Size.rotationEnd < 12)
        {
            connectPos0 -= World::Pos3(World::kRotationOffset[track0Size.rotationEnd], 0);
        }
        const auto rotationConnect0 = track0Size.rotationEnd ^ (1U << 1);

        const auto& track1Size = World::TrackData::getUnkTrack(company.var_85DC);
        const auto connectPos1 = World::Pos3(company.var_85D7, company.var_85DB * World::kSmallZStep) + track1Size.pos;
        const auto rotationConnect1 = track1Size.rotationEnd;

        return connectPos0 == connectPos1 && rotationConnect0 == rotationConnect1;
    }

    // 0x00483E20
    static bool sub_483E20(const Company& company)
    {
        const bool isRoad = _trackRoadObjType112C519 & (1U << 7);
        if (isRoad)
        {
            return sub_483EF2(company);
        }
        else
        {
            return sub_483E2D(company);
        }
    }

    struct QueryTrackRoadPlacementResult
    {
        uint8_t flags;         // 0x0112C59B
        uint16_t minScore;     // 0x0112C3D0 if score is 0 has made to destination, if score is 0xFFFF no track placement possible, else min score after max weighting limit reached
        uint16_t minWeighting; // 0x0112C3D2 only the min weighting if score is 0 otherwise 0xFFFF
    };

    struct QueryTrackRoadPlacementState
    {
        uint32_t numBuildingsRequiredDestroyed; // 0x0112C380
        uint32_t currentWeighting;              // 0x0112C378
        uint32_t bridgeWeighting;               // 0x0112C37C
    };

    // 0x004854B2
    // pos : ax, cx, dl
    // tad : bp
    // unkFlag : ebp & (1U << 31)
    // company : _unk112C390
    //
    // return : _queryTrackRoadPlacementFlags, _queryTrackRoadPlacementMinScore, _queryTrackRoadPlacementMinWeighting
    static void queryTrackPlacementScoreRecurse(Company& company, const World::Pos3 pos, const uint16_t tad, const bool unkFlag, QueryTrackRoadPlacementResult& totalResult, QueryTrackRoadPlacementState& state)
    {
        // bl
        const auto direction = tad & 0x3;
        // dh
        const auto trackId = (tad >> 3) & 0x3F;

        const auto entry = Company::Unk25C0HashTableEntry(pos, trackId, direction);
        if (company.hashTableContains(entry))
        {
            return;
        }

        _createTrackRoadCommandAiUnkFlags = *_createTrackRoadCommandAiUnkFlags | (1U << 22);

        if (company.var_85C3 & (1U << 3))
        {
            {
                const auto diffZ = std::abs(_unk3PosBaseZ112C59C - (pos.z / World::kSmallZStep));

                if (diffZ <= 4)
                {
                    const auto diffX = std::abs(_unk3Pos112C3CC->x - pos.x);
                    const auto diffY = std::abs(_unk3Pos112C3CC->y - pos.y);
                    if (diffX <= 3 * World::kTileSize && diffY <= 3 * World::kTileSize)
                    {
                        _createTrackRoadCommandAiUnkFlags = *_createTrackRoadCommandAiUnkFlags & ~(1U << 22);
                    }
                }
            }
            {
                const auto diffZ = std::abs(_unk1PosBaseZ112C515 - (pos.z / World::kSmallZStep));

                if (diffZ <= 4)
                {
                    const auto diffX = std::abs(_unk1Pos112C3C2->x - pos.x);
                    const auto diffY = std::abs(_unk1Pos112C3C2->y - pos.y);
                    if (diffX <= 3 * World::kTileSize && diffY <= 3 * World::kTileSize)
                    {
                        _createTrackRoadCommandAiUnkFlags = *_createTrackRoadCommandAiUnkFlags & ~(1U << 22);
                    }
                }
            }
        }
        // 0x004855F9
        {
            using enum World::Track::CommonTraitFlags;
            if (!(_createTrackRoadCommandAiUnkFlags & (1U << 22))
                && (World::TrackData::getTrackMiscData(trackId).flags & (slope | steepSlope)) != none)
            {
                return;
            }
        }

        GameCommands::TrackPlacementArgs args;

        args.rotation = direction;
        if (unkFlag)
        {
            args.rotation += 12;
        }
        args.trackId = trackId;
        args.trackObjectId = _trackRoadObjType112C519;
        args.bridge = _createTrackRoadCommandBridge0;
        args.pos = pos;
        args.mods = 0;
        args.unk = false;
        args.unkFlags = *_createTrackRoadCommandAiUnkFlags >> 20;

        {
            auto regs = static_cast<Interop::registers>(args);
            regs.bl = GameCommands::Flags::aiAllocated | GameCommands::Flags::noPayment;
            GameCommands::createTrack(regs);
            if (static_cast<uint32_t>(regs.ebx) == GameCommands::FAILURE)
            {
                return;
            }
        }

        totalResult.flags |= (1U << 0);
        state.currentWeighting += World::TrackData::getTrackMiscData(trackId).unkWeighting;
        // Place track attempt required a bridge
        if (_byte_1136073 & (1U << 0))
        {
            // _byte_1136074 is the bridge height
            const auto unkFactor = (_byte_1136074 * World::TrackData::getTrackMiscData(trackId).unkWeighting) / 2;
            state.bridgeWeighting += unkFactor;
        }
        // Place track attempt requires removing a building
        if (_byte_1136073 & (1U << 4))
        {
            state.numBuildingsRequiredDestroyed++;
        }
        // 0x004856AB

        const auto& trackSize = World::TrackData::getUnkTrack(tad);
        const auto nextPos = pos + trackSize.pos;
        const auto nextRotation = trackSize.rotationEnd & 0x3U;
        const auto newUnkFlag = trackSize.rotationEnd >= 12;
        {
            const auto diffZ = std::abs(_unk3PosBaseZ112C59C - (nextPos.z / World::kSmallZStep));
            const auto diffX = std::abs(_unk3Pos112C3CC->x - nextPos.x) / 8;
            const auto diffY = std::abs(_unk3Pos112C3CC->y - nextPos.y) / 8;

            const auto squareHypot = diffX * diffX + diffY * diffY + diffZ * diffZ;
            const auto distScore = Math::Vector::fastSquareRoot(squareHypot);

            if (distScore == 0)
            {
                if (newUnkFlag)
                {
                    return;
                }
                if ((nextRotation ^ (1U << 1)) != (_unkTad112C4D4 & 0x3U))
                {
                    return;
                }
                totalResult.minScore = 0;
                if ((state.currentWeighting & 0xFFFFU) < totalResult.minWeighting)
                {
                    totalResult.minWeighting = state.currentWeighting & 0xFFFFU;
                }
                return;
            }

            if (_maxTrackRoadWeightingLimit <= state.currentWeighting)
            {
                const auto newScore = state.bridgeWeighting / 32 + distScore * 4 + state.numBuildingsRequiredDestroyed;
                totalResult.minScore = std::min<uint16_t>(newScore, totalResult.minScore);
            }
            else
            {
                for (auto* ptr = &_validTrackRoadIds[0]; *ptr != 0xFFU; ++ptr)
                {
                    const auto newTad = (*ptr << 3) | nextRotation;
                    const auto rotBegin = World::TrackData::getUnkTrack(newTad).rotationBegin;
                    if (newUnkFlag)
                    {
                        if (rotBegin < 12)
                        {
                            continue;
                        }
                    }
                    else
                    {
                        if (rotBegin >= 12)
                        {
                            continue;
                        }
                    }

                    // Make a copy of the state as each track needs to be evaluated independently
                    auto tempState = state;

                    queryTrackPlacementScoreRecurse(company, nextPos, newTad, newUnkFlag, totalResult, tempState);
                }
            }
        }
    }

    static QueryTrackRoadPlacementResult queryTrackPlacementScore(Company& company, const World::Pos3 pos, const uint16_t tad, const bool unkFlag)
    {
        QueryTrackRoadPlacementResult result{};
        result.flags = 1U << 7;
        result.minScore = 0xFFFFU;
        result.minWeighting = 0xFFFFU;

        QueryTrackRoadPlacementState state{};
        state.numBuildingsRequiredDestroyed = 0U;
        state.currentWeighting = 0U;
        state.bridgeWeighting = 0U;

        queryTrackPlacementScoreRecurse(company, pos, tad, unkFlag, result, state);

        return result;
    }

    // 0x00485849
    // pos : ax, cx, dl
    // tad : bp
    // company : _unk112C390
    //
    // return : _queryTrackRoadPlacementFlags, _queryTrackRoadPlacementMinScore, _queryTrackRoadPlacementMinWeighting
    static void queryRoadPlacementScoreRecurse(Company& company, const World::Pos3 pos, const uint16_t tad, QueryTrackRoadPlacementResult& totalResult, QueryTrackRoadPlacementState& state)
    {
        // bl
        const auto direction = tad & 0x3;
        // dh
        const auto roadId = (tad >> 3) & 0xF;

        const auto entry = Company::Unk25C0HashTableEntry(pos, roadId, direction);
        if (company.hashTableContains(entry))
        {
            return;
        }

        GameCommands::RoadPlacementArgs args;

        args.rotation = direction;
        args.roadId = roadId;
        args.roadObjectId = _trackRoadObjType112C519 & ~(1U << 7);
        args.bridge = _createTrackRoadCommandBridge0;
        args.pos = pos;
        args.mods = 0;
        args.unkFlags = *_createTrackRoadCommandAiUnkFlags >> 16;

        {
            auto regs = static_cast<Interop::registers>(args);
            regs.bl = GameCommands::Flags::aiAllocated | GameCommands::Flags::noPayment;
            GameCommands::createRoad(regs);
            if (static_cast<uint32_t>(regs.ebx) == GameCommands::FAILURE)
            {
                if ((_createTrackRoadCommandAiUnkFlags & (1U << 20)) && _alternateTrackObjectId != 0xFFU)
                {
                    args.roadObjectId = _alternateTrackObjectId;
                }
                if (_byte_1136075 != 0xFFU)
                {
                    args.bridge = _byte_1136075;
                }
                regs = static_cast<Interop::registers>(args);
                regs.bl = GameCommands::Flags::aiAllocated | GameCommands::Flags::noPayment;
                GameCommands::createRoad(regs);
                if (static_cast<uint32_t>(regs.ebx) == GameCommands::FAILURE)
                {
                    return;
                }
            }
        }

        totalResult.flags |= (1U << 0);
        auto placementWeighting = World::TrackData::getRoadMiscData(roadId).unkWeighting;

        // Place road attempt overlayed an existing road
        if (_byte_1136073 & (1U << 5))
        {
            placementWeighting -= placementWeighting / 4;
        }
        state.currentWeighting += placementWeighting;

        // Place road attempt required a bridge
        if (_byte_1136073 & (1U << 0))
        {
            // _byte_1136074 is the bridge height
            const auto unkFactor = (_byte_1136074 * placementWeighting) / 2;
            state.bridgeWeighting += unkFactor;
        }
        // Place road attempt requires removing a building
        if (_byte_1136073 & (1U << 4))
        {
            state.numBuildingsRequiredDestroyed++;
        }
        // 0x00485A01

        const auto& roadSize = World::TrackData::getUnkRoad(tad);
        const auto nextPos = pos + roadSize.pos;
        const auto nextRotation = roadSize.rotationEnd & 0x3U;
        {
            const auto diffZ = std::abs(_unk3PosBaseZ112C59C - (nextPos.z / World::kSmallZStep));
            const auto diffX = std::abs(_unk3Pos112C3CC->x - nextPos.x) / 8;
            const auto diffY = std::abs(_unk3Pos112C3CC->y - nextPos.y) / 8;

            const auto squareHypot = diffX * diffX + diffY * diffY + diffZ * diffZ;
            const auto distScore = Math::Vector::fastSquareRoot(squareHypot);

            if (distScore == 0)
            {
                if ((nextRotation ^ (1U << 1)) != (_unkTad112C4D4 & 0x3U))
                {
                    return;
                }
                totalResult.minScore = 0;
                if ((state.currentWeighting & 0xFFFFU) < totalResult.minWeighting)
                {
                    totalResult.minWeighting = state.currentWeighting & 0xFFFFU;
                }
                return;
            }

            if (_maxTrackRoadWeightingLimit <= state.currentWeighting)
            {
                const auto newScore = state.bridgeWeighting / 32 + distScore * 4 + state.numBuildingsRequiredDestroyed;
                totalResult.minScore = std::min<uint16_t>(newScore, totalResult.minScore);
            }
            else
            {
                for (auto* ptr = &_validTrackRoadIds[0]; *ptr != 0xFFU; ++ptr)
                {
                    const auto newTad = (*ptr << 3) | nextRotation;

                    // Make a copy of the state as each track needs to be evaluated independently
                    auto tempState = state;

                    queryRoadPlacementScoreRecurse(company, nextPos, newTad, totalResult, tempState);
                }
            }
        }
    }

    static QueryTrackRoadPlacementResult queryRoadPlacementScore(Company& company, const World::Pos3 pos, const uint16_t tad)
    {
        QueryTrackRoadPlacementResult result{};
        result.flags = 1U << 7;
        result.minScore = 0xFFFFU;
        result.minWeighting = 0xFFFFU;

        QueryTrackRoadPlacementState state{};
        state.numBuildingsRequiredDestroyed = 0U;
        state.currentWeighting = 0U;
        state.bridgeWeighting = 0U;

        queryRoadPlacementScoreRecurse(company, pos, tad, result, state);

        return result;
    }

    // 0x00484648
    // company : _unk112C390
    static void sub_484648(Company& company)
    {
        if (_trackRoadObjType112C519 & (1U << 7))
        {
            // 0x00484D76
            call(0x00484D76);
        }
        else
        {
            // 0x00484655
            if (_112C518 == 0)
            {
                // 0x00484662
                if (_112C398 >= static_cast<int32_t>(company.var_85EA))
                {
                    company.var_85F0 = 0xF000U;
                    return;
                }
                // 0x00484813
                _maxTrackRoadWeightingLimit = (company.var_85C3 & ((1U << 4) | (1U << 2))) ? 138 : 224;

                {
                    auto pos = World::Pos3(_unk3Pos112C3CC, _unk3PosBaseZ112C59C * World::kSmallZStep);
                    auto tad = _unkTad112C4D4 & 0x3FFU;
                    auto& trackSize = World::TrackData::getUnkTrack(tad);
                    pos += trackSize.pos;
                    auto rotation = trackSize.rotationEnd;
                    if (rotation < 12)
                    {
                        pos -= World::Pos3(World::kRotationOffset[rotation], 0);
                    }
                    rotation ^= (1U << 1);
                    _unk3Pos112C3CC = pos;
                    _unk3PosBaseZ112C59C = pos.z / World::kSmallZStep;
                    _unk3Rot112C59E = rotation;
                }

                auto pos = World::Pos3(_unk2Pos112C3C6, _unk2PosBaseZ112C517 * World::kSmallZStep);
                auto tad = _unkTad112C3CA & 0x3FFU;
                auto& trackSize = World::TrackData::getUnkTrack(tad);
                pos += trackSize.pos;
                auto rotation = trackSize.rotationEnd;
                bool diagFlag = rotation >= 12;

                rotation &= 0x3U;
                tad &= 0x3FCU;
                tad |= rotation;
                // 0x0112C55B, 0x0112C3D4, 0x00112C454
                sfl::static_vector<std::pair<uint8_t, QueryTrackRoadPlacementResult>, 64> placementResults;
                for (auto* ptr = &_validTrackRoadIds[0]; *ptr != 0xFFU; ++ptr)
                {
                    const auto rotationBegin = World::TrackData::getUnkTrack(*ptr << 3).rotationBegin;
                    if (diagFlag)
                    {
                        if (rotationBegin < 12)
                        {
                            continue;
                        }
                    }
                    else
                    {
                        if (rotationBegin >= 12)
                        {
                            continue;
                        }
                    }
                    const auto trackId = *ptr;
                    const auto newTad = (trackId << 3) | rotation;
                    placementResults.push_back(std::make_pair(trackId, queryTrackPlacementScore(company, pos, newTad, diagFlag)));
                }
                // 0x00484813
                uint16_t bestMinScore = 0xFFFFU;
                uint16_t bestMinWeighting = 0xFFFFU;
                // edi
                uint8_t bestTrackId = 0xFFU;
                for (auto& [trackId, result] : placementResults)
                {
                    if ((result.flags & ((1U << 0) | (1U << 7))) != ((1U << 0) | (1U << 7)))
                    {
                        continue;
                    }
                    if (bestMinScore < result.minScore)
                    {
                        continue;
                    }
                    else if (bestMinScore == result.minScore && bestMinWeighting <= result.minWeighting)
                    {
                        continue;
                    }
                    bestMinScore = result.minScore;
                    bestMinWeighting = result.minWeighting;
                    bestTrackId = trackId;
                }

                if (bestMinScore != 0xFFFFU)
                {
                    // 0x00484927
                    GameCommands::TrackPlacementArgs args;
                    args.trackId = bestTrackId;
                    args.pos = pos;
                    args.rotation = trackSize.rotationEnd;
                    args.trackObjectId = _trackRoadObjType112C519;
                    args.bridge = _createTrackRoadCommandBridge0;
                    if (_createTrackRoadCommandAiUnkFlags & (1U << 22))
                    {
                        args.bridge = _createTrackRoadCommandBridge1;
                        if (args.trackId != 0)
                        {
                            args.bridge = _createTrackRoadCommandBridge0;
                        }
                    }
                    args.unkFlags = *_createTrackRoadCommandAiUnkFlags >> 20;
                    args.mods = _createTrackRoadCommandMods >> 16;
                    if ((World::TrackData::getTrackMiscData(args.trackId).flags & World::Track::CommonTraitFlags::steepSlope) != World::Track::CommonTraitFlags::none)
                    {
                        args.mods |= _createTrackRoadCommandRackRail >> 16;
                    }
                    args.unk = false;
                    auto argsNoBridge = args;
                    argsNoBridge.bridge = 0xFFU; // no bridge
                    auto res = GameCommands::doCommand(argsNoBridge, GameCommands::Flags::aiAllocated | GameCommands::Flags::noPayment | GameCommands::Flags::apply);
                    if (res == GameCommands::FAILURE)
                    {
                        // Try with bridge
                        res = GameCommands::doCommand(args, GameCommands::Flags::aiAllocated | GameCommands::Flags::noPayment | GameCommands::Flags::apply);
                        if (res == GameCommands::FAILURE)
                        {
                            // 0x00484AB6
                            const auto entry = Company::Unk25C0HashTableEntry(args.pos, args.trackId, args.rotation & 0x3);
                            company.addHashTableEntry(entry);

                            _112C518 = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3);
                            return;
                        }
                    }
                    // 0x00484A05
                    if (_112C398 == 0)
                    {
                        company.var_85E6 = (bestTrackId << 3) | (args.rotation & 0x3U);
                    }

                    _unk2Pos112C3C6 = pos;
                    _unk2PosBaseZ112C517 = pos.z / World::kSmallZStep;
                    _unkTad112C3CA = (bestTrackId << 3) | (args.rotation & 0x3U);
                    _112C398 += static_cast<int32_t>(World::TrackData::getTrackMiscData(bestTrackId).unkWeighting);
                    return;
                }
                else
                {
                    // ax, cx, dl
                    auto pos2 = World::Pos3(_unk2Pos112C3C6, _unk2PosBaseZ112C517 * World::kSmallZStep);
                    // dh
                    auto trackIdStart = (_unkTad112C3CA >> 3) & 0x3F;
                    auto rot = _unkTad112C3CA & 0x3U;
                    const auto entry = Company::Unk25C0HashTableEntry(pos2, trackIdStart, rot);
                    company.addHashTableEntry(entry);
                    _112C518 = 1;
                    return;
                }
            }
            else
            {
                // 0x00484B5C
                if (_112C398 <= 0)
                {
                    _112C518 = 0;
                    return;
                }

                const auto trackId = (_unkTad112C3CA >> 3) & 0x3F;
                _112C398 -= static_cast<int32_t>(World::TrackData::getTrackMiscData(trackId).unkWeighting);
                _112C518--;

                const auto rotation = (_unkTad112C3CA & 0x3U);
                auto& trackPiece0 = World::TrackData::getTrackPiece(trackId)[0];
                const auto pos = World::Pos3(_unk2Pos112C3C6, (_unk2PosBaseZ112C517 * World::kSmallZStep) + trackPiece0.z);

                const auto hasAiAllocatedElTrack = [&pos, rotation, trackId]() {
                    auto tile = World::TileManager::get(pos);
                    for (auto& el : tile)
                    {
                        auto* elTrack = el.as<World::TrackElement>();
                        if (elTrack == nullptr)
                        {
                            continue;
                        }
                        if (elTrack->baseZ() != pos.z / World::kSmallZStep)
                        {
                            continue;
                        }
                        if (elTrack->rotation() != rotation)
                        {
                            continue;
                        }
                        if (!elTrack->isAiAllocated())
                        {
                            continue;
                        }
                        if (elTrack->hasStationElement())
                        {
                            continue;
                        }
                        if (elTrack->trackId() != trackId)
                        {
                            continue;
                        }
                        if (elTrack->sequenceIndex() != 0)
                        {
                            continue;
                        }
                        if (elTrack->trackObjectId() != _trackRoadObjType112C519)
                        {
                            continue;
                        }
                        return true;
                    }
                    return false;
                }();

                if (!hasAiAllocatedElTrack)
                {
                    company.var_85F0 = 0xF000U;
                    return;
                }

                GameCommands::TrackRemovalArgs args;
                args.pos = pos;
                args.rotation = rotation;
                args.trackId = trackId;
                args.index = 0U;
                args.trackObjectId = _trackRoadObjType112C519;
                GameCommands::doCommand(args, GameCommands::Flags::aiAllocated | GameCommands::Flags::noPayment | GameCommands::Flags::apply);
                // No check of failure!

                auto nextPos = World::Pos3(_unk2Pos112C3C6, _unk2PosBaseZ112C517 * World::kSmallZStep);
                const auto rot = World::TrackData::getUnkTrack(_unkTad112C3CA & 0x3FFU).rotationBegin;
                if (rot < 12)
                {
                    nextPos -= World::Pos3(World::kRotationOffset[rot], 0);
                }
                const auto nextRot = World::kReverseRotation[rot];
                const auto tc = World::Track::getTrackConnectionsAi(nextPos, nextRot, company.id(), _trackRoadObjType112C519, 0, 0);

                if (tc.connections.empty())
                {
                    company.var_85F0 = 0xF000U;
                    return;
                }

                {
                    auto newTad = tc.connections[0] & 0x1FFU;
                    auto newPos = nextPos + World::TrackData::getUnkTrack(newTad).pos;
                    const auto rotEnd = World::TrackData::getUnkTrack(newTad).rotationEnd;
                    if (rotEnd < 12)
                    {
                        newPos -= World::Pos3(World::kRotationOffset[rotEnd], 0);
                    }

                    // Odd??
                    newTad ^= (1U << 2);
                    if (newTad & (1U << 2))
                    {
                        newTad &= 0x3;
                        newTad ^= (1U << 2);
                    }

                    _unkTad112C3CA = newTad;
                    _unk2Pos112C3C6 = newPos;
                    _unk2PosBaseZ112C517 = newPos.z / World::kSmallZStep;
                    return;
                }
            }
        }
    }

    void registerHooks()
    {
        Interop::registerHook(
            0x00483A7E,
            [](Interop::registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                Interop::registers backup = regs;

                auto& company = *Interop::X86Pointer<Company>(regs.esi);
                auto& thought = *Interop::X86Pointer<AiThought>(regs.edi);
                const auto validTrackRoadIds = sub_483A7E(company, thought);
                auto* ptr = &_validTrackRoadIds[0];
                for (auto id : validTrackRoadIds)
                {
                    *ptr++ = id;
                }
                *ptr = 0xFFU;

                regs = backup;
                return 0;
            });

        Interop::registerHook(
            0x00483E20,
            [](Interop::registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                Interop::registers backup = regs;

                auto& company = *Interop::X86Pointer<Company>(regs.esi);
                const auto hasConnection = sub_483E20(company);

                regs = backup;
                return hasConnection ? Interop::X86_FLAG_CARRY : 0;
            });

        // 0x004854B2
        Interop::registerHook(
            0x004854B2,
            [](Interop::registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                Interop::registers backup = regs;

                const auto pos = World::Pos3(regs.ax, regs.cx, static_cast<uint8_t>(regs.dl) * World::kSmallZStep);
                const auto tad = regs.bp & 0x3FFU;
                const auto unkFlag = (regs.ebp & (1U << 31)) != 0;
                auto& company = **_unk112C390;

                const auto res = queryTrackPlacementScore(company, pos, tad, unkFlag);
                _queryTrackRoadPlacementFlags = res.flags;
                _queryTrackRoadPlacementMinScore = res.minScore;
                _queryTrackRoadPlacementMinWeighting = res.minWeighting;

                regs = backup;
                return 0;
            });

        // 0x00485849
        Interop::registerHook(
            0x00485849,
            [](Interop::registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                Interop::registers backup = regs;

                const auto pos = World::Pos3(regs.ax, regs.cx, static_cast<uint8_t>(regs.dl) * World::kSmallZStep);
                const auto tad = regs.bp & 0x3FFU;
                auto& company = **_unk112C390;

                const auto res = queryRoadPlacementScore(company, pos, tad);
                _queryTrackRoadPlacementFlags = res.flags;
                _queryTrackRoadPlacementMinScore = res.minScore;
                _queryTrackRoadPlacementMinWeighting = res.minWeighting;

                regs = backup;
                return 0;
            });

        // 0x00484648
        Interop::registerHook(
            0x00484648,
            [](Interop::registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                Interop::registers backup = regs;

                auto& company = **_unk112C390;

                sub_484648(company);

                regs = backup;
                return 0;
            });
    }
}
