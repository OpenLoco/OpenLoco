#include "../CompanyManager.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../OpenLoco.h"
#include "../Things/ThingManager.h"
#include "../Ui/WindowManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Vehicle
{
    namespace Main
    {
        static void onUpdate(window* w);
    }
    namespace VehicleDetails
    {
        static void onUpdate(window* w);
    }
    namespace RouteDetails
    {
        static void onUpdate(window* w);
    }

    static loco_global<window_event_list, 0x005003C0> _mainEvents;
    static loco_global<window_event_list, 0x00500434> _vehicleDetailsEvents;
    static loco_global<window_event_list, 0x00500554> _routeDetailsEvents;

    static loco_global<int32_t, 0x0113614E> _113614E;
    static loco_global<int16_t, 0x01136156> _1136156;

    void registerHooks()
    {
        _mainEvents->on_update = Main::onUpdate;
        _vehicleDetailsEvents->on_update = VehicleDetails::onUpdate;
        _routeDetailsEvents->on_update = RouteDetails::onUpdate;
    }

    static void sub_4B28E2(window* w, int dx)
    {
        registers regs;

        regs.dx = dx;
        regs.esi = (uintptr_t)w;

        call(0x004B28E2, regs);
    }

    namespace Main
    {
        window* open(const OpenLoco::vehicle* vehicle)
        {
            registers regs{};
            regs.edx = (uint32_t)vehicle;
            call(0x004B6033, regs);
            return (window*)regs.esi;
        }

        // 0x004B30F3
        static void onUpdate(window* w)
        {
            w->frame_no += 1;
            w->callPrepareDraw();

            WindowManager::invalidateWidget(WindowType::vehicle, w->number, 4);
            WindowManager::invalidateWidget(WindowType::vehicle, w->number, 10);
            WindowManager::invalidateWidget(WindowType::vehicle, w->number, 13);
            WindowManager::invalidateWidget(WindowType::vehicle, w->number, 14);
            WindowManager::invalidateWidget(WindowType::vehicle, w->number, 15);

            if (w->isDisabled(13))
            {
                Input::toolCancel(WindowType::vehicle, w->number);
                return;
            }

            auto vehicle = ThingManager::get<OpenLoco::vehicle>(w->number);

            if (vehicle->tile_x != -1 && (vehicle->var_38 & Things::Vehicle::Flags38::unk_4) == 0)
            {
                return;
            }

            if (!WindowManager::isInFront(w))
                return;

            if (vehicle->owner != companymgr::getControllingId())
                return;

            if (!Input::isToolActive(WindowType::vehicle, w->number))
            {
                sub_4B28E2(w, 13);
            }
        }
    }

    namespace VehicleDetails
    {
        // 0x004B3C45
        // "Show <vehicle> design details and options" tab in vehicle window
        static void onUpdate(window* w)
        {
            if (w->number == _1136156)
            {
                if (WindowManager::find(WindowType::dragVehiclePart) == nullptr)
                {
                    _1136156 = -1;
                    _113614E = -1;
                    w->invalidate();
                }
            }

            w->frame_no += 1;
            w->callPrepareDraw();

            WindowManager::invalidateWidget(WindowType::vehicle, w->number, 5);

            if (_1136156 == -1 && w->isActivated(11))
            {
                WindowManager::invalidateWidget(WindowType::vehicle, w->number, 11);
            }

            if (w->isDisabled(10))
            {
                Input::toolCancel(WindowType::vehicle, w->number);
                return;
            }

            auto vehicle = ThingManager::get<OpenLoco::vehicle>(w->number);
            if (vehicle->tile_x != -1 && (vehicle->var_38 & Things::Vehicle::Flags38::unk_4) == 0)
                return;

            if (!WindowManager::isInFrontAlt(w))
                return;

            if (vehicle->owner != companymgr::getControllingId())
                return;

            if (!Input::isToolActive(WindowType::vehicle, w->number))
            {
                sub_4B28E2(w, 10);
            }
        }
    }

    namespace RouteDetails
    {
        // 0x004B55D1
        // "Show <vehicle> route details" tab in vehicle window
        static void onUpdate(window* w)
        {
            w->frame_no += 1;
            w->callPrepareDraw();

            WindowManager::invalidateWidget(WindowType::vehicle, w->number, 8);

            auto vehicle = ThingManager::get<OpenLoco::vehicle>(w->number);
            if (vehicle->owner != companymgr::getControllingId())
                return;

            if (!WindowManager::isInFront(w))
                return;

            if (Input::isToolActive(WindowType::vehicle, w->number))
                return;

            if (Input::toolSet(w, 9 /* tool widget*/, 12 /* tool idx? */))
                return;

            w->invalidate();

            registers regs2;
            regs2.esi = (uintptr_t)vehicle;
            call(0x00470824, regs2);
        }
    }
}
