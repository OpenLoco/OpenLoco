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

        constexpr NewsPanel(WidgetId id, Point32 origin, Size32 size, Style newsStyle, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
            : Widget(id, origin, size, kWidgetType, colour, content, tooltip)
        {
            events.draw = &draw;
            styleData = enumValue(newsStyle);
        }

        constexpr NewsPanel(Point32 origin, Size32 size, Style newsStyle, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
            : NewsPanel(WidgetId::none, origin, size, newsStyle, colour, content, tooltip)
        {
        }

        static void draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState);
    };

}
