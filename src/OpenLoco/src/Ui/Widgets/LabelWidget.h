#pragma once

#include "Ui/Widget.h"

namespace OpenLoco::Ui::Widgets
{
    // NOTE: This is currently a label that always centers the text.
    // TODO: Add an align option.
    struct Label : public Widget
    {
        constexpr Label(Ui::Point32 origin, Ui::Size32 size, WindowColour colour, ContentAlign align, StringId content = StringIds::empty, StringId tooltip = StringIds::null)
            : Widget(origin, size, WidgetType::wt_13, colour, content, tooltip)
        {
            events.draw = &draw;
            contentAlign = align;
        }

        static void draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState);
    };

}
