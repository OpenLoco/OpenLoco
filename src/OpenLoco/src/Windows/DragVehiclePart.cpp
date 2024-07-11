#include "Graphics/DrawingContext.h"
#include "Input.h"
#include "OpenLoco.h"
#include "Ui/Widget.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleDraw.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::DragVehiclePart
{
    enum widx
    {
        frame
    };

    // 0x00522504
    static constexpr Widget widgets[] = {
        makeWidget({ 0, 0 }, { 150, 60 }, WidgetType::wt_3, WindowColour::primary),
        widgetEnd()
    };

    // TODO: make vehicles versions of these call into this global, ?make Entity::id instead?
    static loco_global<Vehicles::VehicleBogie*, 0x0113614E> _dragCarComponent;
    static loco_global<EntityId, 0x01136156> _dragVehicleHead;

    static const WindowEventList& getEvents();

    // 0x004B3B7E
    void open(Vehicles::Car& car)
    {
        WindowManager::close(WindowType::dragVehiclePart);
        _dragCarComponent = car.front;
        _dragVehicleHead = car.front->head;
        WindowManager::invalidate(WindowType::vehicle, enumValue(car.front->head));

        uint16_t width = getWidthVehicleInline(car);
        auto pos = Input::getTooltipMouseLocation();
        pos.y -= 30;
        pos.x -= width / 2;
        Ui::Size size = { width, 60 };

        auto self = WindowManager::createWindow(WindowType::dragVehiclePart, pos, size, WindowFlags::transparent | WindowFlags::stickToFront, getEvents());
        self->setWidgets(widgets);
        self->widgets[widx::frame].right = width - 1;

        Input::windowPositionBegin(Input::getTooltipMouseLocation().x, Input::getTooltipMouseLocation().y, self, widx::frame);
    }

    // 0x004B62FE
    static Ui::CursorId cursor(Window& self, [[maybe_unused]] const int16_t widgetIdx, [[maybe_unused]] const int16_t x, [[maybe_unused]] const int16_t y, [[maybe_unused]] const Ui::CursorId fallback)
    {
        self.height = 0; // Set to zero so that skipped in window find
        Vehicle::Details::scrollDrag(Input::getScrollLastLocation());
        self.height = 60;
        return CursorId::dragHand;
    }

    // 0x004B6271
    static void onMove(Window& self, [[maybe_unused]] const int16_t x, [[maybe_unused]] const int16_t y)
    {
        const auto height = self.height;
        self.height = 0; // Set to zero so that skipped in window find
        Vehicle::Details::scrollDragEnd(Input::getScrollLastLocation());
        // Reset the height so that invalidation works correctly
        self.height = height;
        WindowManager::close(&self);
        _dragCarComponent = nullptr;
        WindowManager::invalidate(WindowType::vehicle, enumValue(*_dragVehicleHead));
    }

    // 0x004B6197
    static void draw(Ui::Window& self, Gfx::DrawingContext& drawingCtx)
    {
        const auto& rt = drawingCtx.currentRenderTarget();
        auto clipped = Gfx::clipRenderTarget(rt, Ui::Rect(self.x, self.y, self.width, self.height));
        if (clipped)
        {
            drawingCtx.pushRenderTarget(*clipped);

            Vehicles::Vehicle train(_dragVehicleHead);
            for (auto& car : train.cars)
            {
                if (car.front == _dragCarComponent)
                {
                    drawVehicleInline(drawingCtx, car, { 0, 19 }, VehicleInlineMode::basic);
                    break;
                }
            }
            drawingCtx.popRenderTarget();
        }
    }

    static constexpr WindowEventList kEvents = {
        .cursor = cursor,
        .onMove = onMove,
        .draw = draw,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
