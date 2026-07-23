#pragma once

#include "Ui/Widget.h"
#include "Ui/Widgets/ButtonWidget.h"

namespace OpenLoco::Ui::Widgets
{
    struct ComboBox : public Widget
    {
        static constexpr auto kWidgetType = WidgetType::combobox;

        constexpr ComboBox(WidgetId id, Point origin, Size size, WindowColour colour, StringId content = StringIds::null, StringId tooltip = StringIds::null)
            : Widget(id, origin, size, kWidgetType, colour, content, tooltip)
        {
            events.draw = &draw;
        }

        constexpr ComboBox(Point origin, Size size, WindowColour colour, StringId content = StringIds::null, StringId tooltip = StringIds::null)
            : ComboBox(WidgetId::none, origin, size, colour, content, tooltip)
        {
            events.draw = &draw;
        }

        static void draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState);
    };

    // TODO: Make this a single widget.
    constexpr auto dropdownWidgets(WidgetId comboId, WidgetId buttonId, Ui::Point origin, Ui::Size size, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
    {
        const int16_t xPos = origin.x + size.width - 12;
        const int16_t yPos = origin.y + 1;
        const uint16_t width = 11;
        const uint16_t height = 10;

        return makeWidgets(
            ComboBox(comboId, origin, size, colour, content, tooltip),
            Button(buttonId, { xPos, yPos }, { width, height }, colour, StringIds::dropdown));
    }

    constexpr auto dropdownWidgets(Ui::Point origin, Ui::Size size, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
    {
        return dropdownWidgets(WidgetId::none, WidgetId::none, origin, size, colour, content, tooltip);
    }
}
