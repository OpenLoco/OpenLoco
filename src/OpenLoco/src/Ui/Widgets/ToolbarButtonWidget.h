#pragma once

#include "Ui/Widget.h"

namespace OpenLoco::Ui::Widgets
{
    struct ToolbarButton : public Widget
    {
        static constexpr auto kWidgetType = WidgetType::toolbarTab;

        constexpr ToolbarButton(WidgetId id, Point origin, Size size, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
            : Widget(id, origin, size, kWidgetType, colour, content, tooltip)
        {
            events.draw = &draw;
            contentAlign = ContentAlign::center;
        }

        constexpr ToolbarButton(Point origin, Size size, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
            : ToolbarButton(WidgetId::none, origin, size, colour, content, tooltip)
        {
        }

        static void draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState);
    };

}
