#pragma once

#include "Ui/Widget.h"

namespace OpenLoco::Ui::Widgets
{
    struct Frame : public Widget
    {
        static constexpr auto kWidgetType = WidgetType::frame;

        constexpr Frame(WidgetId id, Point32 origin, Size32 size, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
            : Widget(id, origin, size, kWidgetType, colour, content, tooltip)
        {
            events.draw = &draw;
        }

        constexpr Frame(Point32 origin, Size32 size, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
            : Frame(WidgetId::none, origin, size, colour, content, tooltip)
        {
        }

        static void draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState);
        static void drawBackground(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState);
        static void drawSolid(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState);
        static void drawTransparent(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState);
    };

}
