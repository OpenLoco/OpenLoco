#pragma once

#include "Economy/Currency.h"
#include "Entities/Entity.h"
#include "Map/Tile.h"
#include "Objects/Object.h"
#include "World/Company.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    enum ExpenditureType : uint8_t;
    enum class GameSpeed : uint8_t;
    enum class LoadOrQuitMode : uint16_t;
}

namespace OpenLoco::Vehicles
{
    struct VehicleHead;
}

namespace OpenLoco::GameCommands
{
    namespace Flags
    {
        constexpr uint8_t apply = 1 << 0;         // 0x01
        constexpr uint8_t flag_1 = 1 << 1;        // 0x02
        constexpr uint8_t flag_2 = 1 << 2;        // 0x04
        constexpr uint8_t noErrorWindow = 1 << 3; // 0x08 do not show an error window even on failure (use this with ghosts)
        constexpr uint8_t flag_4 = 1 << 4;        // 0x10
        constexpr uint8_t noPayment = 1 << 5;     // 0x20 calculates cost but does not deduct it
        constexpr uint8_t ghost = 1 << 6;         // 0x40
        constexpr uint8_t flag_7 = 1 << 7;        // 0x80
    }

    enum class GameCommand : uint8_t
    {
        vehicleRearrange = 0,
        vehiclePlace = 1,
        vehiclePickup = 2,
        vehicleReverse = 3,
        vehiclePassSignal = 4,
        vehicleCreate = 5,
        vehicleSell = 6,
        createTrack = 7,
        removeTrack = 8,
        changeLoan = 9,
        vehicleRename = 10,
        changeStationName = 11,
        vehicleChangeRunningMode = 12,
        createSignal = 13,
        removeSignal = 14,
        createTrainStation = 15,
        removeTrackStation = 16,
        createTrackMod = 17,
        removeTrackMod = 18,
        changeCompanyColourScheme = 19,
        pauseGame = 20,
        loadSaveQuitGame = 21,
        removeTree = 22,
        createTree = 23,
        changeLandMaterial = 24,
        raiseLand = 25,
        lowerLand = 26,
        lowerRaiseLandMountain = 27,
        raiseWater = 28,
        lowerWater = 29,
        changeCompanyName = 30,
        changeCompanyOwnerName = 31,
        createWall = 32,
        removeWall = 33,
        gc_unk_34 = 34,
        vehicleOrderInsert = 35,
        vehicleOrderDelete = 36,
        vehicleOrderSkip = 37,
        createRoad = 38,
        removeRoad = 39,
        createRoadMod = 40,
        removeRoadMod = 41,
        createRoadStation = 42,
        removeRoadStation = 43,
        createBuilding = 44,
        removeBuilding = 45,
        renameTown = 46,
        createIndustry = 47,
        removeIndustry = 48,
        createTown = 49,
        removeTown = 50,
        gc_unk_51 = 51,
        gc_unk_52 = 52,
        gc_unk_53 = 53,
        buildCompanyHeadquarters = 54,
        removeCompanyHeadquarters = 55,
        createAirport = 56,
        removeAirport = 57,
        vehiclePlaceAir = 58,
        vehiclePickupAir = 59,
        createPort = 60,
        removePort = 61,
        vehiclePlaceWater = 62,
        vehiclePickupWater = 63,
        vehicleRefit = 64,
        changeCompanyFace = 65,
        clearLand = 66,
        loadMultiplayerMap = 67,
        gc_unk_68 = 68,
        gc_unk_69 = 69,
        gc_unk_70 = 70,
        sendChatMessage = 71,
        multiplayerSave = 72,
        updateOwnerStatus = 73,
        vehicleSpeedControl = 74,
        vehicleOrderUp = 75,
        vehicleOrderDown = 76,
        vehicleApplyShuntCheat = 77,
        applyFreeCashCheat = 78,
        renameIndustry = 79,
        vehicleClone = 80,
        cheat = 81,
        setGameSpeed = 82,
        vehicleOrderReverse = 83,
    };

    constexpr uint32_t FAILURE = 0x80000000;

    void registerHooks();
    uint32_t doCommand(GameCommand command, const registers& registers);
    uint32_t doCommandForReal(GameCommand command, CompanyId company, const registers& registers);
    bool sub_431E6A(const CompanyId company, const World::TileElement* const tile = nullptr);

    template<typename T>
    uint32_t doCommand(const T& args, uint8_t flags)
    {
        registers regs = registers(args);
        regs.bl = flags;
        return doCommand(T::command, regs);
    }

    struct VehicleRearrangeArgs
    {
        static constexpr auto command = GameCommand::vehicleRearrange;

        VehicleRearrangeArgs() = default;
        explicit VehicleRearrangeArgs(const registers& regs)
            : source(static_cast<EntityId>(regs.dx))
            , dest(static_cast<EntityId>(regs.di))
        {
        }

        EntityId source;
        EntityId dest;

        explicit operator registers() const
        {
            registers regs;
            regs.di = enumValue(dest);
            regs.dx = enumValue(source);
            return regs;
        }
    };

    struct VehiclePlacementArgs
    {
        static constexpr auto command = GameCommand::vehiclePlace;

        VehiclePlacementArgs() = default;
        explicit VehiclePlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.dx * World::kSmallZStep)
            , trackAndDirection(regs.bp)
            , trackProgress(regs.ebx >> 16)
            , head(EntityId(regs.di))
            , convertGhost((regs.ebx >> 16) == 0xFFFF)
        {
        }

        World::Pos3 pos;
        uint16_t trackAndDirection;
        uint16_t trackProgress;
        EntityId head;
        bool convertGhost = false;

        explicit operator registers() const
        {
            registers regs;
            regs.ebp = trackAndDirection;
            regs.di = enumValue(head);
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.dx = pos.z / World::kSmallZStep;
            regs.ebx = convertGhost ? 0xFFFF0000 : (trackProgress << 16);
            return regs;
        }
    };

    struct TrackPlacementArgs
    {
        static constexpr auto command = GameCommand::createTrack;

        TrackPlacementArgs() = default;
        explicit TrackPlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh & 0x3)
            , trackId(regs.dh & 0x3F)
            , mods(regs.di >> 16)
            , bridge(regs.edx >> 24)
            , trackObjectId(regs.dl)
            , unk(regs.edi & 0x800000)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t trackId;
        uint8_t mods;
        uint8_t bridge;
        uint8_t trackObjectId;
        bool unk;

        explicit operator registers() const
        {
            registers regs;
            regs.eax = pos.x;
            regs.cx = pos.y;
            regs.edi = (0xFFFF & pos.z) | (mods << 16) | (unk ? 0x800000 : 0);
            regs.bh = rotation;
            regs.edx = trackObjectId | (trackId << 8) | (bridge << 24);
            return regs;
        }
    };

    struct TrackRemovalArgs
    {
        static constexpr auto command = GameCommand::removeTrack;

        TrackRemovalArgs() = default;
        explicit TrackRemovalArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh & 0x3)
            , trackId(regs.dl & 0x3F)
            , index(regs.dh)
            , trackObjectId(regs.ebp)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t trackId;
        uint8_t index;
        uint8_t trackObjectId;

        explicit operator registers() const
        {
            registers regs;
            regs.eax = pos.x;
            regs.cx = pos.y;
            regs.di = pos.z;
            regs.bh = rotation;
            regs.dl = trackId;
            regs.dh = index;
            regs.ebp = trackObjectId;
            return regs;
        }
    };

    struct SignalRemovalArgs
    {
        static constexpr auto command = GameCommand::removeSignal;

        SignalRemovalArgs() = default;
        explicit SignalRemovalArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh & 0x3)
            , trackId(regs.dl & 0x3F)
            , index(regs.dh & 0xF)
            , type(regs.bp & 0xF)
            , flags(regs.edi >> 16)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t trackId;
        uint8_t index;
        uint8_t type;
        uint16_t flags;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.edi = pos.z | (flags << 16);
            regs.bh = rotation;
            regs.dl = trackId;
            regs.dh = index;
            regs.bp = type;
            return regs;
        }
    };

    struct TrackStationPlacementArgs
    {
        static constexpr auto command = GameCommand::createTrainStation;

        TrackStationPlacementArgs() = default;
        explicit TrackStationPlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh & 0x3)
            , trackId(regs.dl & 0xF)
            , index(regs.dh & 0x3)
            , trackObjectId(regs.bp)
            , type(regs.edi >> 16)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t trackId;
        uint8_t index;
        uint8_t trackObjectId;
        uint8_t type;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.edi = pos.z | (type << 16);
            regs.bh = rotation;
            regs.dl = trackId;
            regs.dh = index;
            regs.bp = trackObjectId;
            return regs;
        }
    };

    struct TrackStationRemovalArgs
    {
        static constexpr auto command = GameCommand::removeTrackStation;

        TrackStationRemovalArgs() = default;
        explicit TrackStationRemovalArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh & 0x3)
            , trackId(regs.dl & 0x3F)
            , index(regs.dh & 0xF)
            , type(regs.bp & 0xF)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t trackId;
        uint8_t index;
        uint8_t type;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.di = pos.z;
            regs.bh = rotation;
            regs.dl = trackId;
            regs.dh = index;
            regs.bp = type;
            return regs;
        }
    };

    struct LowerRaiseLandMountainArgs
    {
        static constexpr auto command = GameCommand::lowerRaiseLandMountain;
        LowerRaiseLandMountainArgs() = default;
        explicit LowerRaiseLandMountainArgs(const registers& regs)
            : centre(regs.ax, regs.cx)
            , pointA(regs.dx, regs.bp)
            , pointB(regs.edx >> 16, regs.ebp >> 16)
            , di(regs.di)
        {
        }

        World::Pos2 centre;
        World::Pos2 pointA;
        World::Pos2 pointB;
        uint16_t di;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = centre.x;
            regs.cx = centre.y;
            regs.edx = (pointB.x << 16) | pointA.x;
            regs.ebp = (pointB.y << 16) | pointA.y;
            regs.di = di;
            return regs;
        }
    };

    struct RoadPlacementArgs
    {
        static constexpr auto command = GameCommand::createRoad;

        RoadPlacementArgs() = default;
        explicit RoadPlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh & 0x3)
            , roadId(regs.dh & 0xF)
            , mods(regs.di >> 16)
            , bridge(regs.edx >> 24)
            , roadObjectId(regs.dl)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t roadId;
        uint8_t mods;
        uint8_t bridge;
        uint8_t roadObjectId;

        explicit operator registers() const
        {
            registers regs;
            regs.eax = pos.x;
            regs.cx = pos.y;
            regs.edi = pos.z | (mods << 16);
            regs.bh = rotation;
            regs.edx = roadObjectId | (roadId << 8) | (bridge << 24);
            return regs;
        }
    };

    struct RoadRemovalArgs
    {
        static constexpr auto command = GameCommand::removeRoad;

        RoadRemovalArgs() = default;
        explicit RoadRemovalArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , unkDirection(regs.bh & 0x3)
            , roadId(regs.dl & 0xF)
            , sequenceIndex(regs.dh & 0x3)
            , objectId(regs.bp & 0xF)
        {
        }

        World::Pos3 pos;
        uint8_t unkDirection;
        uint8_t roadId;
        uint8_t sequenceIndex;
        uint8_t objectId;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.di = pos.z;
            regs.bh = unkDirection;
            regs.dl = roadId;
            regs.dh = sequenceIndex;
            regs.bp = objectId;
            return regs;
        }
    };

    struct RoadModsPlacementArgs
    {
        static constexpr auto command = GameCommand::createRoadMod;

        RoadModsPlacementArgs() = default;
        explicit RoadModsPlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh & 0x3)
            , roadId(regs.dl & 0xF)
            , index(regs.dh & 0x3)
            , type((regs.edi >> 16) & 0xF)
            , roadObjType(regs.ebp & 0xFF)
            , modSection((regs.ebp >> 16) & 0xFF)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t roadId;
        uint8_t index;
        uint8_t type;
        uint8_t roadObjType;
        uint8_t modSection;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.bh = rotation;
            regs.dl = roadId;
            regs.dh = index;
            regs.edi = pos.z | (type << 16);
            regs.ebp = roadObjType | (modSection << 16);
            return regs;
        }
    };

    struct RoadModsRemovalArgs
    {
        static constexpr auto command = GameCommand::removeRoadMod;

        RoadModsRemovalArgs() = default;
        explicit RoadModsRemovalArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh & 0x3)
            , roadId(regs.dl & 0xF)
            , index(regs.dh & 0x3)
            , type((regs.edi >> 16) & 0xF)
            , roadObjType(regs.ebp & 0xFF)
            , modSection((regs.ebp >> 16) & 0xFF)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t roadId;
        uint8_t index;
        uint8_t type;
        uint8_t roadObjType;
        uint8_t modSection;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.bh = rotation;
            regs.dl = roadId;
            regs.dh = index;
            regs.edi = pos.z | (type << 16);
            regs.ebp = roadObjType | (modSection << 16);
            return regs;
        }
    };

    struct RoadStationPlacementArgs
    {
        static constexpr auto command = GameCommand::createRoadStation;

        RoadStationPlacementArgs() = default;
        explicit RoadStationPlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh & 0x3)
            , roadId(regs.dl & 0xF)
            , index(regs.dh & 0x3)
            , roadObjectId(regs.bp)
            , type(regs.edi >> 16)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t roadId;
        uint8_t index;
        uint8_t roadObjectId;
        uint8_t type;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.edi = pos.z | (type << 16);
            regs.bh = rotation;
            regs.dl = roadId;
            regs.dh = index;
            regs.bp = roadObjectId;
            return regs;
        }
    };

    struct RoadStationRemovalArgs
    {
        static constexpr auto command = GameCommand::removeRoadStation;

        RoadStationRemovalArgs() = default;
        explicit RoadStationRemovalArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh & 0x3)
            , roadId(regs.dl & 0xF)
            , index(regs.dh & 0x3)
            , type(regs.bp & 0xF)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t roadId;
        uint8_t index;
        uint8_t type;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.di = pos.z;
            regs.bh = rotation;
            regs.dl = roadId;
            regs.dh = index;
            regs.bp = type;
            return regs;
        }
    };

    struct BuildingPlacementArgs
    {
        static constexpr auto command = GameCommand::createBuilding;

        BuildingPlacementArgs() = default;
        explicit BuildingPlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh & 0x3)
            , type(regs.dl)
            , variation(regs.dh)
            , colour(static_cast<Colour>((regs.edi >> 16) & 0x1F))
            , buildImmediately(regs.bh & 0x80)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t type;
        uint8_t variation;
        Colour colour;
        bool buildImmediately = false; // No scaffolding required (editor mode)
        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.edi = pos.z | (enumValue(colour) << 16);
            regs.dl = type;
            regs.dh = variation;
            regs.bh = rotation | (buildImmediately ? 0x80 : 0);
            return regs;
        }
    };

    // Rename town
    inline void do_46(uint16_t cx, uint16_t ax, uint32_t edx, uint32_t ebp, uint32_t edi)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.cx = cx;   // town number or 0
        regs.ax = ax;   // [ 0, 1, 2]
        regs.edx = edx; // part of name buffer
        regs.ebp = ebp; // part of name buffer
        regs.edi = edi; // part of name buffer
        doCommand(GameCommand::renameTown, regs);
    }

    struct Unk52Args
    {
        static constexpr auto command = GameCommand::gc_unk_52;

        Unk52Args() = default;
        explicit Unk52Args(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh & 0x3)
            , unk(regs.dx)
            , trackObjectId(regs.bp)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint16_t unk;
        uint8_t trackObjectId;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.di = pos.z;
            regs.bh = rotation;
            regs.dx = unk;
            regs.bp = trackObjectId;
            return regs;
        }
    };

    struct AirportPlacementArgs
    {
        static constexpr auto command = GameCommand::createAirport;

        AirportPlacementArgs() = default;
        explicit AirportPlacementArgs(const registers regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh)
            , type(regs.dl)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t type;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.di = pos.z;
            regs.bh = rotation;
            regs.dl = type;
            return regs;
        }
    };

    struct AirportRemovalArgs
    {
        static constexpr auto command = GameCommand::removeAirport;

        AirportRemovalArgs() = default;
        explicit AirportRemovalArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
        {
        }

        World::Pos3 pos;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.di = pos.z;
            return regs;
        }
    };

    struct VehicleAirPlacementArgs
    {
        static constexpr auto command = GameCommand::vehiclePlaceAir;

        VehicleAirPlacementArgs() = default;
        explicit VehicleAirPlacementArgs(const registers& regs)
            : stationId(StationId(regs.bp))
            , airportNode(regs.dl)
            , head(EntityId(regs.di))
            , convertGhost((regs.ebx >> 16) == 0xFFFF)
        {
        }

        StationId stationId;
        uint8_t airportNode;
        EntityId head;
        bool convertGhost = false;

        explicit operator registers() const
        {
            registers regs;
            regs.bp = enumValue(stationId);
            regs.di = enumValue(head);
            regs.dl = airportNode;
            regs.ebx = convertGhost ? 0xFFFF0000 : 0;
            return regs;
        }
    };

    struct PortPlacementArgs
    {
        static constexpr auto command = GameCommand::createPort;

        PortPlacementArgs() = default;
        explicit PortPlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh)
            , type(regs.dl)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t type;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.di = pos.z;
            regs.bh = rotation;
            regs.dl = type;
            return regs;
        }
    };

    struct PortRemovalArgs
    {
        static constexpr auto command = GameCommand::removePort;

        PortRemovalArgs() = default;
        explicit PortRemovalArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
        {
        }

        World::Pos3 pos;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.di = pos.z;
            return regs;
        }
    };

    struct VehicleWaterPlacementArgs
    {
        static constexpr auto command = GameCommand::vehiclePlaceWater;

        VehicleWaterPlacementArgs() = default;
        explicit VehicleWaterPlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.dx)
            , head(EntityId(regs.di))
            , convertGhost((regs.ebx >> 16) == 0xFFFF)
        {
        }

        World::Pos3 pos;
        EntityId head;
        bool convertGhost = false;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.dx = pos.z;
            regs.di = enumValue(head);
            regs.ebx = convertGhost ? 0xFFFF0000 : 0;
            return regs;
        }
    };

    // Load multiplayer map
    inline void do_67(const char* filename)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.ebp = X86Pointer(filename);
        doCommand(GameCommand::loadMultiplayerMap, regs);
    }

    // Multiplayer-related
    inline void do_69()
    {
        registers regs;
        regs.bl = Flags::apply;
        doCommand(GameCommand::gc_unk_69, regs);
    }

    // Multiplayer-related
    inline void do_70()
    {
        registers regs;
        regs.bl = Flags::apply;
        doCommand(GameCommand::gc_unk_70, regs);
    }

    // Send chat message
    inline void do_71(int32_t ax, const char* string)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.ax = ax;
        memcpy(&regs.ecx, &string[0], 4);
        memcpy(&regs.edx, &string[4], 4);
        memcpy(&regs.ebp, &string[8], 4);
        memcpy(&regs.edi, &string[12], 4);
        doCommand(GameCommand::sendChatMessage, regs);
    }

    // Multiplayer save
    inline void do_72()
    {
        registers regs;
        regs.bl = Flags::apply;
        doCommand(GameCommand::multiplayerSave, regs);
    }

    // Rename industry
    inline void do_79(IndustryId cl, uint16_t ax, uint32_t edx, uint32_t ebp, uint32_t edi)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.cl = enumValue(cl); // industry number or 0
        regs.ax = ax;            // [ 0, 1, 2]
        regs.edx = edx;          // part of name buffer
        regs.ebp = ebp;          // part of name buffer
        regs.edi = edi;          // part of name buffer
        doCommand(GameCommand::renameIndustry, regs);
    }

    inline bool do_80(EntityId head)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.ax = enumValue(head);
        return GameCommands::doCommand(GameCommand::vehicleClone, regs) != FAILURE;
    }

    enum class CheatCommand : uint8_t;

    inline bool do_81(CheatCommand command, int32_t param1 = 0, int32_t param2 = 0, int32_t param3 = 0)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.eax = static_cast<int32_t>(command);
        regs.ebx = param1;
        regs.ecx = param2;
        regs.edx = param3;
        return GameCommands::doCommand(GameCommand::cheat, regs) != FAILURE;
    }

    const World::Pos3& getPosition();
    void setPosition(const World::Pos3& pos);
    void setErrorText(const StringId message);
    StringId getErrorText();
    void setErrorTitle(const StringId title);
    ExpenditureType getExpenditureType();
    void setExpenditureType(const ExpenditureType type);
    CompanyId getUpdatingCompanyId();
    void setUpdatingCompanyId(CompanyId companyId);
    uint8_t getCommandNestLevel();
}
