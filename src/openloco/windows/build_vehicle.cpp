#include "../companymgr.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../openloco.h"
#include "../things/thingmgr.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::build_vehicle
{
    static loco_global<int16_t, 0x01136268> _1136268;
    static loco_global<uint16_t[1], 0x0113626A> _113626A;
    static loco_global<int32_t, 0x011364E8> _11364E8;
    static loco_global<int8_t[1], 0x011364F0> _11364F0;

    static void sub_4B92A5(ui::window* window);

    window* open(uint32_t vehicle, uint32_t flags)
    {
        registers regs;
        regs.eax = vehicle | flags;
        call(0x004C1AF7, regs);
        return (window*)(uintptr_t)regs.esi;
    }

    void registerHooks()
    {
        register_hook(
            0x004B92A5,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                sub_4B92A5((ui::window*)(uintptr_t)regs.esi);
                regs = backup;
                return 0;
            });
    }

    static void sub_4B9165(uint8_t dl, uint8_t dh, void* esi)
    {
        registers regs;
        regs.dl = dl;
        regs.dh = dh;
        regs.esi = (uintptr_t)esi;
        if (esi == nullptr)
        {
            regs.esi = -1;
        }

        call(0x004B9165, regs);
    }

    static ui::window* getTopEditingVehicleWindow()
    {
        for (auto i = (int32_t)WindowManager::count() - 1; i >= 0; i--)
        {
            auto w = WindowManager::get(i);

            if (w->type != WindowType::vehicle)
                continue;

            if (w->current_tab != 1)
                continue;

            auto vehicle = thingmgr::get<openloco::vehicle>(w->number);
            if (vehicle->owner != companymgr::get_controlling_id())
                continue;

            return w;
        }

        return nullptr;
    }

    /**
     * 0x004B92A5
     *
     * @param window @<esi>
     */
    static void sub_4B92A5(ui::window* window)
    {
        auto w = getTopEditingVehicleWindow();
        int32_t vehicleId = -1;
        if (w != nullptr)
        {
            vehicleId = w->number;
        }

        if (_11364E8 != vehicleId)
        {
            _11364E8 = vehicleId;
            window->var_83C = 0;
            window->invalidate();
        }

        uint16_t vehicleType = window->current_tab;
        uint8_t dh = _11364F0[window->var_874];

        thing_base* vehicle = nullptr;
        if (_11364E8 != -1)
        {
            vehicle = thingmgr::get<thing_base>(_11364E8);
        }

        sub_4B9165(vehicleType, dh, vehicle);

        int cx = _1136268;
        if (window->var_83C == cx)
            return;

        uint16_t* src = _113626A;
        int16_t* dest = window->row_info;
        window->var_83C = cx;
        window->row_count = 0;
        while (cx != 0)
        {
            *dest = *src;
            dest++;
            src++;
            cx--;
        }
        window->row_hover = -1;
        window->invalidate();
    }
}
