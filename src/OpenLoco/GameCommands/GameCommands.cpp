#include "GameCommands.h"
#include "../Audio/Audio.h"
#include "../Company.h"
#include "../CompanyManager.h"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Map/Tile.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/RoadObject.h"
#include "../Objects/TrackObject.h"
#include "../StationManager.h"
#include "../Ui/WindowManager.h"
#include "../Vehicles/Vehicle.h"
#include <cassert>

using namespace OpenLoco::Ui;
using namespace OpenLoco::Map;

namespace OpenLoco::GameCommands
{
    static loco_global<CompanyId, 0x009C68EB> _updatingCompanyId;
    static loco_global<uint8_t, 0x00508F08> game_command_nest_level;
    static loco_global<CompanyId[2], 0x00525E3C> _player_company;
    static loco_global<uint8_t, 0x00508F17> paused_state;

    static uint16_t _gameCommandFlags;

    static loco_global<TileElement*, 0x009C68D0> _9C68D0;

    static loco_global<Pos3, 0x009C68E0> _gGameCommandPosition;
    static loco_global<string_id, 0x009C68E6> _gGameCommandErrorText;
    static loco_global<string_id, 0x009C68E8> _gGameCommandErrorTitle;
    static loco_global<uint8_t, 0x009C68EA> _gGameCommandExpenditureType; // premultiplied by 4
    static loco_global<CompanyId, 0x009C68EE> _errorCompanyId;
    static loco_global<string_id[8], 0x112C826> _commonFormatArgs;

    using GameCommandFunc = void (*)(registers& regs);

    struct GameCommandInfo
    {
        GameCommand id;
        GameCommandFunc implementation;
        uintptr_t originalAddress; // original array: 0x004F9548
        bool unpausesGame;         // original array: 0x004F9688
    };

    // clang-format off
    static constexpr GameCommandInfo kGameCommandDefinitions[83] = {
        { GameCommand::vehicleRearrange,             nullptr,                   0x004AF1DF, true  },
        { GameCommand::vehiclePlace,                 nullptr,                   0x004B01B6, true  },
        { GameCommand::vehiclePickup,                vehiclePickup,             0x004B0826, true  },
        { GameCommand::vehicleReverse,               nullptr,                   0x004ADAA8, true  },
        { GameCommand::vehiclePassSignal,            nullptr,                   0x004B0B50, true  },
        { GameCommand::vehicleCreate,                Vehicles::create,          0x004AE5E4, true  },
        { GameCommand::vehicleSell,                  nullptr,                   0x004AED34, true  },
        { GameCommand::createTrack,                  nullptr,                   0x0049BB98, true  },
        { GameCommand::removeTrack,                  nullptr,                   0x0049C7F2, true  },
        { GameCommand::changeLoan,                   changeLoan,                0x0046DE88, false },
        { GameCommand::vehicleRename,                Vehicles::rename,          0x004B6572, false },
        { GameCommand::changeStationName,            renameStation,             0x00490756, false },
        { GameCommand::vehicleLocalExpress,          nullptr,                   0x004B694B, true  },
        { GameCommand::createSignal,                 nullptr,                   0x00488BDB, true  },
        { GameCommand::removeSignal,                 nullptr,                   0x004891E4, true  },
        { GameCommand::createTrainStation,           nullptr,                   0x0048BB20, true  },
        { GameCommand::removeTrackStation,           nullptr,                   0x0048C402, true  },
        { GameCommand::createTrackMod,               nullptr,                   0x004A6479, true  },
        { GameCommand::removeTrackMod,               nullptr,                   0x004A668A, true  },
        { GameCommand::changeCompanyColourScheme,    changeCompanyColour,       0x0043483D, false },
        { GameCommand::pauseGame,                    togglePause,               0x00431E32, false },
        { GameCommand::loadSaveQuitGame,             loadSaveQuit,              0x0043BFCB, false },
        { GameCommand::removeTree,                   removeTree,                0x004BB392, true  },
        { GameCommand::createTree,                   nullptr,                   0x004BB138, true  },
        { GameCommand::changeLandMaterial,           changeLandMaterial,        0x00468EDD, true  },
        { GameCommand::raiseLand,                    nullptr,                   0x00463702, true  },
        { GameCommand::lowerLand,                    nullptr,                   0x004638C6, true  },
        { GameCommand::lowerRaiseLandMountain,       nullptr,                   0x00462DCE, true  },
        { GameCommand::raiseWater,                   nullptr,                   0x004C4F19, true  },
        { GameCommand::lowerWater,                   nullptr,                   0x004C5126, true  },
        { GameCommand::changeCompanyName,            nullptr,                   0x00434914, false },
        { GameCommand::changeCompanyOwnerName,       nullptr,                   0x00434A58, false },
        { GameCommand::createWall,                   nullptr,                   0x004C436C, true  },
        { GameCommand::removeWall,                   removeWall,                0x004C466C, true  },
        { GameCommand::gc_unk_34,                    nullptr,                   0x004C4717, false },
        { GameCommand::vehicleOrderInsert,           nullptr,                   0x0047036E, false },
        { GameCommand::vehicleOrderDelete,           nullptr,                   0x0047057A, false },
        { GameCommand::vehicleOrderSkip,             Vehicles::orderSkip,       0x0047071A, false },
        { GameCommand::createRoad,                   nullptr,                   0x00475FBC, true  },
        { GameCommand::removeRoad,                   nullptr,                   0x004775A5, true  },
        { GameCommand::createRoadMod,                nullptr,                   0x0047A21E, true  },
        { GameCommand::removeRoadMod,                nullptr,                   0x0047A42F, true  },
        { GameCommand::createRoadStation,            nullptr,                   0x0048C708, true  },
        { GameCommand::removeRoadStation,            nullptr,                   0x0048D2AC, true  },
        { GameCommand::createBuilding,               nullptr,                   0x0042D133, true  },
        { GameCommand::removeBuilding,               nullptr,                   0x0042D74E, true  },
        { GameCommand::renameTown,                   renameTown,                0x0049B11E, false },
        { GameCommand::createIndustry,               nullptr,                   0x0045436B, true  },
        { GameCommand::removeIndustry,               nullptr,                   0x00455943, true  },
        { GameCommand::createTown,                   nullptr,                   0x00496C22, true  },
        { GameCommand::removeTown,                   nullptr,                   0x0049711F, true  },
        { GameCommand::gc_unk_51,                    nullptr,                   0x004A6FDC, true  },
        { GameCommand::gc_unk_52,                    nullptr,                   0x004A734F, true  },
        { GameCommand::gc_unk_53,                    nullptr,                   0x0047AF0B, true  },
        { GameCommand::buildCompanyHeadquarters,     nullptr,                   0x0042ECFC, true  },
        { GameCommand::removeCompanyHeadquarters,    nullptr,                   0x0042EEAF, true  },
        { GameCommand::createAirport,                nullptr,                   0x00492C41, true  },
        { GameCommand::removeAirport,                nullptr,                   0x00493559, true  },
        { GameCommand::vehiclePlaceAir,              nullptr,                   0x004267BE, true  },
        { GameCommand::vehiclePickupAir,             nullptr,                   0x00426B29, true  },
        { GameCommand::createPort,                   nullptr,                   0x00493AA7, true  },
        { GameCommand::removePort,                   nullptr,                   0x00494570, true  },
        { GameCommand::vehiclePlaceWater,            nullptr,                   0x0042773C, true  },
        { GameCommand::vehiclePickupWater,           nullptr,                   0x004279CC, true  },
        { GameCommand::vehicleRefit,                 nullptr,                   0x0042F6DB, false },
        { GameCommand::changeCompanyFace,            nullptr,                   0x00435506, false },
        { GameCommand::clearLand,                    nullptr,                   0x00469CCB, true  },
        { GameCommand::loadMultiplayerMap,           nullptr,                   0x00444DA0, false },
        { GameCommand::gc_unk_68,                    nullptr,                   0x0046F8A5, false },
        { GameCommand::gc_unk_69,                    nullptr,                   0x004454BE, false },
        { GameCommand::gc_unk_70,                    nullptr,                   0x004456C8, false },
        { GameCommand::sendChatMessage,              nullptr,                   0x0046F976, false },
        { GameCommand::multiplayerSave,              nullptr,                   0x004A0ACD, false },
        { GameCommand::updateOwnerStatus,            updateOwnerStatus,         0x004383CA, false },
        { GameCommand::vehicleSpeedControl,          nullptr,                   0x004BAB63, true  },
        { GameCommand::vehicleOrderUp,               nullptr,                   0x00470CD2, false },
        { GameCommand::vehicleOrderDown,             nullptr,                   0x00470E06, false },
        { GameCommand::vehicleApplyShuntCheat,       vehicleShuntCheat,         0x004BAC53, false },
        { GameCommand::applyFreeCashCheat,           freeCashCheat,             0x00438A08, false },
        { GameCommand::renameIndustry,               renameIndustry,            0x00455029, false },
        { GameCommand::vehicleClone,                 Vehicles::cloneVehicle,    0,          true  },
        { GameCommand::cheat,                        cheat,                     0,          true  },
        { GameCommand::setGameSpeed,                 setGameSpeed,              0,          true  },
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
    }

    static uint32_t loc_4314EA();
    static uint32_t loc_4313C6(int esi, const registers& regs);

    static bool commandRequiresUnpausingGame(GameCommand command, uint16_t flags)
    {
        if ((flags & (Flags::flag_4 | Flags::flag_6)) != 0)
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
        if (game_command_nest_level != 0)
            return loc_4313C6(esi, regs);

        if ((flags & Flags::apply) == 0)
        {
            return loc_4313C6(esi, regs);
        }

        if (commandRequiresUnpausingGame(command, flags) && _updatingCompanyId == _player_company[0])
        {
            if (getPauseFlags() & 1)
            {
                paused_state = paused_state ^ 1;
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

        if (_updatingCompanyId == _player_company[0] && isNetworked())
        {
            assert(false);
            registers fnRegs = regs;
            call(0x0046E34A, fnRegs); // some network stuff. Untested
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
        game_command_nest_level++;

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

            if (game_command_nest_level == 1)
            {
                if ((_gameCommandFlags & Flags::flag_2) == 0
                    && (_gameCommandFlags & Flags::flag_6) == 0
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
                game_command_nest_level--;
                return ebx;
            }
        }

        if ((flags & 1) == 0)
        {
            game_command_nest_level--;
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

        game_command_nest_level--;
        if (game_command_nest_level != 0)
            return ebx;

        if ((flagsBackup2 & Flags::flag_5) != 0)
            return ebx;

        // Apply to company money
        CompanyManager::applyPaymentToCompany(CompanyManager::getUpdatingCompanyId(), ebx, getExpenditureType());

        if (ebx != 0 && _updatingCompanyId == _player_company[0])
        {
            // Add flying cost text
            CompanyManager::spendMoneyEffect(getPosition() + Map::Pos3{ 0, 0, 24 }, _updatingCompanyId, ebx);
        }

        return ebx;
    }

    static uint32_t loc_4314EA()
    {
        game_command_nest_level--;
        if (game_command_nest_level != 0)
            return GameCommands::FAILURE;

        if (_updatingCompanyId != _player_company[0])
            return GameCommands::FAILURE;

        if (_gameCommandFlags & Flags::flag_3)
            return GameCommands::FAILURE;

        if (_gGameCommandErrorText != 0xFFFE)
        {
            Windows::showError(_gGameCommandErrorTitle, _gGameCommandErrorText);
            return GameCommands::FAILURE;
        }

        // advanced errors
        if (_9C68D0 != (void*)-1)
        {
            auto tile = (TileElement*)_9C68D0;

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

                case ElementType::road: //0x1C
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
    bool sub_431E6A(const CompanyId company, Map::TileElement* const tile /*= nullptr*/)
    {
        if (company == CompanyId::neutral)
        {
            return true;
        }
        if (_updatingCompanyId == company || _updatingCompanyId == CompanyId::neutral)
        {
            return true;
        }
        _gGameCommandErrorText = -2;
        _errorCompanyId = company;
        _9C68D0 = tile == nullptr ? reinterpret_cast<Map::TileElement*>(-1) : tile;
        return false;
    }

    const Map::Pos3& getPosition()
    {
        return _gGameCommandPosition;
    }

    void setPosition(const Map::Pos3& pos)
    {
        _gGameCommandPosition = pos;
    }

    void setErrorText(const string_id message)
    {
        _gGameCommandErrorText = message;
    }

    string_id getErrorText()
    {
        return _gGameCommandErrorText;
    }

    void setErrorTitle(const string_id title)
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
}
