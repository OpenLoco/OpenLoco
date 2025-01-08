#pragma once

#include "Ui/Widget.h"

namespace OpenLoco::Ui::Widgets
{
    struct Checkbox : public Widget
    {
        constexpr Checkbox(Point32 origin, Size32 size, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
            : Widget(origin, size, WidgetType::checkbox, colour, content, tooltip)
        {
            events.draw = &draw;
        }

        static void draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState);
        static void drawCheckMark(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState);
        static void drawLabel(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState);
    };

}
