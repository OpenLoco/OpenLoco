#pragma once

#include "Ui/Widget.h"
#include "Ui/Widgets/ButtonWidget.h"
#include "Ui/Widgets/TextBoxWidget.h"

namespace OpenLoco::Ui::Widgets
{
    constexpr Button makeStepperDecreaseWidget(WidgetId id, Ui::Point origin, Ui::Size size, WindowColour colour, [[maybe_unused]] uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
    {
        const int16_t xPos = origin.x + size.width - 26;
        const int16_t yPos = origin.y + 1;
        const uint16_t width = 13;
        const uint16_t height = size.height - 2;

        return Button(id, { xPos, yPos }, { width, height }, colour, StringIds::stepper_minus, tooltip);
    }

    constexpr Button makeStepperDecreaseWidget(Ui::Point origin, Ui::Size size, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
    {
        return makeStepperDecreaseWidget(WidgetId::none, origin, size, colour, content, tooltip);
    }

    constexpr Button makeStepperIncreaseWidget(WidgetId id, Ui::Point origin, Ui::Size size, WindowColour colour, [[maybe_unused]] uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
    {
        const int16_t xPos = origin.x + size.width - 13;
        const int16_t yPos = origin.y + 1;
        const uint16_t width = 12;
        const uint16_t height = size.height - 2;

        return Button(id, { xPos, yPos }, { width, height }, colour, StringIds::stepper_plus, tooltip);
    }

    constexpr Button makeStepperIncreaseWidget(Ui::Point origin, Ui::Size size, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
    {
        return makeStepperIncreaseWidget(WidgetId::none, origin, size, colour, content, tooltip);
    }

    // TODO: Make this a single widget.
    constexpr auto stepperWidgets(WidgetId valueId, WidgetId decreaseId, WidgetId increaseId, Ui::Point origin, Ui::Size size, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
    {
        return makeWidgets(
            TextBox(valueId, origin, size, colour, content, tooltip),
            makeStepperDecreaseWidget(decreaseId, origin, size, colour),
            makeStepperIncreaseWidget(increaseId, origin, size, colour));
    }

    constexpr auto stepperWidgets(Ui::Point origin, Ui::Size size, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
    {
        return stepperWidgets(WidgetId::none, WidgetId::none, WidgetId::none, origin, size, colour, content, tooltip);
    }

}
