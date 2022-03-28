#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../OpenLoco.h"
#include "../Ui/WindowManager.h"
#include "../Vehicles/Vehicle.h"
#include "../Widget.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::DragVehiclePart
{
    enum widx
    {
        frame
    };

    static WindowEventList events;

    // 0x00522504
    static Widget widgets[] = {
        makeWidget({ 0, 0 }, { 150, 60 }, WidgetType::wt_3, WindowColour::primary),
        widgetEnd()
    };

    // TODO: make vehicles versions of these call into this global, ?make thing_id instead?
    static loco_global<Vehicles::VehicleBogie*, 0x0113614E> _dragCarComponent;
    static loco_global<EntityId, 0x01136156> _dragVehicleHead;

    static void initEvents();

    // 0x004B3B7E
    void open(Vehicles::Car& car)
    {
        WindowManager::close(WindowType::dragVehiclePart);
        _dragCarComponent = car.front;
        _dragVehicleHead = car.front->head;
        WindowManager::invalidate(WindowType::vehicle, enumValue(car.front->head));

        initEvents();
        uint16_t width = Vehicle::Common::sub_4B743B(1, 0, 0, 0, car.front, nullptr);
        auto pos = Input::getTooltipMouseLocation();
        pos.y -= 30;
        pos.x -= width / 2;
        Ui::Size size = { width, 60 };
        auto self = WindowManager::createWindow(WindowType::dragVehiclePart, pos, size, WindowFlags::transparent | WindowFlags::stickToFront, &events);
        self->widgets = widgets;
        self->widgets[widx::frame].right = width - 1;
        Input::windowPositionBegin(Input::getTooltipMouseLocation().x, Input::getTooltipMouseLocation().y, self, widx::frame);
    }

    // 0x004B62FE
    static Ui::CursorId cursor(Window* const self, const int16_t widgetIdx, const int16_t x, const int16_t y, const Ui::CursorId fallback)
    {
        self->height = 0; // Set to zero so that skipped in window find
        Vehicle::Details::scrollDrag(Input::getScrollLastLocation());
        self->height = 60;
        return CursorId::dragHand;
    }

    static void onMove(Window& self, const int16_t x, const int16_t y)
    {
        self.height = 0; // Set to zero so that skipped in window find
        Vehicle::Details::scrollDragEnd(Input::getScrollLastLocation());
        WindowManager::close(&self);
        _dragCarComponent = nullptr;
        WindowManager::invalidate(WindowType::vehicle, enumValue(*_dragVehicleHead));
    }

    static void draw(Ui::Window* const self, Gfx::Context* const context)
    {
        auto clipped = Gfx::clipContext(*context, Ui::Rect(self->x, self->y, self->width, self->height));
        if (clipped)
        {
            Vehicle::Common::sub_4B743B(0, 0, 0, 19, _dragCarComponent, &*clipped);
        }
    }

    static void initEvents()
    {
        events.cursor = cursor;
        events.onMove = onMove;
        events.draw = draw;
    }
}
