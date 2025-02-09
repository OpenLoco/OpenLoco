#pragma once

#include "Ui/Widget.h"

namespace OpenLoco::Ui::Widgets
{
    struct Caption : public Widget
    {
        enum class Style : uint32_t
        {
            boxed,      // 22
            blackText,  // 23
            colourText, // 24
            whiteText,  // 25
        };

        constexpr Caption(WidgetId id, Point32 origin, Size32 size, Style captionStyle, WindowColour colour, StringId content = StringIds::empty, StringId tooltip = StringIds::null)
            : Widget(id, origin, size, WidgetType::caption, colour, content, tooltip)
        {
            events.draw = &draw;
            styleData = enumValue(captionStyle);
        }

        constexpr Caption(Point32 origin, Size32 size, Style captionStyle, WindowColour colour, StringId content = StringIds::empty, StringId tooltip = StringIds::null)
            : Caption(WidgetId::none, origin, size, captionStyle, colour, content, tooltip)
        {
        }

        static void draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState);
    };

}
