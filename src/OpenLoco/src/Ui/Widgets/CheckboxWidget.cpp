#include "CheckboxWidget.h"
#include "Graphics/Colour.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Ui/Window.h"

namespace OpenLoco::Ui::Widgets
{
    // 0x004CB00B
    static void drawCheckMark(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        if (widgetState.enabled)
        {
            drawingCtx.fillRectInset(
                widget.left,
                widget.top,
                widget.left + 9,
                widget.bottom - 1,
                widgetState.colour,
                widgetState.flags | Gfx::RectInsetFlags::borderInset | Gfx::RectInsetFlags::fillDarker);
        }

        if (widgetState.activated)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);
            static constexpr char strCheckmark[] = "\xAC";
            auto point = Point(widget.left, widget.top);

            auto color = widgetState.colour;
            tr.setCurrentFont(widget.font);
            tr.drawString(point, color.opaque(), strCheckmark);
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

        auto point = Point(widget.left + 14, widget.top);
        tr.drawStringLeft(point, colour, widget.text, formatArgs);
    }

    void Checkbox::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        drawCheckMark(drawingCtx, widget, widgetState);
        drawLabel(drawingCtx, widget, widgetState);
    }
}
