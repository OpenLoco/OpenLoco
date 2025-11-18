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

namespace OpenLoco::World
{
    struct WallElement;
}

namespace OpenLoco::World::TileManager
{
    enum class ElementPositionFlags : uint8_t;
}

namespace OpenLoco::Vehicles
{
    struct VehicleHead;
}

namespace OpenLoco::GameCommands
{
    namespace Flags
    {
        constexpr uint8_t apply = 1 << 0;                   // 0x01
        constexpr uint8_t preventBuildingClearing = 1 << 1; // 0x02
        constexpr uint8_t allowNegativeCashFlow = 1 << 2;   // 0x04
        constexpr uint8_t noErrorWindow = 1 << 3;           // 0x08 do not show an error window even on failure (use this with ghosts)
        constexpr uint8_t aiAllocated = 1 << 4;             // 0x10 ai can place down invisible ghosts with this that blocks players
        constexpr uint8_t noPayment = 1 << 5;               // 0x20 calculates cost but does not deduct it
        constexpr uint8_t ghost = 1 << 6;                   // 0x40
        constexpr uint8_t flag_7 = 1 << 7;                  // 0x80 ai only?
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
        removeTrainStation = 16,
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
        aiCreateTrackAndStation = 51,
        aiTrackReplacement = 52,
        aiCreateRoadAndStation = 53,
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
        vehicleRepaint = 84,
    };

    constexpr uint32_t FAILURE = 0x80000000;

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
    void resetCommandNestLevel();

    // TODO: rework these
    struct LegacyReturnState
    {
        World::TileManager::ElementPositionFlags flags_1136072; // 0x01136072
        uint8_t flags_1136073;                                  // 0x01136073
        World::MicroZ byte_1136074;                             // 0x01136074
        uint8_t byte_1136075;                                   // 0x01136075
        StationId lastPlacedTrackRoadStationId;                 // 0x0112C730
        StationId lastConstructedAdjoiningStation;              // 0x0112C734
        StationId lastPlacedAirport;                            // 0x0112C744
        StationId lastPlacedDock;                               // 0x0112C748
        World::Pos2 lastConstructedAdjoiningStationPos;         // 0x0112C792 centre pos
        IndustryId lastPlacedIndustryId;                        // 0x00E0C3C9
        World::WallElement* lastPlacedWall;                     // 0x01136470
        EntityId lastCreatedVehicleId;                          // 0x0113642A
    };

    // Note: this is deliberately a mutable ref
    LegacyReturnState& getLegacyReturnState();

    void playConstructionPlacementSound(World::Pos3 pos);
    bool shouldInvalidateTile(uint8_t flags);
}
