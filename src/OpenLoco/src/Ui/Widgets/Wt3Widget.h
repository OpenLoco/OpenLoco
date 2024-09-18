#pragma once

#include "Ui/Widget.h"

namespace OpenLoco::Ui::Widgets
{
    struct Wt3Widget : public Widget
    {
        constexpr Wt3Widget(Ui::Point origin, Ui::Size size, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
            : Widget(origin, size, WidgetType::frame, colour, content, tooltip)
        {
        }
    };

}
