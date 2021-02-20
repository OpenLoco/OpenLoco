#include "GameCommands.h"
#include "Audio/Audio.h"
#include "Company.h"
#include "CompanyManager.h"
#include "Map/Tile.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackObject.h"
#include "StationManager.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include <cassert>

using namespace OpenLoco::Ui;
using namespace OpenLoco::Map;

namespace OpenLoco::GameCommands
{
    static loco_global<company_id_t, 0x009C68EB> _updating_company_id;
    static loco_global<uint8_t, 0x00508F08> game_command_nest_level;
    static loco_global<company_id_t[2], 0x00525E3C> _player_company;
    static loco_global<uint8_t, 0x00508F17> paused_state;
    static loco_global<uint8_t, 0x00508F1A> game_speed;
    static loco_global<uint16_t, 0x0050A004> _50A004;

    static uint16_t _gameCommandFlags;

    static loco_global<tile_element*, 0x009C68D0> _9C68D0;

    static loco_global<coord_t, 0x009C68E4> _game_command_map_z;
    static loco_global<string_id, 0x009C68E6> gGameCommandErrorText;
    static loco_global<string_id, 0x009C68E8> gGameCommandErrorTitle;
    static loco_global<uint8_t, 0x009C68EE> _errorCompanyId;
    static loco_global<string_id[8], 0x112C826> _commonFormatArgs;

    using GameCommandFunc = void (*)(registers& regs);

    struct GameCommandInfo
    {
        GameCommandFunc implementation;
        uintptr_t originalAddress; // original array: 0x004F9548
        bool canUseWhilePaused;    // original array: 0x004F9688
    };

    // clang-format off
    static constexpr GameCommandInfo _gameCommandDefinitions[81] = {
        { nullptr,                0x004AF1DF, true  }, // vehicle_rearrange = 0,
        { nullptr,                0x004B01B6, true  }, // vehicle_place = 1,
        { nullptr,                0x004B0826, true  }, // vehicle_pickup = 2,
        { nullptr,                0x004ADAA8, true  }, // vehicle_reverse = 3,
        { nullptr,                0x004B0B50, true  }, // 4
        { Vehicles::create,       0x004AE5E4, true  }, // vehicle_create = 5,
        { nullptr,                0x004AED34, true  }, // vehicle_sell = 6,
        { nullptr,                0x0049BB98, true  }, // 7
        { nullptr,                0x0049C7F2, true  }, // 8
        { nullptr,                0x0046DE88, false }, // build_vehicle = 9,
        { nullptr,                0x004B6572, false }, // vehicle_rename = 10,
        { nullptr,                0x00490756, false }, // change_station_name = 11,
        { nullptr,                0x004B694B, true  }, // vehicle_local_express = 12,
        { nullptr,                0x00488BDB, true  }, // 13
        { nullptr,                0x004891E4, true  }, // 14
        { nullptr,                0x0048BB20, true  }, // 15
        { nullptr,                0x0048C402, true  }, // 16
        { nullptr,                0x004A6479, true  }, // 17
        { nullptr,                0x004A668A, true  }, // 18
        { nullptr,                0x0043483D, false }, // change_company_colour_scheme = 19,
        { nullptr,                0x00431E32, false }, // pause_game = 20,
        { nullptr,                0x0043BFCB, false }, // load_save_quit_game = 21,
        { nullptr,                0x004BB392, true  }, // 22
        { nullptr,                0x004BB138, true  }, // 23
        { nullptr,                0x00468EDD, true  }, // change_land_material = 24,
        { nullptr,                0x00463702, true  }, // raise_land = 25,
        { nullptr,                0x004638C6, true  }, // lower_land = 26,
        { nullptr,                0x00462DCE, true  }, // lower_raise_land_mountain = 27,
        { nullptr,                0x004C4F19, true  }, // raise_water = 28,
        { nullptr,                0x004C5126, true  }, // lower_water = 29,
        { nullptr,                0x00434914, false }, // 30
        { nullptr,                0x00434A58, false }, // 31
        { nullptr,                0x004C436C, true  }, // 32
        { nullptr,                0x004C466C, true  }, // 33
        { nullptr,                0x004C4717, false }, // 34
        { nullptr,                0x0047036E, false }, // vehicle_order_insert = 35,
        { nullptr,                0x0047057A, false }, // vehicle_order_delete = 36,
        { nullptr,                0x0047071A, false }, // vehicle_order_skip = 37,
        { nullptr,                0x00475FBC, true  }, // 38
        { nullptr,                0x004775A5, true  }, // 39
        { nullptr,                0x0047A21E, true  }, // 40
        { nullptr,                0x0047A42F, true  }, // 41
        { nullptr,                0x0048C708, true  }, // 42
        { nullptr,                0x0048D2AC, true  }, // 43
        { nullptr,                0x0042D133, true  }, // 44
        { nullptr,                0x0042D74E, true  }, // 45
        { nullptr,                0x0049B11E, false }, // change_company_name = 46,
        { nullptr,                0x0045436B, true  }, // 47
        { nullptr,                0x00455943, true  }, // 48
        { nullptr,                0x00496C22, true  }, // 49
        { nullptr,                0x0049711F, true  }, // 50
        { nullptr,                0x004A6FDC, true  }, // 51
        { nullptr,                0x004A734F, true  }, // 52
        { nullptr,                0x0047AF0B, true  }, // 53
        { nullptr,                0x0042ECFC, true  }, // remove_industry = 54,
        { nullptr,                0x0042EEAF, true  }, // build_company_headquarters = 55,
        { nullptr,                0x00492C41, true  }, // 56
        { nullptr,                0x00493559, true  }, // 57
        { nullptr,                0x004267BE, true  }, // 58
        { nullptr,                0x00426B29, true  }, // vehicle_abort_pickup_air = 59,
        { nullptr,                0x00493AA7, true  }, // 60
        { nullptr,                0x00494570, true  }, // 61
        { nullptr,                0x0042773C, true  }, // 62
        { nullptr,                0x004279CC, true  }, // vehicle_abort_pickup_water = 63,
        { nullptr,                0x0042F6DB, false }, // 63
        { nullptr,                0x00435506, false }, // 64
        { nullptr,                0x00469CCB, true  }, // change_company_face = 66,
        { nullptr,                0x00444DA0, false }, // load_multiplayer_map = 67,
        { nullptr,                0x0046F8A5, false }, // 68
        { nullptr,                0x004454BE, false }, // 69
        { nullptr,                0x004456C8, false }, // 70
        { nullptr,                0x0046F976, false }, // send_chat_message = 71,
        { nullptr,                0x004A0ACD, false }, // multiplayer_save = 72,
        { nullptr,                0x004383CA, false }, // update_owner_status = 73,
        { nullptr,                0x004BAB63, true  }, // vehicle_speed_control = 74,
        { nullptr,                0x00470CD2, false }, // vehicle_order_up = 75,
        { nullptr,                0x00470E06, false }, // vehicle_order_down = 76,
        { nullptr,                0x004BAC53, false }, // vehicle_apply_shunt_cheat = 77,
        { nullptr,                0x00438A08, false }, // apply_free_cash_cheat = 78,
        { nullptr,                0x00455029, false }, // rename_industry = 79,
        { Vehicles::cloneVehicle, 0,          true  }, // vehicle_clone = 80,
    };
    // clang-format on

    void registerHooks()
    {
        registerHook(
            0x00431315,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto ebx = doCommand(regs.esi, backup);

                regs = backup;
                regs.ebx = ebx;
                return 0;
            });
    }

    static uint32_t loc_4314EA();
    static uint32_t loc_4313C6(int esi, const registers& regs);

    // 0x00431315
    uint32_t doCommand(int esi, const registers& regs)
    {
        uint16_t flags = regs.bx;

        _gameCommandFlags = regs.bx;
        if (game_command_nest_level != 0)
            return loc_4313C6(esi, regs);

        if ((flags & GameCommandFlag::apply) == 0)
        {
            return loc_4313C6(esi, regs);
        }

        auto& gameCommand = _gameCommandDefinitions[esi];
        if ((flags & (GameCommandFlag::flag_4 | GameCommandFlag::flag_6)) == 0
            && gameCommand.canUseWhilePaused
            && _updating_company_id == _player_company[0])
        {
            if (getPauseFlags() & 1)
            {
                paused_state = paused_state ^ 1;
                WindowManager::invalidate(WindowType::timeToolbar);
                Audio::unpauseSound();
                _50A004 = _50A004 | 1;
            }

            if (game_speed != 0)
            {
                game_speed = 0;
                WindowManager::invalidate(WindowType::timeToolbar);
            }

            if (isPaused())
            {
                gGameCommandErrorText = StringIds::empty;
                return 0x80000000;
            }
        }

        if (_updating_company_id == _player_company[0] && isNetworked())
        {
            assert(false);
            registers fnRegs = regs;
            call(0x0046E34A, fnRegs); // some network stuff. Untested
        }

        return loc_4313C6(esi, regs);
    }

    static void callGameCommandFunction(uint32_t command, registers& regs)
    {
        auto& gameCommand = _gameCommandDefinitions[command];
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
        gGameCommandErrorText = StringIds::null;
        game_command_nest_level++;

        uint16_t flagsBackup = _gameCommandFlags;
        registers fnRegs1 = regs;
        fnRegs1.bl &= ~GameCommandFlag::apply;
        callGameCommandFunction(esi, fnRegs1);
        int32_t ebx = fnRegs1.ebx;
        _gameCommandFlags = flagsBackup;

        if (ebx != static_cast<int32_t>(0x80000000))
        {
            if (isEditorMode())
                ebx = 0;

            if (game_command_nest_level == 1)
            {
                if ((_gameCommandFlags & GameCommandFlag::flag_2) == 0
                    && (_gameCommandFlags & GameCommandFlag::flag_6) == 0
                    && ebx != 0)
                {
                    registers regs2;
                    regs2.ebp = ebx;
                    call(0x0046DD06, regs2);
                    ebx = regs2.ebp;
                }
            }
        }

        if (ebx == static_cast<int32_t>(0x80000000))
        {
            if (flags & GameCommandFlag::apply)
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

        if (ebx2 == static_cast<int32_t>(0x80000000))
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

        if ((flagsBackup2 & GameCommandFlag::flag_5) != 0)
            return ebx;

        {
            // Apply to company money
            registers fnRegs;
            fnRegs.ebx = ebx;
            call(0x0046DE2B, fnRegs);
        }

        if (ebx != 0 && _updating_company_id == _player_company[0])
        {
            // Add flying cost text
            registers fnRegs;
            fnRegs.ebx = ebx;
            _game_command_map_z = _game_command_map_z + 24;
            call(0x0046DC9F, fnRegs);
            _game_command_map_z = _game_command_map_z - 24;
        }

        return ebx;
    }

    static uint32_t loc_4314EA()
    {
        game_command_nest_level--;
        if (game_command_nest_level != 0)
            return 0x80000000;

        if (_updating_company_id != _player_company[0])
            return 0x80000000;

        if (_gameCommandFlags & GameCommandFlag::flag_3)
            return 0x80000000;

        if (gGameCommandErrorText != 0xFFFE)
        {
            Windows::showError(gGameCommandErrorTitle, gGameCommandErrorText);
            return 0x80000000;
        }

        // advanced errors
        if (_9C68D0 != (void*)-1)
        {
            auto tile = (tile_element*)_9C68D0;

            switch (tile->type())
            {
                case element_type::track: // 4
                {
                    auto trackElement = tile->asTrack();
                    if (trackElement == nullptr)
                        break; // throw exception?

                    track_object* pObject = ObjectManager::get<track_object>(trackElement->trackObjectId());
                    if (pObject == nullptr)
                        break;

                    _commonFormatArgs[0] = pObject->name;
                    _commonFormatArgs[1] = CompanyManager::get(_errorCompanyId)->name;
                    Windows::Error::openWithCompetitor(gGameCommandErrorTitle, StringIds::error_reason_stringid_belongs_to, _errorCompanyId);
                    return 0x80000000;
                }

                case element_type::road: //0x1C
                {
                    auto roadElement = tile->asRoad();
                    if (roadElement == nullptr)
                        break; // throw exception?

                    road_object* pObject = ObjectManager::get<road_object>(roadElement->roadObjectId());
                    if (pObject == nullptr)
                        break;

                    _commonFormatArgs[0] = pObject->name;
                    _commonFormatArgs[1] = CompanyManager::get(_errorCompanyId)->name;
                    Windows::Error::openWithCompetitor(gGameCommandErrorTitle, StringIds::error_reason_stringid_belongs_to, _errorCompanyId);
                    return 0x80000000;
                }

                case element_type::station: // 8
                {
                    auto stationElement = tile->asStation();
                    if (stationElement == nullptr)
                        break; // throw exception?

                    station* pStation = StationManager::get(stationElement->stationId());
                    if (pStation == nullptr)
                        break;

                    _commonFormatArgs[0] = pStation->name;
                    _commonFormatArgs[1] = pStation->town;
                    _commonFormatArgs[2] = CompanyManager::get(_errorCompanyId)->name;
                    Windows::Error::openWithCompetitor(gGameCommandErrorTitle, StringIds::error_reason_stringid_belongs_to, _errorCompanyId);
                    return 0x80000000;
                }

                case element_type::signal: // 0x0C
                {
                    _commonFormatArgs[0] = CompanyManager::get(_errorCompanyId)->name;
                    Windows::Error::openWithCompetitor(gGameCommandErrorTitle, StringIds::error_reason_signal_belongs_to, _errorCompanyId);
                    return 0x80000000;
                }

                default:
                    break;
            }
        }

        // fallback
        _commonFormatArgs[0] = CompanyManager::get(_errorCompanyId)->name;
        Windows::Error::openWithCompetitor(gGameCommandErrorTitle, StringIds::error_reason_stringid_belongs_to, _errorCompanyId);
        return 0x80000000;
    }

    // 0x00431E6A
    // al  : company
    // esi : tile
    bool sub_431E6A(const company_id_t company, Map::tile_element* const tile /*= nullptr*/)
    {
        if (company == CompanyId::neutral)
        {
            return true;
        }
        if (_updating_company_id == company || _updating_company_id == CompanyId::neutral)
        {
            return true;
        }
        gGameCommandErrorText = -2;
        _errorCompanyId = company;
        _9C68D0 = tile == nullptr ? reinterpret_cast<Map::tile_element*>(-1) : tile;
        return false;
    }
}
