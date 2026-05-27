#pragma once

#include "Ui/Widget.h"

namespace OpenLoco::Ui::Widgets
{
    struct NewsPanel : public Widget
    {
        static constexpr auto kWidgetType = WidgetType::newsPanel;

        enum class Style : uint32_t
        {
            old,
            new_,
        };

        constexpr NewsPanel(WidgetId id, Point origin, Size size, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null, Style newsStyle = Style::old)
            : Widget(id, origin, size, kWidgetType, colour, content, tooltip)
        {
            events.draw = &draw;
            styleData = enumValue(newsStyle);
        }

        constexpr NewsPanel(Point origin, Size size, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null, Style newsStyle = Style::old)
            : NewsPanel(WidgetId::none, origin, size, colour, content, tooltip, newsStyle)
        {
        }

        static void draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState);
    };

}
