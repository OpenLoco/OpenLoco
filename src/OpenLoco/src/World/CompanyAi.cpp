#include "CompanyAi.h"
#include "Company.h"
#include "CompanyManager.h"
#include "Date.h"
#include "GameCommands/Company/BuildCompanyHeadquarters.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Track/CreateTrackMod.h"
#include "Industry.h"
#include "IndustryManager.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Objects/CompetitorObject.h"
#include "Objects/ObjectManager.h"
#include "Random.h"
#include "Station.h"
#include "StationManager.h"
#include "TownManager.h"
#include <OpenLoco/Core/Numerics.hpp>
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    static loco_global<StationId, 0x0112C730> _lastPlacedTrackStationId;
    static loco_global<StationId, 0x0112C744> _lastPlacedAirportStationId;
    static loco_global<StationId, 0x0112C748> _lastPlacedPortStationId;

    static loco_global<World::Pos2[16], 0x00503C6C> _503C6C;

    // 0x004FE720
    static constexpr std::array<uint32_t, kAiThoughtCount> kThoughtTypeFlags = {
        0x849,
        0x4011,
        0x4051,
        0x808,
        0x20808,
        0x1421,
        0x1120,
        0x98E,
        0x2098E,
        0x98A,
        0x2098A,
        0x21A6,
        0x21A2,
        0x8000,
        0x8082,
        0x10000,
        0x10086,
        0x10082,
        0x80A,
        0x2080A
    };

    // 0x00487144
    static void sub_487144(Company& company)
    {
        company.var_85C2 = 0;
        company.var_85C3 = 0;
        company.var_85F0 = 0;
    }

    // 0x00486ECF
    static uint8_t sub_486ECF(Company& company, AiThought& thought)
    {
        // some sort of purchase vehicle
        registers regs;
        regs.esi = X86Pointer(&company);
        regs.edi = X86Pointer(&thought);
        call(0x00486ECF, regs);
        return regs.al;
    }

    // 0x004876CB
    static void sub_4876CB(AiThought& thought)
    {
        // Sets unk vehicle vars and then breakdown flag ???
        registers regs;
        regs.edi = X86Pointer(&thought);
        call(0x004876CB, regs);
    }

    // 0x00494805
    static void tryRemovePortsAndAirports(Company& company)
    {
        // If set the port/airport is not removed?
        std::array<uint8_t, Limits::kMaxStations> unkStationFlags{};
        for (auto& thought : company.aiThoughts)
        {
            if (thought.type == AiThoughtType::null)
            {
                continue;
            }

            for (auto i = 0; i < 4 && i < thought.var_03; ++i)
            {
                const auto& unk2 = thought.var_06[i];
                if (kThoughtTypeFlags[enumValue(thought.type)] & (1ULL << 15))
                {
                    unkStationFlags[enumValue(unk2.var_00)] |= 1U << 0;
                }
                if (kThoughtTypeFlags[enumValue(thought.type)] & (1ULL << 16))
                {
                    unkStationFlags[enumValue(unk2.var_00)] |= 1U << 1;
                }
            }
        }

        for (auto& station : StationManager::stations())
        {
            if (station.owner != company.id())
            {
                continue;
            }

            if (((station.flags & StationFlags::transportModeAir) != StationFlags::none)
                && !(unkStationFlags[enumValue(station.id())] & (1U << 0)))
            {

                for (auto i = 0; i < station.stationTileSize; ++i)
                {
                    auto& tileLoc = station.stationTiles[i];

                    const auto tile = World::TileManager::get(tileLoc);
                    bool stationElFound = false;
                    for (auto& el : tile)
                    {
                        auto* elStation = el.as<World::StationElement>();
                        if (elStation == nullptr)
                        {
                            continue;
                        }

                        if (elStation->baseZ() != tileLoc.z / World::kSmallZStep)
                        {
                            continue;
                        }

                        if (elStation->stationType() != StationType::airport)
                        {
                            continue;
                        }
                        stationElFound = true;
                        break;
                    }

                    if (!stationElFound)
                    {
                        continue;
                    }

                    GameCommands::AirportRemovalArgs args{};
                    args.pos = tileLoc;
                    args.pos.z = World::heightFloor(args.pos.z);
                    GameCommands::doCommand(args, GameCommands::Flags::apply);
                    break;
                }
            }
            if (((station.flags & StationFlags::transportModeWater) != StationFlags::none)
                && !(unkStationFlags[enumValue(station.id())] & (1U << 1)))
            {

                for (auto i = 0; i < station.stationTileSize; ++i)
                {
                    auto& tileLoc = station.stationTiles[i];

                    const auto tile = World::TileManager::get(tileLoc);
                    bool stationElFound = false;
                    for (auto& el : tile)
                    {
                        auto* elStation = el.as<World::StationElement>();
                        if (elStation == nullptr)
                        {
                            continue;
                        }

                        if (elStation->baseZ() != tileLoc.z / World::kSmallZStep)
                        {
                            continue;
                        }

                        if (elStation->stationType() != StationType::docks)
                        {
                            continue;
                        }
                        stationElFound = true;
                        break;
                    }

                    if (!stationElFound)
                    {
                        continue;
                    }

                    GameCommands::PortRemovalArgs args{};
                    args.pos = tileLoc;
                    args.pos.z = World::heightFloor(args.pos.z);
                    GameCommands::doCommand(args, GameCommands::Flags::apply);
                    break;
                }
            }
        }
    }

    // 0x004308D4
    static void aiThinkState0(Company& company)
    {
        company.var_85F6++;
        if (company.var_85F6 < 672)
        {
            company.var_4A4 = AiThinkState::unk2;
            company.var_4A5 = 0;
            return;
        }

        if ((company.challengeFlags & CompanyFlags::bankrupt) != CompanyFlags::none)
        {
            bool hasAssets = false;
            for (auto& thought : company.aiThoughts)
            {
                if (thought.type == AiThoughtType::null)
                {
                    hasAssets = true;
                    break;
                }
            }
            if (!hasAssets)
            {
                for (auto& station : StationManager::stations())
                {
                    if (station.owner == company.id())
                    {
                        hasAssets = true;
                        break;
                    }
                }
            }
            if (!hasAssets)
            {
                company.var_4A4 = AiThinkState::unk10;
                company.var_85C4 = 0;
                company.var_85C6 = 0;
                return;
            }
        }

        company.var_85F6 = 0;
        company.var_4A4 = AiThinkState::unk1;
        company.var_2578 = 0xFF;
        tryRemovePortsAndAirports(company);
    }

    // 0x00487F8D
    static bool sub_487F8D(const Company& company, const AiThought& thought)
    {
        if ((company.challengeFlags & CompanyFlags::bankrupt) != CompanyFlags::none)
        {
            return true;
        }
        if (thought.var_88 < 3)
        {
            return false;
        }
        // 27 / 8 ???
        const auto val = thought.var_7C * 3;
        const auto val2 = val + (val / 8);
        return thought.var_84 < val2;
    }

    // 0x00488050
    static bool sub_488050(const Company& company, const AiThought& thought)
    {
        registers regs;
        regs.esi = X86Pointer(&company);
        regs.edi = X86Pointer(&thought);
        return call(0x00488050, regs) & X86_FLAG_CARRY;
    }

    // 0x00430971
    static void aiThinkState1(Company& company)
    {
        company.var_2578++;
        if (company.var_2578 < 60)
        {
            const auto& thought = company.aiThoughts[company.var_2578];
            if (thought.type == AiThoughtType::null)
            {
                aiThinkState1(company);
                return;
            }

            if (sub_487F8D(company, thought))
            {
                company.var_4A4 = AiThinkState::unk7;
                company.var_4A5 = 0;
                StationManager::sub_437F29(company.id(), 8);
                return;
            }

            if (sub_488050(company, thought))
            {
                company.var_4A4 = AiThinkState::unk8;
                company.var_4A5 = 0;
            }
            return;
        }
        if (((company.challengeFlags & CompanyFlags::unk0) != CompanyFlags::none)
            || (getCurrentDay() - company.startedDate <= 42))
        {
            company.var_4A4 = AiThinkState::unk2;
            company.var_4A5 = 0;
            return;
        }
        CompanyManager::aiDestroy(company.id());
    }

    // 0x004309FD
    static void aiThinkState2(Company& company)
    {
        registers regs;
        regs.esi = X86Pointer(&company);
        call(0x004309FD, regs);
    }

    // 0x00430DB6
    static void aiThinkState3(Company& company)
    {
        registers regs;
        regs.esi = X86Pointer(&company);
        call(0x00430DB6, regs);
    }

    // 0x0047BA2C
    static uint32_t sub_47BA2C(World::Pos3 pos, uint8_t rotation, uint8_t roadObjectId, uint32_t unk, uint8_t flags)
    {
        registers regs;
        regs.ax = pos.x;
        regs.cx = pos.y;
        regs.di = pos.z;
        regs.bl = flags;
        regs.bh = rotation;
        regs.bp = roadObjectId;
        regs.edx = unk;
        call(0x0047BA2C, regs);
        return regs.ebx;
    }

    // 0x004866C8
    static uint8_t sub_4866C8(const Company& company, AiThought& thought)
    {
        if ((company.challengeFlags & CompanyFlags::bankrupt) != CompanyFlags::none)
        {
            return 2;
        }

        bool found = false;
        auto i = 0U;
        for (; i < thought.var_03; ++i)
        {
            if (!(thought.var_06[i].var_02 & (1U << 0)))
            {
                continue;
            }
            if (thought.var_06[i].var_02 & (1U << 1))
            {
                continue;
            }
            found = true;
            break;
        }

        if (!found)
        {
            return 1;
        }

        auto& stationUnk = thought.var_06[i];
        const auto pos = World::Pos3(stationUnk.pos, stationUnk.baseZ * World::kSmallZStep);
        if (kThoughtTypeFlags[enumValue(thought.type)] & (1U << 15))
        {
            {
                GameCommands::AirportRemovalArgs removeArgs{};
                removeArgs.pos = pos;
                GameCommands::doCommand(removeArgs, GameCommands::Flags::apply | GameCommands::Flags::noPayment | GameCommands::Flags::flag_4);
            }

            GameCommands::AirportPlacementArgs placeArgs{};
            placeArgs.pos = pos;
            placeArgs.rotation = stationUnk.rotation;
            placeArgs.type = thought.var_89;

            if (GameCommands::doCommand(placeArgs, GameCommands::Flags::apply) == GameCommands::FAILURE)
            {
                if (GameCommands::doCommand(placeArgs, GameCommands::Flags::apply | GameCommands::Flags::noPayment) == GameCommands::FAILURE)
                {
                    return 2;
                }
            }

            if (_lastPlacedAirportStationId != StationId::null)
            {
                stationUnk.var_00 = _lastPlacedAirportStationId;
            }
        }
        else if (kThoughtTypeFlags[enumValue(thought.type)] & (1U << 16))
        {
            {
                GameCommands::PortRemovalArgs removeArgs{};
                removeArgs.pos = pos;
                GameCommands::doCommand(removeArgs, GameCommands::Flags::apply | GameCommands::Flags::noPayment | GameCommands::Flags::flag_4);
            }

            GameCommands::PortPlacementArgs placeArgs{};
            placeArgs.pos = pos;
            placeArgs.rotation = stationUnk.rotation;
            placeArgs.type = thought.var_89;

            if (GameCommands::doCommand(placeArgs, GameCommands::Flags::apply) == GameCommands::FAILURE)
            {
                if (GameCommands::doCommand(placeArgs, GameCommands::Flags::apply | GameCommands::Flags::noPayment) == GameCommands::FAILURE)
                {
                    return 2;
                }
            }

            if (_lastPlacedPortStationId != StationId::null)
            {
                stationUnk.var_00 = _lastPlacedPortStationId;
            }
        }
        else
        {
            if (thought.trackObjId & (1U << 7))
            {
                // Road

                // TODO: Vanilla bug passes rotation for flags???
                if (sub_47BA2C(pos, stationUnk.rotation, thought.trackObjId & ~(1U << 7), 0, stationUnk.rotation) == GameCommands::FAILURE)
                {
                    return 2;
                }

                if (_lastPlacedTrackStationId != StationId::null)
                {
                    stationUnk.var_00 = _lastPlacedTrackStationId;
                }
            }
            else
            {
                // Track
                GameCommands::Unk52Args placeArgs{};
                placeArgs.rotation = stationUnk.rotation;
                placeArgs.trackObjectId = thought.trackObjId;
                placeArgs.unk = 0;
                placeArgs.pos = pos;

                for (auto j = 0U; j < thought.var_04; ++j)
                {
                    if (GameCommands::doCommand(placeArgs, GameCommands::Flags::apply) == GameCommands::FAILURE)
                    {
                        if (GameCommands::doCommand(placeArgs, GameCommands::Flags::apply | GameCommands::Flags::noPayment) == GameCommands::FAILURE)
                        {
                            return 2;
                        }
                    }

                    if (_lastPlacedTrackStationId != StationId::null)
                    {
                        stationUnk.var_00 = _lastPlacedTrackStationId;
                    }
                    placeArgs.pos += World::Pos3{ _503C6C[placeArgs.rotation], 0 };
                }
            }
        }
        stationUnk.var_02 &= ~(1 << 0);
        stationUnk.var_02 |= (1 << 1);
        return 0;
    }

    // 0x004FE7AC
    constexpr std::array<uint8_t, 9> kAgrressivenessTable1 = { 25, 20, 15, 11, 8, 5, 3, 2, 1 };

    // 0x004869C2
    static void sub_4869C2(Company& company)
    {
        company.var_85C2 = 0xFF;
        company.var_85C3 = 0;
        company.var_85DE = 0;
        const auto* competitorObj = ObjectManager::get<CompetitorObject>(company.competitorId);
        company.var_85EA = kAgrressivenessTable1[competitorObj->aggressiveness - 1];
    }

    // 0x0043106B
    static void sub_43106B(Company& company, AiThought& thought)
    {
        const auto res = sub_4866C8(company, thought);
        if (res == 2)
        {
            company.var_4A4 = AiThinkState::unk6;
            company.var_4A5 = 1;
            sub_487144(company);
        }
        else if (res == 1)
        {
            company.var_4A5 = 1;
            sub_4869C2(company);
        }
    }

    // 0x004869F7
    static uint8_t sub_4869F7(const Company& company, const AiThought& thought)
    {
        // gc_unk_52?
        registers regs;
        regs.esi = X86Pointer(&company);
        regs.edi = X86Pointer(&thought);
        call(0x004869F7, regs);
        return regs.al;
    }

    // 0x0043109A
    static void sub_43109A(Company& company, AiThought& thought)
    {
        const auto res = sub_4869F7(company, thought);
        if (res == 2)
        {
            company.var_4A4 = AiThinkState::unk6;
            company.var_4A5 = 1;
            sub_487144(company);
        }
        else if (res == 1)
        {
            company.var_4A5 = 2;
        }
    }

    // 0x004310C4
    static void sub_4310C4(Company& company, AiThought& thought)
    {
        const auto res = sub_486ECF(company, thought);
        if (res == 2)
        {
            company.var_4A4 = AiThinkState::unk6;
            company.var_4A5 = 0;
        }
        else if (res == 1)
        {
            company.var_4A5 = 3;
        }
    }

    // 0x004310E9
    static void sub_4310E9(Company& company, AiThought& thought)
    {
        sub_4876CB(thought);
        StationManager::sub_437F29(company.id(), 1);
        company.var_4A4 = AiThinkState::unk0;
    }

    using AiThinkState4Function = void (*)(Company&, AiThought&);

    static constexpr std::array<AiThinkState4Function, 4> _funcs_4F9500 = {
        sub_43106B,
        sub_43109A,
        sub_4310C4,
        sub_4310E9,
    };

    // 0x00431035
    static void aiThinkState4(Company& company)
    {
        StationManager::sub_437F29(company.id(), 3);
        company.var_85F6++;

        _funcs_4F9500[company.var_4A5](company, company.aiThoughts[company.var_2578]);
    }

    static void nullsub_3([[maybe_unused]] Company& company)
    {
    }

    // 0x00431104
    static void aiThinkState6(Company& company)
    {
        // try sell a vehicle?
        registers regs;
        regs.esi = X86Pointer(&company);
        call(0x00431104, regs);
    }

    // 0x00487BA3
    static bool sub_487BA3(AiThought& thought)
    {
        // sell a vehicle ??
        registers regs;
        regs.edi = X86Pointer(&thought);
        return call(0x00487BA3, regs) & X86_FLAG_CARRY;
    }

    // 0x004311B5
    static void sub_4311B5(Company& company, AiThought& thought)
    {
        if (sub_487BA3(thought))
        {
            company.var_4A5 = 1;
            sub_487144(company);
        }
    }

    // 0x0048715C
    static bool sub_48715C(Company& company, AiThought& thought)
    {
        // fair amount of logic
        // removes roads/tracks
        registers regs;
        regs.esi = X86Pointer(&company);
        regs.edi = X86Pointer(&thought);
        return call(0x0048715C, regs) & X86_FLAG_CARRY;
    }

    // 0x004311CA
    static void sub_4311CA(Company& company, AiThought& thought)
    {
        if (sub_48715C(company, thought))
        {
            company.var_4A5 = 2;
        }
    }

    // 0x00483778
    static void clearThought(AiThought& thought)
    {
        thought.type = AiThoughtType::null;
    }

    // 0x004311DA
    static void sub_4311DA(Company& company, AiThought& thought)
    {
        clearThought(thought);
        company.var_4A4 = AiThinkState::unk1;
    }

    using AiThinkState7Function = void (*)(Company&, AiThought&);

    static constexpr std::array<AiThinkState7Function, 3> _funcs_4F9524 = {
        sub_4311B5,
        sub_4311CA,
        sub_4311DA,
    };

    // 0x00431193
    static void aiThinkState7(Company& company)
    {
        _funcs_4F9524[company.var_4A5](company, company.aiThoughts[company.var_2578]);
    }

    // 0x00487C83
    static void sub_487C83(AiThought& thought)
    {
        // Gets refund costs for vehicles and costs for track mods
        registers regs;
        regs.edi = X86Pointer(&thought);
        call(0x00487C83, regs);
    }

    // 0x00431209
    static void sub_431209(Company& company, AiThought& thought)
    {
        sub_487C83(thought);
        company.var_4A5 = 1;
    }

    // 0x00431216
    static void sub_431216(Company& company, AiThought&)
    {
        // branch on sub_487E6D (which is a nop) would have made var_4A4 = 1
        company.var_4A5 = 2;
    }

    // 0x00487DAD
    static uint32_t tryPlaceTrackOrRoadMods(AiThought& thought, uint8_t flags)
    {
        if (thought.trackObjId & (1U << 7))
        {
            GameCommands::RoadModsPlacementArgs args{};
            args.pos = World::Pos3(thought.var_06[0].pos, thought.var_06[0].baseZ * World::kSmallZStep);
            args.rotation = thought.var_06[0].rotation;
            args.roadObjType = thought.trackObjId & ~(1U << 7);
            args.index = 0;
            args.modSection = 2;
            args.roadId = 0;
            args.type = thought.mods;
            const auto cost = GameCommands::doCommand(args, flags);
            if (cost != GameCommands::FAILURE)
            {
                return cost;
            }
            return GameCommands::doCommand(args, flags | GameCommands::Flags::noPayment);
        }
        else
        {
            GameCommands::TrackModsPlacementArgs args{};
            args.pos = World::Pos3(thought.var_06[0].pos, thought.var_06[0].baseZ * World::kSmallZStep);
            args.rotation = thought.var_06[0].rotation;
            args.trackObjType = thought.trackObjId;
            args.index = 0;
            args.modSection = 2;
            args.trackId = 0;
            args.type = thought.mods;
            const auto cost = GameCommands::doCommand(args, flags);
            if (cost != GameCommands::FAILURE)
            {
                return cost;
            }
            return GameCommands::doCommand(args, flags | GameCommands::Flags::noPayment);
        }
    }

    // 0x00487E74
    static bool sub_487E74(Company& company, AiThought& thought)
    {
        if ((company.challengeFlags & CompanyFlags::bankrupt) != CompanyFlags::none)
        {
            return true;
        }

        if (!(thought.var_8B & (1U << 3)))
        {
            return false;
        }

        return tryPlaceTrackOrRoadMods(thought, GameCommands::Flags::apply) == GameCommands::FAILURE;
    }

    // 0x0043122D
    static void sub_43122D(Company& company, AiThought& thought)
    {
        if (sub_487E74(company, thought))
        {
            company.var_4A4 = AiThinkState::unk1;
        }
        else
        {
            company.var_4A5 = 3;
        }
    }

    // 0x00487EA0
    static bool sub_487EA0(AiThought& thought)
    {
        // some sort of sell of vehicle
        registers regs;
        regs.edi = X86Pointer(&thought);
        return call(0x00487EA0, regs) & X86_FLAG_CARRY;
    }

    // 0x00431244
    static void sub_431244(Company& company, AiThought& thought)
    {
        if (sub_487EA0(thought))
        {
            company.var_4A5 = 4;
        }
    }

    // 0x00431254
    static void sub_431254(Company& company, AiThought& thought)
    {
        const auto res = sub_486ECF(company, thought);
        if (res == 2)
        {
            company.var_4A4 = AiThinkState::unk7;
            company.var_4A5 = 0;
        }
        else if (res == 1)
        {
            company.var_4A5 = 5;
        }
    }

    // 0x00431279
    static void sub_431279(Company& company, AiThought& thought)
    {
        sub_4876CB(thought);
        company.var_4A4 = AiThinkState::unk1;
    }

    using Unk4311E7ThinkFunction = void (*)(Company&, AiThought&);

    static constexpr std::array<Unk4311E7ThinkFunction, 6> _funcs_4F9530 = {
        sub_431209,
        sub_431216,
        sub_43122D,
        sub_431244,
        sub_431254,
        sub_431279,
    };

    // 0x004311E7
    static void aiThinkState8(Company& company)
    {
        _funcs_4F9530[company.var_4A5](company, company.aiThoughts[company.var_2578]);
    }

    static void nullsub_4([[maybe_unused]] Company& company)
    {
    }

    // 0x00431287
    static void aiThinkState10(Company& company)
    {
        registers regs;
        regs.esi = X86Pointer(&company);
        call(0x00431287, regs);
    }

    using UnknownThinkFunction = void (*)(Company&);

    static constexpr std::array<UnknownThinkFunction, 11> _funcs_430786 = {
        aiThinkState0,
        aiThinkState1,
        aiThinkState2,
        aiThinkState3,
        aiThinkState4,
        nullsub_3,
        aiThinkState6,
        aiThinkState7,
        aiThinkState8,
        nullsub_4,
        aiThinkState10,
    };

    // 0x00431295
    static void sub_431295(Company& company)
    {
        company.var_4A6 = 1;
    }

    // 0x0043129D
    static void sub_43129D(Company& company)
    {
        company.var_4A6 = 2;
        company.var_259E = 0;
    }

    // 0x00487784
    static bool tryPlaceVehicles(Company& company)
    {
        registers regs;
        regs.esi = X86Pointer(&company);
        return call(0x00487784, regs) & X86_FLAG_CARRY;
    }

    // 0x004312AF
    static void sub_4312AF(Company& company)
    {
        if (tryPlaceVehicles(company))
        {
            company.var_4A6 = 3;
        }
    }

    // 0x004312BF
    static void sub_4312BF(Company& company)
    {
        company.var_4A6 = 0;
    }

    static void callThinkFunc2(Company& company)
    {
        switch (company.var_4A6)
        {
            case 0:
                sub_431295(company);
                break;
            case 1:
                sub_43129D(company);
                break;
            case 2:
                sub_4312AF(company);
                break;
            case 3:
                sub_4312BF(company);
                break;
            default:
                assert(false);
                return;
        }
    }

    // 0x00430762
    void aiThink(const CompanyId id)
    {
        // const auto updatingCompanyId = CompanyManager::getUpdatingCompanyId();

        // Ensure this is only used for Non-Player controlled companies.
        if (CompanyManager::isPlayerCompany(id))
            return;

        auto* company = CompanyManager::get(id);

        const auto thinkFunc1 = _funcs_430786[enumValue(company->var_4A4)];
        thinkFunc1(*company);

        if (company->empty())
            return;

        callThinkFunc2(*company);

        if (company->headquartersX != -1 || (company->challengeFlags & CompanyFlags::bankrupt) != CompanyFlags::none || (company->challengeFlags & CompanyFlags::unk0) == CompanyFlags::none)
        {
            return;
        }

        // Look for an entry with either town or industry assigned.
        auto index = std::size(company->aiThoughts);
        while (company->aiThoughts[--index].type == AiThoughtType::null)
        {
            if (index == 0)
                return;
        }

        auto& thought = company->aiThoughts[index];

        World::Pos2 pos;
        if ((kThoughtTypeFlags[enumValue(thought.type)] & 2) != 0)
        {
            auto* industry = IndustryManager::get(static_cast<IndustryId>(thought.var_01));
            pos = { industry->x, industry->y };
        }
        else
        {
            auto* town = TownManager::get(static_cast<TownId>(thought.var_01));
            pos = { town->x, town->y };
        }

        auto& prng = gPrng1();
        const auto randPick = prng.randNext();
        // Random tile position 32x32 tiles centered on 0,0 i.e. +-16 tiles
        const auto randPos = World::Pos2{
            static_cast<coord_t>(randPick & 0x3E0),
            static_cast<coord_t>(Numerics::ror<uint32_t>(randPick, 5) & 0x3E0)
        } - World::toWorldSpace(World::TilePos2{ 16, 16 });

        const auto selectedPos = randPos + pos;
        if (World::validCoords(selectedPos))
        {
            auto tile = World::TileManager::get(selectedPos);
            auto* surface = tile.surface();

            coord_t z = surface->baseHeight();
            if (surface->slope() != 0)
                z += 16;

            const auto rot = (Numerics::ror<uint32_t>(randPick, 10)) & 0x3;
            const auto buildingType = CompanyManager::getHeadquarterBuildingType();

            GameCommands::HeadquarterPlacementArgs args;
            args.pos = World::Pos3(selectedPos, z);
            args.rotation = rot;
            args.type = buildingType;
            GameCommands::doCommand(args, GameCommands::Flags::apply);
        }
    }
}
