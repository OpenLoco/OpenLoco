#include "../config.h"
#include "../console.h"
#include "../industry.h"
#include "../industrymgr.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/stringmgr.h"
#include "../map/tile.h"
#include "../objects/cargo_object.h"
#include "../objects/objectmgr.h"
#include "../stationmgr.h"
#include "../things/thingmgr.h"
#include "../townmgr.h"
#include "../ui.h"
#include "../ui/scrollview.h"
#include "../viewportmgr.h"
#include "../window.h"
#include "WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::viewport_interaction
{

    static bool _station(map::station_element* station);
    static bool _station(station_id_t id);

    static bool _track(map::track_element* track)
    {
        if (!track->has_station_element())
            return false;

        auto next = (map::station_element*)track++;
        if (next->is_flag_5())
            return false;

        return _station(next);
    }

    // 0x004CD974
    static bool _road(map::road_element* road)
    {
        if (!road->has_station_element())
            return false;

        map::station_element* station;
        for (auto tile = (map::tile_element*)road++; !tile->is_last(); tile++)
        {
            if (tile->type() == map::element_type::station)
            {
                station = tile->as_station();
                break;
            }
        }

        if (station->is_flag_5())
            return false;

        return _station(station);
    }

    // 0x004CD99A
    static bool _station(map::station_element* station)
    {
        if (station->is_flag_4())
            return false;

        return _station(station->station_id();
    }

    static loco_global<uint16_t, 0x00F24484> _mapSelectionFlags;
    static loco_global<uint16_t, 0x00F252A4> _hoveredStationId;
    static loco_global<company_id_t, 0x0050A040> _mapTooltipOwner;

    // 0x004CD9B0
    static bool _station(station_id_t id)
    {
        _hoveredStationId = id;

        auto station = stationmgr::get(id);

        *_mapSelectionFlags |= (1 << 6);
        viewportmgr::invalidate(station);

        _mapTooltipOwner = station->owner;

        args + 2 = station->name;
        args + 4 = station->town;
        args + 6 = label_icons[station->flags & 0x0F];
        args + 10 = town->population; // 32-bit

        station->getStatusString(); // save to buffer 338

        string_id prefix = 1444; // "{NEWLINE}Accepts "
        for (int i = 0; i < max_cargo_stats; i++)
        {
            auto& cargo = station->cargo_stats[i];
            if (cargo.is_accepted())
            {
                out += stringmgr::format_string(prefix);

                out += stringmgr::format_string(objectmgr::get<cargo_object>(i)->name);

                prefix = string_id::unit_separator;
            }
        }

        args + 8 = string_ids::buffer_338;
        args + 0 = 1239; //'{STRINGID} {STRINGID}{NEWLINE}{COLOUR WINDOW_3}{STRINGID}'

        return true;
    }

    static bool _town(town_id_t id)
    {
        auto town = townmgr::get(id);

        args + 2 = town->name;
        args + 6 = 1304;              // "{STRINGID} population {INT32}"
        args + 8 = 617 + town->size;  // 'hamlet' etc;
        args + 10 = town->population; // 32-bit
        args + 0 = 627;               // "{STRINGID}{NEWLINE}{COLOUR WINDOW_3}{STRINGID}"

        return true;
    }

    // 0x0045935F

    /**
     *
     * @param id @<dx>
     */
    static std::string sub_0045935F(industry_id_t id)
    {
        auto industry = industrymgr::get(id);

        if (industry->flags & industry_flags::flag_02)
        {
            return 1419; // Closing down
        }

        if (industry->var_11 != 0xFF)
        {
            return 1366; // Under construction
        }

        auto object = industry->object();

        string_id produces = 1367; // "Producing "
        std::string out;

        if (object->accepts[0] != -1 || object->accepts[1] != -1 || object->accepts[2] != -1)
        {
            out += stringmgr::format_string(1371); // "Requires "

            string_id voegwoord = 1374; // " or "
            if (object->flags & 0x20000)
            {
                voegwoord = 1373; // " and "
            }

            bool dl = false;
            for (int i = 0; i < 3; i++)
            {
                if (object->accepts[i] != -1)
                {
                    if (dl)
                    {
                        out += stringmgr::format_string(1373); // " and "
                    }
                    out += stringmgr::format_string(objectmgr::get<cargo_object>(object->accepts[i])->name); // "Producing "
                    dl = true;
                }
            }

            produces = 1370; // " to produce "
        }

        if (object->produces[0] != -1 || object->produces[1] != -1)
        {
            out += stringmgr::format_string(produces);

            bool dl = false;
            for (int i = 0; i < 2; i++)
            {
                if (object->produces[i] != -1)
                {
                    if (dl)
                    {
                        out += stringmgr::format_string(1373); // " and "
                    }
                    out += stringmgr::format_string(objectmgr::get<cargo_object>(object->produces[i])->name); // "Producing "
                    dl = true;
                }
            }
        }
    }

    static bool _industry(map::tree_element* tile)
    {
        if (tile->is_flag_4())
            return false;

        auto industry = tile->industry();

        args + 2 = industry->name;
        args + 4 = industry->var_D5; // likely town id
        args + 6 = string_ids::buffer_338;
        args + 0 = 627; // "{STRINGID}{NEWLINE}{COLOUR WINDOW_3}{STRINGID}"
    }

    void _vehicle(openloco::vehicle* vehicle)
    {

        thingmgr::get<vehicle>(vehicle->var_26);

        if (vehicle->tile_x == -1)
        {
            bx = 473;
            cx = string_ids::null;
        }
        else if (false)
        {
            bx = 464;
            cx = string_ids::null;
        }
        else
        {
        }
    }

    void _headquarter(map::tile_element* tile)
    {
    }

    // 0x004CD658
    InteractionItem get_item_left(const int16_t tempX, const int16_t tempY, InteractionArg* arg)
    {
        if (openloco::is_title_mode())
            return InteractionItem::t_0;

        int16_t x, y;
        void* edx = nullptr;

        // 0x2000 | 0x1000 | 0x2
        // station | town | thing
        auto bl = get_map_coordinates_from_pos(tempX, tempY, 0b11111111111111111100111111111101, &x, &y, &edx);
        if (bl != InteractionItem::thing)
        {
            // 0x10000 | 0x2000 | 0x1000 | 0x800 | 0x200 | 0x20 | 0x4 | 0x2
            // industry | station | town | (7|8|9|10) | headquarterBuilding | road | track | thing
            bl = get_map_coordinates_from_pos(tempX, tempY, 0b11111111111111101100010111011001, &x, &y, &edx);
        }

        switch (bl)
        {
            case InteractionItem::track: // 4
                return _track(edx);

            case InteractionItem::road: // 16
                return _road(edx);

            case InteractionItem::town: // 14
                return _town((town_id_t)edx);

            case InteractionItem::station: // 15
                return _station((station_id_t)edx);

            case InteractionItem::trackStation: // 7
            case InteractionItem::roadStation:  // 8
            case InteractionItem::airport:      // 9
            case InteractionItem::dock:         // 10
                return _station((map::station_element*)edx);

            case InteractionItem::industry: // 20
                return _industry(edx);

            case InteractionItem::headquarterBuilding: // 21
                return _headquarter(edx);

                    case InteractionItem::thing: // 3
                {
                    auto t = (Thing*)edx;
                    auto vehicle = t->as_vehicle();
                    if (vehicle == nullptr)
                        break;

                    return _vehicle(vehicle);
                }
        }

        auto window = WindowManager::findAt(tempX, tempY);
        if (window == nullptr)
            return InteractionItem::t_0;

        auto viewport = window->viewports[0];
        if (viewport == nullptr)
            return InteractionItem::t_0;

        if (viewport->zoom > config::get().vehicles_min_scale)
            return InteractionItem::t_0;

        uint16_t bp = std::numeric_limits<uint16_t>().max();

        vehicle_base* esi;

        auto v = thingmgr::first_id(thingmgr::thing_list::vehicle);
        while (v != thing_id::null)
        {
            auto vehicle = thingmgr::get<openloco::vehicle>(v);
            auto car = vehicle->next_car();
            for (auto car = vehicle->next_car(); car != nullptr; car = car->next_car())
            {
                if (car->sprite_left == 0x8000)
                    continue;

                esi = car;

                if (car->next_car() == nullptr)
                {
                    vehicle = car->next_vehicle();

                    // determine distance from click; save to bp
                }
            }
        }

        if (bp <= 32)
        {
            arg->object = esi;
            arg->x = esi->x;
            arg->y = esi->y;

            return InteractionItem::thing;
        }

        //        return InteractionItem::t_0;
        //
        //        registers regs;
        //        regs.ax = tempX;
        //        regs.bx = tempY;
        //
        //        call(0x004CD658, regs);
        //
        //        if (arg != nullptr)
        //        {
        //            arg->x = regs.ax;
        //            arg->y = regs.cx;
        //            arg->object = (void*)regs.edx;
        //        }
        //
        //        return (InteractionItem)regs.bl;
    }

    // 0x004CDB2B
    InteractionItem right_over(int16_t x, int16_t y)
    {
        registers regs;
        regs.ax = x;
        regs.bx = y;

        call(0x004CDB2B, regs);

        return (InteractionItem)regs.bl;
    }
}
