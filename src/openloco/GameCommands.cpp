#include "audio/audio.h"
#include "company.h"
#include "companymgr.h"
#include "game_commands.h"
#include "map/tile.h"
#include "objects/objectmgr.h"
#include "objects/road_object.h"
#include "objects/track_object.h"
#include "stationmgr.h"
#include "ui/WindowManager.h"
#include <cassert>

using namespace openloco::ui;
using namespace openloco::map;

namespace openloco::game_commands
{
    static loco_global<company_id_t, 0x009C68EB> _updating_company_id;
    static loco_global<uint8_t, 0x00508F08> game_command_nest_level;
    static loco_global<company_id_t[2], 0x00525E3C> _player_company;
    static loco_global<uint8_t, 0x00508F17> paused_state;
    static loco_global<uint8_t, 0x00508F1A> game_speed;
    static loco_global<uint16_t, 0x0050A004> _50A004;

    static uint16_t _gameCommandFlags;
    static loco_global<uintptr_t[80], 0x004F9548> _4F9548;
    static loco_global<uint8_t[80], 0x004F9688> _4F9688;

    static loco_global<tile_element*, 0x009C68D0> _9C68D0;

    static loco_global<coord_t, 0x009C68E4> _game_command_map_z;
    static loco_global<string_id, 0x009C68E6> gGameCommandErrorText;
    static loco_global<string_id, 0x009C68E8> gGameCommandErrorTitle;
    static loco_global<uint8_t, 0x009C68EE> _errorCompanyId;
    static loco_global<string_id[8], 0x112C826> _commonFormatArgs;

    void registerHooks()
    {
        register_hook(
            0x00431315,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto ebx = do_command(regs.esi, backup);

                regs = backup;
                regs.ebx = ebx;
                return 0;
            });
    }

    static uint32_t loc_4314EA();
    static uint32_t loc_4313C6(int esi, const registers& regs);

    // 0x00431315
    uint32_t do_command(int esi, const registers& regs)
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
            && _4F9688[esi] == 1
            && _updating_company_id == _player_company[0])
        {
            if (get_pause_flags() & 1)
            {
                paused_state = paused_state ^ 1;
                WindowManager::invalidate(WindowType::timeToolbar);
                audio::unpause_sound();
                _50A004 = _50A004 | 1;
            }

            if (game_speed != 0)
            {
                game_speed = 0;
                WindowManager::invalidate(WindowType::timeToolbar);
            }

            if (is_paused())
            {
                gGameCommandErrorText = string_ids::empty;
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

    static uint32_t loc_4313C6(int esi, const registers& regs)
    {
        uint16_t flags = regs.bx;
        gGameCommandErrorText = string_ids::null;
        game_command_nest_level++;

        auto addr = _4F9548[esi];

        uint16_t flagsBackup = _gameCommandFlags;
        registers fnRegs1 = regs;
        fnRegs1.bl &= ~GameCommandFlag::apply;
        call(addr, fnRegs1);
        int32_t ebx = fnRegs1.ebx;
        _gameCommandFlags = flagsBackup;

        if (ebx != 0x80000000)
        {
            if (is_editor_mode())
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

        if (ebx == 0x80000000)
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
        call(addr, fnRegs2);
        int32_t ebx2 = fnRegs2.ebx;
        _gameCommandFlags = flagsBackup2;

        if (ebx2 == 0x80000000)
        {
            return loc_4314EA();
        }

        if (is_editor_mode())
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

    static void sub_431908(company_id_t al, string_id bx, string_id dx)
    {
        registers regs;
        regs.al = al;
        regs.bx = bx;
        regs.dx = dx;
        call(0x431908, regs);
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
            windows::show_error(gGameCommandErrorTitle, gGameCommandErrorText);
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
                    auto trackElement = tile->as_track();
                    if (trackElement == nullptr)
                        break; // throw exception?

                    track_object* pObject = objectmgr::get<track_object>(trackElement->track_object_id());
                    if (pObject == nullptr)
                        break;

                    _commonFormatArgs[0] = pObject->name;
                    _commonFormatArgs[1] = companymgr::get(_errorCompanyId)->name;
                    sub_431908(_errorCompanyId, gGameCommandErrorTitle, string_ids::error_reason_stringid_belongs_to);
                    return 0x80000000;
                }

                case element_type::road: //0x1C
                {
                    auto roadElement = tile->as_road();
                    if (roadElement == nullptr)
                        break; // throw exception?

                    road_object* pObject = objectmgr::get<road_object>(roadElement->road_object_id());
                    if (pObject == nullptr)
                        break;

                    _commonFormatArgs[0] = pObject->name;
                    _commonFormatArgs[1] = companymgr::get(_errorCompanyId)->name;
                    sub_431908(_errorCompanyId, gGameCommandErrorTitle, string_ids::error_reason_stringid_belongs_to);
                    return 0x80000000;
                }

                case element_type::station: // 8
                {
                    auto stationElement = tile->as_station();
                    if (stationElement == nullptr)
                        break; // throw exception?

                    station* pStation = stationmgr::get(stationElement->station_id());
                    if (pStation == nullptr)
                        break;

                    _commonFormatArgs[0] = pStation->name;
                    _commonFormatArgs[1] = pStation->town;
                    _commonFormatArgs[2] = companymgr::get(_errorCompanyId)->name;
                    sub_431908(_errorCompanyId, gGameCommandErrorTitle, string_ids::error_reason_stringid_belongs_to);
                    return 0x80000000;
                }

                case element_type::signal: // 0x0C
                {
                    _commonFormatArgs[0] = companymgr::get(_errorCompanyId)->name;
                    sub_431908(_errorCompanyId, gGameCommandErrorTitle, string_ids::error_reason_signal_belongs_to);
                    return 0x80000000;
                }

                default:
                    break;
            }
        }

        // fallback
        _commonFormatArgs[0] = companymgr::get(_errorCompanyId)->name;
        sub_431908(_errorCompanyId, gGameCommandErrorTitle, string_ids::error_reason_belongs_to);
        return 0x80000000;
    }
}
