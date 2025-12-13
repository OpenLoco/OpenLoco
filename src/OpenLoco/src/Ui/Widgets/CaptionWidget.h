#pragma once

#include "Ui/Widget.h"

namespace OpenLoco::Ui::Widgets
{
    struct Caption : public Widget
    {
        static constexpr auto kWidgetType = WidgetType::caption;

        enum class Style : uint32_t
        {
            boxed,      // 22
            blackText,  // 23
            colourText, // 24
            whiteText,  // 25
        };

        constexpr Caption(WidgetId id, Point origin, Size size, Style captionStyle, WindowColour colour, StringId content = StringIds::empty, StringId tooltip = StringIds::null)
            : Widget(id, origin, size, kWidgetType, colour, content, tooltip)
        {
            events.draw = &draw;
            styleData = enumValue(captionStyle);
        }

        constexpr Caption(Point origin, Size size, Style captionStyle, WindowColour colour, StringId content = StringIds::empty, StringId tooltip = StringIds::null)
            : Caption(WidgetId::none, origin, size, captionStyle, colour, content, tooltip)
        {
        }

        static void draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState);
    };

}
