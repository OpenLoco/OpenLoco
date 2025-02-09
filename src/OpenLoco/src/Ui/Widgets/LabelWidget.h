#pragma once

#include "Ui/Widget.h"

namespace OpenLoco::Ui::Widgets
{

    struct Label : public Widget
    {
        constexpr Label(WidgetId id, Point32 origin, Size32 size, WindowColour colour, ContentAlign align, StringId content = StringIds::empty, StringId tooltip = StringIds::null)
            : Widget(id, origin, size, WidgetType::label, colour, content, tooltip)
        {
            events.draw = &draw;
            contentAlign = align;
        }

        constexpr Label(Point32 origin, Size32 size, WindowColour colour, ContentAlign align, StringId content = StringIds::empty, StringId tooltip = StringIds::null)
            : Label(WidgetId::none, origin, size, colour, align, content, tooltip)
        {
        }

        static void draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState);
    };

}
