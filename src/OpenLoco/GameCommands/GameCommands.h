#pragma once

#include "../Economy/Currency.h"
#include "../Entities/Entity.h"
#include "../Interop/Interop.hpp"
#include "../Map/Tile.h"
#include "../Objects/ObjectManager.h"

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
        gc_unk_7 = 7,
        gc_unk_8 = 8,
        changeLoan = 9,
        vehicleRename = 10,
        changeStationName = 11,
        vehicleLocalExpress = 12,
        gc_unk_13 = 13,
        gc_unk_14 = 14,
        gc_unk_15 = 15,
        gc_unk_16 = 16,
        gc_unk_17 = 17,
        gc_unk_18 = 18,
        changeCompanyColourScheme = 19,
        pauseGame = 20,
        loadSaveQuitGame = 21,
        gc_unk_22 = 22,
        gc_unk_23 = 23,
        changeLandMaterial = 24,
        raiseLand = 25,
        lowerLand = 26,
        lowerRaiseLandMountain = 27,
        raiseWater = 28,
        lowerWater = 29,
        changeCompanyName = 30,
        changeCompanyOwnerName = 31,
        gc_unk_32 = 32,
        gc_unk_33 = 33,
        gc_unk_34 = 34,
        vehicleOrderInsert = 35,
        vehicleOrderDelete = 36,
        vehicleOrderSkip = 37,
        gc_unk_38 = 38,
        gc_unk_39 = 39,
        gc_unk_40 = 40,
        gc_unk_41 = 41,
        gc_unk_42 = 42,
        gc_unk_43 = 43,
        gc_unk_44 = 44,
        gc_unk_45 = 45,
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
        gc_unk_56 = 56,
        gc_unk_57 = 57,
        gc_unk_58 = 58,
        vehicleAbortPickupAir = 59,
        gc_unk_60 = 60,
        gc_unk_61 = 61,
        gc_unk_62 = 62,
        vehicleAbortPickupWater = 63,
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
    bool sub_431E6A(const CompanyId_t company, Map::TileElement* const tile = nullptr);

    inline void do_0(EntityId_t source, EntityId_t dest)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.dx = source;
        regs.di = dest;
        doCommand(GameCommand::vehicleRearrange, regs);
    }

    inline bool do_2(EntityId_t head)
    {
        registers regs;
        regs.bl = Flags::apply | Flags::flag_3 | Flags::flag_6;
        regs.di = head;
        return doCommand(GameCommand::vehiclePickup, regs) != FAILURE;
    }

    // Reverse (vehicle)
    inline void do_3(EntityId_t vehicleHead, Vehicles::VehicleHead* const head)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.dx = vehicleHead;
        // Bug in game command 3 requires to set edi to a vehicle prior to calling
        regs.edi = reinterpret_cast<uint32_t>(head);

        doCommand(GameCommand::vehicleReverse, regs);
    }

    // Pass signal (vehicle)
    inline void do_4(EntityId_t vehicleHead)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.di = vehicleHead;

        doCommand(GameCommand::vehiclePassSignal, regs);
    }

    // Build vehicle
    inline uint32_t do_5(uint16_t vehicle_type, uint16_t vehicle_id = 0xFFFF)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.di = vehicle_id;
        regs.edx = vehicle_type;

        return doCommand(GameCommand::vehicleCreate, regs);
    }

    // Build vehicle
    inline uint32_t queryDo_5(uint16_t vehicle_type, uint16_t vehicle_id = 0xFFFF)
    {
        registers regs;
        regs.bl = 0;
        regs.di = vehicle_id;
        regs.edx = vehicle_type;

        return doCommand(GameCommand::vehicleCreate, regs);
    }

    inline void do_6(EntityId_t car)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.dx = car;
        doCommand(GameCommand::vehicleSell, regs);
    }

    // Change loan
    inline void do_9(currency32_t newLoan)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.edx = newLoan;
        doCommand(GameCommand::changeLoan, regs);
    }

    // Change vehicle name
    inline void do_10(EntityId_t head, uint16_t i, uint32_t edx, uint32_t ebp, uint32_t edi)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.cx = head; // vehicle head id
        regs.ax = i;    // [ 0, 1, 2]
        regs.edx = edx; // part of name buffer
        regs.ebp = ebp; // part of name buffer
        regs.edi = edi; // part of name buffer
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

    inline void do12(EntityId_t head, uint8_t bh)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.bh = bh;
        regs.dx = head;
        doCommand(GameCommand::vehicleLocalExpress, regs);
    }

    // Change company colour scheme
    inline void do_19(int8_t isPrimary, int8_t value, int8_t colourType, int8_t setColourMode, uint8_t companyId)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.cl = colourType;    // vehicle type or main
        regs.dh = setColourMode; // [ 0, 1 ] -- 0 = set colour, 1 = toggle enabled/disabled;
        regs.dl = companyId;     // company id

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

    // Change Land Material
    inline void do_24(Map::Pos2 pointA, Map::Pos2 pointB, uint8_t landType, uint8_t flags)
    {
        registers regs;
        regs.ax = pointA.x;
        regs.cx = pointA.y;
        regs.di = pointB.x;
        regs.bp = pointB.y;
        regs.dl = landType;
        regs.bl = flags;
        doCommand(GameCommand::changeLandMaterial, regs);
    }

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
    inline bool do_30(uint16_t cx, uint16_t ax, uint32_t edx, uint32_t ebp, uint32_t edi)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.cx = cx;   // company id
        regs.ax = ax;   // [ 0, 1, 2]
        regs.edx = edx; // part of name buffer
        regs.ebp = ebp; // part of name buffer
        regs.edi = edi; // part of name buffer
        return doCommand(GameCommand::changeCompanyName, regs) != FAILURE;
    }

    // Change company owner name
    inline bool do_31(uint16_t cx, uint16_t ax, uint32_t edx, uint32_t ebp, uint32_t edi)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.cx = cx;   // company id
        regs.ax = ax;   // [ 0, 1, 2]
        regs.edx = edx; // part of name buffer
        regs.ebp = ebp; // part of name buffer
        regs.edi = edi; // part of name buffer
        return doCommand(GameCommand::changeCompanyOwnerName, regs) != FAILURE;
    }

    inline bool do_35(EntityId_t head, uint64_t rawOrder, uint32_t orderOffset)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.eax = rawOrder & 0xFFFFFFFF;
        regs.cx = rawOrder >> 32;
        regs.di = head;
        regs.edx = orderOffset;
        return doCommand(GameCommand::vehicleOrderInsert, regs);
    }

    inline bool do_36(EntityId_t head, uint32_t orderOffset)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.di = head;
        regs.edx = orderOffset;
        return doCommand(GameCommand::vehicleOrderDelete, regs);
    }

    inline bool do_37(EntityId_t head)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.di = head;
        return doCommand(GameCommand::vehicleOrderSkip, regs);
    }

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

    // Remove industry
    inline bool do_48(uint8_t industryId)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.dx = industryId;
        return doCommand(GameCommand::removeIndustry, regs) != FAILURE;
    }

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
        HeadquarterPlacementArgs() = default;
        explicit HeadquarterPlacementArgs(const registers regs)
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
        HeadquarterRemovalArgs() = default;
        explicit HeadquarterRemovalArgs(const HeadquarterPlacementArgs& place)
            : pos(place.pos)
        {
        }
        explicit HeadquarterRemovalArgs(const registers regs)
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

    // Build company headquarters
    inline uint32_t do_54(uint8_t bl, const HeadquarterPlacementArgs& args)
    {
        registers regs = registers(args);
        regs.bl = bl; // flags
        return doCommand(GameCommand::buildCompanyHeadquarters, regs);
    }

    // Remove company headquarters
    inline void do_55(uint8_t bl, const HeadquarterRemovalArgs& args)
    {
        registers regs = registers(args);
        regs.bl = bl; // flags
        doCommand(GameCommand::removeCompanyHeadquarters, regs);
    }

    inline bool do_59(EntityId_t head)
    {
        registers regs;
        regs.bl = Flags::apply | Flags::flag_3 | Flags::flag_6;
        regs.di = head;
        return doCommand(GameCommand::vehicleAbortPickupAir, regs) != FAILURE;
    }

    inline bool do_63(EntityId_t head)
    {
        registers regs;
        regs.bl = Flags::apply | Flags::flag_3 | Flags::flag_6;
        regs.di = head;
        return doCommand(GameCommand::vehicleAbortPickupWater, regs) != FAILURE;
    }

    // Refit vehicle
    inline void do_64(EntityId_t vehicleHead, uint16_t option)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.di = vehicleHead;
        regs.dx = option;
        doCommand(GameCommand::vehicleRefit, regs);
    }

    // Change company face
    inline bool do_65(const ObjectHeader& object, uint8_t company)
    {
        auto objPtr = reinterpret_cast<const int32_t*>(&object);
        registers regs;
        regs.bl = Flags::apply;
        regs.eax = *objPtr++;
        regs.ecx = *objPtr++;
        regs.edx = *objPtr++;
        regs.edi = *objPtr;
        regs.bh = company;
        return doCommand(GameCommand::changeCompanyFace, regs) != FAILURE;
    }

    // Clear Land
    inline void do_66(Map::Pos2 centre, Map::Pos2 pointA, Map::Pos2 pointB, uint8_t flags)
    {
        registers regs;
        regs.ax = centre.x;
        regs.cx = centre.y;
        regs.edx = pointB.x << 16 | pointA.x;
        regs.ebp = pointB.y << 16 | pointA.y;
        regs.bl = flags;
        doCommand(GameCommand::clearLand, regs);
    }

    // Load multiplayer map
    inline void do_67(const char* filename)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.ebp = reinterpret_cast<int32_t>(filename);
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

    // Update owner status
    inline void do_73(EntityId_t id)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.ax = -2;
        regs.cx = id;
        doCommand(GameCommand::updateOwnerStatus, regs);
    }

    // Update owner status
    inline void do_73(Map::Pos2 position)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.ax = position.x;
        regs.cx = position.y;
        doCommand(GameCommand::updateOwnerStatus, regs);
    }

    inline uint32_t do_74(EntityId_t head, int16_t speed)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.cx = head;
        regs.dx = speed;
        return doCommand(GameCommand::vehicleSpeedControl, regs);
    }

    inline uint32_t do_75(EntityId_t head, uint32_t orderOffset)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.di = head;
        regs.edx = orderOffset;
        return doCommand(GameCommand::vehicleOrderUp, regs);
    }

    inline uint32_t do_76(EntityId_t head, uint32_t orderOffset)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.di = head;
        regs.edx = orderOffset;
        return doCommand(GameCommand::vehicleOrderDown, regs);
    }

    inline void do_77(EntityId_t head)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.cx = head;
        doCommand(GameCommand::vehicleApplyShuntCheat, regs);
    }

    inline void do_78()
    {
        registers regs;
        regs.bl = Flags::apply;
        GameCommands::doCommand(GameCommand::applyFreeCashCheat, regs);
    }

    // Rename industry
    inline void do_79(uint16_t cx, uint16_t ax, uint32_t edx, uint32_t ebp, uint32_t edi)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.cx = cx;   // industry number or 0
        regs.ax = ax;   // [ 0, 1, 2]
        regs.edx = edx; // part of name buffer
        regs.ebp = ebp; // part of name buffer
        regs.edi = edi; // part of name buffer
        doCommand(GameCommand::renameIndustry, regs);
    }

    inline bool do_80(uint16_t head)
    {
        registers regs;
        regs.bl = Flags::apply;
        regs.ax = head;
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

    // Defined in GameCommands/Cheat.cpp
    void cheat(registers& regs);

    // Defined in GameCommands/LoadSaveQuit.cpp
    void loadSaveQuit(registers& regs);

    // Defined in GameCommands/RenameIndustry.cpp
    void renameIndustry(registers& regs);

    // Defined in GameCommands/RenameStation.cpp
    void renameStation(registers& regs);

    // Defined in GameCommands/RenameTown.cpp
    void renameTown(registers& regs);

    // Defined in GameCommands/TogglePause.cpp
    uint32_t togglePause(uint8_t flags);
    void togglePause(registers& regs);

    // Defined in GameCommands/VehiclePickup.cpp
    void vehiclePickup(registers& regs);

    const Map::Pos3& getPosition();
    void setPosition(const Map::Pos3& pos);
    void setErrorText(const string_id message);
    string_id getErrorText();
    void setErrorTitle(const string_id title);
    ExpenditureType getExpenditureType();
    void setExpenditureType(const ExpenditureType type);
}
