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
    static loco_global<uintptr_t[80], 0x004F9548> _4F9548;

    // 0x004F9688
    bool _gameCommandCanBeUsedWhenPaused[80] = {
        true,
        true,
        true,
        true,
        true,
        true,
        true,
        true,
        true,
        false,
        false,
        false,
        true,
        true,
        true,
        true,
        true,
        true,
        true,
        false,
        false,
        false,
        true,
        true,
        true,
        true,
        true,
        true,
        true,
        true,
        false,
        false,
        true,
        true,
        false,
        false,
        false,
        false,
        true,
        true,
        true,
        true,
        true,
        true,
        true,
        true,
        false,
        true,
        true,
        true,
        true,
        true,
        true,
        true,
        true,
        true,
        true,
        true,
        true,
        true,
        true,
        true,
        true,
        true,
        false,
        false,
        true,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        true,
        false,
        false,
        false,
        false,
        false,
    };

    static loco_global<tile_element*, 0x009C68D0> _9C68D0;

    static loco_global<coord_t, 0x009C68E4> _game_command_map_z;
    static loco_global<string_id, 0x009C68E6> gGameCommandErrorText;
    static loco_global<string_id, 0x009C68E8> gGameCommandErrorTitle;
    static loco_global<uint8_t, 0x009C68EE> _errorCompanyId;
    static loco_global<string_id[8], 0x112C826> _commonFormatArgs;

    using GameCommandFunc = void (*)(registers& regs);
    static GameCommandFunc _gameCommandFunctions[80] = {
        nullptr,          // vehicle_rearrange = 0,             0x004AF1DF
        nullptr,          // vehicle_place = 1,                 0x004B01B6
        nullptr,          // vehicle_pickup = 2,                0x004B0826
        nullptr,          // vehicle_reverse = 3,               0x004ADAA8
        nullptr,          // 4                                  0x004B0B50
        Vehicles::create, // vehicle_create = 5,
        nullptr,          // vehicle_sell = 6,                  0x004AED34
        nullptr,          // 7                                  0x0049BB98
        nullptr,          // 8                                  0x0049C7F2
        nullptr,          // build_vehicle = 9,                 0x0046DE88
        nullptr,          // vehicle_rename = 10,               0x004B6572
        nullptr,          // change_station_name = 11,          0x00490756
        nullptr,          // vehicle_local_express = 12,        0x004B694B
        nullptr,          // 13                                 0x00488BDB
        nullptr,          // 14                                 0x004891E4
        nullptr,          // 15                                 0x0048BB20
        nullptr,          // 16                                 0x0048C402
        nullptr,          // 17                                 0x004A6479
        nullptr,          // 18                                 0x004A668A
        nullptr,          // change_company_colour_scheme = 19, 0x0043483D
        nullptr,          // pause_game = 20,                   0x00431E32
        nullptr,          // load_save_quit_game = 21,          0x0043BFCB
        nullptr,          // 22                                 0x004BB392
        nullptr,          // 23                                 0x004BB138
        nullptr,          // change_land_material = 24,         0x00468EDD
        nullptr,          // raise_land = 25,                   0x00463702
        nullptr,          // lower_land = 26,                   0x004638C6
        nullptr,          // lower_raise_land_mountain = 27,    0x00462DCE
        nullptr,          // raise_water = 28,                  0x004C4F19
        nullptr,          // lower_water = 29,                  0x004C5126
        nullptr,          // 30                                 0x00434914
        nullptr,          // 31                                 0x00434A58
        nullptr,          // 32                                 0x004C436C
        nullptr,          // 33                                 0x004C466C
        nullptr,          // 34                                 0x004C4717
        nullptr,          // vehicle_order_insert = 35,         0x0047036E
        nullptr,          // vehicle_order_delete = 36,         0x0047057A
        nullptr,          // vehicle_order_skip = 37,           0x0047071A
        nullptr,          // 38                                 0x00475FBC
        nullptr,          // 39                                 0x004775A5
        nullptr,          // 40                                 0x0047A21E
        nullptr,          // 41                                 0x0047A42F
        nullptr,          // 42                                 0x0048C708
        nullptr,          // 43                                 0x0048D2AC
        nullptr,          // 44                                 0x0042D133
        nullptr,          // 45                                 0x0042D74E
        nullptr,          // change_company_name = 46,          0x0049B11E
        nullptr,          // 47                                 0x0045436B
        nullptr,          // 48                                 0x00455943
        nullptr,          // 49                                 0x00496C22
        nullptr,          // 50                                 0x0049711F
        nullptr,          // 51                                 0x004A6FDC
        nullptr,          // 52                                 0x004A734F
        nullptr,          // 53                                 0x0047AF0B
        nullptr,          // remove_industry = 54,              0x0042ECFC
        nullptr,          // build_company_headquarters = 55,   0x0042EEAF
        nullptr,          // 56                                 0x00492C41
        nullptr,          // 57                                 0x00493559
        nullptr,          // 58                                 0x004267BE
        nullptr,          // vehicle_abort_pickup_air = 59,     0x00426B29
        nullptr,          // 60                                 0x00493AA7
        nullptr,          // 61                                 0x00494570
        nullptr,          // 62                                 0x0042773C
        nullptr,          // vehicle_abort_pickup_water = 63,   0x004279CC
        nullptr,          // 63                                 0x0042F6DB
        nullptr,          // 64                                 0x00435506
        nullptr,          // change_company_face = 66,          0x00469CCB
        nullptr,          // load_multiplayer_map = 67,         0x00444DA0
        nullptr,          // 68                                 0x0046F8A5
        nullptr,          // 69                                 0x004454BE
        nullptr,          // 70                                 0x004456C8
        nullptr,          // send_chat_message = 71,            0x0046F976
        nullptr,          // multiplayer_save = 72,             0x004A0ACD
        nullptr,          // update_owner_status = 73,          0x004383CA
        nullptr,          // vehicle_speed_control = 74,        0x004BAB63
        nullptr,          // vehicle_order_up = 75,             0x00470CD2
        nullptr,          // vehicle_order_down = 76,           0x00470E06
        nullptr,          // vehicle_apply_shunt_cheat = 77,    0x004BAC53
        nullptr,          // apply_free_cash_cheat = 78,        0x00438A08
        nullptr,          // rename_industry = 79,              0x00455029
    };

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

        if ((flags & (GameCommandFlag::flag_4 | GameCommandFlag::flag_6)) != 0
            && _gameCommandCanBeUsedWhenPaused[esi]
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
        if (_gameCommandFunctions[command] != nullptr)
        {
            _gameCommandFunctions[command](regs);
        }
        else
        {
            auto addr = _4F9548[command];
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
