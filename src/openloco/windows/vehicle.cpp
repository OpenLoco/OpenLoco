#include "../companymgr.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../openloco.h"
#include "../things/thingmgr.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::vehicle
{
    namespace main
    {
        static void on_update(window* w);
    }
    namespace vehicle_details
    {
        static void on_update(window* w);
    }
    namespace finances
    {
        static void on_update(window* w);
    }

    static loco_global<window_event_list, 0x005003C0> _mainEvents;
    static loco_global<window_event_list, 0x00500434> _vehicleDetailsEvents;
    static loco_global<window_event_list, 0x00500554> _financesEvents;

    static loco_global<int32_t, 0x0113614E> _113614E;
    static loco_global<int16_t, 0x01136156> _1136156;

    void register_hooks()
    {
        _mainEvents->on_update = main::on_update;
        _vehicleDetailsEvents->on_update = vehicle_details::on_update;
        _financesEvents->on_update = finances::on_update;
    }

    static void sub_4B28E2(window* w, int dx)
    {
        registers regs;

        regs.dx = dx;
        regs.esi = (uintptr_t)w;

        call(0x004B28E2, regs);
    }

    namespace main
    {
        // 0x4B30F3
        static void on_update(window* w)
        {
            w->frame_no += 1;
            w->call_prepare_draw();
            WindowManager::invalidateWidget(w->type, w->number, w->current_tab + 4);

            WindowManager::invalidateWidget(WindowType::vehicle, w->number, 0x4);
            WindowManager::invalidateWidget(WindowType::vehicle, w->number, 0xA);
            WindowManager::invalidateWidget(WindowType::vehicle, w->number, 0xE);
            WindowManager::invalidateWidget(WindowType::vehicle, w->number, 0xD);
            WindowManager::invalidateWidget(WindowType::vehicle, w->number, 0xF);

            if (w->is_disabled(13))
            {
                input::cancel_tool(WindowType::vehicle, w->number);
                return;
            }

            auto thing = thingmgr::get<openloco::vehicle>(w->number);

            if (thing->tile_x != -1 && (thing->var_38 & (1 << 4)) == 0)
            {
                return;
            }

            if (!WindowManager::isInFront(w))
                return;

            if (thing->var_21 != companymgr::get_controlling_id())
                return;

            if (!input::is_tool_active(WindowType::vehicle, w->number))
            {
                sub_4B28E2(w, 13);
            }
        }
    }

    namespace vehicle_details
    {
        // 0x4B3C45
        static void on_update(window* w)
        {
            if (w->number == _1136156)
            {
                if (WindowManager::find(WindowType::dragVehiclePart) == nullptr)
                {
                    _1136156 = -1;
                    _113614E = -1;
                    w->invalidate();
                };
            }

            w->frame_no += 1;
            w->call_prepare_draw();

            WindowManager::invalidateWidget(WindowType::vehicle, w->number, 5);

            if (_1136156 == -1 && w->is_activated(11))
            {
                WindowManager::invalidateWidget(WindowType::vehicle, w->number, 11);
            }

            if (w->is_disabled(4))
            {
                input::cancel_tool(WindowType::vehicle, w->number);
                return;
            }

            auto thing = thingmgr::get<openloco::vehicle>(w->number);
            if (thing->tile_x != -1 && (thing->var_38 & (1 << 4)) == 0)
                return;

            if (!WindowManager::isInFrontAlt(w))
                return;

            if (thing->var_21 != companymgr::get_controlling_id())
                return;

            if (!input::is_tool_active(WindowType::vehicle, w->number))
            {
                sub_4B28E2(w, 10);
            }
        }
    }

    namespace finances
    {
        // 0x4B55D1
        static void on_update(window* w)
        {
            w->frame_no += 1;
            w->call_prepare_draw();
            WindowManager::invalidateWidget(WindowType::vehicle, w->number, 8);

            auto thing = thingmgr::get<openloco::vehicle>(w->number);
            if (thing->var_21 != companymgr::get_controlling_id())
                return;

            if (!WindowManager::isInFront(w))
                return;

            if (input::is_tool_active(WindowType::vehicle, w->number))
                return;

            registers regs;
            regs.dx = 9; // tool widget
            regs.al = 12;
            regs.esi = (uintptr_t)w;

            auto result = call(0x004CE367, regs);
            bool cf = (result & (1 << 8)) != 0;
            if (cf)
                return;

            w->invalidate();

            registers regs2;
            regs.esi = (uintptr_t)thing;
            call(0x00470824, regs2);
        }
    }
}
