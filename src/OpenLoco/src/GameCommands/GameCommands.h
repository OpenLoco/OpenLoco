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
        constexpr uint8_t apply = 1 << 0;  // 0x01
        constexpr uint8_t flag_1 = 1 << 1; // 0x02
        constexpr uint8_t flag_2 = 1 << 2; // 0x04
        constexpr uint8_t flag_3 = 1 << 3; // 0x08
        constexpr uint8_t flag_4 = 1 << 4; // 0x10
        constexpr uint8_t flag_5 = 1 << 5; // 0x20
        constexpr uint8_t flag_6 = 1 << 6; // 0x40
        constexpr uint8_t flag_7 = 1 << 7; // 0x80
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
    bool sub_431E6A(const CompanyId company, World::TileElement* const tile = nullptr);

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

    struct VehiclePickupArgs
    {
        static constexpr auto command = GameCommand::vehiclePickup;

        VehiclePickupArgs() = default;
        explicit VehiclePickupArgs(const registers& regs)
            : head(static_cast<EntityId>(regs.di))
        {
        }

        EntityId head;

        explicit operator registers() const
        {
            registers regs;
            regs.di = enumValue(head);
            return regs;
        }
    };

    struct VehicleReverseArgs
    {
        static constexpr auto command = GameCommand::vehicleReverse;

        VehicleReverseArgs() = default;
        explicit VehicleReverseArgs(const registers& regs)
            : head(static_cast<EntityId>(regs.dx))
        {
        }

        EntityId head;

        explicit operator registers() const
        {
            registers regs;
            regs.dx = enumValue(head);
            return regs;
        }
    };

    struct VehiclePassSignalArgs
    {
        static constexpr auto command = GameCommand::vehiclePassSignal;

        VehiclePassSignalArgs() = default;
        explicit VehiclePassSignalArgs(const registers& regs)
            : head(static_cast<EntityId>(regs.di))
        {
        }

        EntityId head;

        explicit operator registers() const
        {
            registers regs;
            regs.di = enumValue(head);
            return regs;
        }
    };

    struct VehicleCreateArgs
    {
        static constexpr auto command = GameCommand::vehicleCreate;

        VehicleCreateArgs() = default;
        explicit VehicleCreateArgs(const registers& regs)
            : vehicleId(static_cast<EntityId>(regs.di))
            , vehicleType(regs.dx)
        {
        }

        EntityId vehicleId; // Optional id representing where it will attach
        uint16_t vehicleType;

        explicit operator registers() const
        {
            registers regs;
            regs.di = enumValue(vehicleId);
            regs.edx = vehicleType;
            return regs;
        }
    };

    struct VehicleSellArgs
    {
        static constexpr auto command = GameCommand::vehicleSell;

        VehicleSellArgs() = default;
        explicit VehicleSellArgs(const registers& regs)
            : car(static_cast<EntityId>(regs.dx))
        {
        }

        EntityId car;

        explicit operator registers() const
        {
            registers regs;
            regs.dx = enumValue(car);
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

    struct ChangeLoanArgs
    {
        static constexpr auto command = GameCommand::changeLoan;
        ChangeLoanArgs() = default;
        explicit ChangeLoanArgs(const registers& regs)
            : newLoan(regs.edx)
        {
        }

        currency32_t newLoan;

        explicit operator registers() const
        {
            registers regs;
            regs.edx = newLoan;
            return regs;
        }
    };

    struct SetGameSpeedArgs
    {
        static constexpr auto command = GameCommand::setGameSpeed;
        SetGameSpeedArgs() = default;
        explicit SetGameSpeedArgs(const registers& regs)
            : newSpeed(static_cast<GameSpeed>(regs.edi))
        {
        }

        explicit SetGameSpeedArgs(const GameSpeed speed)
        {
            newSpeed = speed;
        }

        GameSpeed newSpeed;

        explicit operator registers() const
        {
            registers regs;
            regs.edi = static_cast<std::underlying_type_t<GameSpeed>>(newSpeed);
            return regs;
        }
    };

    struct VehicleRenameArgs
    {
        static constexpr auto command = GameCommand::vehicleRename;

        VehicleRenameArgs() = default;
        explicit VehicleRenameArgs(const registers& regs)
            : head(static_cast<EntityId>(regs.cx))
            , buffer{}
            , i(regs.ax)
        {
            // Copies it into the first 12 bytes not into the specific slot as per i
            std::memcpy(buffer, &regs.edx, 4);
            std::memcpy(buffer + 4, &regs.ebp, 4);
            std::memcpy(buffer + 8, &regs.edi, 4);
        }

        EntityId head;
        char buffer[37];
        uint16_t i;

        explicit operator registers() const
        {
            registers regs;
            regs.cx = enumValue(head);
            regs.ax = i;
            constexpr std::array<uint8_t, 3> iToOffset = { 24, 0, 12 };
            const auto offset = iToOffset[i];

            std::memcpy(&regs.edx, buffer + offset, 4);
            std::memcpy(&regs.ebp, buffer + offset + 4, 4);
            std::memcpy(&regs.edi, buffer + offset + 8, 4);
            return regs;
        }
    };

    struct RenameStationArgs
    {
        static constexpr auto command = GameCommand::changeStationName;

        RenameStationArgs() = default;
        explicit RenameStationArgs(const registers& regs)
            : stationId(StationId(regs.cx))
            , nameBufferIndex(regs.ax)
            , buffer{}
        {
            std::memcpy(buffer, &regs.edx, 4);
            std::memcpy(buffer + 4, &regs.ebp, 4);
            std::memcpy(buffer + 8, &regs.edi, 4);
        }

        StationId stationId;
        uint8_t nameBufferIndex;
        char buffer[37];

        explicit operator registers() const
        {
            registers regs;

            regs.cx = enumValue(stationId);
            regs.ax = nameBufferIndex;
            constexpr std::array<uint8_t, 3> iToOffset = { 24, 0, 12 };
            const auto offset = iToOffset[nameBufferIndex];

            std::memcpy(&regs.edx, buffer + offset, 4);
            std::memcpy(&regs.ebp, buffer + offset + 4, 4);
            std::memcpy(&regs.edi, buffer + offset + 8, 4);

            return regs;
        }
    };

    struct VehicleChangeRunningModeArgs
    {
        enum class Mode : uint8_t
        {
            stopVehicle,
            startVehicle,
            toggleLocalExpress,
            driveManually,
        };

        static constexpr auto command = GameCommand::vehicleChangeRunningMode;

        VehicleChangeRunningModeArgs() = default;
        explicit VehicleChangeRunningModeArgs(const registers& regs)
            : head(static_cast<EntityId>(regs.dx))
            , mode(static_cast<Mode>(regs.bh))
        {
        }

        EntityId head;
        Mode mode;

        explicit operator registers() const
        {
            registers regs;
            regs.dx = enumValue(head);
            regs.bh = enumValue(mode);
            return regs;
        }
    };

    struct SignalPlacementArgs
    {
        static constexpr auto command = GameCommand::createSignal;

        SignalPlacementArgs() = default;
        explicit SignalPlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh & 0x3)
            , trackId(regs.dl & 0x3F)
            , index(regs.dh & 0x3)
            , type((regs.edi >> 16) & 0xFF)
            , trackObjType(regs.ebp & 0xFF)
            , sides((regs.edi >> 16) & 0xC000)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t trackId;
        uint8_t index;
        uint8_t type;
        uint8_t trackObjType;
        uint16_t sides;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.bh = rotation;
            regs.dl = trackId;
            regs.dh = index;
            regs.edi = pos.z | (type << 16) | ((sides & 0xC000) << 16);
            regs.ebp = trackObjType;
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

    struct TrackModsPlacementArgs
    {
        static constexpr auto command = GameCommand::createTrackMod;

        TrackModsPlacementArgs() = default;
        explicit TrackModsPlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh & 0x3)
            , trackId(regs.dl & 0x3F)
            , index(regs.dh & 0x3)
            , type((regs.edi >> 16) & 0xF)
            , trackObjType(regs.ebp & 0xFF)
            , modSection((regs.ebp >> 16) & 0xFF)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t trackId;
        uint8_t index;
        uint8_t type;
        uint8_t trackObjType;
        uint8_t modSection;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.bh = rotation;
            regs.dl = trackId;
            regs.dh = index;
            regs.edi = pos.z | (type << 16);
            regs.ebp = trackObjType | (modSection << 16);
            return regs;
        }
    };

    struct TrackModsRemovalArgs
    {
        static constexpr auto command = GameCommand::removeTrackMod;

        TrackModsRemovalArgs() = default;
        explicit TrackModsRemovalArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh & 0x3)
            , trackId(regs.dl & 0x3F)
            , index(regs.dh & 0x3)
            , type((regs.edi >> 16) & 0xF)
            , trackObjType(regs.ebp & 0xFF)
            , modSection((regs.ebp >> 16) & 0xFF)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t trackId;
        uint8_t index;
        uint8_t type;
        uint8_t trackObjType;
        uint8_t modSection;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.bh = rotation;
            regs.dl = trackId;
            regs.dh = index;
            regs.edi = pos.z | (type << 16);
            regs.ebp = trackObjType | (modSection << 16);
            return regs;
        }
    };

    struct ChangeCompanyColourSchemeArgs
    {
        static constexpr auto command = GameCommand::changeCompanyColourScheme;

        ChangeCompanyColourSchemeArgs() = default;
        explicit ChangeCompanyColourSchemeArgs(const registers& regs)
            : companyId(CompanyId(regs.dl))
            , isPrimary()
            , value(regs.al)
            , colourType(regs.cl)
            , setColourMode(regs.dh)
        {
            if (!setColourMode)
            {
                isPrimary = regs.ah == 0;
            }
        }

        CompanyId companyId;
        bool isPrimary;
        uint8_t value;
        uint8_t colourType;
        bool setColourMode;

        explicit operator registers() const
        {
            registers regs;

            regs.cl = colourType;           // vehicle type or main
            regs.dh = setColourMode;        // [ 0, 1 ] -- 0 = set colour, 1 = toggle enabled/disabled;
            regs.dl = enumValue(companyId); // company id

            if (!setColourMode)
            {
                // cl is divided by 2 when used
                regs.ah = isPrimary ? 1 : 0; // [ 0, 1 ] -- primary or secondary palette
                regs.al = value;             // new colour
            }
            else if (setColourMode)
            {
                regs.al = value; // [ 0, 1 ] -- off or on
            }

            return regs;
        }
    };

    struct PauseGameArgs
    {
        static constexpr auto command = GameCommand::pauseGame;

        PauseGameArgs() = default;
        explicit PauseGameArgs(const registers&)
        {
        }

        explicit operator registers() const
        {
            return registers();
        }
    };

    struct LoadSaveQuitGameArgs
    {
        enum class Options : uint8_t
        {
            save,
            closeSavePrompt,
            dontSave,
        };
        static constexpr auto command = GameCommand::loadSaveQuitGame;

        LoadSaveQuitGameArgs() = default;
        explicit LoadSaveQuitGameArgs(const registers& regs)
            : option1(static_cast<Options>(regs.dl))
            , option2(static_cast<LoadOrQuitMode>(regs.di))
        {
        }

        Options option1;
        LoadOrQuitMode option2;

        explicit operator registers() const
        {
            registers regs;
            regs.dl = enumValue(option1); // [ 0 = save, 1 = close save prompt, 2 = don't save ]
            regs.di = enumValue(option2); // [ 0 = load game, 1 = return to title screen, 2 = quit to desktop ]
            return regs;
        }
    };

    struct TreeRemovalArgs
    {
        static constexpr auto command = GameCommand::removeTree;

        TreeRemovalArgs() = default;
        explicit TreeRemovalArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.dl * World::kSmallZStep)
            , type(regs.dh)
            , elementType(regs.bh)
        {
        }

        World::Pos3 pos;
        uint8_t type;
        uint8_t elementType;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.dl = pos.z / World::kSmallZStep;
            regs.dh = type;
            regs.bh = elementType;
            return regs;
        }
    };

    struct TreePlacementArgs
    {
        static constexpr auto command = GameCommand::createTree;

        TreePlacementArgs() = default;
        explicit TreePlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx)
            , rotation(regs.di & 0x3)
            , type(regs.bh)
            , quadrant(regs.dl)
            , colour(static_cast<Colour>(regs.dh & 0x1F))
            , buildImmediately(regs.di & 0x8000)
            , requiresFullClearance(regs.di & 0x4000)
        {
        }

        World::Pos2 pos;
        uint8_t rotation;
        uint8_t type;
        uint8_t quadrant;
        Colour colour;
        bool buildImmediately = false;
        bool requiresFullClearance = false;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.dl = quadrant;
            regs.dh = enumValue(colour);
            regs.di = rotation | (buildImmediately ? 0x8000 : 0) | (requiresFullClearance ? 0x4000 : 0);
            regs.bh = type;
            return regs;
        }
    };

    struct ChangeLandMaterialArgs
    {
        static constexpr auto command = GameCommand::changeLandMaterial;
        ChangeLandMaterialArgs() = default;
        explicit ChangeLandMaterialArgs(const registers& regs)
            : pointA(regs.ax, regs.cx)
            , pointB(regs.di, regs.bp)
            , landType(regs.dl)
        {
        }

        World::Pos2 pointA;
        World::Pos2 pointB;
        uint8_t landType;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pointA.x;
            regs.cx = pointA.y;
            regs.di = pointB.x;
            regs.bp = pointB.y;
            regs.dl = landType;
            return regs;
        }
    };

    struct RaiseLandArgs
    {
        static constexpr auto command = GameCommand::raiseLand;
        RaiseLandArgs() = default;
        explicit RaiseLandArgs(const registers& regs)
            : centre(regs.ax, regs.cx)
            , pointA(regs.dx, regs.bp)
            , pointB(regs.edx >> 16, regs.ebp >> 16)
            , corner(regs.di)
        {
        }

        World::Pos2 centre;
        World::Pos2 pointA;
        World::Pos2 pointB;
        uint16_t corner;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = centre.x;
            regs.cx = centre.y;
            regs.edx = pointB.x << 16 | pointA.x;
            regs.ebp = pointB.y << 16 | pointA.y;
            regs.di = corner;
            return regs;
        }
    };

    struct LowerLandArgs
    {
        static constexpr auto command = GameCommand::lowerLand;
        LowerLandArgs() = default;
        explicit LowerLandArgs(const registers& regs)
            : centre(regs.ax, regs.cx)
            , pointA(regs.dx, regs.bp)
            , pointB(regs.edx >> 16, regs.ebp >> 16)
            , corner(regs.di)
        {
        }

        World::Pos2 centre;
        World::Pos2 pointA;
        World::Pos2 pointB;
        uint16_t corner;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = centre.x;
            regs.cx = centre.y;
            regs.edx = pointB.x << 16 | pointA.x;
            regs.ebp = pointB.y << 16 | pointA.y;
            regs.di = corner;
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
            regs.edx = pointB.x << 16 | pointA.x;
            regs.ebp = pointB.y << 16 | pointA.y;
            regs.di = di;
            return regs;
        }
    };

    // Raise Water
    inline uint32_t do_28(World::Pos2 pointA, World::Pos2 pointB, uint8_t flags)
    {
        registers regs;
        regs.ax = pointA.x;
        regs.cx = pointA.y;
        regs.di = pointB.x;
        regs.bp = pointB.y;
        regs.bl = flags;
        return doCommand(GameCommand::raiseWater, regs);
    }

    // Lower Water
    inline uint32_t do_29(World::Pos2 pointA, World::Pos2 pointB, uint8_t flags)
    {
        registers regs;
        regs.ax = pointA.x;
        regs.cx = pointA.y;
        regs.di = pointB.x;
        regs.bp = pointB.y;
        regs.bl = flags;
        return doCommand(GameCommand::lowerWater, regs);
    }

    struct ChangeCompanyNameArgs
    {
        static constexpr auto command = GameCommand::changeCompanyName;

        ChangeCompanyNameArgs() = default;
        explicit ChangeCompanyNameArgs(const registers& regs)
            : companyId(CompanyId(regs.cx))
            , bufferIndex(regs.ax)
        {
            memcpy(buffer, &regs.edx, 4);
            memcpy(buffer + 4, &regs.ebp, 4);
            memcpy(buffer + 8, &regs.edi, 4);
        }

        CompanyId companyId;
        uint16_t bufferIndex;
        char buffer[37];

        explicit operator registers() const
        {
            registers regs;
            regs.cl = enumValue(companyId);
            regs.ax = bufferIndex; // [ 0, 1, 2]
            constexpr std::array<uint8_t, 3> iToOffset = { 24, 0, 12 };
            const auto offset = iToOffset[bufferIndex];

            std::memcpy(&regs.edx, buffer + offset, 4);
            std::memcpy(&regs.ebp, buffer + offset + 4, 4);
            std::memcpy(&regs.edi, buffer + offset + 8, 4);

            return regs;
        }
    };

    struct ChangeCompanyOwnerNameArgs
    {
        static constexpr auto command = GameCommand::changeCompanyOwnerName;

        ChangeCompanyOwnerNameArgs() = default;
        explicit ChangeCompanyOwnerNameArgs(const registers& regs)
            : companyId(CompanyId(regs.cx))
            , bufferIndex(regs.ax)
        {
            memcpy(newName, &regs.edx, 4);
            memcpy(newName + 4, &regs.ebp, 4);
            memcpy(newName + 8, &regs.edi, 4);
        }

        CompanyId companyId;
        uint16_t bufferIndex;
        char newName[37];

        explicit operator registers() const
        {
            registers regs;
            regs.cl = enumValue(companyId);
            regs.ax = bufferIndex; // [ 0, 1, 2]
            constexpr std::array<uint8_t, 3> iToOffset = { 24, 0, 12 };
            const auto offset = iToOffset[bufferIndex];

            std::memcpy(&regs.edx, newName + offset, 4);
            std::memcpy(&regs.ebp, newName + offset + 4, 4);
            std::memcpy(&regs.edi, newName + offset + 8, 4);

            return regs;
        }
    };

    struct WallPlacementArgs
    {
        static constexpr auto command = GameCommand::createWall;

        WallPlacementArgs() = default;
        explicit WallPlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.dl)
            , type(regs.bh)
            , unk(regs.dh)
            , primaryColour(static_cast<Colour>(regs.bp & 0x1F))
            , secondaryColour(static_cast<Colour>((regs.bp >> 8) & 0x1F))
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t type;
        uint8_t unk;
        Colour primaryColour;
        Colour secondaryColour;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.dl = rotation;
            regs.dh = unk;
            regs.di = pos.z;
            regs.bp = enumValue(primaryColour) | (enumValue(secondaryColour) << 8);
            regs.bh = type;
            return regs;
        }
    };

    struct WallRemovalArgs
    {
        static constexpr auto command = GameCommand::removeWall;

        WallRemovalArgs() = default;
        explicit WallRemovalArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.dh * World::kSmallZStep)
            , rotation(regs.dl)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.dh = pos.z / World::kSmallZStep;
            regs.dl = rotation;
            return regs;
        }
    };

    struct VehicleOrderInsertArgs
    {
        static constexpr auto command = GameCommand::vehicleOrderInsert;

        VehicleOrderInsertArgs() = default;
        explicit VehicleOrderInsertArgs(const registers& regs)
            : head(EntityId(regs.di))
            , orderOffset(regs.edx)
            , rawOrder((uint64_t(regs.cx) << 32ULL) | regs.eax)
        {
        }

        EntityId head;
        uint32_t orderOffset;
        uint64_t rawOrder;

        explicit operator registers() const
        {
            registers regs;
            regs.di = enumValue(head);
            regs.edx = orderOffset;
            regs.eax = rawOrder & 0xFFFFFFFF;
            regs.cx = rawOrder >> 32;
            return regs;
        }
    };

    struct VehicleOrderDeleteArgs
    {
        static constexpr auto command = GameCommand::vehicleOrderDelete;

        VehicleOrderDeleteArgs() = default;
        explicit VehicleOrderDeleteArgs(const registers& regs)
            : head(EntityId(regs.di))
            , orderOffset(regs.edx)
        {
        }

        EntityId head;
        uint32_t orderOffset;

        explicit operator registers() const
        {
            registers regs;
            regs.di = enumValue(head);
            regs.edx = orderOffset;
            return regs;
        }
    };

    struct VehicleOrderSkipArgs
    {
        static constexpr auto command = GameCommand::vehicleOrderSkip;

        VehicleOrderSkipArgs() = default;
        explicit VehicleOrderSkipArgs(const registers& regs)
            : head(EntityId(regs.di))
        {
        }

        EntityId head;

        explicit operator registers() const
        {
            registers regs;
            regs.di = enumValue(head);
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

    struct BuildingRemovalArgs
    {
        static constexpr auto command = GameCommand::removeBuilding;

        BuildingRemovalArgs() = default;
        explicit BuildingRemovalArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
        {
        }
        explicit BuildingRemovalArgs(const BuildingPlacementArgs& place)
            : pos(place.pos)
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

    struct IndustryPlacementArgs
    {
        static constexpr auto command = GameCommand::createIndustry;

        IndustryPlacementArgs() = default;
        explicit IndustryPlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx)
            , type(regs.dl)
            , buildImmediately(regs.dl & 0x80)
            , srand0(regs.ebp)
            , srand1(regs.edi)
        {
        }

        World::Pos2 pos;
        uint8_t type;
        bool buildImmediately = false; // No scaffolding required (editor mode)
        uint32_t srand0;
        uint32_t srand1;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.dl = type | (buildImmediately ? 0x80 : 0);
            regs.ebp = srand0;
            regs.edi = srand1;
            regs.esi = enumValue(command); // Vanilla bug? Investigate when doing createIndustry
            return regs;
        }
    };

    struct IndustryRemovalArgs
    {
        static constexpr auto command = GameCommand::removeIndustry;

        IndustryRemovalArgs() = default;
        explicit IndustryRemovalArgs(const registers& regs)
            : industryId(static_cast<IndustryId>(regs.dl))
        {
        }

        IndustryId industryId;

        explicit operator registers() const
        {
            registers regs;
            regs.dl = enumValue(industryId);
            return regs;
        }
    };

    struct TownPlacementArgs
    {
        static constexpr auto command = GameCommand::createTown;

        TownPlacementArgs() = default;
        explicit TownPlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx)
            , size(regs.dl)
        {
        }

        World::Pos2 pos;
        uint8_t size;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.edx = size;
            return regs;
        }
    };

    // Remove town
    inline bool do_50(uint8_t townId)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.edi = townId;
        return doCommand(GameCommand::removeTown, regs) != FAILURE;
    }

    struct HeadquarterPlacementArgs
    {
        static constexpr auto command = GameCommand::buildCompanyHeadquarters;

        HeadquarterPlacementArgs() = default;
        explicit HeadquarterPlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh & 0x3)
            , type(regs.dl)
            , buildImmediately(regs.bh & 0x80)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t type;
        bool buildImmediately = false; // No scaffolding required (editor mode)
        explicit operator registers() const
        {

            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.di = pos.z;
            regs.dx = type;
            regs.bh = rotation | (buildImmediately ? 0x80 : 0);
            return regs;
        }
    };

    struct HeadquarterRemovalArgs
    {
        static constexpr auto command = GameCommand::removeCompanyHeadquarters;

        HeadquarterRemovalArgs() = default;
        explicit HeadquarterRemovalArgs(const HeadquarterPlacementArgs& place)
            : pos(place.pos)
        {
        }
        explicit HeadquarterRemovalArgs(const registers& regs)
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

    inline bool do_59(EntityId head)
    {
        registers regs;
        regs.bl = Flags::apply | Flags::flag_3 | Flags::flag_6;
        regs.di = enumValue(head);
        return doCommand(GameCommand::vehiclePickupAir, regs) != FAILURE;
    }

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

    inline bool do_63(EntityId head)
    {
        registers regs;
        regs.bl = Flags::apply | Flags::flag_3 | Flags::flag_6;
        regs.di = enumValue(head);
        return doCommand(GameCommand::vehiclePickupWater, regs) != FAILURE;
    }

    struct VehicleRefitArgs
    {
        static constexpr auto command = GameCommand::vehicleRefit;

        VehicleRefitArgs() = default;
        explicit VehicleRefitArgs(const registers& regs)
            : head(static_cast<EntityId>(regs.di))
            , cargoType(regs.dl)
        {
        }

        EntityId head;
        uint8_t cargoType;

        explicit operator registers() const
        {
            registers regs;
            regs.di = enumValue(head);
            regs.dl = cargoType;
            return regs;
        }
    };

    // Change company face
    inline bool do_65(const ObjectHeader& object, CompanyId company)
    {
        auto objPtr = reinterpret_cast<const int32_t*>(&object);
        registers regs;
        regs.bl = Flags::apply;
        regs.eax = *objPtr++;
        regs.ecx = *objPtr++;
        regs.edx = *objPtr++;
        regs.edi = *objPtr;
        regs.bh = enumValue(company);
        return doCommand(GameCommand::changeCompanyFace, regs) != FAILURE;
    }

    struct ClearLandArgs
    {
        static constexpr auto command = GameCommand::clearLand;
        ClearLandArgs() = default;
        explicit ClearLandArgs(const registers& regs)
            : centre(regs.ax, regs.cx)
            , pointA(regs.edx & 0xFFFF, regs.ebp & 0xFFFF)
            , pointB(regs.edx >> 16, regs.ebp >> 16)
        {
        }

        World::Pos2 centre;
        World::Pos2 pointA;
        World::Pos2 pointB;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = centre.x;
            regs.cx = centre.y;
            regs.edx = pointB.x << 16 | pointA.x;
            regs.ebp = pointB.y << 16 | pointA.y;
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

    struct UpdateOwnerStatusArgs
    {
        static constexpr auto command = GameCommand::updateOwnerStatus;
        UpdateOwnerStatusArgs() = default;
        explicit UpdateOwnerStatusArgs(const registers& regs)
            : ownerStatus(regs.ax, regs.cx)
        {
        }

        OwnerStatus ownerStatus;

        explicit operator registers() const
        {
            registers regs;
            int16_t res[2];
            ownerStatus.getData(res);
            regs.ax = res[0];
            regs.cx = res[1];
            return regs;
        }
    };

    struct VehicleSpeedControlArgs
    {
        static constexpr auto command = GameCommand::vehicleSpeedControl;

        VehicleSpeedControlArgs() = default;
        explicit VehicleSpeedControlArgs(const registers& regs)
            : head(static_cast<EntityId>(regs.cx))
            , speed(regs.dx)
        {
        }

        EntityId head;
        int16_t speed;

        explicit operator registers() const
        {
            registers regs;
            regs.cx = enumValue(head);
            regs.dx = speed;
            return regs;
        }
    };

    struct VehicleOrderUpArgs
    {
        static constexpr auto command = GameCommand::vehicleOrderUp;

        VehicleOrderUpArgs() = default;
        explicit VehicleOrderUpArgs(const registers& regs)
            : head(EntityId(regs.di))
            , orderOffset(regs.edx)
        {
        }

        EntityId head;
        uint32_t orderOffset;

        explicit operator registers() const
        {
            registers regs;
            regs.di = enumValue(head);
            regs.edx = orderOffset;
            return regs;
        }
    };

    struct VehicleOrderDownArgs
    {
        static constexpr auto command = GameCommand::vehicleOrderDown;

        VehicleOrderDownArgs() = default;
        explicit VehicleOrderDownArgs(const registers& regs)
            : head(EntityId(regs.di))
            , orderOffset(regs.edx)
        {
        }

        EntityId head;
        uint32_t orderOffset;

        explicit operator registers() const
        {
            registers regs;
            regs.di = enumValue(head);
            regs.edx = orderOffset;
            return regs;
        }
    };

    struct VehicleApplyShuntCheatArgs
    {
        static constexpr auto command = GameCommand::vehicleApplyShuntCheat;

        VehicleApplyShuntCheatArgs() = default;
        explicit VehicleApplyShuntCheatArgs(const registers& regs)
            : head(EntityId(regs.cx))
        {
        }

        EntityId head;

        explicit operator registers() const
        {
            registers regs;
            regs.cx = enumValue(head);
            return regs;
        }
    };

    struct ApplyFreeCashCheatArgs
    {
        static constexpr auto command = GameCommand::applyFreeCashCheat;

        ApplyFreeCashCheatArgs() = default;
        explicit ApplyFreeCashCheatArgs(const registers&)
        {
        }

        explicit operator registers() const
        {
            return registers();
        }
    };

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

    struct VehicleOrderReverseArgs
    {
        static constexpr auto command = GameCommand::vehicleOrderReverse;

        VehicleOrderReverseArgs() = default;
        explicit VehicleOrderReverseArgs(const registers& regs)
            : head(EntityId(regs.di))
        {
        }

        EntityId head;

        explicit operator registers() const
        {
            registers regs;
            regs.di = enumValue(head);
            return regs;
        }
    };

    // Defined in GameCommands/ChangeCompanyColour.cpp
    void changeCompanyColour(registers& regs);

    // Defined in GameCommands/ChangeCompanyFace.cpp
    void changeCompanyFace(registers& regs);

    // Defined in GameCommands/ChangeLandMaterial.cpp
    void changeLandMaterial(registers& regs);

    // Defined in GameCommands/ChangeLoan.cpp
    void changeLoan(registers& regs);

    // Defined in GameCommands/Cheat.cpp
    void cheat(registers& regs);
    void vehicleShuntCheat(registers& regs);
    void freeCashCheat(registers& regs);

    // Defined in GameCommands/ClearLand.cpp
    void clearLand(registers& regs);

    // Defined in GameCommands/CloneVehicle.cpp
    void cloneVehicle(registers& regs);

    // Defined in GameCommands/CreateVehicle.cpp
    void createVehicle(registers& regs);

    // Defined in GameCommands/LoadSaveQuit.cpp
    void loadSaveQuit(registers& regs);

    // Defined in GameCommands/RemoveBuilding.cpp
    void removeBuilding(registers& regs);

    // Defined in GameCommands/RemoveIndustry.cpp
    void removeIndustry(registers& regs);

    // Defined in GameCommands/RemoveTree.cpp
    void removeTree(registers& regs);

    // Defined in GameCommands/CreateTree.cpp
    void createTree(registers& regs);

    // Defined in GameCommands/RemoveWall.cpp
    void removeWall(registers& regs);

    // Defined in GameCommands/RenameCompanyName.cpp
    void changeCompanyName(registers& regs);

    // Defined in GameCommands/RenameCompanyOwner.cpp
    void changeCompanyOwnerName(registers& regs);

    // Defined in GameCommands/RenameIndustry.cpp
    void renameIndustry(registers& regs);

    // Defined in GameCommands/RenameStation.cpp
    void renameStation(registers& regs);

    // Defined in GameCommands/RenameTown.cpp
    void renameTown(registers& regs);

    // Defined in GameCommands/RenameVehicle.cpp
    void renameVehicle(registers& regs);

    // Defined in GameCommands/SellVehicle.cpp
    void sellVehicle(registers& regs);

    // Defined in GameCommands/SetGameSpeed.cpp
    void setGameSpeed(registers& regs);

    // Defined in GameCommands/TogglePause.cpp
    uint32_t togglePause(uint8_t flags);
    void togglePause(registers& regs);

    // Defined in GameCommands/VehicleChangeRunningMode.cpp
    void vehicleChangeRunningMode(registers& regs);

    // Defined in GameCommands/VehicleOrderDelete.cpp
    void vehicleOrderDelete(registers& regs);

    // Defined in GameCommands/VehicleOrderDown.cpp
    void vehicleOrderDown(registers& regs);

    // Defined in GameCommands/VehicleOrderInsert.cpp
    void vehicleOrderInsert(registers& regs);

    // Defined in GameCommands/VehicleOrderReverse.cpp
    void vehicleOrderReverse(registers& regs);

    // Defined in GameCommands/VehicleOrderSkip.cpp
    void vehicleOrderSkip(registers& regs);

    // Defined in GameCommands/VehicleOrderUp.cpp
    void vehicleOrderUp(registers& regs);

    // Defined in GameCommands/VehiclePassSignal.cpp
    void vehiclePassSignal(registers& regs);

    // Defined in GameCommands/VehiclePickup.cpp
    void vehiclePickup(registers& regs);

    // Defined in GameCommands/VehiclePickupWater.cpp
    void vehiclePickupWater(registers& regs);

    // Defined in GameCommands/VehicleRefit.cpp
    void vehicleRefit(registers& regs);

    // Defined in GameCommands/VehicleReverse.cpp
    void vehicleReverse(registers& regs);

    // Defined in GameCommands/VehicleSpeedControl.cpp
    void vehicleSpeedControl(registers& regs);

    // Defined in GameCommands/UpdateOwnerStatus.cpp
    void updateOwnerStatus(registers& regs);

    const World::Pos3& getPosition();
    void setPosition(const World::Pos3& pos);
    void setErrorText(const string_id message);
    string_id getErrorText();
    void setErrorTitle(const string_id title);
    ExpenditureType getExpenditureType();
    void setExpenditureType(const ExpenditureType type);
    CompanyId getUpdatingCompanyId();
    void setUpdatingCompanyId(CompanyId companyId);
}
