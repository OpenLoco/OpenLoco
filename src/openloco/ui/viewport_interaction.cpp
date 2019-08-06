#include "../config.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/stringmgr.h"
#include "../stationmgr.h"
#include "../things/thingmgr.h"
#include "../ui.h"
#include "../ui/scrollview.h"
#include "../window.h"
#include "WindowManager.h"
#include "../console.h"

using namespace openloco::interop;

namespace openloco::ui::viewport_interaction
{
    // 0x004CD658
    InteractionItem get_item_left(const int16_t tempX, const int16_t tempY, InteractionArg* arg)
    {
        if (openloco::is_title_mode())
            return InteractionItem::t_0;

        int16_t x, y;
        void* edx = nullptr;

        auto bl = get_map_coordinates_from_pos(0xFFFFCFFD, tempX, tempY, &x, &y, &edx);
        if (bl == InteractionItem::thing)
        {
            bl = get_map_coordinates_from_pos(0x0FFFEC5D9, tempX, tempY, &x, &y, &edx);
        }

        switch (bl)
        {
            case InteractionItem::t_4:
            case InteractionItem::t_16:
            case InteractionItem::thing:   // 3
            case InteractionItem::town:    // 14
            case InteractionItem::station: // 15
            case InteractionItem::t_7:
            case InteractionItem::t_8:
            case InteractionItem::t_9:
            case InteractionItem::t_10:
            case InteractionItem::industry:            // 20
            case InteractionItem::headquarterBuilding: // 21
            console::log("%d %X", bl,edx);
                break;

            default:
            {
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

                break;
            }
        }

        return InteractionItem::t_0;

        registers regs;
        regs.ax = tempX;
        regs.bx = tempY;

        call(0x004CD658, regs);

        if (arg != nullptr)
        {
            arg->x = regs.ax;
            arg->y = regs.cx;
            arg->object = (void*)regs.edx;
        }

        return (InteractionItem)regs.bl;
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
