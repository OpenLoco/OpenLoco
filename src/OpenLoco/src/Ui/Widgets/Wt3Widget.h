#pragma once

#include "Ui/Widget.h"

namespace OpenLoco::Ui::Widgets
{
    // TODO: This is just another Frame widget, merge this with FrameWidget.
    struct Wt3Widget : public Widget
    {
        constexpr Wt3Widget(WidgetId id, Point32 origin, Size32 size, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
            : Widget(id, origin, size, WidgetType::wt_3, colour, content, tooltip)
        {
            events.draw = &draw;
        }

        constexpr Wt3Widget(Point32 origin, Size32 size, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
            : Wt3Widget(WidgetId::none, origin, size, colour, content, tooltip)
        {
        }

        static void draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState);
    };

}
