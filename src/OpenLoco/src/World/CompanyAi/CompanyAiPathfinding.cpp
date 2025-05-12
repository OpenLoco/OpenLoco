#include "CompanyAiPathfinding.h"
#include "CompanyAi.h"
#include "Map/Tile.h"
#include "Map/Track/TrackData.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackObject.h"
#include "World/Company.h"

#include <OpenLoco/Interop/Interop.hpp>

namespace OpenLoco::CompanyAi
{
    static Interop::loco_global<uint8_t, 0x0112C519> _trackRoadObjType112C519;
    static Interop::loco_global<World::Pos2, 0x0112C3C2> _unk1Pos112C3C2;
    static Interop::loco_global<World::SmallZ, 0x0112C515> _unk1PosBaseZ112C515;
    static Interop::loco_global<World::Pos2, 0x0112C3C6> _unk2Pos112C3C6;
    static Interop::loco_global<World::SmallZ, 0x0112C3C6> _unk2PosBaseZ112C517;
    static Interop::loco_global<World::Pos2, 0x0112C3CC> _unk3Pos112C3CC;
    static Interop::loco_global<World::SmallZ, 0x0112C59C> _unk3PosBaseZ112C59C;
    static Interop::loco_global<uint32_t, 0x0112C388> _createTrackRoadCommandMods;
    static Interop::loco_global<uint32_t, 0x0112C38C> _createTrackRoadCommandRackRail;
    static Interop::loco_global<Company*, 0x0112C390> _unk112C390;
    static Interop::loco_global<uint32_t, 0x0112C374> _createTrackRoadCommandAiUnkFlags;
    static Interop::loco_global<uint8_t, 0x0112C59F> _createTrackRoadCommandBridge0;
    static Interop::loco_global<uint8_t, 0x0112C5A0> _createTrackRoadCommandBridge1;
    static Interop::loco_global<uint8_t, 0x0112C5A1> _createTrackRoadCommandBridge2;
    static Interop::loco_global<uint8_t[65], 0x0112C51A> _validTrackRoadIds;

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

    // 0x004854B2
    // pos : ax, cx, dl
    // tad : ebp
    // company : _unk112C390
    static void sub_4854B2(Company& company, const World::Pos3 pos, const uint16_t tad)
    {
        // bl
        const auto direction = tad & 0x3;
        // dh
        const auto trackId = (tad >> 3) & 0x3F;

        const auto hash1 = direction ^ (pos.z / World::kSmallZStep);
        const auto hash2 = hash1 ^ ((pos.x / 32) * 8);
        const auto hash3 = hash2 ^ (pos.y / 32);
        const auto hash4 = hash3 ^ (trackId * 64);

        uint32_t index = hash4;
        while (company.var_25C0[index].var_00 != 0xFFFF)
        {
            auto& rhsEntry = company.var_25C0[index];
            if (pos.x == rhsEntry.var_00
                && (pos.z / World::kSmallZStep) == rhsEntry.var_04
                && (trackId | (direction << 6)) == rhsEntry.var_05
                && pos.y == (rhsEntry.var_02 & 0xFFFE))
            {
                return;
            }

            if (!(rhsEntry.var_02 & 0b1))
            {
                break;
            }

            index++;
            if (index >= std::size(company.var_25C0))
            {
                index = 0;
            }
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
    }
}
