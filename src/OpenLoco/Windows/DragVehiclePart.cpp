#include "../Interop/Interop.hpp"
#include "../OpenLoco.h"
#include "../Things/Vehicle.h"
#include "../Ui/WindowManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::DragVehiclePart
{
    static loco_global<OpenLoco::vehicle_bogie*, 0x0113614E> _113614E;
    static loco_global<thing_id_t, 0x01136156> _1136156;

    // 0x004B3B7E
    void open(OpenLoco::Things::Vehicle::Car& car)
    {
        WindowManager::close(WindowType::dragVehiclePart);
        _113614E = car.front;
        _1136156 = car.front->head;
        WindowManager::invalidate(WindowType::vehicle, car.front->head);
        registers regs{};
        regs.ebp = reinterpret_cast<uint32_t>(car.front);
        regs.bx = car.front->head;
        call(0x004B3BA0, regs);
    }
}