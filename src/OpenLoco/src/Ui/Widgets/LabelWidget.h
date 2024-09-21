#pragma once

#include "Ui/Widget.h"

namespace OpenLoco::Ui::Widgets
{
    struct Label : public Widget
    {
        constexpr Label(Ui::Point32 origin, Ui::Size32 size, WindowColour colour, StringId content = StringIds::null, StringId tooltip = StringIds::null)
            : Widget(origin, size, WidgetType::wt_13, colour, content, tooltip)
        {
            events.draw = &draw;
        }

        static void draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState);
    };

}
