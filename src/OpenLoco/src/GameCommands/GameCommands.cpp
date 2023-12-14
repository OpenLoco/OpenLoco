#include "GameCommands.h"
#include "Audio/Audio.h"
#include "Buildings/RemoveBuilding.h"
#include "Cheats/Cheat.h"
#include "Company/BuildCompanyHeadquarters.h"
#include "Company/ChangeCompanyColour.h"
#include "Company/ChangeCompanyFace.h"
#include "Company/ChangeLoan.h"
#include "Company/RemoveCompanyHeadquarters.h"
#include "Company/RenameCompanyName.h"
#include "Company/RenameCompanyOwner.h"
#include "Company/UpdateOwnerStatus.h"
#include "General/LoadSaveQuit.h"
#include "General/RenameStation.h"
#include "General/SetGameSpeed.h"
#include "General/TogglePause.h"
#include "Industries/CreateIndustry.h"
#include "Industries/RemoveIndustry.h"
#include "Industries/RenameIndustry.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Map/RoadElement.h"
#include "Map/StationElement.h"
#include "Map/Tile.h"
#include "Map/TileManager.h"
#include "Map/TrackElement.h"
#include "Network/Network.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackObject.h"
#include "SceneManager.h"
#include "Terraform/ChangeLandMaterial.h"
#include "Terraform/ClearLand.h"
#include "Terraform/CreateTree.h"
#include "Terraform/CreateWall.h"
#include "Terraform/LowerLand.h"
#include "Terraform/LowerRaiseLandMountain.h"
#include "Terraform/LowerWater.h"
#include "Terraform/RaiseLand.h"
#include "Terraform/RaiseWater.h"
#include "Terraform/RemoveTree.h"
#include "Terraform/RemoveWall.h"
#include "Town/CreateTown.h"
#include "Town/RemoveTown.h"
#include "Town/RenameTown.h"
#include "Track/CreateSignal.h"
#include "Track/CreateTrackMod.h"
#include "Track/RemoveTrackMod.h"
#include "Ui/WindowManager.h"
#include "Vehicles/CloneVehicle.h"
#include "Vehicles/CreateVehicle.h"
#include "Vehicles/RenameVehicle.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleChangeRunningMode.h"
#include "Vehicles/VehicleOrderDelete.h"
#include "Vehicles/VehicleOrderDown.h"
#include "Vehicles/VehicleOrderInsert.h"
#include "Vehicles/VehicleOrderReverse.h"
#include "Vehicles/VehicleOrderSkip.h"
#include "Vehicles/VehicleOrderUp.h"
#include "Vehicles/VehiclePassSignal.h"
#include "Vehicles/VehiclePickup.h"
#include "Vehicles/VehiclePickupAir.h"
#include "Vehicles/VehiclePickupWater.h"
#include "Vehicles/VehicleRearrange.h"
#include "Vehicles/VehicleRefit.h"
#include "Vehicles/VehicleReverse.h"
#include "Vehicles/VehicleSell.h"
#include "Vehicles/VehicleSpeedControl.h"
#include "World/Company.h"
#include "World/CompanyManager.h"
#include "World/StationManager.h"
#include <cassert>

using namespace OpenLoco::Ui;
using namespace OpenLoco::World;

namespace OpenLoco::GameCommands
{
    static loco_global<CompanyId, 0x009C68EB> _updatingCompanyId;
    static loco_global<uint8_t, 0x00508F08> _gameCommandNestLevel;
    static loco_global<uint8_t, 0x00508F17> _pausedState;

    static uint16_t _gameCommandFlags;

    static loco_global<const TileElement*, 0x009C68D0> _9C68D0;

    static loco_global<Pos3, 0x009C68E0> _gGameCommandPosition;
    static loco_global<StringId, 0x009C68E6> _gGameCommandErrorText;
    static loco_global<StringId, 0x009C68E8> _gGameCommandErrorTitle;
    static loco_global<uint8_t, 0x009C68EA> _gGameCommandExpenditureType; // premultiplied by 4
    static loco_global<CompanyId, 0x009C68EE> _errorCompanyId;
    static loco_global<StringId[8], 0x112C826> _commonFormatArgs;

    using GameCommandFunc = void (*)(registers& regs);

    struct GameCommandInfo
    {
        GameCommand id;
        GameCommandFunc implementation;
        uintptr_t originalAddress; // original array: 0x004F9548
        bool unpausesGame;         // original array: 0x004F9688
    };

    // clang-format off
    static constexpr GameCommandInfo kGameCommandDefinitions[84] = {
        { GameCommand::vehicleRearrange,             vehicleRearrange,          0x004AF1DF, true  },
        { GameCommand::vehiclePlace,                 nullptr,                   0x004B01B6, true  },
        { GameCommand::vehiclePickup,                vehiclePickup,             0x004B0826, true  },
        { GameCommand::vehicleReverse,               vehicleReverse,            0x004ADAA8, true  },
        { GameCommand::vehiclePassSignal,            vehiclePassSignal,         0x004B0B50, true  },
        { GameCommand::vehicleCreate,                createVehicle,             0x004AE5E4, true  },
        { GameCommand::vehicleSell,                  sellVehicle,               0x004AED34, true  },
        { GameCommand::createTrack,                  nullptr,                   0x0049BB98, true  },
        { GameCommand::removeTrack,                  nullptr,                   0x0049C7F2, true  },
        { GameCommand::changeLoan,                   changeLoan,                0x0046DE88, false },
        { GameCommand::vehicleRename,                renameVehicle,             0x004B6572, false },
        { GameCommand::changeStationName,            renameStation,             0x00490756, false },
        { GameCommand::vehicleChangeRunningMode,     vehicleChangeRunningMode,  0x004B694B, true  },
        { GameCommand::createSignal,                 createSignal,              0x00488BDB, true  },
        { GameCommand::removeSignal,                 nullptr,                   0x004891E4, true  },
        { GameCommand::createTrainStation,           nullptr,                   0x0048BB20, true  },
        { GameCommand::removeTrackStation,           nullptr,                   0x0048C402, true  },
        { GameCommand::createTrackMod,               createTrackMod,            0x004A6479, true  },
        { GameCommand::removeTrackMod,               removeTrackMod,            0x004A668A, true  },
        { GameCommand::changeCompanyColourScheme,    changeCompanyColour,       0x0043483D, false },
        { GameCommand::pauseGame,                    togglePause,               0x00431E32, false },
        { GameCommand::loadSaveQuitGame,             loadSaveQuit,              0x0043BFCB, false },
        { GameCommand::removeTree,                   removeTree,                0x004BB392, true  },
        { GameCommand::createTree,                   createTree,                0x004BB138, true  },
        { GameCommand::changeLandMaterial,           changeLandMaterial,        0x00468EDD, true  },
        { GameCommand::raiseLand,                    raiseLand,                 0x00463702, true  },
        { GameCommand::lowerLand,                    lowerLand,                 0x004638C6, true  },
        { GameCommand::lowerRaiseLandMountain,       lowerRaiseLandMountain,    0x00462DCE, true  },
        { GameCommand::raiseWater,                   raiseWater,                0x004C4F19, true  },
        { GameCommand::lowerWater,                   lowerWater,                0x004C5126, true  },
        { GameCommand::changeCompanyName,            changeCompanyName,         0x00434914, false },
        { GameCommand::changeCompanyOwnerName,       changeCompanyOwnerName,    0x00434A58, false },
        { GameCommand::createWall,                   createWall,                0x004C436C, true  },
        { GameCommand::removeWall,                   removeWall,                0x004C466C, true  },
        { GameCommand::gc_unk_34,                    nullptr,                   0x004C4717, false },
        { GameCommand::vehicleOrderInsert,           vehicleOrderInsert,        0x0047036E, false },
        { GameCommand::vehicleOrderDelete,           vehicleOrderDelete,        0x0047057A, false },
        { GameCommand::vehicleOrderSkip,             vehicleOrderSkip,          0x0047071A, false },
        { GameCommand::createRoad,                   nullptr,                   0x00475FBC, true  },
        { GameCommand::removeRoad,                   nullptr,                   0x004775A5, true  },
        { GameCommand::createRoadMod,                nullptr,                   0x0047A21E, true  },
        { GameCommand::removeRoadMod,                nullptr,                   0x0047A42F, true  },
        { GameCommand::createRoadStation,            nullptr,                   0x0048C708, true  },
        { GameCommand::removeRoadStation,            nullptr,                   0x0048D2AC, true  },
        { GameCommand::createBuilding,               nullptr,                   0x0042D133, true  },
        { GameCommand::removeBuilding,               removeBuilding,            0x0042D74E, true  },
        { GameCommand::renameTown,                   renameTown,                0x0049B11E, false },
        { GameCommand::createIndustry,               createIndustry,            0x0045436B, true  },
        { GameCommand::removeIndustry,               removeIndustry,            0x00455943, true  },
        { GameCommand::createTown,                   createTown,                0x00496C22, true  },
        { GameCommand::removeTown,                   removeTown,                0x0049711F, true  },
        { GameCommand::gc_unk_51,                    nullptr,                   0x004A6FDC, true  },
        { GameCommand::gc_unk_52,                    nullptr,                   0x004A734F, true  },
        { GameCommand::gc_unk_53,                    nullptr,                   0x0047AF0B, true  },
        { GameCommand::buildCompanyHeadquarters,     buildCompanyHeadquarters,  0x0042ECFC, true  },
        { GameCommand::removeCompanyHeadquarters,    removeCompanyHeadquarters, 0x0042EEAF, true  },
        { GameCommand::createAirport,                nullptr,                   0x00492C41, true  },
        { GameCommand::removeAirport,                nullptr,                   0x00493559, true  },
        { GameCommand::vehiclePlaceAir,              nullptr,                   0x004267BE, true  },
        { GameCommand::vehiclePickupAir,             vehiclePickupAir,          0x00426B29, true  },
        { GameCommand::createPort,                   nullptr,                   0x00493AA7, true  },
        { GameCommand::removePort,                   nullptr,                   0x00494570, true  },
        { GameCommand::vehiclePlaceWater,            nullptr,                   0x0042773C, true  },
        { GameCommand::vehiclePickupWater,           vehiclePickupWater,        0x004279CC, true  },
        { GameCommand::vehicleRefit,                 vehicleRefit,              0x0042F6DB, false },
        { GameCommand::changeCompanyFace,            changeCompanyFace,         0x00435506, false },
        { GameCommand::clearLand,                    clearLand,                 0x00469CCB, true  },
        { GameCommand::loadMultiplayerMap,           nullptr,                   0x00444DA0, false },
        { GameCommand::gc_unk_68,                    nullptr,                   0x0046F8A5, false },
        { GameCommand::gc_unk_69,                    nullptr,                   0x004454BE, false },
        { GameCommand::gc_unk_70,                    nullptr,                   0x004456C8, false },
        { GameCommand::sendChatMessage,              nullptr,                   0x0046F976, false },
        { GameCommand::multiplayerSave,              nullptr,                   0x004A0ACD, false },
        { GameCommand::updateOwnerStatus,            updateOwnerStatus,         0x004383CA, false },
        { GameCommand::vehicleSpeedControl,          vehicleSpeedControl,       0x004BAB63, true  },
        { GameCommand::vehicleOrderUp,               vehicleOrderUp,            0x00470CD2, false },
        { GameCommand::vehicleOrderDown,             vehicleOrderDown,          0x00470E06, false },
        { GameCommand::vehicleApplyShuntCheat,       vehicleShuntCheat,         0x004BAC53, false },
        { GameCommand::applyFreeCashCheat,           freeCashCheat,             0x00438A08, false },
        { GameCommand::renameIndustry,               renameIndustry,            0x00455029, false },
        { GameCommand::vehicleClone,                 cloneVehicle,              0,          true  },
        { GameCommand::cheat,                        cheat,                     0,          true  },
        { GameCommand::setGameSpeed,                 setGameSpeed,              0,          true  },
        { GameCommand::vehicleOrderReverse,          vehicleOrderReverse,       0,          false },
    };
    // clang-format on

    void registerHooks()
    {
        registerHook(
            0x00431315,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto ebx = doCommand(GameCommand(regs.esi), backup);

                regs = backup;
                regs.ebx = ebx;
                return 0;
            });

        // Used by a number of functions instead of going via doCommand
        registerHook(0x004BB138, [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
            registers backup = regs;
            createTree(backup);

            regs.ebx = backup.ebx;
            return 0;
        });

        registerMountainHooks();
    }

    static uint32_t loc_4314EA();
    static uint32_t loc_4313C6(int esi, const registers& regs);

    static bool commandRequiresUnpausingGame(GameCommand command, uint16_t flags)
    {
        if ((flags & (Flags::aiAllocated | Flags::ghost)) != 0)
            return false;

        auto& gameCommand = kGameCommandDefinitions[static_cast<uint32_t>(command)];
        if (!gameCommand.unpausesGame || isPauseOverrideEnabled())
            return false;

        return true;
    }

    // 0x00431315
    uint32_t doCommand(GameCommand command, const registers& regs)
    {
        uint16_t flags = regs.bx;
        uint32_t esi = static_cast<uint32_t>(command);

        _gameCommandFlags = regs.bx;
        if (_gameCommandNestLevel != 0)
            return loc_4313C6(esi, regs);

        if ((flags & Flags::apply) == 0)
        {
            return loc_4313C6(esi, regs);
        }

        auto isGhost = (flags & Flags::ghost) != 0;
        if (!isGhost && Network::isConnected())
        {
            // For network games, we need to delay the command apply processing
            // Just return the result without applying for now
            registers copyRegs = regs;
            copyRegs.esi = static_cast<int32_t>(command);
            Network::queueGameCommand(_updatingCompanyId, copyRegs);

            copyRegs.bx &= ~Flags::apply;
            return loc_4313C6(esi, copyRegs);
        }

        return doCommandForReal(command, _updatingCompanyId, regs);
    }

    uint32_t doCommandForReal(GameCommand command, CompanyId company, const registers& regs)
    {
        _updatingCompanyId = company;

        uint16_t flags = regs.bx;
        uint32_t esi = static_cast<uint32_t>(command);

        if (commandRequiresUnpausingGame(command, flags) && _updatingCompanyId == CompanyManager::getControllingId())
        {
            if (getPauseFlags() & 1)
            {
                _pausedState = _pausedState ^ 1;
                WindowManager::invalidate(WindowType::timeToolbar);
                Audio::unpauseSound();
                Ui::Windows::PlayerInfoPanel::invalidateFrame();
            }

            if (getGameSpeed() != GameSpeed::Normal)
            {
                // calling the command setGameSpeed will cause infinite recursion here, so just call the real function
                OpenLoco::setGameSpeed(GameSpeed::Normal);
            }

            if (isPaused())
            {
                _gGameCommandErrorText = StringIds::empty;
                return GameCommands::FAILURE;
            }
        }

        if (_updatingCompanyId == CompanyManager::getControllingId() && isNetworked())
        {
            // assert(false);
            // registers fnRegs = regs;
            // call(0x0046E34A, fnRegs); // some network stuff. Untested
        }

        return loc_4313C6(esi, regs);
    }

    static void callGameCommandFunction(uint32_t command, registers& regs)
    {
        auto& gameCommand = kGameCommandDefinitions[command];
        if (gameCommand.implementation != nullptr)
        {
            gameCommand.implementation(regs);
        }
        else
        {
            auto addr = gameCommand.originalAddress;
            call(addr, regs);
        }
    }

    static uint32_t loc_4313C6(int esi, const registers& regs)
    {
        uint16_t flags = regs.bx;
        _gGameCommandErrorText = StringIds::null;
        _gameCommandNestLevel++;

        uint16_t flagsBackup = _gameCommandFlags;
        registers fnRegs1 = regs;
        fnRegs1.bl &= ~Flags::apply;
        callGameCommandFunction(esi, fnRegs1);
        int32_t ebx = fnRegs1.ebx;
        _gameCommandFlags = flagsBackup;

        if (ebx != static_cast<int32_t>(GameCommands::FAILURE))
        {
            if (isEditorMode())
                ebx = 0;

            if (_gameCommandNestLevel == 1)
            {
                if ((_gameCommandFlags & Flags::flag_2) == 0
                    && (_gameCommandFlags & Flags::ghost) == 0
                    && ebx != 0)
                {
                    registers regs2;
                    regs2.ebp = ebx;
                    call(0x0046DD06, regs2);
                    ebx = regs2.ebp;
                }
            }
        }

        if (ebx == static_cast<int32_t>(GameCommands::FAILURE))
        {
            if (flags & Flags::apply)
            {
                return loc_4314EA();
            }
            else
            {
                _gameCommandNestLevel--;
                return ebx;
            }
        }

        if ((flags & 1) == 0)
        {
            _gameCommandNestLevel--;
            return ebx;
        }

        uint16_t flagsBackup2 = _gameCommandFlags;
        registers fnRegs2 = regs;
        callGameCommandFunction(esi, fnRegs2);
        int32_t ebx2 = fnRegs2.ebx;
        _gameCommandFlags = flagsBackup2;

        if (ebx2 == static_cast<int32_t>(GameCommands::FAILURE))
        {
            return loc_4314EA();
        }

        if (isEditorMode())
        {
            ebx = 0;
        }

        if (ebx2 < ebx)
        {
            ebx = ebx2;
        }

        _gameCommandNestLevel--;
        if (_gameCommandNestLevel != 0)
            return ebx;

        if ((flagsBackup2 & Flags::noPayment) != 0)
            return ebx;

        // Apply to company money
        CompanyManager::applyPaymentToCompany(CompanyManager::getUpdatingCompanyId(), ebx, getExpenditureType());

        if (ebx != 0 && _updatingCompanyId == CompanyManager::getControllingId())
        {
            // Add flying cost text
            CompanyManager::spendMoneyEffect(getPosition() + World::Pos3{ 0, 0, 24 }, _updatingCompanyId, ebx);
        }

        return ebx;
    }

    static uint32_t loc_4314EA()
    {
        _gameCommandNestLevel--;
        if (_gameCommandNestLevel != 0)
            return GameCommands::FAILURE;

        if (_updatingCompanyId != CompanyManager::getControllingId())
            return GameCommands::FAILURE;

        if (_gameCommandFlags & Flags::noErrorWindow)
            return GameCommands::FAILURE;

        if (_gGameCommandErrorText != 0xFFFE)
        {
            Windows::showError(_gGameCommandErrorTitle, _gGameCommandErrorText);
            return GameCommands::FAILURE;
        }

        // advanced errors
        if (_9C68D0 != World::TileManager::kInvalidTile)
        {
            auto tile = *_9C68D0;

            switch (tile->type())
            {
                case ElementType::track: // 4
                {
                    auto& trackElement = tile->get<TrackElement>();

                    TrackObject* pObject = ObjectManager::get<TrackObject>(trackElement.trackObjectId());
                    if (pObject == nullptr)
                        break;

                    auto formatter = FormatArguments::common();
                    formatter.push(pObject->name);
                    formatter.push(CompanyManager::get(_errorCompanyId)->name);
                    Windows::Error::openWithCompetitor(_gGameCommandErrorTitle, StringIds::error_reason_stringid_belongs_to, _errorCompanyId);
                    return GameCommands::FAILURE;
                }

                case ElementType::road: // 0x1C
                {
                    auto& roadElement = tile->get<RoadElement>();

                    RoadObject* pObject = ObjectManager::get<RoadObject>(roadElement.roadObjectId());
                    if (pObject == nullptr)
                        break;

                    auto formatter = FormatArguments::common();
                    formatter.push(pObject->name);
                    formatter.push(CompanyManager::get(_errorCompanyId)->name);
                    Windows::Error::openWithCompetitor(_gGameCommandErrorTitle, StringIds::error_reason_stringid_belongs_to, _errorCompanyId);
                    return GameCommands::FAILURE;
                }

                case ElementType::station: // 8
                {
                    auto& stationElement = tile->get<StationElement>();

                    Station* pStation = StationManager::get(stationElement.stationId());
                    if (pStation == nullptr)
                        break;

                    auto formatter = FormatArguments::common();
                    formatter.push(pStation->name);
                    formatter.push(pStation->town);
                    formatter.push(CompanyManager::get(_errorCompanyId)->name);
                    Windows::Error::openWithCompetitor(_gGameCommandErrorTitle, StringIds::error_reason_stringid_belongs_to, _errorCompanyId);
                    return GameCommands::FAILURE;
                }

                case ElementType::signal: // 0x0C
                {
                    auto formatter = FormatArguments::common();
                    formatter.push(CompanyManager::get(_errorCompanyId)->name);
                    Windows::Error::openWithCompetitor(_gGameCommandErrorTitle, StringIds::error_reason_signal_belongs_to, _errorCompanyId);
                    return GameCommands::FAILURE;
                }

                default:
                    break;
            }
        }

        // fallback
        auto formatter = FormatArguments::common();
        formatter.push(CompanyManager::get(_errorCompanyId)->name);
        Windows::Error::openWithCompetitor(_gGameCommandErrorTitle, StringIds::error_reason_stringid_belongs_to, _errorCompanyId);
        return GameCommands::FAILURE;
    }

    // 0x00431E6A
    // al  : company
    // esi : tile
    bool sub_431E6A(const CompanyId company, const World::TileElement* const tile /*= nullptr*/)
    {
        if (company == CompanyId::neutral)
        {
            return true;
        }
        if (_updatingCompanyId == company || _updatingCompanyId == CompanyId::neutral)
        {
            return true;
        }
        _gGameCommandErrorText = 0xFFFEU;
        _errorCompanyId = company;
        _9C68D0 = tile == nullptr ? World::TileManager::kInvalidTile : tile;
        return false;
    }

    const World::Pos3& getPosition()
    {
        return _gGameCommandPosition;
    }

    void setPosition(const World::Pos3& pos)
    {
        _gGameCommandPosition = pos;
    }

    void setErrorText(const StringId message)
    {
        _gGameCommandErrorText = message;
    }

    StringId getErrorText()
    {
        return _gGameCommandErrorText;
    }

    void setErrorTitle(const StringId title)
    {
        _gGameCommandErrorTitle = title;
    }

    ExpenditureType getExpenditureType()
    {
        return ExpenditureType(_gGameCommandExpenditureType / 4);
    }

    void setExpenditureType(const ExpenditureType type)
    {
        _gGameCommandExpenditureType = static_cast<uint8_t>(type) * 4;
    }

    CompanyId getUpdatingCompanyId()
    {
        return _updatingCompanyId;
    }

    void setUpdatingCompanyId(const CompanyId companyId)
    {
        _updatingCompanyId = companyId;
    }

    uint8_t getCommandNestLevel()
    {
        return _gameCommandNestLevel;
    }
}
