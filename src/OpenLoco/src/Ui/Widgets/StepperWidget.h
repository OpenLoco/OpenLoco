#pragma once

#include "Ui/Widget.h"
#include "Ui/Widgets/ButtonWidget.h"
#include "Ui/Widgets/TextBoxWidget.h"

namespace OpenLoco::Ui::Widgets
{
    constexpr Button makeStepperDecreaseWidget(Ui::Point32 origin, Ui::Size size, WindowColour colour, [[maybe_unused]] uint32_t content = Widget::kContentNull, [[maybe_unused]] StringId tooltip = StringIds::null)
    {
        const int16_t xPos = origin.x + size.width - 26;
        const int16_t yPos = origin.y + 1;
        const uint16_t width = 13;
        const uint16_t height = size.height - 2;

        return Button({ xPos, yPos }, { width, height }, colour, StringIds::stepper_minus, tooltip);
    }

    constexpr Button makeStepperIncreaseWidget(Ui::Point32 origin, Ui::Size size, WindowColour colour, [[maybe_unused]] uint32_t content = Widget::kContentNull, [[maybe_unused]] StringId tooltip = StringIds::null)
    {
        const int16_t xPos = origin.x + size.width - 13;
        const int16_t yPos = origin.y + 1;
        const uint16_t width = 12;
        const uint16_t height = size.height - 2;

        return Button({ xPos, yPos }, { width, height }, colour, StringIds::stepper_plus, tooltip);
    }

    // TODO: Make this a single widget.
    constexpr auto stepperWidgets(Ui::Point32 origin, Ui::Size size, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
    {
        return makeWidgets(
            TextBox(origin, size, colour, content, tooltip),
            makeStepperDecreaseWidget(origin, size, colour),
            makeStepperIncreaseWidget(origin, size, colour));
    }

}
