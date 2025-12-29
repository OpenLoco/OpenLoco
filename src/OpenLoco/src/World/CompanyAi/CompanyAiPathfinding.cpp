#include "CompanyAiPathfinding.h"
#include "CompanyAi.h"
#include "Economy/Economy.h"
#include "GameCommands/CompanyAi/AiTrackReplacement.h"
#include "GameCommands/Road/CreateRoad.h"
#include "GameCommands/Road/RemoveRoad.h"
#include "GameCommands/Track/CreateTrack.h"
#include "GameCommands/Track/RemoveTrack.h"
#include "GameState.h"
#include "Map/BuildingElement.h"
#include "Map/RoadElement.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/Tile.h"
#include "Map/TileClearance.h"
#include "Map/TileManager.h"
#include "Map/Track/Track.h"
#include "Map/Track/TrackData.h"
#include "Map/TrackElement.h"
#include "Map/TreeElement.h"
#include "Objects/BridgeObject.h"
#include "Objects/BuildingObject.h"
#include "Objects/LevelCrossingObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadExtraObject.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackObject.h"
#include "Objects/TreeObject.h"
#include "World/Company.h"
#include "World/Station.h"

namespace OpenLoco::CompanyAi
{
    using ValidTrackRoadIds = sfl::static_vector<uint8_t, 64>;

    // TODO: Integrate these globals into the functions
    static uint8_t _pathFindUndoCount112C518 = {};          // 0x0112C518 company 0x85EE
    static int32_t _pathFindTotalTrackRoadWeighting = {};   // 0x0112C398 company 0x85DE
    static World::Pos2 _unk1Pos112C3C2 = {};                // 0x0112C3C2
    static World::SmallZ _unk1PosBaseZ112C515 = {};         // 0x0112C515
    static uint8_t _unk1Rot112C516 = {};                    // 0x0112C516
    static World::Pos2 _unk2Pos112C3C6 = {};                // 0x0112C3C6
    static World::SmallZ _unk2PosBaseZ112C517 = {};         // 0x0112C517
    static World::Pos2 _unk3Pos112C3CC = {};                // 0x0112C3CC
    static World::SmallZ _unk3PosBaseZ112C59C = {};         // 0x0112C59C
    static uint32_t _maxTrackRoadWeightingLimit = {};       // 0x0112C358 Limits the extent of the track/road placement search
    static uint32_t _createTrackRoadCommandAiUnkFlags = {}; // 0x0112C374
    static uint16_t _unkTad112C4D4 = {};                    // 0x0112C4D4
    static uint16_t _unkTad112C3CA = {};                    // 0x0112C3CA

    struct PlacementVars
    {
        uint8_t trackRoadObjId;             // 0x0112C519 1 << 7 if road
        uint32_t mods;                      // 0x0112C388 << 16
        uint32_t rackRailType;              // 0x0112C38C << 16
        std::array<uint8_t, 3> bridgeTypes; // 0x0112C59F, 0x0112C5A0, 0x0112C5A1
        ValidTrackRoadIds validIds;         // valid track/road ids
    };

    // 0x00483A7E
    static PlacementVars sub_483A7E(const Company& company, const AiThought& thought)
    {
        // 0x0112C384
        bool allowSteepSlopes = false;

        PlacementVars result{};
        result.trackRoadObjId = thought.trackObjId;
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
        result.mods = unkMods;
        result.rackRailType = unkRackRail;
        _createTrackRoadCommandAiUnkFlags = 1U << 22;

        if (company.var_85C3 & (1U << 3))
        {
            // 0x00483BAF
            if (_unk2PosBaseZ112C517 == _unk3PosBaseZ112C59C)
            {
                const auto diff = _unk2Pos112C3C6 - _unk3Pos112C3CC;
                const auto absDiff = World::Pos2(std::abs(diff.x), std::abs(diff.y));
                if (absDiff.x <= 3 * World::kTileSize && absDiff.y <= 3 * World::kTileSize)
                {
                    _createTrackRoadCommandAiUnkFlags = _createTrackRoadCommandAiUnkFlags & ~(1U << 22);
                }
            }
            if (_unk2PosBaseZ112C517 == _unk1PosBaseZ112C515)
            {
                const auto diff = _unk2Pos112C3C6 - _unk1Pos112C3C2;
                const auto absDiff = World::Pos2(std::abs(diff.x), std::abs(diff.y));
                if (absDiff.x <= 3 * World::kTileSize && absDiff.y <= 3 * World::kTileSize)
                {
                    _createTrackRoadCommandAiUnkFlags = _createTrackRoadCommandAiUnkFlags & ~(1U << 22);
                }
            }
        }

        if (isRoad)
        {
            _createTrackRoadCommandAiUnkFlags = _createTrackRoadCommandAiUnkFlags & ~(1U << 22);

            auto* roadObj = ObjectManager::get<RoadObject>(thought.trackObjId & ~(1U << 7));
            _createTrackRoadCommandAiUnkFlags |= (1U << 21);
            if (roadObj->hasFlags(RoadObjectFlags::unk_03))
            {
                _createTrackRoadCommandAiUnkFlags = _createTrackRoadCommandAiUnkFlags & ~(1U << 21);
                _createTrackRoadCommandAiUnkFlags |= (1U << 20);
            }
        }

        result.bridgeTypes[0] = company.var_259A;
        result.bridgeTypes[1] = company.var_259B;
        result.bridgeTypes[2] = company.var_259C;

        if (isRoad)
        {
            // 0x00483DAA
            auto* roadObj = ObjectManager::get<RoadObject>(thought.trackObjId & ~(1U << 7));

            result.validIds.push_back(0U); // straight

            using enum World::Track::RoadTraitFlags;

            if (roadObj->hasTraitFlags(turnaround))
            {
                result.validIds.push_back(9U); // turnaround
            }
            if (roadObj->hasTraitFlags(smallCurve))
            {
                result.validIds.push_back(3U); // leftCurveSmall
                result.validIds.push_back(4U); // rightCurveSmall
            }
            if (roadObj->hasTraitFlags(verySmallCurve))
            {
                result.validIds.push_back(1U); // leftCurveVerySmall
                result.validIds.push_back(2U); // rightCurveVerySmall
            }
            if (roadObj->hasTraitFlags(slope))
            {
                result.validIds.push_back(5U); // straightSlopeUp
                result.validIds.push_back(6U); // straightSlopeDown
                if (roadObj->hasTraitFlags(steepSlope) && allowSteepSlopes)
                {
                    result.validIds.push_back(7U); // straightSteepSlopeUp
                    result.validIds.push_back(8U); // straightSteepSlopeDown
                }
            }
            return result;
        }
        else
        {
            // 0x00483CB6
            auto* trackObj = ObjectManager::get<TrackObject>(thought.trackObjId);

            result.validIds.push_back(0U); // straight

            using enum World::Track::TrackTraitFlags;
            if (trackObj->hasTraitFlags(diagonal))
            {
                result.validIds.push_back(1U); // diagonal
            }
            if (trackObj->hasTraitFlags(sBend))
            {
                result.validIds.push_back(12U); // sBendLeft
                result.validIds.push_back(13U); // sBendRight
            }
            if (trackObj->hasTraitFlags(largeCurve))
            {
                result.validIds.push_back(8U);  // leftCurveLarge
                result.validIds.push_back(9U);  // rightCurveLarge
                result.validIds.push_back(10U); // diagonalLeftCurveLarge
                result.validIds.push_back(11U); // diagonalRightCurveLarge
            }
            if (trackObj->hasTraitFlags(normalCurve))
            {
                result.validIds.push_back(6U); // leftCurve
                result.validIds.push_back(7U); // rightCurve
            }
            if (trackObj->hasTraitFlags(smallCurve))
            {
                result.validIds.push_back(4U); // leftCurveSmall
                result.validIds.push_back(5U); // rightCurveSmall
                if (trackObj->hasTraitFlags(slopedCurve))
                {
                    result.validIds.push_back(18U); // leftCurveSmallSlopeUp
                    result.validIds.push_back(19U); // rightCurveSmallSlopeUp
                    result.validIds.push_back(20U); // leftCurveSmallSlopeDown
                    result.validIds.push_back(21U); // rightCurveSmallSlopeDown
                    if (trackObj->hasTraitFlags(steepSlope) && allowSteepSlopes)
                    {
                        result.validIds.push_back(22U); // leftCurveSmallSteepSlopeUp
                        result.validIds.push_back(23U); // rightCurveSmallSteepSlopeUp
                        result.validIds.push_back(24U); // leftCurveSmallSteepSlopeDown
                        result.validIds.push_back(25U); // rightCurveSmallSteepSlopeDown
                    }
                }
            }
            if (trackObj->hasTraitFlags(verySmallCurve))
            {
                result.validIds.push_back(2U); // leftCurveVerySmall
                result.validIds.push_back(3U); // rightCurveVerySmall
            }
            if (trackObj->hasTraitFlags(slope))
            {
                result.validIds.push_back(14U); // straightSlopeUp
                result.validIds.push_back(15U); // straightSlopeDown
                if (trackObj->hasTraitFlags(steepSlope) && allowSteepSlopes)
                {
                    result.validIds.push_back(16U); // straightSteepSlopeUp
                    result.validIds.push_back(17U); // straightSteepSlopeDown
                }
            }
            return result;
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
    static bool sub_483E20(const Company& company, const bool isRoad)
    {
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
    // return via ref argument totalResult
    static void queryTrackPlacementScoreRecurse(Company& company, const World::Pos3 pos, const uint16_t tad, const bool unkFlag, const PlacementVars& placementVars, QueryTrackRoadPlacementResult& totalResult, QueryTrackRoadPlacementState& state)
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

        _createTrackRoadCommandAiUnkFlags = _createTrackRoadCommandAiUnkFlags | (1U << 22);

        if (company.var_85C3 & (1U << 3))
        {
            {
                const auto diffZ = std::abs(_unk3PosBaseZ112C59C - (pos.z / World::kSmallZStep));

                if (diffZ <= 4)
                {
                    const auto diffX = std::abs(_unk3Pos112C3CC.x - pos.x);
                    const auto diffY = std::abs(_unk3Pos112C3CC.y - pos.y);
                    if (diffX <= 3 * World::kTileSize && diffY <= 3 * World::kTileSize)
                    {
                        _createTrackRoadCommandAiUnkFlags = _createTrackRoadCommandAiUnkFlags & ~(1U << 22);
                    }
                }
            }
            {
                const auto diffZ = std::abs(_unk1PosBaseZ112C515 - (pos.z / World::kSmallZStep));

                if (diffZ <= 4)
                {
                    const auto diffX = std::abs(_unk1Pos112C3C2.x - pos.x);
                    const auto diffY = std::abs(_unk1Pos112C3C2.y - pos.y);
                    if (diffX <= 3 * World::kTileSize && diffY <= 3 * World::kTileSize)
                    {
                        _createTrackRoadCommandAiUnkFlags = _createTrackRoadCommandAiUnkFlags & ~(1U << 22);
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
        args.trackObjectId = placementVars.trackRoadObjId;
        args.bridge = placementVars.bridgeTypes[0];
        args.pos = pos;
        args.mods = 0;
        args.unk = false;
        args.unkFlags = _createTrackRoadCommandAiUnkFlags >> 20;

        {
            auto regs = static_cast<GameCommands::registers>(args);
            regs.bl = GameCommands::Flags::aiAllocated | GameCommands::Flags::noPayment;
            GameCommands::createTrack(regs);
            if (static_cast<uint32_t>(regs.ebx) == GameCommands::FAILURE)
            {
                return;
            }
        }

        totalResult.flags |= (1U << 0);
        state.currentWeighting += World::TrackData::getTrackMiscData(trackId).unkWeighting;

        auto& returnState = GameCommands::getLegacyReturnState();

        // Place track attempt required a bridge
        if (returnState.flags_1136073 & (1U << 0))
        {
            // returnState.byte_1136074 is the bridge height
            const auto unkFactor = (returnState.byte_1136074 * World::TrackData::getTrackMiscData(trackId).unkWeighting) / 2;
            state.bridgeWeighting += unkFactor;
        }
        // Place track attempt requires removing a building
        if (returnState.flags_1136073 & (1U << 4))
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
            const auto diffX = std::abs(_unk3Pos112C3CC.x - nextPos.x) / 8;
            const auto diffY = std::abs(_unk3Pos112C3CC.y - nextPos.y) / 8;

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
                for (auto newTrackId : placementVars.validIds)
                {
                    const auto newTad = (newTrackId << 3) | nextRotation;
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

                    queryTrackPlacementScoreRecurse(company, nextPos, newTad, newUnkFlag, placementVars, totalResult, tempState);
                }
            }
        }
    }

    static QueryTrackRoadPlacementResult queryTrackPlacementScore(Company& company, const World::Pos3 pos, const uint16_t tad, const bool unkFlag, const PlacementVars& placementVars)
    {
        QueryTrackRoadPlacementResult result{};
        result.flags = 1U << 7;
        result.minScore = 0xFFFFU;
        result.minWeighting = 0xFFFFU;

        QueryTrackRoadPlacementState state{};
        state.numBuildingsRequiredDestroyed = 0U;
        state.currentWeighting = 0U;
        state.bridgeWeighting = 0U;

        queryTrackPlacementScoreRecurse(company, pos, tad, unkFlag, placementVars, result, state);

        return result;
    }

    // 0x00485849
    // pos : ax, cx, dl
    // tad : bp
    // company : _unk112C390
    //
    // return via ref argument totalResult
    static void queryRoadPlacementScoreRecurse(Company& company, const World::Pos3 pos, const uint16_t tad, const PlacementVars& placementVars, QueryTrackRoadPlacementResult& totalResult, QueryTrackRoadPlacementState& state)
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
        args.roadObjectId = placementVars.trackRoadObjId & ~(1U << 7);
        args.bridge = placementVars.bridgeTypes[0];
        args.pos = pos;
        args.mods = 0;
        args.unkFlags = _createTrackRoadCommandAiUnkFlags >> 16;

        auto& returnState = GameCommands::getLegacyReturnState();

        {
            auto regs = static_cast<GameCommands::registers>(args);
            regs.bl = GameCommands::Flags::aiAllocated | GameCommands::Flags::noPayment;
            GameCommands::createRoad(regs);
            if (static_cast<uint32_t>(regs.ebx) == GameCommands::FAILURE)
            {
                if ((_createTrackRoadCommandAiUnkFlags & (1U << 20)) && returnState.alternateRoadObjectId != 0xFFU)
                {
                    args.roadObjectId = returnState.alternateRoadObjectId;
                }
                if (returnState.byte_1136075 != 0xFFU)
                {
                    args.bridge = returnState.byte_1136075;
                }
                regs = static_cast<GameCommands::registers>(args);
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
        if (returnState.flags_1136073 & (1U << 5))
        {
            placementWeighting -= placementWeighting / 4;
        }
        state.currentWeighting += placementWeighting;

        // Place road attempt required a bridge
        if (returnState.flags_1136073 & (1U << 0))
        {
            // returnState.byte_1136074 is the bridge height
            const auto unkFactor = (returnState.byte_1136074 * placementWeighting) / 2;
            state.bridgeWeighting += unkFactor;
        }
        // Place road attempt requires removing a building
        if (returnState.flags_1136073 & (1U << 4))
        {
            state.numBuildingsRequiredDestroyed++;
        }
        // 0x00485A01

        const auto& roadSize = World::TrackData::getUnkRoad(tad);
        const auto nextPos = pos + roadSize.pos;
        const auto nextRotation = roadSize.rotationEnd & 0x3U;
        {
            const auto diffZ = std::abs(_unk3PosBaseZ112C59C - (nextPos.z / World::kSmallZStep));
            const auto diffX = std::abs(_unk3Pos112C3CC.x - nextPos.x) / 8;
            const auto diffY = std::abs(_unk3Pos112C3CC.y - nextPos.y) / 8;

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
                for (const auto newRoadId : placementVars.validIds)
                {
                    const auto newTad = (newRoadId << 3) | nextRotation;

                    // Make a copy of the state as each track needs to be evaluated independently
                    auto tempState = state;

                    queryRoadPlacementScoreRecurse(company, nextPos, newTad, placementVars, totalResult, tempState);
                }
            }
        }
    }

    static QueryTrackRoadPlacementResult queryRoadPlacementScore(Company& company, const World::Pos3 pos, const uint16_t tad, const PlacementVars& placementVars)
    {
        QueryTrackRoadPlacementResult result{};
        result.flags = 1U << 7;
        result.minScore = 0xFFFFU;
        result.minWeighting = 0xFFFFU;

        QueryTrackRoadPlacementState state{};
        state.numBuildingsRequiredDestroyed = 0U;
        state.currentWeighting = 0U;
        state.bridgeWeighting = 0U;

        queryRoadPlacementScoreRecurse(company, pos, tad, placementVars, result, state);

        return result;
    }

    // 0x00484B5C
    static void pathFindTrackUndoSection(Company& company, const uint8_t trackObjId)
    {
        if (_pathFindTotalTrackRoadWeighting <= 0)
        {
            _pathFindUndoCount112C518 = 0;
            return;
        }

        const auto trackId = (_unkTad112C3CA >> 3) & 0x3F;
        _pathFindTotalTrackRoadWeighting -= static_cast<int32_t>(World::TrackData::getTrackMiscData(trackId).unkWeighting);
        _pathFindUndoCount112C518--;

        const auto rotation = (_unkTad112C3CA & 0x3U);
        auto& trackPiece0 = World::TrackData::getTrackPiece(trackId)[0];
        const auto pos = World::Pos3(_unk2Pos112C3C6, (_unk2PosBaseZ112C517 * World::kSmallZStep) + trackPiece0.z);

        const auto hasAiAllocatedElTrack = [&pos, rotation, trackId, trackObjId]() {
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
                if (elTrack->trackObjectId() != trackObjId)
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
        args.trackObjectId = trackObjId;
        GameCommands::doCommand(args, GameCommands::Flags::aiAllocated | GameCommands::Flags::noPayment | GameCommands::Flags::apply);
        // No check of failure!

        auto nextPos = World::Pos3(_unk2Pos112C3C6, _unk2PosBaseZ112C517 * World::kSmallZStep);
        const auto rot = World::TrackData::getUnkTrack(_unkTad112C3CA & 0x3FFU).rotationBegin;
        if (rot < 12)
        {
            nextPos -= World::Pos3(World::kRotationOffset[rot], 0);
        }
        const auto nextRot = World::kReverseRotation[rot];
        const auto tc = World::Track::getTrackConnectionsAi(nextPos, nextRot, company.id(), trackObjId, 0, 0);

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

            // Normalise the rotation (remove the reverse bit and reverse the rotation if required)
            newTad ^= (1U << 2);
            if (newTad & (1U << 2))
            {
                newTad &= 0x3;
                newTad ^= (1U << 1);
            }

            _unkTad112C3CA = newTad;
            _unk2Pos112C3C6 = newPos;
            _unk2PosBaseZ112C517 = newPos.z / World::kSmallZStep;
        }
    }

    // 0x00484655
    static void pathFindTrackSection(Company& company, const PlacementVars& placementVars)
    {
        if (_pathFindUndoCount112C518 == 0)
        {
            // 0x00484662
            if (_pathFindTotalTrackRoadWeighting >= static_cast<int32_t>(company.var_85EA))
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
                const auto rotation = trackSize.rotationEnd;
                if (rotation < 12)
                {
                    pos -= World::Pos3(World::kRotationOffset[rotation], 0);
                }
                _unk3Pos112C3CC = pos;
                _unk3PosBaseZ112C59C = pos.z / World::kSmallZStep;
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
            for (const auto trackId : placementVars.validIds)
            {
                const auto rotationBegin = World::TrackData::getUnkTrack(trackId << 3).rotationBegin;
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
                const auto newTad = (trackId << 3) | rotation;
                placementResults.push_back(std::make_pair(trackId, queryTrackPlacementScore(company, pos, newTad, diagFlag, placementVars)));
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
                args.trackObjectId = placementVars.trackRoadObjId;
                args.bridge = placementVars.bridgeTypes[0];
                if (_createTrackRoadCommandAiUnkFlags & (1U << 22))
                {
                    args.bridge = placementVars.bridgeTypes[1];
                    if (args.trackId != 0)
                    {
                        args.bridge = placementVars.bridgeTypes[0];
                    }
                }
                args.unkFlags = _createTrackRoadCommandAiUnkFlags >> 20;
                args.mods = placementVars.mods >> 16;
                if ((World::TrackData::getTrackMiscData(args.trackId).flags & World::Track::CommonTraitFlags::steepSlope) != World::Track::CommonTraitFlags::none)
                {
                    args.mods |= placementVars.rackRailType >> 16;
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

                        _pathFindUndoCount112C518 = 15;
                        return;
                    }
                }
                // 0x00484A05
                if (_pathFindTotalTrackRoadWeighting == 0)
                {
                    company.var_85E6 = (bestTrackId << 3) | (args.rotation & 0x3U);
                }

                _unk2Pos112C3C6 = pos;
                _unk2PosBaseZ112C517 = pos.z / World::kSmallZStep;
                _unkTad112C3CA = (bestTrackId << 3) | (args.rotation & 0x3U);
                _pathFindTotalTrackRoadWeighting += static_cast<int32_t>(World::TrackData::getTrackMiscData(bestTrackId).unkWeighting);
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
                _pathFindUndoCount112C518 = 1;
                return;
            }
        }
        else
        {
            pathFindTrackUndoSection(company, placementVars.trackRoadObjId);
        }
    }

    // 0x00485283
    static void pathFindRoadUndoSection(Company& company, const uint8_t roadObjId)
    {
        if (_pathFindTotalTrackRoadWeighting <= 0)
        {
            _pathFindUndoCount112C518 = 0;
            return;
        }

        const auto roadId = (_unkTad112C3CA >> 3) & 0xF;
        _pathFindTotalTrackRoadWeighting -= static_cast<int32_t>(World::TrackData::getRoadMiscData(roadId).unkWeighting);
        _pathFindUndoCount112C518--;

        const auto rotation = (_unkTad112C3CA & 0x3U);
        auto& roadPiece0 = World::TrackData::getRoadPiece(roadId)[0];
        const auto pos = World::Pos3(_unk2Pos112C3C6, (_unk2PosBaseZ112C517 * World::kSmallZStep) + roadPiece0.z);

        const auto aiAllocatedElRoad = [&pos, rotation, roadId, companyId = company.id()]() -> World::RoadElement* {
            auto tile = World::TileManager::get(pos);
            for (auto& el : tile)
            {
                auto* elRoad = el.as<World::RoadElement>();
                if (elRoad == nullptr)
                {
                    continue;
                }
                if (elRoad->baseZ() != pos.z / World::kSmallZStep)
                {
                    continue;
                }
                if (elRoad->rotation() != rotation)
                {
                    continue;
                }
                if (!elRoad->isAiAllocated())
                {
                    continue;
                }
                if (elRoad->hasStationElement())
                {
                    continue;
                }
                if (elRoad->roadId() != roadId)
                {
                    continue;
                }
                if (elRoad->sequenceIndex() != 0)
                {
                    continue;
                }
                if (elRoad->owner() != companyId)
                {
                    continue;
                }

                return elRoad;
            }
            return nullptr;
        }();

        if (aiAllocatedElRoad == nullptr)
        {
            company.var_85F0 = 0xF000U;
            return;
        }

        GameCommands::RoadRemovalArgs args;
        args.pos = pos;
        args.rotation = rotation;
        args.roadId = roadId;
        args.sequenceIndex = 0U;
        args.objectId = aiAllocatedElRoad->roadObjectId();
        GameCommands::doCommand(args, GameCommands::Flags::aiAllocated | GameCommands::Flags::noPayment | GameCommands::Flags::apply);
        // No check of failure!

        auto nextPos = World::Pos3(_unk2Pos112C3C6, _unk2PosBaseZ112C517 * World::kSmallZStep);
        const auto rot = World::TrackData::getUnkRoad(_unkTad112C3CA & 0x3FFU).rotationBegin;
        nextPos -= World::Pos3(World::kRotationOffset[rot], 0);

        const auto nextRot = World::kReverseRotation[rot];
        auto adjustedRoadObjId = roadObjId;
        auto* roadObj = ObjectManager::get<RoadObject>(roadObjId);
        if (roadObj->hasFlags(RoadObjectFlags::unk_03))
        {
            adjustedRoadObjId = 0xFFU;
        }
        const auto rc = World::Track::getRoadConnectionsAiAllocated(nextPos, nextRot, company.id(), adjustedRoadObjId, 0, 0);

        {
            auto newTad = rc.connections.empty() ? (nextRot | (0U << 3)) : rc.connections[0] & 0x1FFU;
            auto newPos = nextPos + World::TrackData::getUnkRoad(newTad).pos;
            const auto rotEnd = World::TrackData::getUnkRoad(newTad).rotationEnd;
            newPos -= World::Pos3(World::kRotationOffset[rotEnd], 0);

            // Normalise the rotation (remove the reverse bit and reverse the rotation if required)
            newTad ^= (1U << 2);
            if (newTad & (1U << 2))
            {
                newTad &= 0x3;
                newTad ^= (1U << 1);
            }

            _unkTad112C3CA = newTad;
            _unk2Pos112C3C6 = newPos;
            _unk2PosBaseZ112C517 = newPos.z / World::kSmallZStep;
        }
    }

    // 0x00484D76
    static void pathFindRoadSection(Company& company, const PlacementVars& placementVars)
    {
        const auto roadObjId = placementVars.trackRoadObjId & ~(1U << 7);
        if (_pathFindUndoCount112C518 == 0)
        {
            // 0x00484D83
            if (_pathFindTotalTrackRoadWeighting >= static_cast<int32_t>(company.var_85EA))
            {
                company.var_85F0 = 0xF000U;
                return;
            }
            // 0x00484D9A
            _maxTrackRoadWeightingLimit = 138;

            {
                auto pos = World::Pos3(_unk3Pos112C3CC, _unk3PosBaseZ112C59C * World::kSmallZStep);
                auto tad = _unkTad112C4D4 & 0x3FFU;
                const auto& roadSize = World::TrackData::getUnkRoad(tad);
                pos += roadSize.pos;
                const auto rotation = roadSize.rotationEnd;
                pos -= World::Pos3(World::kRotationOffset[rotation], 0);

                _unk3Pos112C3CC = pos;
                _unk3PosBaseZ112C59C = pos.z / World::kSmallZStep;
            }

            auto pos = World::Pos3(_unk2Pos112C3C6, _unk2PosBaseZ112C517 * World::kSmallZStep);
            auto tad = _unkTad112C3CA & 0x3FFU;
            auto& roadSize = World::TrackData::getUnkRoad(tad);
            pos += roadSize.pos;
            auto rotation = roadSize.rotationEnd & 0x3U;

            tad &= 0x3FCU;
            tad |= rotation;

            // 0x0112C55B, 0x0112C3D4, 0x00112C454
            sfl::static_vector<std::pair<uint8_t, QueryTrackRoadPlacementResult>, 64> placementResults;
            for (const auto roadId : placementVars.validIds)
            {
                const auto newTad = (roadId << 3) | rotation;
                placementResults.push_back(std::make_pair(roadId, queryRoadPlacementScore(company, pos, newTad, placementVars)));
            }
            // 0x00484EF0
            uint16_t bestMinScore = 0xFFFFU;
            uint16_t bestMinWeighting = 0xFFFFU;
            // edi
            uint8_t bestRoadId = 0xFFU;
            for (auto& [roadId, result] : placementResults)
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
                bestRoadId = roadId;
            }

            if (bestMinScore != 0xFFFFU)
            {
                // 0x00485004
                GameCommands::RoadPlacementArgs args;
                args.roadId = bestRoadId;
                args.pos = pos;
                args.rotation = roadSize.rotationEnd;
                args.roadObjectId = roadObjId;
                args.bridge = placementVars.bridgeTypes[1];

                if (args.roadId != 0)
                {
                    args.bridge = placementVars.bridgeTypes[0];
                }
                args.unkFlags = _createTrackRoadCommandAiUnkFlags >> 16;
                args.mods = placementVars.mods >> 16;
                if ((World::TrackData::getRoadMiscData(args.roadId).flags & World::Track::CommonTraitFlags::steepSlope) != World::Track::CommonTraitFlags::none)
                {
                    args.mods |= placementVars.rackRailType >> 16;
                }
                auto argsNoBridge = args;
                argsNoBridge.bridge = 0xFFU; // no bridge
                auto res = GameCommands::doCommand(argsNoBridge, GameCommands::Flags::aiAllocated | GameCommands::Flags::noPayment | GameCommands::Flags::apply);
                if (res == GameCommands::FAILURE)
                {
                    // Try with bridge
                    res = GameCommands::doCommand(args, GameCommands::Flags::aiAllocated | GameCommands::Flags::noPayment | GameCommands::Flags::apply);
                    if (res == GameCommands::FAILURE)
                    {
                        auto argsUnk = args;
                        argsUnk.bridge = 0xFFU;

                        auto& returnState = GameCommands::getLegacyReturnState();
                        if (returnState.byte_1136075 != 0xFFU)
                        {
                            argsUnk.bridge = returnState.byte_1136075;
                        }
                        if (_createTrackRoadCommandAiUnkFlags & (1U << 20) && returnState.alternateRoadObjectId != 0xFFU)
                        {
                            argsUnk.roadObjectId = returnState.alternateRoadObjectId;
                        }
                        res = GameCommands::doCommand(argsUnk, GameCommands::Flags::aiAllocated | GameCommands::Flags::noPayment | GameCommands::Flags::apply);
                        if (res == GameCommands::FAILURE)
                        {
                            // 0x004851DD
                            const auto entry = Company::Unk25C0HashTableEntry(args.pos, args.roadId, args.rotation & 0x3);
                            company.addHashTableEntry(entry);

                            _pathFindUndoCount112C518 = 15;
                            return;
                        }
                    }
                }
                // 0x0048512C
                if (_pathFindTotalTrackRoadWeighting == 0)
                {
                    company.var_85E6 = (bestRoadId << 3) | (args.rotation & 0x3U);
                }

                _unk2Pos112C3C6 = pos;
                _unk2PosBaseZ112C517 = pos.z / World::kSmallZStep;
                _unkTad112C3CA = (bestRoadId << 3) | (args.rotation & 0x3U);
                _pathFindTotalTrackRoadWeighting += static_cast<int32_t>(World::TrackData::getRoadMiscData(bestRoadId).unkWeighting);
                return;
            }
            else
            {
                // ax, cx, dl
                auto pos2 = World::Pos3(_unk2Pos112C3C6, _unk2PosBaseZ112C517 * World::kSmallZStep);
                // dh
                auto roadIdStart = (_unkTad112C3CA >> 3) & 0xF;
                auto rot = _unkTad112C3CA & 0x3U;
                const auto entry = Company::Unk25C0HashTableEntry(pos2, roadIdStart, rot);
                company.addHashTableEntry(entry);
                _pathFindUndoCount112C518 = 1;
                return;
            }
        }
        else
        {
            pathFindRoadUndoSection(company, roadObjId);
        }
    }

    // 0x00484648
    // company : _unk112C390
    static void sub_484648(Company& company, const PlacementVars& placementVars)
    {
        if (placementVars.trackRoadObjId & (1U << 7))
        {
            pathFindRoadSection(company, placementVars);
        }
        else
        {
            pathFindTrackSection(company, placementVars);
        }
    }

    namespace RoadReplacePrice
    {
        static const World::RoadElement* getRoadElement(const World::Pos3 pos, const uint8_t rotation, const uint8_t roadId, const uint8_t sequenceIndex, const CompanyId companyId)
        {

            auto tile = World::TileManager::get(pos);
            for (const auto& el : tile)
            {
                if (el.baseHeight() != pos.z)
                {
                    continue;
                }
                auto* elRoad = el.as<World::RoadElement>();
                if (elRoad == nullptr)
                {
                    continue;
                }
                if (elRoad->rotation() != rotation)
                {
                    continue;
                }
                if (elRoad->sequenceIndex() != sequenceIndex)
                {
                    continue;
                }
                if (elRoad->owner() != companyId)
                {
                    continue;
                }
                if (!elRoad->isAiAllocated())
                {
                    continue;
                }
                if (elRoad->roadId() != roadId)
                {
                    continue;
                }
                return elRoad;
            }
            return nullptr;
        }

        // 0x0047C159
        static World::TileClearance::ClearFuncResult clearFunction(
            World::TileElement& el,
            currency32_t& totalCost,
            bool& hasLevelCrossing)
        {
            switch (el.type())
            {
                case World::ElementType::track:
                {
                    hasLevelCrossing = true;
                    return World::TileClearance::ClearFuncResult::noCollision;
                }
                case World::ElementType::station:
                {
                    auto* elStation = el.as<World::StationElement>();
                    if (elStation->stationType() == StationType::roadStation)
                    {
                        return World::TileClearance::ClearFuncResult::noCollision;
                    }
                    return World::TileClearance::ClearFuncResult::collision;
                }
                case World::ElementType::building:
                {
                    auto* elBuilding = el.as<World::BuildingElement>();
                    if (elBuilding == nullptr)
                    {
                        return World::TileClearance::ClearFuncResult::noCollision;
                    }
                    auto* buildingObj = ObjectManager::get<BuildingObject>(elBuilding->objectId());
                    totalCost += Economy::getInflationAdjustedCost(buildingObj->clearCostFactor, buildingObj->clearCostIndex, 8);

                    return World::TileClearance::ClearFuncResult::noCollision;
                }
                case World::ElementType::tree:
                {
                    auto* elTree = el.as<World::TreeElement>();
                    if (elTree == nullptr)
                    {
                        return World::TileClearance::ClearFuncResult::noCollision;
                    }
                    auto* treeObj = ObjectManager::get<TreeObject>(elTree->treeObjectId());
                    totalCost += Economy::getInflationAdjustedCost(treeObj->clearCostFactor, treeObj->costIndex, 12);

                    return World::TileClearance::ClearFuncResult::noCollision;
                }
                case World::ElementType::road:
                    return World::TileClearance::ClearFuncResult::noCollision;

                case World::ElementType::signal:
                case World::ElementType::surface:
                case World::ElementType::wall:
                case World::ElementType::industry:
                    return World::TileClearance::ClearFuncResult::collision;
            }
            return World::TileClearance::ClearFuncResult::collision;
        }

        // 0x0047BD6D
        // pos: ax, cx, di
        // rotation: bh
        // index: dh
        // roadId: dl
        // roadObjId: bp (unused)
        static currency32_t aiRoadReplacementCost(const World::Pos3 pos, uint8_t rotation, uint8_t index, uint8_t roadId, CompanyId companyId)
        {
            auto* elRoadSeq = getRoadElement(pos, rotation, roadId, index, companyId);
            if (elRoadSeq == nullptr)
            {
                return 0;
            }
            bool isOnWater = World::TileManager::get(pos).surface()->water() != 0;

            auto* roadObj = ObjectManager::get<RoadObject>(elRoadSeq->roadObjectId());

            currency32_t totalCost = 0;
            const auto roadIdCostFactor = World::TrackData::getRoadMiscData(roadId).costFactor;

            {
                const auto roadBaseCost = Economy::getInflationAdjustedCost(roadObj->buildCostFactor, roadObj->costIndex, 10);
                const auto cost = (roadBaseCost * roadIdCostFactor) / 256;
                totalCost += cost;
            }
            if (!roadObj->hasFlags(RoadObjectFlags::unk_03))
            {
                for (auto i = 0U; i < 2; ++i)
                {
                    if (elRoadSeq->hasMod(i))
                    {
                        auto* extraObj = ObjectManager::get<RoadExtraObject>(roadObj->mods[i]);
                        const auto roadExtraBaseCost = Economy::getInflationAdjustedCost(extraObj->buildCostFactor, extraObj->costIndex, 10);
                        const auto cost = (roadExtraBaseCost * roadIdCostFactor) / 256;
                        totalCost += cost;
                    }
                }
            }

            const auto& roadPieces = World::TrackData::getRoadPiece(roadId);
            const auto& roadPieceSeq = roadPieces[index];
            const auto roadLoc0 = pos - World::Pos3{ Math::Vector::rotate(World::Pos2{ roadPieceSeq.x, roadPieceSeq.y }, rotation), roadPieceSeq.z };

            bool hasBridge = false;
            uint8_t bridgeType = 0xFFU;

            for (auto& piece : roadPieces)
            {
                const auto roadLoc = roadLoc0 + World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, rotation), piece.z };

                auto* elRoad = getRoadElement(roadLoc, rotation, roadId, piece.index, companyId);
                if (elRoad == nullptr)
                {
                    continue;
                }
                if (elRoad->hasBridge())
                {
                    hasBridge = true;
                    bridgeType = elRoad->bridge();
                }

                bool hasLevelCrossing = false;
                // As all level crossings will be new its always going to be currentDefaultLevelCrossingType
                const auto levelCrossingObjId = getGameState().currentDefaultLevelCrossingType;

                auto clearFunc = [&totalCost, &hasLevelCrossing](World::TileElement& el) {
                    return clearFunction(el, totalCost, hasLevelCrossing);
                };
                World::TileClearance::applyClearAtStandardHeight(roadLoc, elRoad->baseZ(), elRoad->clearZ(), World::QuarterTile(elRoad->occupiedQuarter(), 0), clearFunc);

                if (hasLevelCrossing)
                {
                    auto* levelCrossingObj = ObjectManager::get<LevelCrossingObject>(levelCrossingObjId);
                    totalCost += Economy::getInflationAdjustedCost(levelCrossingObj->costFactor, levelCrossingObj->costIndex, 10);
                }
            }

            if (hasBridge)
            {
                auto* bridgeObj = ObjectManager::get<BridgeObject>(bridgeType);
                // TODO: When we diverge reactivate this code see track version of function
                const auto heightCost = 0 * bridgeObj->heightCostFactor; // Why 0 probably a bug
                const auto bridgeBaseCost = Economy::getInflationAdjustedCost(bridgeObj->baseCostFactor + heightCost, bridgeObj->costIndex, 10);
                auto cost = (bridgeBaseCost * roadIdCostFactor) / 256;
                if (isOnWater)
                {
                    cost *= 2;
                }
                totalCost += cost;
            }

            // TODO: When we diverge reactivate this code see track version of function
            // if (0) // Likely another bug
            //{
            //     const auto tunnelBaseCost = Economy::getInflationAdjustedCost(roadObj->tunnelCostFactor, 2, 8);
            //     auto cost = (tunnelBaseCost * roadIdCostFactor) / 256;
            //     totalCost += cost;
            // }

            return totalCost;
        }
    }

    // 0x0047B336
    // pos: ax, cx, di
    // rotation: bh
    // sequenceIndex: dh
    // roadId: dl
    // roadObjId : bp (unused)
    static bool sub_47B336(World::Pos3 pos, uint8_t rotation, uint8_t sequenceIndex, uint8_t roadId, CompanyId companyId)
    {
        const auto traitFlags = World::TrackData::getRoadMiscData(roadId).flags;
        using enum World::Track::CommonTraitFlags;
        // 0x1136088
        const bool allowWaterBridge = (traitFlags & (slope | steepSlope | verySmallCurve)) != none;
        {
            auto elRoad = [pos, rotation, sequenceIndex, roadId, companyId]() -> const World::RoadElement* {
                auto tile = World::TileManager::get(pos);
                for (const auto& el : tile)
                {
                    if (el.baseHeight() != pos.z)
                    {
                        continue;
                    }
                    auto* elRoad = el.as<World::RoadElement>();
                    if (elRoad == nullptr)
                    {
                        continue;
                    }
                    if (elRoad->rotation() != rotation)
                    {
                        continue;
                    }
                    if (elRoad->sequenceIndex() != sequenceIndex)
                    {
                        continue;
                    }
                    if (elRoad->owner() != companyId)
                    {
                        continue;
                    }
                    if (!elRoad->isAiAllocated() || elRoad->isGhost())
                    {
                        continue;
                    }
                    if (elRoad->roadId() != roadId)
                    {
                        continue;
                    }
                    return elRoad;
                }
                return nullptr;
            }();
            if (elRoad == nullptr)
            {
                return false;
            }
            if (!elRoad->hasBridge())
            {
                return false;
            }
        }

        auto& roadPieces = World::TrackData::getRoadPiece(roadId);
        auto& roadPiece = roadPieces[sequenceIndex];
        const auto roadPos0 = pos - World::Pos3{ Math::Vector::rotate(World::Pos2{ roadPiece.x, roadPiece.y }, rotation), roadPiece.z };
        for (auto& piece : roadPieces)
        {
            const auto roadPos = roadPos0 + World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, rotation), piece.z };
            auto tile = World::TileManager::get(roadPos);
            auto* elSurface = tile.surface();
            // ah
            const auto bridgeZ = roadPos.z / World::kSmallZStep - elSurface->baseZ();
            if (elSurface->water() != 0)
            {
                // 0x0047B57B
                if (allowWaterBridge)
                {
                    if (bridgeZ > 16)
                    {
                        continue;
                    }
                    return false;
                }
                else
                {
                    if (roadId != 0)
                    {
                        return false;
                    }
                    bool passedSurface = false;
                    bool shouldContinue = false;
                    for (auto& el : tile)
                    {
                        if (el.type() == World::ElementType::surface)
                        {
                            passedSurface = true;
                        }
                        if (!passedSurface)
                        {
                            continue;
                        }
                        if (roadPos.z <= el.baseHeight())
                        {
                            return false;
                        }
                        auto* elTrack = el.as<World::TrackElement>();
                        if (elTrack != nullptr)
                        {
                            if (elTrack->trackId() != 0)
                            {
                                continue;
                            }
                            if (elTrack->rotation() == rotation
                                || (elTrack->rotation() ^ (1U << 1)) == rotation)
                            {
                                shouldContinue = true;
                                break;
                            }
                        }
                        auto* elRoad = el.as<World::RoadElement>();
                        if (elRoad != nullptr)
                        {
                            if (elRoad->roadId() != 0)
                            {
                                continue;
                            }
                            if (elRoad->rotation() == rotation
                                || (elRoad->rotation() ^ (1U << 1)) == rotation)
                            {
                                shouldContinue = true;
                                break;
                            }
                        }
                    }
                    if (shouldContinue)
                    {
                        continue;
                    }
                    return false;
                }
            }
            else
            {
                // 0x0047B4DD
                bool passedSurface = false;
                bool shouldContinue = false;
                bool shouldReturn = false; // May be overridden by a shouldContinue
                for (auto& el : tile)
                {
                    if (el.type() == World::ElementType::surface)
                    {
                        passedSurface = true;
                    }
                    if (!passedSurface)
                    {
                        continue;
                    }
                    if (roadPos.z <= el.baseHeight())
                    {
                        break;
                    }
                    auto* elTrack = el.as<World::TrackElement>();
                    if (elTrack != nullptr)
                    {
                        if (roadId != 0)
                        {
                            return false;
                        }
                        if (elTrack->trackId() != 0)
                        {
                            shouldReturn = true;
                            continue;
                        }
                        if (elTrack->rotation() == rotation
                            || (elTrack->rotation() ^ (1U << 1)) == rotation)
                        {
                            shouldContinue = true;
                            break;
                        }
                        shouldReturn = true;
                    }
                    auto* elRoad = el.as<World::RoadElement>();
                    if (elRoad != nullptr)
                    {
                        if (roadId != 0)
                        {
                            return false;
                        }
                        if (elRoad->roadId() != 0)
                        {
                            shouldReturn = true;
                            continue;
                        }
                        if (elRoad->rotation() == rotation
                            || (elRoad->rotation() ^ (1U << 1)) == rotation)
                        {
                            shouldContinue = true;
                            break;
                        }
                        shouldReturn = true;
                    }
                }
                if (shouldContinue)
                {
                    continue;
                }
                if (shouldReturn)
                {
                    return false;
                }
                if (bridgeZ > 16)
                {
                    return false;
                }
                if (elSurface->slope() != 0)
                {
                    return false;
                }
            }
        }
        return true;
    }

    // 0x004A80E1
    // pos: ax, cx, di
    // rotation: bh
    // sequenceIndex: dh
    // trackId: dl
    // trackObjId : bp
    static bool sub_4A80E1(World::Pos3 pos, uint8_t rotation, uint8_t sequenceIndex, uint8_t trackId, uint8_t trackObjId)
    {
        const auto traitFlags = World::TrackData::getTrackMiscData(trackId).flags;
        using enum World::Track::CommonTraitFlags;
        // 0x1136088
        const bool allowWaterBridge = (traitFlags & (slope | steepSlope | verySmallCurve)) != none;
        {
            auto elTrack = [pos, rotation, sequenceIndex, trackId, trackObjId]() -> const World::TrackElement* {
                auto tile = World::TileManager::get(pos);
                for (const auto& el : tile)
                {
                    if (el.baseHeight() != pos.z)
                    {
                        continue;
                    }
                    auto* elTrack = el.as<World::TrackElement>();
                    if (elTrack == nullptr)
                    {
                        continue;
                    }
                    if (elTrack->rotation() != rotation)
                    {
                        continue;
                    }
                    if (elTrack->sequenceIndex() != sequenceIndex)
                    {
                        continue;
                    }
                    if (elTrack->trackObjectId() != trackObjId)
                    {
                        continue;
                    }
                    if (!elTrack->isAiAllocated() || elTrack->isGhost())
                    {
                        continue;
                    }
                    if (elTrack->trackId() != trackId)
                    {
                        continue;
                    }
                    return elTrack;
                }
                return nullptr;
            }();
            if (elTrack == nullptr)
            {
                return false;
            }
            if (!elTrack->hasBridge())
            {
                return false;
            }
        }

        auto& trackPieces = World::TrackData::getTrackPiece(trackId);
        auto& trackPiece = trackPieces[sequenceIndex];
        const auto trackPos0 = pos - World::Pos3{ Math::Vector::rotate(World::Pos2{ trackPiece.x, trackPiece.y }, rotation), trackPiece.z };
        for (auto& piece : trackPieces)
        {
            const auto trackPos = trackPos0 + World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, rotation), piece.z };
            auto tile = World::TileManager::get(trackPos);
            auto* elSurface = tile.surface();
            // ah
            const auto bridgeZ = trackPos.z / World::kSmallZStep - elSurface->baseZ();
            if (elSurface->water() != 0)
            {
                // 0x004A8326
                if (allowWaterBridge)
                {
                    if (bridgeZ > 16)
                    {
                        continue;
                    }
                    return false;
                }
                else
                {
                    if (trackId != 0)
                    {
                        return false;
                    }
                    bool passedSurface = false;
                    bool shouldContinue = false;
                    for (auto& el : tile)
                    {
                        if (el.type() == World::ElementType::surface)
                        {
                            passedSurface = true;
                            continue;
                        }
                        if (!passedSurface)
                        {
                            continue;
                        }
                        if (trackPos.z <= el.baseHeight())
                        {
                            return false;
                        }
                        auto* elTrack = el.as<World::TrackElement>();
                        if (elTrack != nullptr)
                        {
                            if (elTrack->trackId() != 0)
                            {
                                continue;
                            }
                            if (elTrack->rotation() == rotation
                                || (elTrack->rotation() ^ (1U << 1)) == rotation)
                            {
                                shouldContinue = true;
                                break;
                            }
                        }
                        auto* elRoad = el.as<World::RoadElement>();
                        if (elRoad != nullptr)
                        {
                            if (elRoad->roadId() != 0)
                            {
                                continue;
                            }
                            if (elRoad->rotation() == rotation
                                || (elRoad->rotation() ^ (1U << 1)) == rotation)
                            {
                                shouldContinue = true;
                                break;
                            }
                        }
                    }
                    if (shouldContinue)
                    {
                        continue;
                    }
                    return false;
                }
            }
            else
            {
                // 0x004A8288
                bool passedSurface = false;
                bool shouldContinue = false;
                bool shouldReturn = false; // May be overridden by a shouldContinue
                for (auto& el : tile)
                {
                    if (el.type() == World::ElementType::surface)
                    {
                        passedSurface = true;
                        continue;
                    }
                    if (!passedSurface)
                    {
                        continue;
                    }
                    if (trackPos.z <= el.baseHeight())
                    {
                        break;
                    }
                    auto* elTrack = el.as<World::TrackElement>();
                    if (elTrack != nullptr)
                    {
                        if (trackId != 0)
                        {
                            return false;
                        }
                        if (elTrack->trackId() != 0)
                        {
                            shouldReturn = true;
                            continue;
                        }
                        if (elTrack->rotation() == rotation
                            || (elTrack->rotation() ^ (1U << 1)) == rotation)
                        {
                            shouldContinue = true;
                            break;
                        }
                        shouldReturn = true;
                    }
                    auto* elRoad = el.as<World::RoadElement>();
                    if (elRoad != nullptr)
                    {
                        if (trackId != 0)
                        {
                            return false;
                        }
                        if (elRoad->roadId() != 0)
                        {
                            shouldReturn = true;
                            continue;
                        }
                        if (elRoad->rotation() == rotation
                            || (elRoad->rotation() ^ (1U << 1)) == rotation)
                        {
                            shouldContinue = true;
                            break;
                        }
                        shouldReturn = true;
                    }
                }
                if (shouldContinue)
                {
                    continue;
                }
                if (shouldReturn)
                {
                    return false;
                }
                if (bridgeZ > 16)
                {
                    return false;
                }
                if (elSurface->slope() != 0)
                {
                    return false;
                }
            }
        }
        return true;
    }

    // 0x004A7E86
    // pos: ax, cx, di
    // rotation: bh
    // sequenceIndex: dh
    // trackId: dl
    // trackObjId : bp
    static bool connectsToExistingTrack(World::Pos3 pos, uint8_t rotation, uint8_t sequenceIndex, uint8_t trackId, uint8_t trackObjId)
    {
        auto getElTrack = [rotation, trackId, trackObjId](World::Pos3 pos, uint8_t sequenceIndex) -> const World::TrackElement* {
            auto tile = World::TileManager::get(pos);
            for (const auto& el : tile)
            {
                if (el.baseHeight() != pos.z)
                {
                    continue;
                }
                auto* elTrack = el.as<World::TrackElement>();
                if (elTrack == nullptr)
                {
                    continue;
                }
                if (elTrack->rotation() != rotation)
                {
                    continue;
                }
                if (elTrack->sequenceIndex() != sequenceIndex)
                {
                    continue;
                }
                if (elTrack->trackObjectId() != trackObjId)
                {
                    continue;
                }
                if (!elTrack->isAiAllocated() || elTrack->isGhost())
                {
                    continue;
                }
                if (elTrack->trackId() != trackId)
                {
                    continue;
                }
                return elTrack;
            }
            return nullptr;
        };
        auto* elTrackSeq = getElTrack(pos, sequenceIndex);
        if (elTrackSeq == nullptr)
        {
            return false;
        }

        auto& trackPieces = World::TrackData::getTrackPiece(trackId);
        auto& trackPiece = trackPieces[sequenceIndex];
        const auto trackPos0 = pos - World::Pos3{ Math::Vector::rotate(World::Pos2{ trackPiece.x, trackPiece.y }, rotation), trackPiece.z };
        for (auto& piece : trackPieces)
        {
            const auto trackPos = trackPos0 + World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, rotation), piece.z };
            auto tile = World::TileManager::get(trackPos);
            auto* elTrack = getElTrack(trackPos, piece.index);
            if (elTrack == nullptr)
            {
                continue;
            }
            for (auto& el : tile)
            {
                auto* elConnectTrack = el.as<World::TrackElement>();
                if (elConnectTrack == nullptr)
                {
                    continue;
                }
                if (elConnectTrack == elTrack)
                {
                    continue;
                }
                if (elConnectTrack->baseHeight() != trackPos.z)
                {
                    continue;
                }
                if (elConnectTrack->occupiedQuarter() == elTrack->occupiedQuarter())
                {
                    continue;
                }

                const auto connectFlags1 = piece.connectFlags[rotation];
                auto& piece2 = World::TrackData::getTrackPiece(elConnectTrack->trackId())[elConnectTrack->sequenceIndex()];
                const auto connectFlags2 = piece2.connectFlags[elConnectTrack->rotation()];
                if ((connectFlags1 & connectFlags2) != 0)
                {
                    return true;
                }
            }
        }
        return false;
    }

    // 0x0047B7CC
    // pos: ax, cx, di
    // rotation: bh
    // sequenceIndex: dh
    // roadId: dl
    // roadObjId : bp (unused)
    static bool connectsToExistingRoad(World::Pos3 pos, uint8_t rotation, uint8_t sequenceIndex, uint8_t roadId, CompanyId companyId)
    {
        auto getElRoad = [rotation, roadId, companyId](World::Pos3 pos, uint8_t sequenceIndex) -> const World::RoadElement* {
            auto tile = World::TileManager::get(pos);
            for (const auto& el : tile)
            {
                if (el.baseHeight() != pos.z)
                {
                    continue;
                }
                auto* elRoad = el.as<World::RoadElement>();
                if (elRoad == nullptr)
                {
                    continue;
                }
                if (elRoad->rotation() != rotation)
                {
                    continue;
                }
                if (elRoad->sequenceIndex() != sequenceIndex)
                {
                    continue;
                }
                if (elRoad->owner() != companyId)
                {
                    continue;
                }
                if (elRoad->roadId() != roadId)
                {
                    continue;
                }
                if (!elRoad->isAiAllocated() || elRoad->isGhost())
                {
                    continue;
                }
                return elRoad;
            }
            return nullptr;
        };
        auto* elRoadSeq = getElRoad(pos, sequenceIndex);
        if (elRoadSeq == nullptr)
        {
            return false;
        }

        auto& roadPieces = World::TrackData::getRoadPiece(roadId);
        auto& roadPiece = roadPieces[sequenceIndex];
        const auto roadPos0 = pos - World::Pos3{ Math::Vector::rotate(World::Pos2{ roadPiece.x, roadPiece.y }, rotation), roadPiece.z };
        for (auto& piece : roadPieces)
        {
            const auto roadPos = roadPos0 + World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, rotation), piece.z };
            auto tile = World::TileManager::get(roadPos);
            auto* elRoad = getElRoad(roadPos, piece.index);
            if (elRoad == nullptr)
            {
                continue;
            }
            for (auto& el : tile)
            {
                auto* elConnectRoad = el.as<World::RoadElement>();
                if (elConnectRoad == nullptr)
                {
                    continue;
                }
                if (elConnectRoad == elRoad)
                {
                    continue;
                }
                if (!elConnectRoad->isAiAllocated())
                {
                    continue;
                }
                if (elConnectRoad->baseHeight() != roadPos.z)
                {
                    continue;
                }
                if (elConnectRoad->occupiedQuarter() == elRoad->occupiedQuarter())
                {
                    continue;
                }

                const auto connectFlags1 = piece.connectFlags[rotation];
                auto& piece2 = World::TrackData::getRoadPiece(elConnectRoad->roadId())[elConnectRoad->sequenceIndex()];
                const auto connectFlags2 = piece2.connectFlags[elConnectRoad->rotation()];
                if ((connectFlags1 & connectFlags2) != 0)
                {
                    return true;
                }
            }
        }
        return false;
    }

    // 0x0047B615
    // pos: ax, cx, di
    // rotation: bh
    // sequenceIndex: dh
    // roadId: dl
    // roadObjId : bp (unused)
    static bool willRoadDestroyABuilding(World::Pos3 pos, uint8_t rotation, uint8_t sequenceIndex, uint8_t roadId, CompanyId companyId)
    {
        auto getElRoad = [rotation, roadId, companyId](World::Pos3 pos, uint8_t sequenceIndex) -> const World::RoadElement* {
            auto tile = World::TileManager::get(pos);
            for (const auto& el : tile)
            {
                if (el.baseHeight() != pos.z)
                {
                    continue;
                }
                auto* elRoad = el.as<World::RoadElement>();
                if (elRoad == nullptr)
                {
                    continue;
                }
                if (elRoad->rotation() != rotation)
                {
                    continue;
                }
                if (elRoad->sequenceIndex() != sequenceIndex)
                {
                    continue;
                }
                if (elRoad->owner() != companyId)
                {
                    continue;
                }
                if (elRoad->roadId() != roadId)
                {
                    continue;
                }
                if (!elRoad->isAiAllocated() || elRoad->isGhost())
                {
                    continue;
                }
                return elRoad;
            }
            return nullptr;
        };
        auto* elRoadSeq = getElRoad(pos, sequenceIndex);
        if (elRoadSeq == nullptr)
        {
            return false;
        }

        auto& roadPieces = World::TrackData::getRoadPiece(roadId);
        auto& roadPiece = roadPieces[sequenceIndex];
        const auto roadPos0 = pos - World::Pos3{ Math::Vector::rotate(World::Pos2{ roadPiece.x, roadPiece.y }, rotation), roadPiece.z };
        for (auto& piece : roadPieces)
        {
            const auto roadPos = roadPos0 + World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, rotation), piece.z };
            auto tile = World::TileManager::get(roadPos);
            bool passedSurface = false;
            for (auto& el : tile)
            {
                if (el.type() == World::ElementType::surface)
                {
                    passedSurface = true;
                    continue;
                }
                if (!passedSurface)
                {
                    continue;
                }
                auto* elBuilding = el.as<World::BuildingElement>();
                if (elBuilding == nullptr)
                {
                    continue;
                }
                if (roadPos.z >= elBuilding->clearHeight())
                {
                    continue;
                }
                if (roadPos.z + 8 * World::kSmallZStep <= elBuilding->baseHeight())
                {
                    continue;
                }
                return true;
            }
        }
        return false;
    }

    enum class PathfindResultState : uint8_t
    {
        targetReached = 0U,
        noRoute = 1U,
        targetReachedWithConnection = 2U, // Connection to existing track/road
    };

    struct PathfindResult
    {
        PathfindResultState state;
        currency32_t totalCost;     // 0x0112C34C
        uint32_t totalPenalties;    // 0x0112C35C penalties such as destroying buildings (road only), something to do with bridges
        uint32_t euclideanDistance; // 0x0112C364 only set when path to target found (state == targetReached or targetReachedWithConnection)
        uint32_t totalWeighting;    // 0x0112C36C
    };

    // 0x00485B75
    // startPos.x: 0x0112C3C6
    // startPos.y: 0x0112C3C8
    // startPos.z: 0x0112C517 * World::kSmallZStep
    // startTad: 0x0112C3CA
    // targetPos.x: 0x0112C3C2
    // targetPos.y: 0x0112C3C4
    // targetPos.z: 0x0112C515 * World::kSmallZStep
    // targetRot: 0x0112C516
    // trackObjId: 0x0112C519
    static PathfindResult sub_485B75(const World::Pos3 startPos, const uint16_t startTad, const World::Pos3 targetPos, const uint8_t targetRot, const uint8_t trackObjId, const CompanyId companyId)
    {
        PathfindResult result{};
        bool hasExistingTrackConnection = false;
        uint32_t unk112C360 = _pathFindTotalTrackRoadWeighting;
        World::Pos3 pos = startPos;
        uint16_t tad = startTad;
        for (auto i = 0U; i < 400; ++i)
        {
            if (pos == targetPos)
            {
                // 0x00485DBD
                const auto posA = startPos + World::TrackData::getUnkTrack(startTad).pos;
                const auto posB = targetPos + World::Pos3(World::kRotationOffset[targetRot], 0);
                result.euclideanDistance = Math::Vector::distance3D(posA, posB);
                result.state = hasExistingTrackConnection ? PathfindResultState::targetReachedWithConnection : PathfindResultState::targetReached;
                return result;
            }

            const uint8_t trackId = (tad >> 3U) & 0x3F;
            const uint8_t rotation = tad & 0x3U;
            const auto unkWeighting = World::TrackData::getTrackMiscData(trackId).unkWeighting;
            result.totalWeighting += unkWeighting;
            unk112C360 -= unkWeighting;

            auto posAdjusted = pos;
            posAdjusted.z += World::TrackData::getTrackPiece(trackId)[0].z;

            {
                GameCommands::AiTrackReplacementArgs args{};
                args.pos = posAdjusted;
                args.rotation = tad & 0x3U;
                args.sequenceIndex = 0;
                args.trackId = trackId;
                args.trackObjectId = trackObjId;

                auto regs(static_cast<GameCommands::registers>(args));
                regs.bl = 0;
                GameCommands::aiTrackReplacement(regs);
                if (static_cast<uint32_t>(regs.ebx) != GameCommands::FAILURE)
                {
                    result.totalCost += static_cast<uint32_t>(regs.ebx);
                }
            }
            if (sub_4A80E1(posAdjusted, rotation, 0, trackId, trackObjId))
            {
                result.totalPenalties += unkWeighting;
            }
            if (result.totalWeighting > 128 && unk112C360 > 64)
            {
                if (connectsToExistingTrack(posAdjusted, rotation, 0, trackId, trackObjId))
                {
                    hasExistingTrackConnection = true;
                }
            }
            const auto rotationBegin = World::TrackData::getUnkTrack(tad).rotationBegin;
            auto nextPos = pos;
            if (rotationBegin < 12)
            {
                nextPos -= World::Pos3(World::kRotationOffset[rotationBegin], 0);
            }
            const auto nextRot = World::kReverseRotation[rotationBegin];
            const auto tc = World::Track::getTrackConnectionsAi(nextPos, nextRot, companyId, trackObjId, 0, 0);
            if (tc.connections.empty() || tc.connections.size() > 1)
            {
                result.state = PathfindResultState::noRoute;
                return result;
            }

            tad = tc.connections[0] & World::Track::AdditionalTaDFlags::basicTaDMask;
            const auto& trackSize = World::TrackData::getUnkTrack(tad);
            pos = nextPos + trackSize.pos;
            if (trackSize.rotationEnd < 12)
            {
                pos -= World::Pos3(World::kRotationOffset[trackSize.rotationEnd], 0);
            }
            tad ^= (1U << 2);
            if (tad & (1U << 2))
            {
                // Odd? what is this doing
                tad = (tad & 0x3) | (0U << 3);
            }
        }
        result.state = PathfindResultState::noRoute;
        return result;
    }

    // 0x00485E6A
    // startPos.x: 0x0112C3C6
    // startPos.y: 0x0112C3C8
    // startPos.z: 0x0112C517 * World::kSmallZStep
    // startTad: 0x0112C3CA
    // targetPos.x: 0x0112C3C2
    // targetPos.y: 0x0112C3C4
    // targetPos.z: 0x0112C515 * World::kSmallZStep
    // targetRot: 0x0112C516
    // roadObjId: 0x0112C519
    static PathfindResult sub_485E6A(const World::Pos3 startPos, const uint16_t startTad, const World::Pos3 targetPos, const uint8_t targetRot, const uint8_t roadObjId, const CompanyId companyId)
    {
        PathfindResult result{};
        bool hasExistingRoadConnection = false;
        World::Pos3 pos = startPos;
        uint16_t tad = startTad;
        bool targetReached = false;
        for (auto i = 0U; i < 400; ++i)
        {
            if (pos == targetPos)
            {
                targetReached = true;
                break;
            }

            const uint8_t roadId = (tad >> 3U) & 0xF;
            const uint8_t rotation = tad & 0x3U;
            const auto unkWeighting = World::TrackData::getRoadMiscData(roadId).unkWeighting;
            result.totalWeighting += unkWeighting;

            auto posAdjusted = pos;
            posAdjusted.z += World::TrackData::getRoadPiece(roadId)[0].z;

            result.totalCost += static_cast<uint32_t>(RoadReplacePrice::aiRoadReplacementCost(posAdjusted, rotation, 0, roadId, companyId));

            if (sub_47B336(posAdjusted, rotation, 0, roadId, companyId))
            {
                result.totalPenalties += unkWeighting;
            }

            if (willRoadDestroyABuilding(posAdjusted, rotation, 0, roadId, companyId))
            {
                result.totalPenalties += unkWeighting;
            }

            if (connectsToExistingRoad(posAdjusted, rotation, 0, roadId, companyId))
            {
                hasExistingRoadConnection = true;
            }

            const auto rotationBegin = World::TrackData::getUnkRoad(tad).rotationBegin;
            const auto nextPos = pos - World::Pos3(World::kRotationOffset[rotationBegin], 0);
            const auto nextRot = World::kReverseRotation[rotationBegin];
            uint8_t matchRoadObjId = roadObjId;
            auto* roadObj = ObjectManager::get<RoadObject>(roadObjId);
            if (roadObj->hasFlags(RoadObjectFlags::unk_03))
            {
                matchRoadObjId = 0xFFU; // any road object
            }

            const auto rc = World::Track::getRoadConnectionsAiAllocated(nextPos, nextRot, companyId, matchRoadObjId, 0, 0);
            if (rc.connections.size() > 1)
            {
                result.state = PathfindResultState::noRoute;
                return result;
            }
            if (rc.connections.empty())
            {
                if (nextPos == targetPos)
                {
                    targetReached = true;
                }
                break;
            }

            tad = rc.connections[0] & World::Track::AdditionalTaDFlags::basicTaDMask;
            const auto& roadSize = World::TrackData::getUnkRoad(tad);
            pos = nextPos + roadSize.pos - World::Pos3(World::kRotationOffset[roadSize.rotationEnd], 0);

            tad ^= (1U << 2);
            if (tad & (1U << 2))
            {
                // Odd? what is this doing
                tad = (tad & 0x3) | (0U << 3);
            }
        }
        if (targetReached)
        {
            // 0x004860F4
            const auto posA = startPos + World::TrackData::getUnkRoad(startTad).pos;
            const auto posB = targetPos + World::Pos3(World::kRotationOffset[targetRot], 0);
            result.euclideanDistance = Math::Vector::distance3D(posA, posB);
            result.state = hasExistingRoadConnection ? PathfindResultState::targetReachedWithConnection : PathfindResultState::targetReached;
            return result;
        }
        else
        {
            result.state = PathfindResultState::noRoute;
            return result;
        }
    }

    // 0x00485B68
    static PathfindResult sub_485B68(const uint8_t trackRoadObjId)
    {
        const auto startPos = World::Pos3{ _unk2Pos112C3C6.x, _unk2Pos112C3C6.y, _unk2PosBaseZ112C517 * World::kSmallZStep };
        const auto startTad = _unkTad112C3CA;
        const auto targetPos = World::Pos3{ _unk1Pos112C3C2.x, _unk1Pos112C3C2.y, _unk1PosBaseZ112C515 * World::kSmallZStep };
        const auto targetRot = _unk1Rot112C516;
        const auto companyId = GameCommands::getUpdatingCompanyId();
        if (trackRoadObjId & (1U << 7))
        {
            const auto roadObjId = trackRoadObjId & ~(1U << 7);
            return sub_485E6A(startPos, startTad, targetPos, targetRot, roadObjId, companyId);
        }
        else
        {
            const auto trackObjId = trackRoadObjId;
            return sub_485B75(startPos, startTad, targetPos, targetRot, trackObjId, companyId);
        }
    }

    // 0x004845FF
    static void aiPathfindNextState(Company& company)
    {
        company.var_85E8++;
        company.var_85F0 = 0;
        company.var_85EE = 0;
        company.var_85EF = 0;
        // TODO: When diverging just set this all to a fixed value rather than only first entry
        for (auto& htEntry : company.var_25C0)
        {
            htEntry.var_00 = 0xFFFFU;
        }
        company.var_25C0_length = 0;
    }

    // 0x00484508
    static bool evaluatePathfound(Company& company, AiThought& thought, const uint8_t trackRoadObjId)
    {
        const auto pathResult = sub_485B68(trackRoadObjId);
        const auto pathfindState = pathResult.state;
        if (pathfindState == PathfindResultState::noRoute)
        {
            return true;
        }
        if (pathfindState == PathfindResultState::targetReachedWithConnection)
        {
            // 0x004845FF
            aiPathfindNextState(company);
            return false;
        }
        else
        {
            const auto distance = std::max<uint32_t>(pathResult.euclideanDistance, 256);
            // 1.75 x distance
            const auto distanceWeighting = distance + distance / 2 + distance / 4;
            if (distanceWeighting < pathResult.totalWeighting)
            {
                // 0x004845FF
                aiPathfindNextState(company);
                return false;
            }
            if (pathResult.totalPenalties * 5 >= pathResult.totalWeighting)
            {
                // 0x004845FF
                aiPathfindNextState(company);
                return false;
            }
            auto& aiStation = thought.stations[company.var_85C2];
            uint8_t nextStationIdx = 0xFFU;
            if (company.var_85C3 & (1U << 0))
            {
                nextStationIdx = aiStation.var_A;
                if (aiStation.var_C & ((1U << 2) | (1U << 1)))
                {
                    aiStation.var_C |= (1U << 3);
                }
                else
                {
                    aiStation.var_C |= (1U << 1);
                }
            }
            else
            {
                nextStationIdx = aiStation.var_9;
                if (aiStation.var_B & ((1U << 2) | (1U << 1)))
                {
                    aiStation.var_B |= (1U << 3);
                }
                else
                {
                    aiStation.var_B |= (1U << 1);
                }
            }
            auto& aiStation2 = thought.stations[nextStationIdx];
            if (aiStation2.var_9 != company.var_85C2)
            {
                if (aiStation2.var_C & ((1U << 2) | (1U << 1)))
                {
                    aiStation2.var_C |= (1U << 3);
                }
                else
                {
                    aiStation2.var_C |= (1U << 1);
                }
            }
            else
            {
                if (aiStation2.var_B & ((1U << 2) | (1U << 1)))
                {
                    aiStation2.var_B |= (1U << 3);
                }
                else
                {
                    aiStation2.var_B |= (1U << 1);
                }
            }
            company.var_85C2 = 0xFFU;
            thought.var_76 += pathResult.totalCost;
            return false;
        }
    }

    // 0x00483FBA
    bool aiPathfind(Company& company, AiThought& thought)
    {
        switch (company.var_85E8)
        {
            case 0:
            {
                _unk1Pos112C3C2 = company.var_85C4;
                _unk1PosBaseZ112C515 = company.var_85C8;
                _unk1Rot112C516 = company.var_85CE;

                _unk2Pos112C3C6 = company.var_85D0;
                _unk2PosBaseZ112C517 = company.var_85D4;
                _unkTad112C3CA = company.var_85D5;

                _unk3Pos112C3CC = company.var_85D7;
                _unk3PosBaseZ112C59C = company.var_85DB;
                _unkTad112C4D4 = company.var_85DC;

                _pathFindTotalTrackRoadWeighting = company.var_85DE;
                _pathFindUndoCount112C518 = company.var_85EE;

                const auto placementVars = sub_483A7E(company, thought);

                if (sub_483E20(company, placementVars.trackRoadObjId & (1U << 7)))
                {
                    // 0x00484508
                    return evaluatePathfound(company, thought, placementVars.trackRoadObjId);
                }
                else
                {
                    // 0x004850A0
                    company.var_85F0++;
                    if (company.var_85F0 > 384)
                    {
                        // 0x004845EF

                        const auto pathfindState = sub_485B68(placementVars.trackRoadObjId).state;
                        if (pathfindState == PathfindResultState::noRoute)
                        {
                            return true;
                        }

                        // 0x004845FF
                        aiPathfindNextState(company);
                        return false;
                    }

                    sub_484648(company, placementVars);
                    company.var_85DE = _pathFindTotalTrackRoadWeighting;
                    company.var_85EE = _pathFindUndoCount112C518;
                    company.var_85D0 = _unk2Pos112C3C6;
                    company.var_85D4 = _unk2PosBaseZ112C517;
                    company.var_85D5 = _unkTad112C3CA;
                    return false;
                }
            }
            case 1:
            {
                _unk1Pos112C3C2 = company.var_85C4;
                _unk1PosBaseZ112C515 = company.var_85C8;
                _unk1Rot112C516 = company.var_85CE;

                _unk2Pos112C3C6 = company.var_85D0;
                _unk2PosBaseZ112C517 = company.var_85D4;
                _unkTad112C3CA = company.var_85D5;

                _unk3Pos112C3CC = company.var_85D7;
                _unk3PosBaseZ112C59C = company.var_85DB;
                _unkTad112C4D4 = company.var_85DC;

                _pathFindTotalTrackRoadWeighting = company.var_85DE;
                _pathFindUndoCount112C518 = company.var_85EE;

                const auto placementVars = sub_483A7E(company, thought);

                if (_pathFindTotalTrackRoadWeighting == 0)
                {
                    // 0x004845FF
                    aiPathfindNextState(company);
                    return false;
                }
                else
                {
                    // 0x00484338
                    company.var_85F0++;
                    if (company.var_85F0 > 384)
                    {
                        return true;
                    }

                    _pathFindUndoCount112C518 = 1;
                    sub_484648(company, placementVars);
                    company.var_85DE = _pathFindTotalTrackRoadWeighting;
                    company.var_85EE = _pathFindUndoCount112C518;
                    company.var_85D0 = _unk2Pos112C3C6;
                    company.var_85D4 = _unk2PosBaseZ112C517;
                    company.var_85D5 = _unkTad112C3CA;
                    return false;
                }
            }
            case 2:
            {
                // Different to case 0 and 1
                _unk1Pos112C3C2 = company.var_85C9;
                _unk1PosBaseZ112C515 = company.var_85CD;
                _unk1Rot112C516 = company.var_85CF;

                _unk2Pos112C3C6 = company.var_85D7;
                _unk2PosBaseZ112C517 = company.var_85DB;
                _unkTad112C3CA = company.var_85DC;

                _unk3Pos112C3CC = company.var_85D0;
                _unk3PosBaseZ112C59C = company.var_85D4;
                _unkTad112C4D4 = company.var_85D5;

                _pathFindTotalTrackRoadWeighting = company.var_85E2;
                _pathFindUndoCount112C518 = company.var_85EF;

                const auto placementVars = sub_483A7E(company, thought);

                if (sub_483E20(company, placementVars.trackRoadObjId & (1U << 7)))
                {
                    // 0x00484508
                    return evaluatePathfound(company, thought, placementVars.trackRoadObjId);
                }
                else
                {
                    // 0x004841EE

                    company.var_85F0++;
                    if (company.var_85F0 > 384)
                    {
                        // 0x004845EF duplicate

                        const auto pathfindState = sub_485B68(placementVars.trackRoadObjId).state;
                        if (pathfindState == PathfindResultState::noRoute)
                        {
                            return true;
                        }

                        // 0x004845FF
                        aiPathfindNextState(company);
                        return false;
                    }

                    sub_484648(company, placementVars);
                    company.var_85E2 = _pathFindTotalTrackRoadWeighting;
                    company.var_85EF = _pathFindUndoCount112C518;
                    company.var_85D7 = _unk2Pos112C3C6;
                    company.var_85DB = _unk2PosBaseZ112C517;
                    company.var_85DC = _unkTad112C3CA;
                    return false;
                }
            }
            case 3:
                return true;
            default:
                assert(false);
                return true;
        }
    }
}
