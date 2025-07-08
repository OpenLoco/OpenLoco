#include "CheckboxWidget.h"
#include "Graphics/Colour.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Ui/Window.h"

namespace OpenLoco::Ui::Widgets
{
    static constexpr auto kCheckMarkSize = Ui::Size{ 10, 10 };
    static constexpr auto kLabelMarginLeft = 4;

    // 0x004CB00B
    static void drawCheckBox(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        drawingCtx.fillRectInset(
            {},
            kCheckMarkSize,
            widgetState.colour,
            widgetState.flags | Gfx::RectInsetFlags::borderInset | Gfx::RectInsetFlags::fillDarker);

        if (widgetState.activated)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);
            static constexpr char strCheckmark[] = "\xAC";

            auto colour = widgetState.colour;
            if (widgetState.disabled)
            {
                colour = colour.inset();
            }

            tr.setCurrentFont(widget.font);
            tr.drawString({}, colour.opaque(), strCheckmark);
        }
    }

    // 0x004CB080
    static void drawLabel(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        if (widget.content == Widget::kContentNull)
        {
            return;
        }

        auto colour = widgetState.colour;
        colour = colour.opaque();

        if (widgetState.disabled)
        {
            colour = colour.inset();
        }

        auto formatArgs = FormatArguments(widget.textArgs);

        auto tr = Gfx::TextRenderer(drawingCtx);
        tr.setCurrentFont(widget.font);

        tr.drawStringLeft(Point{ kCheckMarkSize.width + kLabelMarginLeft, 0 }, colour, widget.text, formatArgs);
    }

    void Checkbox::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        drawCheckBox(drawingCtx, widget, widgetState);
        drawLabel(drawingCtx, widget, widgetState);
    }
}
