#include "../Interop/Interop.hpp"
#include "../OpenLoco.h"
#include "../Ui/WindowManager.h"
#include <stdexcept>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::VehicleList
{
    static const Gfx::ui_size_t window_size = { 559, 213 };
    static const Gfx::ui_size_t max_dimensions = { 640, 1200 };
    static const Gfx::ui_size_t min_dimensions = { 192, 100 };

    static window_event_list _events;

    enum widx
    {
        frame = 0,
        caption = 1,
        close_button = 2,
        panel = 3,
        tab_trains,
        tab_buses,
        tab_trucks,
        tab_trams,
        tab_aircraft,
        tab_ships,
        company_select,
        sort_name,
        sort_profit,
        sort_age,
        sort_reliability,
        scrollview,
    };

    static const uint8_t row_heights[] = {
        28,
        28,
        28,
        28,
        48,
        36
    };

    widx getTabFromType(uint8_t type);

    static void sub_4C1D4F(window* self)
    {
        registers regs;
        regs.esi = (int32_t)self;
        call(0x004C1D4F, regs);
    }

    // 0x004C19DC
    window* open(uint16_t companyId, uint8_t type)
    {
        window* self = WindowManager::bringToFront(WindowType::vehicleList, companyId);
        if (self != nullptr)
        {
            self->callOnMouseUp(VehicleList::getTabFromType(type));
            return self;
        }

        // 0x004C1A05
        self = WindowManager::createWindow(
            WindowType::vehicleList,
            window_size,
            0,
            &_events);

        self->current_tab = VehicleList::getTabFromType(type);
        self->row_height = row_heights[type];
        self->sort_mode = 0;
        self->row_hover = -1;

        VehicleList::sub_4C1D4F(self);

        self->invalidate();

        self->callOnResize();
        self->callPrepareDraw();
        self->initScrollWidgets();

        return self;
    }

    widx getTabFromType(uint8_t type)
    {
        if (type > 5)
        {
            throw std::domain_error("Unexpected vehicle type");
        }

        static widx type_to_widx[] = {
            tab_trains,
            tab_buses,
            tab_trucks,
            tab_trams,
            tab_aircraft,
            tab_ships,
        };
        return type_to_widx[type];
    }

}
