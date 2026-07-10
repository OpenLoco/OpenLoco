#include "GameCommands/GameCommands.h"
#include "Audio/Audio.h"
#include "Config.h"
#include "GameCommands/Airports/CreateAirport.h"
#include "GameCommands/Airports/RemoveAirport.h"
#include "GameCommands/Buildings/CreateBuilding.h"
#include "GameCommands/Buildings/RemoveBuilding.h"
#include "GameCommands/Cheats/Cheat.h"
#include "GameCommands/Company/BuildCompanyHeadquarters.h"
#include "GameCommands/Company/ChangeCompanyColour.h"
#include "GameCommands/Company/ChangeCompanyFace.h"
#include "GameCommands/Company/ChangeLoan.h"
#include "GameCommands/Company/RemoveCompanyHeadquarters.h"
#include "GameCommands/Company/RenameCompanyName.h"
#include "GameCommands/Company/RenameCompanyOwner.h"
#include "GameCommands/Company/UpdateOwnerStatus.h"
#include "GameCommands/CompanyAi/AiCreateRoadAndStation.h"
#include "GameCommands/CompanyAi/AiCreateTrackAndStation.h"
#include "GameCommands/CompanyAi/AiTrackReplacement.h"
#include "GameCommands/Docks/CreatePort.h"
#include "GameCommands/Docks/RemovePort.h"
#include "GameCommands/General/LoadSaveQuit.h"
#include "GameCommands/General/RenameStation.h"
#include "GameCommands/General/SetGameSpeed.h"
#include "GameCommands/General/TogglePause.h"
#include "GameCommands/Industries/CreateIndustry.h"
#include "GameCommands/Industries/RemoveIndustry.h"
#include "GameCommands/Industries/RenameIndustry.h"
#include "GameCommands/Road/CreateRoad.h"
#include "GameCommands/Road/CreateRoadMod.h"
#include "GameCommands/Road/CreateRoadStation.h"
#include "GameCommands/Road/RemoveRoad.h"
#include "GameCommands/Road/RemoveRoadMod.h"
#include "GameCommands/Road/RemoveRoadStation.h"
#include "GameCommands/Terraform/ChangeLandMaterial.h"
#include "GameCommands/Terraform/ClearLand.h"
#include "GameCommands/Terraform/CreateTree.h"
#include "GameCommands/Terraform/CreateWall.h"
#include "GameCommands/Terraform/LowerLand.h"
#include "GameCommands/Terraform/LowerRaiseLandMountain.h"
#include "GameCommands/Terraform/LowerWater.h"
#include "GameCommands/Terraform/RaiseLand.h"
#include "GameCommands/Terraform/RaiseWater.h"
#include "GameCommands/Terraform/RemoveTree.h"
#include "GameCommands/Terraform/RemoveWall.h"
#include "GameCommands/Town/CreateTown.h"
#include "GameCommands/Town/RemoveTown.h"
#include "GameCommands/Town/RenameTown.h"
#include "GameCommands/Track/CreateSignal.h"
#include "GameCommands/Track/CreateTrack.h"
#include "GameCommands/Track/CreateTrackMod.h"
#include "GameCommands/Track/CreateTrainStation.h"
#include "GameCommands/Track/RemoveSignal.h"
#include "GameCommands/Track/RemoveTrack.h"
#include "GameCommands/Track/RemoveTrackMod.h"
#include "GameCommands/Track/RemoveTrainStation.h"
#include "GameCommands/Vehicles/CloneVehicle.h"
#include "GameCommands/Vehicles/CreateVehicle.h"
#include "GameCommands/Vehicles/RenameVehicle.h"
#include "GameCommands/Vehicles/VehicleChangeRunningMode.h"
#include "GameCommands/Vehicles/VehicleOrderDelete.h"
#include "GameCommands/Vehicles/VehicleOrderDown.h"
#include "GameCommands/Vehicles/VehicleOrderInsert.h"
#include "GameCommands/Vehicles/VehicleOrderReverse.h"
#include "GameCommands/Vehicles/VehicleOrderSkip.h"
#include "GameCommands/Vehicles/VehicleOrderUp.h"
#include "GameCommands/Vehicles/VehiclePassSignal.h"
#include "GameCommands/Vehicles/VehiclePickup.h"
#include "GameCommands/Vehicles/VehiclePickupAir.h"
#include "GameCommands/Vehicles/VehiclePickupWater.h"
#include "GameCommands/Vehicles/VehiclePlace.h"
#include "GameCommands/Vehicles/VehiclePlaceAir.h"
#include "GameCommands/Vehicles/VehiclePlaceWater.h"
#include "GameCommands/Vehicles/VehicleRearrange.h"
#include "GameCommands/Vehicles/VehicleRefit.h"
#include "GameCommands/Vehicles/VehicleRepaint.h"
#include "GameCommands/Vehicles/VehicleReverse.h"
#include "GameCommands/Vehicles/VehicleSell.h"
#include "GameCommands/Vehicles/VehicleSpeedControl.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Logging.h"
#include "Map/RoadElement.h"
#include "Map/StationElement.h"
#include "Map/Tile.h"
#include "Map/TileManager.h"
#include "Map/TrackElement.h"
#include "Network/Network.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackObject.h"
#include "Random.h"
#include "SceneManager.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include "World/Company.h"
#include "World/CompanyManager.h"
#include "World/StationManager.h"
#include <cassert>

using namespace OpenLoco::Ui;

namespace OpenLoco::GameCommands
{
    static uint8_t _gameCommandNestLevel = 0; // 0x00508F08

    static CompanyId _updatingCompanyId; // 0x009C68EB
    enum class ErrorElementKind : uint8_t
    {
        none,
        track,
        road,
        station,
    };
    struct ErrorData
    {
        ErrorElementKind kind = ErrorElementKind::none;
        union
        {
            uint8_t trackObjectId = 0;
            uint8_t roadObjectId;
            StationId stationId;
        };
    };
    static ErrorData _errorElementData;
    static World::Pos3 _gGameCommandPosition;            // 0x009C68E0
    static StringId _gGameCommandErrorText;              // 0x009C68E6
    static StringId _gGameCommandErrorTitle;             // 0x009C68E8
    static bool _gGameCommandErrorSound = true;          // 0x00508F09
    static ExpenditureType _gGameCommandExpenditureType; // 0x009C68EA
    static CompanyId _errorCompanyId;                    // 0x009C68EE

    static LegacyReturnState _legacyReturnState; // 0x01136072

    using GameCommandFunc = void (*)(registers& regs, const uint8_t flags);

    struct GameCommandInfo
    {
        GameCommand id;
        GameCommandFunc implementation;
        uintptr_t originalAddress; // original array: 0x004F9548
        bool unpausesGame;         // original array: 0x004F9688
    };

    static constexpr StringId kErrorBelongsToOther = 0xFFFEU;

    // clang-format off
    static constexpr GameCommandInfo kGameCommandDefinitions[85] = {
        { GameCommand::vehicleRearrange,             vehicleRearrange,          0x004AF1DF, true  },
        { GameCommand::vehiclePlace,                 vehiclePlace,              0x004B01B6, true  },
        { GameCommand::vehiclePickup,                vehiclePickup,             0x004B0826, true  },
        { GameCommand::vehicleReverse,               vehicleReverse,            0x004ADAA8, true  },
        { GameCommand::vehiclePassSignal,            vehiclePassSignal,         0x004B0B50, true  },
        { GameCommand::vehicleCreate,                createVehicle,             0x004AE5E4, true  },
        { GameCommand::vehicleSell,                  sellVehicle,               0x004AED34, true  },
        { GameCommand::createTrack,                  createTrack,               0x0049BB98, true  },
        { GameCommand::removeTrack,                  removeTrack,               0x0049C7F2, true  },
        { GameCommand::changeLoan,                   changeLoan,                0x0046DE88, false },
        { GameCommand::vehicleRename,                renameVehicle,             0x004B6572, false },
        { GameCommand::changeStationName,            renameStation,             0x00490756, false },
        { GameCommand::vehicleChangeRunningMode,     vehicleChangeRunningMode,  0x004B694B, true  },
        { GameCommand::createSignal,                 createSignal,              0x00488BDB, true  },
        { GameCommand::removeSignal,                 removeSignal,              0x004891E4, true  },
        { GameCommand::createTrainStation,           createTrainStation,        0x0048BB20, true  },
        { GameCommand::removeTrainStation,           removeTrainStation,        0x0048C402, true  },
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
        { GameCommand::createRoad,                   createRoad,                0x00475FBC, true  },
        { GameCommand::removeRoad,                   removeRoad,                0x004775A5, true  },
        { GameCommand::createRoadMod,                createRoadMod,             0x0047A21E, true  },
        { GameCommand::removeRoadMod,                removeRoadMod,             0x0047A42F, true  },
        { GameCommand::createRoadStation,            createRoadStation,         0x0048C708, true  },
        { GameCommand::removeRoadStation,            removeRoadStation,         0x0048D2AC, true  },
        { GameCommand::createBuilding,               createBuilding,            0x0042D133, true  },
        { GameCommand::removeBuilding,               removeBuilding,            0x0042D74E, true  },
        { GameCommand::renameTown,                   renameTown,                0x0049B11E, false },
        { GameCommand::createIndustry,               createIndustry,            0x0045436B, true  },
        { GameCommand::removeIndustry,               removeIndustry,            0x00455943, true  },
        { GameCommand::createTown,                   createTown,                0x00496C22, true  },
        { GameCommand::removeTown,                   removeTown,                0x0049711F, true  },
        { GameCommand::aiCreateTrackAndStation,      aiCreateTrackAndStation,   0x004A6FDC, true  },
        { GameCommand::aiTrackReplacement,           aiTrackReplacement,        0x004A734F, true  },
        { GameCommand::aiCreateRoadAndStation,       aiCreateRoadAndStation,    0x0047AF0B, true  },
        { GameCommand::buildCompanyHeadquarters,     buildCompanyHeadquarters,  0x0042ECFC, true  },
        { GameCommand::removeCompanyHeadquarters,    removeCompanyHeadquarters, 0x0042EEAF, true  },
        { GameCommand::createAirport,                createAirport,             0x00492C41, true  },
        { GameCommand::removeAirport,                removeAirport,             0x00493559, true  },
        { GameCommand::vehiclePlaceAir,              vehiclePlaceAir,           0x004267BE, true  },
        { GameCommand::vehiclePickupAir,             vehiclePickupAir,          0x00426B29, true  },
        { GameCommand::createPort,                   createPort,                0x00493AA7, true  },
        { GameCommand::removePort,                   removePort,                0x00494570, true  },
        { GameCommand::vehiclePlaceWater,            vehiclePlaceWater,         0x0042773C, true  },
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
        { GameCommand::vehicleRepaint,               vehicleRepaint,            0,          false },
    };
    // clang-format on

    static uint32_t loc_4314EA(const uint8_t flags);
    static uint32_t loc_4313C6(int esi, const registers& regs, const uint8_t flags);

    static bool commandRequiresUnpausingGame(GameCommand command, uint16_t flags)
    {
        if ((flags & (Flags::aiAllocated | Flags::ghost)) != 0)
        {
            return false;
        }

        auto& gameCommand = kGameCommandDefinitions[static_cast<uint32_t>(command)];
        if (!gameCommand.unpausesGame || SceneManager::isPauseOverrideEnabled())
        {
            return false;
        }

        return true;
    }

    // 0x00431315
    uint32_t doCommand(GameCommand command, const registers& regs)
    {
        const uint8_t flags = regs.bl;
        uint32_t esi = static_cast<uint32_t>(command);

        if (_gameCommandNestLevel != 0)
        {
            return loc_4313C6(esi, regs, flags);
        }

        if ((flags & Flags::apply) == 0)
        {
            return loc_4313C6(esi, regs, flags);
        }

        auto isGhost = (flags & Flags::ghost) != 0;
        if (!isGhost && Network::isConnected())
        {
            // For network games, we need to delay the command apply processing
            // Just return the result without applying for now
            registers copyRegs = regs;
            copyRegs.esi = static_cast<int32_t>(command);
            Network::queueGameCommand(_updatingCompanyId, copyRegs, flags);

            return loc_4313C6(esi, copyRegs, flags & ~Flags::apply);
        }

        return doCommandForReal(command, _updatingCompanyId, regs, flags);
    }

    uint32_t doCommandForReal(GameCommand command, CompanyId company, const registers& regs, const uint8_t flags)
    {
        _updatingCompanyId = company;

        uint32_t esi = static_cast<uint32_t>(command);

        if (commandRequiresUnpausingGame(command, flags) && _updatingCompanyId == CompanyManager::getControllingId())
        {
            if ((SceneManager::getPauseFlags() & PauseFlags::player) != PauseFlags::none)
            {
                SceneManager::unsetPauseFlag(PauseFlags::player);
                Ui::Windows::PlayerInfoPanel::invalidateFrame();
            }

            if (SceneManager::getGameSpeed() != GameSpeed::Normal)
            {
                // calling the command setGameSpeed will cause infinite recursion here, so just call the real function
                SceneManager::setGameSpeed(GameSpeed::Normal);
            }

            if (SceneManager::isPaused())
            {
                _gGameCommandErrorText = StringIds::empty;
                return GameCommands::kFailure;
            }
        }

        if (_updatingCompanyId == CompanyManager::getControllingId() && SceneManager::isNetworked())
        {
            // assert(false);
            // registers fnRegs = regs;
            // call(0x0046E34A, fnRegs); // some network stuff. Untested
        }

        return loc_4313C6(esi, regs, flags);
    }

    static void callGameCommandFunction(uint32_t command, registers& regs, const uint8_t flags)
    {
        auto& gameCommand = kGameCommandDefinitions[command];
        if (gameCommand.implementation != nullptr)
        {
            gameCommand.implementation(regs, flags);
        }
        else
        {
            auto addr = gameCommand.originalAddress;
            Diagnostics::Logging::error("Unimplemented game command called: id:{}, address:{}", static_cast<uint32_t>(gameCommand.id), addr);
        }
    }

    static uint32_t loc_4313C6(int esi, const registers& regs, const uint8_t flags)
    {
        _gGameCommandErrorText = StringIds::null;
        _gameCommandNestLevel++;

        registers fnRegs1 = regs;
        callGameCommandFunction(esi, fnRegs1, flags & ~Flags::apply);
        int32_t ebx = fnRegs1.ebx;

        if (ebx != static_cast<int32_t>(GameCommands::kFailure))
        {
            if (SceneManager::isEditorMode())
            {
                ebx = 0;
            }

            if (_gameCommandNestLevel == 1)
            {
                if ((flags & Flags::allowNegativeCashFlow) == 0
                    && (flags & Flags::noPayment) == 0
                    && ebx != 0)
                {
                    if (!CompanyManager::ensureCompanyFunding(getUpdatingCompanyId(), ebx))
                    {
                        ebx = GameCommands::kFailure;
                    }
                }
            }
        }

        if (ebx == static_cast<int32_t>(GameCommands::kFailure))
        {
            if (flags & Flags::apply)
            {
                return loc_4314EA(flags);
            }
            else
            {
                _gameCommandNestLevel--;
                return ebx;
            }
        }

        if ((flags & Flags::apply) == 0)
        {
            _gameCommandNestLevel--;
            return ebx;
        }

        registers fnRegs2 = regs;
        callGameCommandFunction(esi, fnRegs2, flags);
        int32_t ebx2 = fnRegs2.ebx;

        if (ebx2 == static_cast<int32_t>(GameCommands::kFailure))
        {
            return loc_4314EA(flags);
        }

        if (SceneManager::isEditorMode())
        {
            ebx = 0;
        }

        if (ebx2 < ebx)
        {
            ebx = ebx2;
        }

        _gameCommandNestLevel--;
        if (_gameCommandNestLevel != 0)
        {
            return ebx;
        }

        if ((flags & Flags::noPayment) != 0)
        {
            return ebx;
        }

        // Apply to company money
        CompanyManager::applyPaymentToCompany(GameCommands::getUpdatingCompanyId(), ebx, getExpenditureType());

        if (ebx != 0 && _updatingCompanyId == CompanyManager::getControllingId())
        {
            // Add flying cost text
            CompanyManager::spendMoneyEffect(getPosition() + World::Pos3{ 0, 0, 24 }, _updatingCompanyId, ebx);
        }

        return ebx;
    }

    static uint32_t loc_4314EA(const uint8_t flags)
    {
        _gameCommandNestLevel--;
        if (_gameCommandNestLevel != 0)
        {
            return GameCommands::kFailure;
        }

        if (_updatingCompanyId != CompanyManager::getControllingId())
        {
            return GameCommands::kFailure;
        }

        if (flags & Flags::noErrorWindow)
        {
            return GameCommands::kFailure;
        }

        if (_gGameCommandErrorText != kErrorBelongsToOther)
        {
            auto openError = _gGameCommandErrorSound ? Windows::Error::open : Windows::Error::openQuiet;
            openError(_gGameCommandErrorTitle, _gGameCommandErrorText);
            return GameCommands::kFailure;
        }

        // advanced errors
        switch (_errorElementData.kind)
        {
            case ErrorElementKind::track:
            {
                const TrackObject* pObject = ObjectManager::get<TrackObject>(_errorElementData.trackObjectId);
                if (pObject != nullptr)
                {
                    auto formatter = FormatArguments::common();
                    formatter.push(pObject->name);
                    formatter.push(CompanyManager::get(_errorCompanyId)->name);
                    Windows::Error::openWithCompetitor(_gGameCommandErrorTitle, StringIds::error_reason_stringid_belongs_to, _errorCompanyId);
                    return GameCommands::kFailure;
                }
                break;
            }
            case ErrorElementKind::road:
            {
                const RoadObject* pObject = ObjectManager::get<RoadObject>(_errorElementData.roadObjectId);
                if (pObject != nullptr)
                {
                    auto formatter = FormatArguments::common();
                    formatter.push(pObject->name);
                    formatter.push(CompanyManager::get(_errorCompanyId)->name);
                    Windows::Error::openWithCompetitor(_gGameCommandErrorTitle, StringIds::error_reason_stringid_belongs_to, _errorCompanyId);
                    return GameCommands::kFailure;
                }
                break;
            }
            case ErrorElementKind::station:
            {
                const Station* pStation = StationManager::get(_errorElementData.stationId);
                if (pStation != nullptr)
                {
                    auto formatter = FormatArguments::common();
                    formatter.push(pStation->name);
                    formatter.push(pStation->town);
                    formatter.push(CompanyManager::get(_errorCompanyId)->name);
                    Windows::Error::openWithCompetitor(_gGameCommandErrorTitle, StringIds::error_reason_stringid_belongs_to, _errorCompanyId);
                    return GameCommands::kFailure;
                }
                break;
            }
            case ErrorElementKind::none:
                break;
        }

        // Fallback + ErrorElementKind::none is a valid case which just means the error belongs to a company without an associated element
        auto formatter = FormatArguments::common();
        formatter.push(CompanyManager::get(_errorCompanyId)->name);
        Windows::Error::openWithCompetitor(_gGameCommandErrorTitle, StringIds::error_reason_belongs_to, _errorCompanyId);
        return GameCommands::kFailure;
    }

    // 0x00431E6A
    // al  : company
    // esi : tile
    static bool checkCompanyCompatibilityCommon(const CompanyId company)
    {
        if (company == CompanyId::neutral)
        {
            return true;
        }
        if (_updatingCompanyId == company || _updatingCompanyId == CompanyId::neutral)
        {
            return true;
        }
        _gGameCommandErrorText = kErrorBelongsToOther;
        _errorCompanyId = company;
        return false;
    }

    bool checkCompanyCompatibility(const CompanyId company)
    {
        if (checkCompanyCompatibilityCommon(company))
        {
            return true;
        }
        _errorElementData.kind = ErrorElementKind::none;
        return false;
    }

    bool checkCompanyCompatibility(const CompanyId company, const World::TrackElement& elTrack)
    {
        if (checkCompanyCompatibilityCommon(company))
        {
            return true;
        }
        _errorElementData.kind = ErrorElementKind::track;
        _errorElementData.trackObjectId = elTrack.trackObjectId();
        return false;
    }

    bool checkCompanyCompatibility(const CompanyId company, const World::RoadElement& elRoad)
    {
        if (checkCompanyCompatibilityCommon(company))
        {
            return true;
        }
        _errorElementData.kind = ErrorElementKind::road;
        _errorElementData.roadObjectId = elRoad.roadObjectId();
        return false;
    }

    bool checkCompanyCompatibility(const CompanyId company, const World::StationElement& elStation)
    {
        if (checkCompanyCompatibilityCommon(company))
        {
            return true;
        }
        _errorElementData.kind = ErrorElementKind::station;
        _errorElementData.stationId = elStation.stationId();
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

    void setErrorSound(bool state)
    {
        _gGameCommandErrorSound = state;
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
        return _gGameCommandExpenditureType;
    }

    void setExpenditureType(const ExpenditureType type)
    {
        _gGameCommandExpenditureType = type;
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

    void resetCommandNestLevel()
    {
        _gameCommandNestLevel = 0;
    }

    LegacyReturnState& getLegacyReturnState()
    {
        return _legacyReturnState;
    }

    // TODO: Maybe move this somewhere else used by multiple game commands
    // 0x0048B013
    void playConstructionPlacementSound(World::Pos3 pos)
    {
        const auto frequency = gPrng2().randNext(17955, 26146);
        Audio::playSound(Audio::SoundId::construct, Audio::ChannelId::effects, pos, 0, frequency);
    }

    // TODO: Maybe move this somewhere else used by multiple game commands
    bool shouldInvalidateTile(uint8_t flags)
    {
        return !(flags & Flags::aiAllocated) || Config::get().showAiPlanningAsGhosts;
    }
}
