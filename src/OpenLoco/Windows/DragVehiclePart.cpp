#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../OpenLoco.h"
#include "../Things/Vehicle.h"
#include "../Ui/WindowManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::DragVehiclePart
{
    enum widx
    {
        frame
    };

    static window_event_list events;

    // 0x00522504
    static widget_t widgets[] = {
        makeWidget({ 0, 0 }, { 150, 60 }, widget_type::wt_3, 0),
        widgetEnd()
    };

    // TODO: make vehicles versions of these call into this global, ?make thing_id instead?
    static loco_global<OpenLoco::vehicle_bogie*, 0x0113614E> _dragCarComponent;
    static loco_global<thing_id_t, 0x01136156> _dragVehicleHead;

    static void initEvents();

    // 0x004B3B7E
    void open(OpenLoco::Things::Vehicle::Car& car)
    {
        WindowManager::close(WindowType::dragVehiclePart);
        _dragCarComponent = car.front;
        _dragVehicleHead = car.front->head;
        WindowManager::invalidate(WindowType::vehicle, car.front->head);

        initEvents();
        uint16_t width = Vehicle::Common::sub_4B743B(1, 0, 0, 0, car.front, nullptr);
        auto pos = Input::getTooltipMouseLocation();
        pos.y -= 30;
        pos.x -= width / 2;
        Gfx::ui_size_t size = { width, 60 };
        auto self = WindowManager::createWindow(WindowType::dragVehiclePart, pos, size, WindowFlags::transparent | WindowFlags::stick_to_front, &events);
        self->widgets = widgets;
        self->widgets[widx::frame].right = width - 1;
        Input::windowPositionBegin(Input::getTooltipMouseLocation().x, Input::getTooltipMouseLocation().y, self, widx::frame);
    }

    // 0x004B62FE
    static Ui::cursor_id cursor(window* const self, const int16_t widgetIdx, const int16_t x, const int16_t y, const Ui::cursor_id fallback)
    {
        self->height = 0; // Set to zero so that skipped in window find
        Vehicle::Details::scrollDrag(Input::getScrollLastLocation());
        self->height = 60;
        return cursor_id::unk_26;
    }

    static void onMove(window& self, const int16_t x, const int16_t y)
    {
        self.height = 0; // Set to zero so that skipped in window find
        Vehicle::Details::scrollDragEnd(Input::getScrollLastLocation());
        WindowManager::close(&self);
        _dragCarComponent = nullptr;
        WindowManager::invalidate(WindowType::vehicle, _dragVehicleHead);
    }

    static void draw(Ui::window* const self, Gfx::drawpixelinfo_t* const context)
    {
        Gfx::drawpixelinfo_t* clipped;
        if (Gfx::clipDrawpixelinfo(&clipped, context, self->x, self->y, self->width, self->height))
        {
            Vehicle::Common::sub_4B743B(0, 0, 0, 19, _dragCarComponent, clipped);
        }
    }

    static void initEvents()
    {
        events.cursor = cursor;
        events.on_move = onMove;
        events.draw = draw;
    }
}
