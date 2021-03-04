#include "../Interop/Interop.hpp"
#include "../OpenLoco.h"
#include "../Ui/WindowManager.h"
#include <stdexcept>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::VehicleList
{
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

    // 0x004C19DC
    window* open(uint16_t companyId, uint8_t type)
    {
        window* vehicle_list = WindowManager::bringToFront(WindowType::vehicleList, companyId);
        if (vehicle_list == nullptr)
        {
            registers regs;
            regs.eax = (uint32_t)companyId;
            regs.ebx = type;
            call(0x004C1A05, regs);
            vehicle_list = ToPtr(window, regs.esi);
        }
        else
        {
            if (type > 5)
                throw std::domain_error("Unexpected vehicle type");

            static widx type_to_widx[] = {
                tab_trains,
                tab_buses,
                tab_trucks,
                tab_trams,
                tab_aircraft,
                tab_ships,
            };

            widx target = type_to_widx[type];
            vehicle_list->callOnMouseUp(target);
        }

        return vehicle_list;
    }
}
