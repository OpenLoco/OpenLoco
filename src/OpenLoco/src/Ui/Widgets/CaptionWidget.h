#pragma once

#include "Ui/Widget.h"

namespace OpenLoco::Ui
{
    enum class CaptionVariant : uint8_t
    {
        boxed,      // 22
        blackText,  // 23
        colourText, // 24
        whiteText,  // 25
    };
}

namespace OpenLoco::Ui::Widgets
{
    constexpr WidgetType getCaptionWidgetType(CaptionVariant variant)
    {
        switch (variant)
        {
            case CaptionVariant::boxed:
                return WidgetType::caption_22;
            case CaptionVariant::blackText:
                return WidgetType::caption_23;
            case CaptionVariant::colourText:
                return WidgetType::caption_24;
            case CaptionVariant::whiteText:
                return WidgetType::caption_25;
            default:
                return WidgetType::empty;
        }
    }

    struct Caption : public Widget
    {
        constexpr Caption(Ui::Point32 origin, Ui::Size32 size, CaptionVariant variant, WindowColour colour, StringId content = StringIds::empty, StringId tooltip = StringIds::null)
            : Widget(origin, size, getCaptionWidgetType(variant), colour, content, tooltip)
        {
            events.draw = &draw;
        }

        static void draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState);
    };

}
