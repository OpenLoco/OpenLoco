#pragma once

#include "Ui/Widget.h"

namespace OpenLoco::Ui::Widgets
{
    struct Slider : public Widget
    {
        constexpr Slider(Point32 origin, Size32 size, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
            : Widget(origin, size, WidgetType::slider, colour, content, tooltip)
        {
            events.draw = &draw;
        }

        static void draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState);
    };

}
