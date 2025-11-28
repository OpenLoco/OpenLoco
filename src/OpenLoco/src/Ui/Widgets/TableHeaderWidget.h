#pragma once

#include "Ui/Widget.h"

namespace OpenLoco::Ui::Widgets
{
    struct TableHeader : public Widget
    {
        static constexpr auto kWidgetType = WidgetType::buttonTableHeader;

        enum class Style : uint32_t
        {
            notSorted,
            sortedAscending,
            sortedDescending,
        };

        constexpr TableHeader(WidgetId id, Point32 origin, Size32 size, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null, Style tableHeaderStyle = Style::notSorted)
            : Widget(id, origin, size, kWidgetType, colour, content, tooltip)
        {
            events.draw = &draw;
            styleData = enumValue(tableHeaderStyle);
        }

        constexpr TableHeader(Point32 origin, Size32 size, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null, Style tableHeaderStyle = Style::notSorted)
            : TableHeader(WidgetId::none, origin, size, colour, content, tooltip, tableHeaderStyle)
        {
        }

        static void draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState);
    };
}
