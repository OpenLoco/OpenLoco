#pragma once

#include "Ui/Widget.h"
#include "Ui/Widgets/ButtonWidget.h"

namespace OpenLoco::Ui::Widgets
{
    struct ComboBox : public Widget
    {
        static constexpr auto kWidgetType = WidgetType::combobox;

        constexpr ComboBox(WidgetId id, Point32 origin, Size32 size, WindowColour colour, StringId content = StringIds::null, StringId tooltip = StringIds::null)
            : Widget(id, origin, size, kWidgetType, colour, content, tooltip)
        {
            events.draw = &draw;
        }

        constexpr ComboBox(Point32 origin, Size32 size, WindowColour colour, StringId content = StringIds::null, StringId tooltip = StringIds::null)
            : ComboBox(WidgetId::none, origin, size, colour, content, tooltip)
        {
            events.draw = &draw;
        }

        static void draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState);
    };

    constexpr auto dropdownWidgets(Ui::Point32 origin, Ui::Size32 size, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
    {
        const auto makeDropdownButtonWidget = [](Ui::Point32 origin, Ui::Size32 size, WindowColour colour) {
            const int16_t xPos = origin.x + size.width - 12;
            const int16_t yPos = origin.y + 1;
            const uint16_t width = 11;
            const uint16_t height = 10;

            return Button({ xPos, yPos }, { width, height }, colour, StringIds::dropdown);
        };

        // TODO: Make this a single widget.
        return makeWidgets(
            ComboBox(origin, size, colour, content, tooltip),
            makeDropdownButtonWidget(origin, size, colour));
    }
}
