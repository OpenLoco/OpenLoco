#include "../interop/interop.hpp"
#include "../openloco.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows::station_list
{
    enum widx
    {
        frame = 0,
        caption = 1,
        close_button = 2,
        panel = 3,
        tab_all_stations,
        tab_rail_stations,
        tab_road_stations,
        tab_airports,
        tab_ship_ports,
        company_select,
        sort_name,
        sort_status,
        sort_total_waiting,
        sort_accepts,
        scrollview,
    };

    window* open(uint16_t companyId, uint8_t type)
    {
        if (type > 4)
            throw std::domain_error("Unexpected station type");

        registers regs;
        regs.eax = (uint32_t)companyId;
        call(0x00490F6C, regs);

        auto station_list = (window*)regs.esi;

        static widx type_to_widx[] = {
            tab_all_stations,
            tab_rail_stations,
            tab_road_stations,
            tab_airports,
            tab_ship_ports,
        };

        widx target = type_to_widx[type];
        station_list->call_on_mouse_up(target);

        return station_list;
    }
}
