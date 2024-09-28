#pragma once

#include "Ui/Widget.h"

namespace OpenLoco::Ui::Widgets
{
    struct Button : public Widget
    {
        constexpr Button(Ui::Point32 origin, Ui::Size32 size, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
            : Widget(origin, size, WidgetType::button, colour, content, tooltip)
        {
            events.draw = &draw;
            contentAlign = ContentAlign::Center;
        }

        static void draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState);
    };

}
