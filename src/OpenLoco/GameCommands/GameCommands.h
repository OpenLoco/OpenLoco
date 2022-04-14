#pragma once

#include "../Company.h"
#include "../Economy/Currency.h"
#include "../Entities/Entity.h"
#include "../Interop/Interop.hpp"
#include "../Map/Tile.h"
#include "../Objects/Object.h"

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    enum ExpenditureType : uint8_t;
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
        vehicleLocalExpress = 12,
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
    };

    enum class LoadOrQuitMode : uint16_t
    {
        loadGamePrompt,
        returnToTitlePrompt,
        quitGamePrompt,
    };

    constexpr uint32_t FAILURE = 0x80000000;

    void registerHooks();
    uint32_t doCommand(GameCommand command, const registers& registers);
    bool sub_431E6A(const CompanyId company, Map::TileElement* const tile = nullptr);

    template<typename T>
    uint32_t doCommand(const T& args, uint8_t flags)
    {
        registers regs = registers(args);
        regs.bl = flags;
        return doCommand(T::command, regs);
    }

    inline void do_0(EntityId source, EntityId dest)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.dx = enumValue(source);
        regs.di = enumValue(dest);
        doCommand(GameCommand::vehicleRearrange, regs);
    }

    struct VehiclePlacementArgs
    {
        static constexpr auto command = GameCommand::vehiclePlace;

        VehiclePlacementArgs() = default;
        explicit VehiclePlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.dx * 4)
            , trackAndDirection(regs.bp)
            , trackProgress(regs.ebx >> 16)
            , head(EntityId(regs.di))
            , convertGhost((regs.ebx >> 16) == 0xFFFF)
        {
        }

        Map::Pos3 pos;
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
            regs.dx = pos.z / 4;
            regs.ebx = convertGhost ? 0xFFFF0000 : (trackProgress << 16);
            return regs;
        }
    };

    inline bool do_2(EntityId head)
    {
        registers regs;
        regs.bl = Flags::apply | Flags::flag_3 | Flags::flag_6;
        regs.di = enumValue(head);
        return doCommand(GameCommand::vehiclePickup, regs) != FAILURE;
    }

    // Reverse (vehicle)
    inline void do_3(EntityId vehicleHead, Vehicles::VehicleHead* const head)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.dx = enumValue(vehicleHead);
        // Bug in game command 3 requires to set edi to a vehicle prior to calling
        regs.edi = X86Pointer(head);

        doCommand(GameCommand::vehicleReverse, regs);
    }

    // Pass signal (vehicle)
    inline void do_4(EntityId vehicleHead)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.di = enumValue(vehicleHead);

        doCommand(GameCommand::vehiclePassSignal, regs);
    }

    // Build vehicle
    inline uint32_t do_5(uint16_t vehicle_type, EntityId vehicle_id = EntityId::null)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.di = enumValue(vehicle_id);
        regs.edx = vehicle_type;

        return doCommand(GameCommand::vehicleCreate, regs);
    }

    // Build vehicle
    inline uint32_t queryDo_5(uint16_t vehicle_type, EntityId vehicle_id = EntityId::null)
    {
        registers regs;
        regs.bl = 0;
        regs.di = enumValue(vehicle_id);
        regs.edx = vehicle_type;

        return doCommand(GameCommand::vehicleCreate, regs);
    }

    inline void do_6(EntityId car)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.dx = enumValue(car);
        doCommand(GameCommand::vehicleSell, regs);
    }

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

        Map::Pos3 pos;
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

        Map::Pos3 pos;
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

    // Change vehicle name
    inline void do_10(EntityId head, uint16_t i, uint32_t edx, uint32_t ebp, uint32_t edi)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.cx = enumValue(head); // vehicle head id
        regs.ax = i;               // [ 0, 1, 2]
        regs.edx = edx;            // part of name buffer
        regs.ebp = ebp;            // part of name buffer
        regs.edi = edi;            // part of name buffer
        doCommand(GameCommand::vehicleRename, regs);
    }

    // Change station name
    inline void do_11(uint16_t cx, uint16_t ax, uint32_t edx, uint32_t ebp, uint32_t edi)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.cx = cx;   // station number or 0
        regs.ax = ax;   // [ 0, 1, 2]
        regs.edx = edx; // part of name buffer
        regs.ebp = ebp; // part of name buffer
        regs.edi = edi; // part of name buffer
        doCommand(GameCommand::changeStationName, regs);
    }

    inline void do12(EntityId head, uint8_t bh)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.bh = bh;
        regs.dx = enumValue(head);
        doCommand(GameCommand::vehicleLocalExpress, regs);
    }

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

        Map::Pos3 pos;
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

        Map::Pos3 pos;
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

        Map::Pos3 pos;
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

        Map::Pos3 pos;
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

        Map::Pos3 pos;
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

        Map::Pos3 pos;
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

    // Change company colour scheme
    inline void do_19(int8_t isPrimary, int8_t value, int8_t colourType, int8_t setColourMode, CompanyId companyId)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.cl = colourType;           // vehicle type or main
        regs.dh = setColourMode;        // [ 0, 1 ] -- 0 = set colour, 1 = toggle enabled/disabled;
        regs.dl = enumValue(companyId); // company id

        if (setColourMode == 0)
        {
            // cl is divided by 2 when used
            regs.ah = isPrimary; // [ 0, 1 ] -- primary or secondary palette
            regs.al = value;     // new colour
        }
        else if (setColourMode == 1)
        {
            regs.al = value; // [ 0, 1 ] -- off or on
        }

        doCommand(GameCommand::changeCompanyColourScheme, regs);
    }

    // Pause game
    inline void do_20()
    {
        registers regs;
        regs.bl = Flags::apply;
        doCommand(GameCommand::pauseGame, regs);
    }

    // Load/save/quit game
    inline void do_21(uint8_t dl, uint8_t di)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.dl = dl; // [ 0 = save, 1 = close save prompt, 2 = don't save ]
        regs.di = di; // [ 0 = load game, 1 = return to title screen, 2 = quit to desktop ]
        doCommand(GameCommand::loadSaveQuitGame, regs);
    }

    struct TreeRemovalArgs
    {
        static constexpr auto command = GameCommand::removeTree;

        TreeRemovalArgs() = default;
        explicit TreeRemovalArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.dl * 4)
            , type(regs.dh)
            , elementType(regs.bh)
        {
        }

        Map::Pos3 pos;
        uint8_t type;
        uint8_t elementType;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.dl = pos.z / 4;
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

        Map::Pos2 pos;
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

        Map::Pos2 pointA;
        Map::Pos2 pointB;
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

    // Raise Land
    inline uint32_t do_25(Map::Pos2 centre, Map::Pos2 pointA, Map::Pos2 pointB, uint16_t di, uint8_t flags)
    {
        registers regs;
        regs.ax = centre.x;
        regs.cx = centre.y;
        regs.edx = pointB.x << 16 | pointA.x;
        regs.ebp = pointB.y << 16 | pointA.y;
        regs.bl = flags;
        regs.di = di;
        return doCommand(GameCommand::raiseLand, regs);
    }

    // Lower Land
    inline uint32_t do_26(Map::Pos2 centre, Map::Pos2 pointA, Map::Pos2 pointB, uint16_t di, uint8_t flags)
    {
        registers regs;
        regs.ax = centre.x;
        regs.cx = centre.y;
        regs.edx = pointB.x << 16 | pointA.x;
        regs.ebp = pointB.y << 16 | pointA.y;
        regs.bl = flags;
        regs.di = di;
        return doCommand(GameCommand::lowerLand, regs);
    }

    // Lower/Raise Land Mountain
    inline uint32_t do_27(Map::Pos2 centre, Map::Pos2 pointA, Map::Pos2 pointB, uint16_t di, uint8_t flags)
    {
        registers regs;
        regs.ax = centre.x;
        regs.cx = centre.y;
        regs.edx = pointB.x << 16 | pointA.x;
        regs.ebp = pointB.y << 16 | pointA.y;
        regs.bl = flags;
        regs.di = di;
        return doCommand(GameCommand::lowerRaiseLandMountain, regs);
    }

    // Raise Water
    inline uint32_t do_28(Map::Pos2 pointA, Map::Pos2 pointB, uint8_t flags)
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
    inline uint32_t do_29(Map::Pos2 pointA, Map::Pos2 pointB, uint8_t flags)
    {
        registers regs;
        regs.ax = pointA.x;
        regs.cx = pointA.y;
        regs.di = pointB.x;
        regs.bp = pointB.y;
        regs.bl = flags;
        return doCommand(GameCommand::lowerWater, regs);
    }

    // Change company name
    inline bool do_30(CompanyId cx, uint16_t ax, uint32_t edx, uint32_t ebp, uint32_t edi)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.cx = enumValue(cx); // company id
        regs.ax = ax;            // [ 0, 1, 2]
        regs.edx = edx;          // part of name buffer
        regs.ebp = ebp;          // part of name buffer
        regs.edi = edi;          // part of name buffer
        return doCommand(GameCommand::changeCompanyName, regs) != FAILURE;
    }

    // Change company owner name
    inline bool do_31(CompanyId cx, uint16_t ax, uint32_t edx, uint32_t ebp, uint32_t edi)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.cx = enumValue(cx); // company id
        regs.ax = ax;            // [ 0, 1, 2]
        regs.edx = edx;          // part of name buffer
        regs.ebp = ebp;          // part of name buffer
        regs.edi = edi;          // part of name buffer
        return doCommand(GameCommand::changeCompanyOwnerName, regs) != FAILURE;
    }

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

        Map::Pos3 pos;
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
            : pos(regs.ax, regs.cx, regs.dh * 4)
            , rotation(regs.dl)
        {
        }

        Map::Pos3 pos;
        uint8_t rotation;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.dh = pos.z / 4;
            regs.dl = rotation;
            return regs;
        }
    };

    inline bool do_35(EntityId head, uint64_t rawOrder, uint32_t orderOffset)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.eax = rawOrder & 0xFFFFFFFF;
        regs.cx = rawOrder >> 32;
        regs.di = enumValue(head);
        regs.edx = orderOffset;
        return doCommand(GameCommand::vehicleOrderInsert, regs);
    }

    inline bool do_36(EntityId head, uint32_t orderOffset)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.di = enumValue(head);
        regs.edx = orderOffset;
        return doCommand(GameCommand::vehicleOrderDelete, regs);
    }

    inline bool do_37(EntityId head)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.di = enumValue(head);
        return doCommand(GameCommand::vehicleOrderSkip, regs);
    }

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

        Map::Pos3 pos;
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

        Map::Pos3 pos;
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

        Map::Pos3 pos;
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

        Map::Pos3 pos;
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

        Map::Pos3 pos;
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

        Map::Pos3 pos;
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

        Map::Pos3 pos;
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

        Map::Pos3 pos;

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
            , buildImmediately(regs.bh & 0x80)
            , srand0(regs.ebp)
            , srand1(regs.edi)
        {
        }

        Map::Pos2 pos;
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
            return regs;
        }
    };

    // Remove industry
    inline bool do_48(uint8_t flags, uint8_t industryId)
    {
        registers regs;
        regs.bl = flags;
        regs.dx = industryId;
        return doCommand(GameCommand::removeIndustry, regs) != FAILURE;
    }

    struct TownPlacementArgs
    {
        static constexpr auto command = GameCommand::createTown;

        TownPlacementArgs() = default;
        explicit TownPlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx)
            , size(regs.dl)
        {
        }

        Map::Pos2 pos;
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

        Map::Pos3 pos;
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

        Map::Pos3 pos;
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

        Map::Pos3 pos;
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

        Map::Pos3 pos;

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

        Map::Pos3 pos;
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

        Map::Pos3 pos;

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

        Map::Pos3 pos;
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

    // Refit vehicle
    inline void do_64(EntityId vehicleHead, uint16_t option)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.di = enumValue(vehicleHead);
        regs.dx = option;
        doCommand(GameCommand::vehicleRefit, regs);
    }

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

    // Clear Land
    inline uint32_t do_66(Map::Pos2 centre, Map::Pos2 pointA, Map::Pos2 pointB, uint8_t flags)
    {
        registers regs;
        regs.ax = centre.x;
        regs.cx = centre.y;
        regs.edx = pointB.x << 16 | pointA.x;
        regs.ebp = pointB.y << 16 | pointA.y;
        regs.bl = flags;
        return doCommand(GameCommand::clearLand, regs);
    }

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

    inline uint32_t do_74(EntityId head, int16_t speed)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.cx = enumValue(head);
        regs.dx = speed;
        return doCommand(GameCommand::vehicleSpeedControl, regs);
    }

    inline uint32_t do_75(EntityId head, uint32_t orderOffset)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.di = enumValue(head);
        regs.edx = orderOffset;
        return doCommand(GameCommand::vehicleOrderUp, regs);
    }

    inline uint32_t do_76(EntityId head, uint32_t orderOffset)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.di = enumValue(head);
        regs.edx = orderOffset;
        return doCommand(GameCommand::vehicleOrderDown, regs);
    }

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
        explicit ApplyFreeCashCheatArgs(const registers& regs)
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

    // Defined in GameCommands/ChangeCompanyColour.cpp
    void changeCompanyColour(registers& regs);

    // Defined in GameCommands/ChangeLoan.cpp
    void changeLoan(registers& regs);

    // Defined in GameCommands/Cheat.cpp
    void cheat(registers& regs);
    void vehicleShuntCheat(registers& regs);
    void freeCashCheat(registers& regs);

    // Defined in GameCommands/LoadSaveQuit.cpp
    void loadSaveQuit(registers& regs);

    // Defined in GameCommands/RemoveTree.cpp
    void removeTree(registers& regs);

    // Defined in GameCommands/RemoveWall.cpp
    void removeWall(registers& regs);

    // Defined in GameCommands/RenameIndustry.cpp
    void renameIndustry(registers& regs);

    // Defined in GameCommands/RenameStation.cpp
    void renameStation(registers& regs);

    // Defined in GameCommands/RenameTown.cpp
    void renameTown(registers& regs);

    // Defined in GameCommands/SetGameSpeed.cpp
    void setGameSpeed(registers& regs);

    // Defined in GameCommands/TogglePause.cpp
    uint32_t togglePause(uint8_t flags);
    void togglePause(registers& regs);

    // Defined in GameCommands/VehiclePickup.cpp
    void vehiclePickup(registers& regs);

    // Defined in GameCommands/UpdateOwnerStatus.cpp
    void updateOwnerStatus(registers& regs);

    // Defined in GameCommands/ChangeLandMaterial.cpp
    void changeLandMaterial(registers& regs);

    const Map::Pos3& getPosition();
    void setPosition(const Map::Pos3& pos);
    void setErrorText(const string_id message);
    string_id getErrorText();
    void setErrorTitle(const string_id title);
    ExpenditureType getExpenditureType();
    void setExpenditureType(const ExpenditureType type);
    CompanyId getUpdatingCompanyId();
    void setUpdatingCompanyId(CompanyId companyId);
}
